'use strict';

const path = require('path');
const db = require('./db');

const g_TypeKindToPropertyType = {
  [db.TypeKind.FLOAT]:      'FLOAT',
  [db.TypeKind.BOOL]:       'BOOL',
  [db.TypeKind.INT]:        'INT',
  [db.TypeKind.UINT8]:      'UINT8',
  [db.TypeKind.UINT16]:     'UINT16',
  [db.TypeKind.UINT32]:     'UINT32',
  [db.TypeKind.UINT64]:     'UINT64',
  [db.TypeKind.NAME]:       'NAME',
  [db.TypeKind.STRING]:     'STRING',
  [db.TypeKind.HANDLE]:     'HANDLE',
  [db.TypeKind.VECTOR2]:    'VECTOR2',
  [db.TypeKind.VECTOR3]:    'VECTOR3',
  [db.TypeKind.VECTOR4]:    'VECTOR4',
  [db.TypeKind.QUATERNION]: 'QUATERNION',
  [db.TypeKind.ENUM]:       'ENUM',
  [db.TypeKind.STRUCT]:     'STRUCT',
  [db.TypeKind.UNKNOWN]:    'UNKNOWN',
};

// Maps a raw C++ type string to Low::Core::Scripting::TypeKind
const g_TypeKindMap = [
  { kinds: ['void'],                                          kind: 'Void'   },
  { kinds: ['bool'],                                          kind: 'Bool'   },
  { kinds: ['int', 'int32_t'],                                kind: 'Int32'  },
  { kinds: ['uint8_t', 'u8'],                                 kind: 'UInt32' },
  { kinds: ['uint16_t', 'u16'],                               kind: 'UInt32' },
  { kinds: ['uint32_t', 'u32', 'unsigned int'],               kind: 'UInt32' },
  { kinds: ['uint64_t', 'u64'],                               kind: 'UInt64' },
  { kinds: ['float'],                                         kind: 'Float'  },
  { kinds: ['Low::Util::String', 'Util::String', 'String'],   kind: 'String' },
  { kinds: ['Low::Util::Name', 'Util::Name', 'Name'],         kind: 'Name'   },
];

function get_type_kind(p_Type) {
  const l_Clean = p_Type.replace(/\s*[*&]+\s*$/, '').replace(/^const\s+/, '').trim();
  for (const i_Entry of g_TypeKindMap) {
    if (i_Entry.kinds.includes(l_Clean)) return i_Entry.kind;
  }

  const l_Resolved = db.resolve_type(p_Type);
  if (l_Resolved.kind === db.TypeKind.ENUM) return 'Enum';
  if (l_Resolved.kind === db.TypeKind.STRUCT) return 'Struct';
  if (l_Resolved.kind === db.TypeKind.HANDLE) return 'Handle';

  return 'Handle';
}

function get_pointer_level(p_Type) {
  return (p_Type.match(/\*/g) || []).length;
}

function clean_type(p_Type) {
  return p_Type.replace(/\s*[*&]+\s*$/, '').replace(/^const\s+/, '').trim();
}

function is_string_type(p_Type) {
  const l_Clean = clean_type(p_Type);
  return [
    'Low::Util::String',
    'Util::String',
    'String',
  ].includes(l_Clean);
}

function is_list_type(p_Type) {
  const l_Clean = clean_type(p_Type);
  return /^(Low::)?Util::List\s*</.test(l_Clean) || /^List\s*</.test(l_Clean);
}

function get_list_element_type(p_Type) {
  const l_Clean = clean_type(p_Type);
  const l_Match = l_Clean.match(/^(?:(?:Low::)?Util::)?List\s*<([\s\S]+)>$/);
  return l_Match ? l_Match[1].trim() : '';
}

function get_list_element_as_type(p_Type) {
  const l_ElementType = get_list_element_type(p_Type);
  return get_script_base_type(l_ElementType);
}

function get_list_as_type(p_Type) {
  const l_ElementAsType = get_list_element_as_type(p_Type);
  return l_ElementAsType ? `array<${l_ElementAsType}>` : '';
}

function is_const(p_Type) {
  return p_Type.startsWith('const ');
}

function is_reference(p_Type) {
  return p_Type.trimEnd().endsWith('&');
}

function is_pointer(p_Type) {
  return p_Type.includes('*');
}

function get_param_direction_override(p_Param) {
  const l_Args = p_Param.param_args || {};
  if (l_Args.in === true) return 'in';
  if (l_Args.out === true) return 'out';
  if (l_Args.inout === true) return 'inout';
  if (l_Args.value === true) return 'value';
  return '';
}

function infer_param_direction(p_Param) {
  const l_Override = get_param_direction_override(p_Param);
  if (l_Override) return l_Override;
  if (is_const(p_Param.type) && (is_reference(p_Param.type) || is_pointer(p_Param.type))) return 'in';
  if (is_reference(p_Param.type) || is_pointer(p_Param.type)) return 'inout';
  return '';
}

function get_script_base_type(p_Type) {
  const l_AsType = db.get_as_type_string(p_Type);
  if (l_AsType) return l_AsType;

  const l_Clean = clean_type(p_Type);
  const l_Fallback = {
    void: 'void',
    bool: 'bool',
    int: 'int',
    int32_t: 'int',
    uint8_t: 'u8',
    u8: 'u8',
    uint16_t: 'u16',
    u16: 'u16',
    uint32_t: 'u32',
    u32: 'u32',
    'unsigned int': 'u32',
    uint64_t: 'u64',
    u64: 'u64',
    float: 'float',
  };

  return l_Fallback[l_Clean] || '';
}

function wrapper_name(p_ModuleName, p_Fn, p_Index) {
  return `LowLens_${p_ModuleName}_${p_Fn.name}_${p_Index}`;
}

function qualified_wrapper_name(p_Fn) {
  return p_Fn.namespace ? `${p_Fn.namespace}::${p_Fn.wrapper_name}` : p_Fn.wrapper_name;
}

function needs_function_wrapper(p_Fn) {
  if (is_list_type(p_Fn.return_type)) {
    return true;
  }

  if (is_string_type(p_Fn.return_type)) {
    return true;
  }

  for (const i_Param of p_Fn.params) {
    if (is_list_type(i_Param.type)) {
      return true;
    }

    if (is_string_type(i_Param.type)) {
      return true;
    }

    const l_Direction = infer_param_direction(i_Param);
    if (l_Direction === 'value' &&
        (is_pointer(i_Param.type) || is_reference(i_Param.type))) {
      return true;
    }
  }

  return false;
}

function needs_script_array_include(p_Functions, p_Structs) {
  for (const i_Fn of p_Functions) {
    if (is_list_type(i_Fn.return_type)) {
      return true;
    }

    for (const i_Param of i_Fn.params) {
      if (is_list_type(i_Param.type)) {
        return true;
      }
    }
  }

  for (const i_Struct of p_Structs) {
    if (!i_Struct.scripting) continue;

    for (const i_Field of i_Struct.fields) {
      if (is_list_type(i_Field.type)) {
        return true;
      }
    }
  }

  return false;
}

function full_enum_type(p_Enum) {
  return p_Enum.namespace ? `${p_Enum.namespace}::${p_Enum.name}` : p_Enum.name;
}

function helper_namespace(p_Enum) {
  return `${full_enum_type(p_Enum)}Enum`;
}

// ---------------------------------------------------------------------------
// Function registration
// ---------------------------------------------------------------------------

function generate_type_info(p_VarName, p_Type, p_Indent, p_EmitPointerLevel = true, p_IsReturn = false) {
  const l_I = '  '.repeat(p_Indent);
  const l_Resolved = db.resolve_type(p_Type);
  let t = '';
  const l_IsList = is_list_type(p_Type);
  const l_ElementType = l_IsList ? get_list_element_type(p_Type) : '';
  const l_TypeForKind = l_IsList ? l_ElementType : p_Type;
  t += `${l_I}${p_VarName}.kind = Low::Core::Scripting::TypeKind::${get_type_kind(l_TypeForKind)};\n`;
  const l_AsType = l_IsList ? get_script_base_type(l_ElementType) : get_script_base_type(p_Type);
  if (l_AsType) t += `${l_I}${p_VarName}.as_type = "${l_AsType}";\n`;
  const l_ResolvedForReference = l_IsList ? db.resolve_type(l_ElementType) : l_Resolved;
  if (l_ResolvedForReference.entry) {
    t += `${l_I}${p_VarName}.referenced_type = Low::Util::TypeIdentifier(N(${l_ResolvedForReference.entry.module}), N(${l_ResolvedForReference.entry.name}));\n`;
  }
  if (l_IsList) {
    t += `${l_I}${p_VarName}.container = Low::Core::Scripting::TypeContainer::List;\n`;
    if (p_IsReturn) {
      t += `${l_I}${p_VarName}.pointer_level = 1;\n`;
    }
  }
  const l_PtrLevel = get_pointer_level(p_Type);
  if (!l_IsList && p_EmitPointerLevel && l_PtrLevel > 0) t += `${l_I}${p_VarName}.pointer_level = ${l_PtrLevel};\n`;
  if (is_const(p_Type) &&
      (is_reference(p_Type) || is_pointer(p_Type) || l_IsList)) {
    t += `${l_I}${p_VarName}.constant = true;\n`;
  }
  if (!l_IsList && is_reference(p_Type)) t += `${l_I}${p_VarName}.reference = true;\n`;
  return t;
}

function generate_parameter_type_info(p_VarName, p_Param, p_Indent) {
  const l_I = '  '.repeat(p_Indent);
  const l_Direction = infer_param_direction(p_Param);
  let t = generate_type_info(p_VarName, p_Param.type, p_Indent, !is_pointer(p_Param.type));

  if (is_pointer(p_Param.type) || is_list_type(p_Param.type)) {
    t += `${l_I}${p_VarName}.reference = true;\n`;
  }

  if (l_Direction === 'value') {
    t += `${l_I}${p_VarName}.reference = false;\n`;
    t += `${l_I}${p_VarName}.direction = "";\n`;
  } else if (l_Direction) {
    t += `${l_I}${p_VarName}.direction = "${l_Direction}";\n`;
  }

  return t;
}

function generate_function_registration(p_Fn) {
  const l_FunctionPtr = p_Fn.wrapper_name ? qualified_wrapper_name(p_Fn) : (p_Fn.namespace ? `${p_Fn.namespace}::${p_Fn.name}` : p_Fn.name);

  let t = '';
  t += `  {\n`;
  t += `    Low::Core::Scripting::FunctionInfo l_FunctionInfo;\n`;
  t += `    l_FunctionInfo.name = N(${p_Fn.name});\n`;
  t += `    l_FunctionInfo.bind_name = N(${p_Fn.bind_name});\n`;
  t += `    l_FunctionInfo.bind_namespace = "${p_Fn.bind_namespace}";\n`;
  t += `    l_FunctionInfo.is_property = ${p_Fn.is_property};\n`;
  t += `    l_FunctionInfo.ptr = (void*)&${l_FunctionPtr};\n`;
  t += `\n`;
  t += generate_type_info('l_FunctionInfo.return_type', p_Fn.return_type, 2, true, true);

  for (const i_Param of p_Fn.params) {
    t += `\n`;
    t += `    {\n`;
    t += `      Low::Core::Scripting::FunctionParameterInfo l_Param;\n`;
    t += `      l_Param.name = N(${i_Param.bind_name || i_Param.name});\n`;
    t += generate_parameter_type_info('l_Param.type', i_Param, 3);
    t += `      l_FunctionInfo.parameters.push_back(l_Param);\n`;
    t += `    }\n`;
  }

  t += `\n`;
  t += `    Low::Core::Scripting::register_function(l_FunctionInfo);\n`;
  t += `  }\n`;
  return t;
}

function wrapper_param_type(p_Param) {
  const l_Direction = infer_param_direction(p_Param);
  const l_BaseType = clean_type(p_Param.type);

  if (is_list_type(p_Param.type)) {
    if (l_Direction === 'value' || (!is_reference(p_Param.type) && !is_pointer(p_Param.type))) {
      return 'const CScriptArray &';
    }
    if (is_const(p_Param.type)) {
      return 'const CScriptArray &';
    }
    return 'CScriptArray &';
  }

  if (is_string_type(p_Param.type)) {
    if (l_Direction === 'value' || (!is_reference(p_Param.type) && !is_pointer(p_Param.type))) {
      return 'std::string';
    }
    if (is_const(p_Param.type)) {
      return 'const std::string &';
    }
    return 'std::string &';
  }

  if (l_Direction === 'value') {
    return l_BaseType;
  }

  return p_Param.type;
}

function wrapper_call_arg(p_Param) {
  const l_Direction = infer_param_direction(p_Param);
  const l_Name = p_Param.name;

  if (is_string_type(p_Param.type)) {
    if (l_Direction === 'out') return l_Name;
    if (l_Direction === 'inout') return `Low::Util::String(${l_Name}.c_str())`;
    return `Low::Util::String(${l_Name}.c_str())`;
  }

  if (l_Direction === 'value') {
    if (is_pointer(p_Param.type)) return `&${l_Name}`;
    if (is_reference(p_Param.type)) return l_Name;
  }

  return l_Name;
}

function string_wrapper_local_name(p_Param) {
  return `l_${p_Param.name}_String`;
}

function list_wrapper_local_name(p_Param) {
  return `l_${p_Param.name}_List`;
}

function generate_list_to_array(p_ListExpr, p_Type, p_ArrayVar, p_Indent) {
  const l_I = '  '.repeat(p_Indent);
  const l_ElementType = get_list_element_type(p_Type);
  const l_ElementAsType = get_list_element_as_type(p_Type);
  const l_ArrayType = get_list_as_type(p_Type);
  let t = '';
  t += `${l_I}asITypeInfo *${p_ArrayVar}_Type = Low::Core::Scripting::get_engine()->GetTypeInfoByDecl("${l_ArrayType}");\n`;
  t += `${l_I}LOW_ASSERT(${p_ArrayVar}_Type, "Could not find AngelScript array type.");\n`;
  t += `${l_I}CScriptArray *${p_ArrayVar} = CScriptArray::Create(${p_ArrayVar}_Type, static_cast<asUINT>(${p_ListExpr}.size()));\n`;
  t += `${l_I}for (asUINT i = 0; i < ${p_ArrayVar}->GetSize(); ++i) {\n`;
  if (is_string_type(l_ElementType)) {
    t += `${l_I}  std::string l_Value(${p_ListExpr}[i].c_str());\n`;
  } else {
    t += `${l_I}  ${l_ElementType} l_Value = ${p_ListExpr}[i];\n`;
  }
  t += `${l_I}  ${p_ArrayVar}->SetValue(i, &l_Value);\n`;
  t += `${l_I}}\n`;
  return t;
}

function generate_array_to_list(p_ArrayExpr, p_Type, p_ListVar, p_Indent) {
  const l_I = '  '.repeat(p_Indent);
  const l_ElementType = get_list_element_type(p_Type);
  let t = '';
  t += `${l_I}${clean_type(p_Type)} ${p_ListVar};\n`;
  t += `${l_I}${p_ListVar}.reserve(${p_ArrayExpr}.GetSize());\n`;
  t += `${l_I}for (asUINT i = 0; i < ${p_ArrayExpr}.GetSize(); ++i) {\n`;
  if (is_string_type(l_ElementType)) {
    t += `${l_I}  const std::string &l_Value = *static_cast<const std::string *>(${p_ArrayExpr}.At(i));\n`;
    t += `${l_I}  ${p_ListVar}.push_back(Low::Util::String(l_Value.c_str()));\n`;
  } else {
    t += `${l_I}  ${p_ListVar}.push_back(*static_cast<const ${l_ElementType} *>(${p_ArrayExpr}.At(i)));\n`;
  }
  t += `${l_I}}\n`;
  return t;
}

function wrapper_pre_call(p_Param) {
  if (is_list_type(p_Param.type)) {
    const l_Direction = infer_param_direction(p_Param);
    if (l_Direction === 'out') {
      return `  ${clean_type(p_Param.type)} ${list_wrapper_local_name(p_Param)};\n`;
    }
    return generate_array_to_list(p_Param.name, p_Param.type, list_wrapper_local_name(p_Param), 1);
  }

  if (!is_string_type(p_Param.type)) return '';

  const l_Direction = infer_param_direction(p_Param);
  const l_Local = string_wrapper_local_name(p_Param);

  if (l_Direction === 'out') {
    return `  Low::Util::String ${l_Local};\n`;
  }

  return `  Low::Util::String ${l_Local}(${p_Param.name}.c_str());\n`;
}

function wrapper_post_call(p_Param) {
  if (is_list_type(p_Param.type)) {
    const l_Direction = infer_param_direction(p_Param);
    if (l_Direction !== 'out' && l_Direction !== 'inout') return '';

    const l_ArrayVar = `l_${p_Param.name}_Array`;
    let t = generate_list_to_array(list_wrapper_local_name(p_Param), p_Param.type, l_ArrayVar, 1);
    t += `  ${p_Param.name} = *${l_ArrayVar};\n`;
    t += `  ${l_ArrayVar}->Release();\n`;
    return t;
  }

  if (!is_string_type(p_Param.type)) return '';

  const l_Direction = infer_param_direction(p_Param);
  if (l_Direction !== 'out' && l_Direction !== 'inout') return '';

  const l_Local = string_wrapper_local_name(p_Param);
  return `  ${p_Param.name} = std::string(${l_Local}.c_str());\n`;
}

function wrapper_call_arg_with_conversions(p_Param) {
  if (is_list_type(p_Param.type)) {
    const l_Local = list_wrapper_local_name(p_Param);
    if (is_pointer(p_Param.type)) {
      return `&${l_Local}`;
    }
    return l_Local;
  }

  if (!is_string_type(p_Param.type)) {
    return wrapper_call_arg(p_Param);
  }

  const l_Direction = infer_param_direction(p_Param);
  const l_Local = string_wrapper_local_name(p_Param);

  if (is_pointer(p_Param.type)) {
    return `&${l_Local}`;
  }

  if (is_reference(p_Param.type)) {
    return l_Local;
  }

  if (l_Direction === 'value') {
    return `Low::Util::String(${p_Param.name}.c_str())`;
  }

  return `Low::Util::String(${p_Param.name}.c_str())`;
}

function generate_function_wrapper(p_Fn) {
  const l_FullName = p_Fn.namespace ? `${p_Fn.namespace}::${p_Fn.name}` : p_Fn.name;
  const l_ReturnType = is_list_type(p_Fn.return_type) ? 'CScriptArray *' : (is_string_type(p_Fn.return_type) ? 'std::string' : p_Fn.return_type);
  const l_Params = p_Fn.params
      .map(i_Param => `${wrapper_param_type(i_Param)} ${i_Param.name}`)
      .join(', ');
  const l_CallArgs = p_Fn.params.map(wrapper_call_arg_with_conversions).join(', ');

  let t = '';
  if (p_Fn.namespace) t += `namespace ${p_Fn.namespace} {\n`;
  t += `static ${l_ReturnType} ${p_Fn.wrapper_name}(${l_Params})\n`;
  t += `{\n`;
  for (const i_Param of p_Fn.params) {
    t += wrapper_pre_call(i_Param);
  }

  if (p_Fn.return_type.trim() === 'void') {
    t += `  ${l_FullName}(${l_CallArgs});\n`;
    for (const i_Param of p_Fn.params) {
      t += wrapper_post_call(i_Param);
    }
  } else if (is_string_type(p_Fn.return_type)) {
    t += `  Low::Util::String l_ReturnValue = ${l_FullName}(${l_CallArgs});\n`;
    for (const i_Param of p_Fn.params) {
      t += wrapper_post_call(i_Param);
    }
    t += `  return std::string(l_ReturnValue.c_str());\n`;
  } else if (is_list_type(p_Fn.return_type)) {
    t += `  ${clean_type(p_Fn.return_type)} l_ReturnValue = ${l_FullName}(${l_CallArgs});\n`;
    for (const i_Param of p_Fn.params) {
      t += wrapper_post_call(i_Param);
    }
    t += generate_list_to_array('l_ReturnValue', p_Fn.return_type, 'l_ReturnArray', 1);
    t += `  return l_ReturnArray;\n`;
  } else {
    t += `  ${p_Fn.return_type} l_ReturnValue = ${l_FullName}(${l_CallArgs});\n`;
    for (const i_Param of p_Fn.params) {
      t += wrapper_post_call(i_Param);
    }
    t += `  return l_ReturnValue;\n`;
  }
  t += `}\n`;
  if (p_Fn.namespace) t += `} // namespace ${p_Fn.namespace}\n`;
  return t;
}

// ---------------------------------------------------------------------------
// Enum registration (in the register function body)
// ---------------------------------------------------------------------------

function generate_enum_rtti_registration(p_Enum, p_ModuleName) {
  const l_HelperNs = helper_namespace(p_Enum);
  const l_EnumIdVar = `${l_HelperNs}::g_EnumId`;

  let t = '';
  t += `  {\n`;
  t += `    ${l_HelperNs}::IDENTIFIER = Low::Util::TypeIdentifier(N(${p_ModuleName}), N(${p_Enum.name}));\n`;
  t += `    Low::Util::RTTI::EnumInfo l_EnumInfo;\n`;
  t += `    l_EnumInfo.name = N(${p_Enum.name});\n`;
  t += `    l_EnumInfo.entry_name = &${l_HelperNs}::_entry_name;\n`;
  t += `    l_EnumInfo.entry_value = &${l_HelperNs}::_entry_value;\n`;

  for (const i_Val of p_Enum.values) {
    t += `    {\n`;
    t += `      Low::Util::RTTI::EnumEntryInfo l_Entry;\n`;
    t += `      l_Entry.name = N(${i_Val.name});\n`;
    t += `      l_Entry.value = ${i_Val.value};\n`;
    t += `      l_EnumInfo.entries.push_back(l_Entry);\n`;
    t += `    }\n`;
  }

  t += `    ${l_EnumIdVar} = Low::Util::register_enum_info(${l_HelperNs}::IDENTIFIER, l_EnumInfo);\n`;
  t += `  }\n`;
  return t;
}

function generate_enum_scripting_registration(p_Enum) {
  const l_HelperNs = helper_namespace(p_Enum);

  let t = '';
  t += `  {\n`;
  t += `    Low::Core::Scripting::EnumInfo l_ScriptingInfo;\n`;
  t += `    l_ScriptingInfo.bind_name = N(${p_Enum.bind_name});\n`;
  t += `    l_ScriptingInfo.bind_namespace = "${p_Enum.bind_namespace}";\n`;
  t += `    l_ScriptingInfo.identifier = ${l_HelperNs}::IDENTIFIER;\n`;
  t += `    l_ScriptingInfo.entry_name_ptr = (void*)&${l_HelperNs}::entry_name;\n`;
  t += `    l_ScriptingInfo.entry_value_ptr = (void*)&${l_HelperNs}::entry_value;\n`;
  t += `    Low::Core::Scripting::register_enum(l_ScriptingInfo);\n`;
  t += `  }\n`;
  return t;
}

// ---------------------------------------------------------------------------
// Enum helper implementations (in .gen.cpp, outside the register function)
// ---------------------------------------------------------------------------

function generate_enum_impl(p_Enum, p_ModuleName) {
  const l_FullType = full_enum_type(p_Enum);
  const l_Ns = p_Enum.namespace ? `namespace ${p_Enum.namespace}` : '';

  let t = '';
  if (l_Ns) t += `${l_Ns} {\n`;
  t += `namespace ${p_Enum.name}Enum {\n`;
  t += `  Low::Util::TypeIdentifier IDENTIFIER;\n`;
  t += `  u16 g_EnumId = 0;\n`;
  t += `\n`;

  // entry_name
  t += `  Low::Util::Name entry_name(${l_FullType} p_Value)\n  {\n`;
  for (const i_Val of p_Enum.values) {
    t += `    if (p_Value == ${l_FullType}::${i_Val.name}) return N(${i_Val.name});\n`;
  }
  t += `    LOW_ASSERT(false, "Could not find entry in enum ${p_Enum.name}.");\n`;
  t += `    return N();\n`;
  t += `  }\n\n`;

  // _entry_name (raw u8, for RTTI function pointer)
  t += `  Low::Util::Name _entry_name(u8 p_Value)\n  {\n`;
  t += `    return entry_name(static_cast<${l_FullType}>(p_Value));\n`;
  t += `  }\n\n`;

  // entry_value
  t += `  ${l_FullType} entry_value(Low::Util::Name p_Name)\n  {\n`;
  for (const i_Val of p_Enum.values) {
    t += `    if (p_Name == N(${i_Val.name})) return ${l_FullType}::${i_Val.name};\n`;
  }
  t += `    LOW_ASSERT(false, "Could not find entry in enum ${p_Enum.name}.");\n`;
  t += `    return static_cast<${l_FullType}>(0);\n`;
  t += `  }\n\n`;

  // _entry_value (raw u8, for RTTI function pointer)
  t += `  u8 _entry_value(Low::Util::Name p_Name)\n  {\n`;
  t += `    return static_cast<u8>(entry_value(p_Name));\n`;
  t += `  }\n\n`;

  // get_enum_id
  t += `  u16 get_enum_id() { return g_EnumId; }\n\n`;

  // get_entry_count
  t += `  u8 get_entry_count() { return ${p_Enum.values.length}; }\n`;

  t += `\n} // namespace ${p_Enum.name}Enum\n`;
  if (l_Ns) t += `} // ${l_Ns}\n`;

  return t;
}

// ---------------------------------------------------------------------------
// Struct helpers
// ---------------------------------------------------------------------------

function full_struct_type(p_Struct) {
  return p_Struct.namespace ? `${p_Struct.namespace}::${p_Struct.name}` : p_Struct.name;
}

function struct_helper_namespace(p_Struct) {
  return `${full_struct_type(p_Struct)}Struct`;
}

function generate_struct_header_declarations(p_Struct) {
  const l_FullType = full_struct_type(p_Struct);
  const l_Ns = p_Struct.namespace ? `namespace ${p_Struct.namespace}` : '';

  let t = '';
  if (l_Ns) t += `${l_Ns} {\n`;
  t += `namespace ${p_Struct.name}Struct {\n`;
  t += `  extern Low::Util::TypeIdentifier IDENTIFIER;\n`;
  t += `  extern u16 g_StructId;\n`;
  t += `  void *get_field(${l_FullType} &p_Instance, Low::Util::Name p_FieldName);\n`;

  for (const i_Field of p_Struct.fields) {
    const l_Ref = i_Field.type.endsWith('*') ? '*' : '&';
    t += `  ${i_Field.type} ${l_Ref}get_${i_Field.name}(${l_FullType} &p_Instance);\n`;
    t += `  void set_${i_Field.name}(${l_FullType} &p_Instance, const ${i_Field.type} &p_Value);\n`;
  }

  t += `} // namespace ${p_Struct.name}Struct\n`;
  if (l_Ns) t += `} // ${l_Ns}\n`;

  // Converter<T> specialization — enables node = instance and node.as<T>()
  t += `\n`;
  t += `template <>\n`;
  t += `struct Low::Util::Serial::Converter<${l_FullType}, void>\n`;
  t += `{\n`;
  t += `  static Low::Util::Serial::Node encode(const ${l_FullType} &p_Value)\n`;
  t += `  {\n`;
  t += `    Low::Util::Serial::Node n;\n`;
  t += `    Low::Util::Serial::serialize_struct(\n`;
  t += `        n, ${l_FullType}Struct::IDENTIFIER, (void *)&p_Value);\n`;
  t += `    return n;\n`;
  t += `  }\n`;
  t += `  static bool decode(const Low::Util::Serial::Node &p_Node,\n`;
  t += `                     ${l_FullType} &p_Out)\n`;
  t += `  {\n`;
  t += `    if (!p_Node["struct_type"] || !p_Node["fields"]) return false;\n`;
  t += `    Low::Util::Serial::deserialize_struct(\n`;
  t += `        const_cast<Low::Util::Serial::Node &>(p_Node),\n`;
  t += `        (void *)&p_Out);\n`;
  t += `    return true;\n`;
  t += `  }\n`;
  t += `};\n`;

  return t;
}

function generate_struct_impl(p_Struct, p_ModuleName) {
  const l_FullType = full_struct_type(p_Struct);
  const l_HelperNs = struct_helper_namespace(p_Struct);
  const l_Ns = p_Struct.namespace ? `namespace ${p_Struct.namespace}` : '';

  let t = '';
  if (l_Ns) t += `${l_Ns} {\n`;
  t += `namespace ${p_Struct.name}Struct {\n`;
  t += `  Low::Util::TypeIdentifier IDENTIFIER;\n`;
  t += `  u16 g_StructId = 0;\n\n`;

  // get_field generic
  t += `  void *get_field(${l_FullType} &p_Instance, Low::Util::Name p_FieldName)\n  {\n`;
  for (const i_Field of p_Struct.fields) {
    t += `    if (p_FieldName == N(${i_Field.name})) return &p_Instance.${i_Field.name};\n`;
  }
  t += `    LOW_ASSERT(false, "Could not find field in struct ${p_Struct.name}.");\n`;
  t += `    return nullptr;\n`;
  t += `  }\n\n`;

  // typed getters/setters
  for (const i_Field of p_Struct.fields) {
    const l_Ref = i_Field.type.endsWith('*') ? '*' : '&';
    t += `  ${i_Field.type} ${l_Ref}get_${i_Field.name}(${l_FullType} &p_Instance)\n  {\n`;
    t += `    return p_Instance.${i_Field.name};\n`;
    t += `  }\n\n`;
    t += `  void set_${i_Field.name}(${l_FullType} &p_Instance, const ${i_Field.type} &p_Value)\n  {\n`;
    t += `    p_Instance.${i_Field.name} = p_Value;\n`;
    t += `  }\n\n`;

    if (p_Struct.scripting && is_string_type(i_Field.type)) {
      t += `  std::string as_get_${i_Field.name}(const ${l_FullType} &p_Instance)\n  {\n`;
      t += `    return std::string(p_Instance.${i_Field.name}.c_str());\n`;
      t += `  }\n\n`;
      t += `  void as_set_${i_Field.name}(${l_FullType} &p_Instance, const std::string &p_Value)\n  {\n`;
      t += `    p_Instance.${i_Field.name} = Low::Util::String(p_Value.c_str());\n`;
      t += `  }\n\n`;
    } else if (p_Struct.scripting && is_list_type(i_Field.type)) {
      t += `  CScriptArray *as_get_${i_Field.name}(const ${l_FullType} &p_Instance)\n  {\n`;
      t += generate_list_to_array(`p_Instance.${i_Field.name}`, i_Field.type, 'l_Array', 2);
      t += `    return l_Array;\n`;
      t += `  }\n\n`;
      t += `  void as_set_${i_Field.name}(${l_FullType} &p_Instance, const CScriptArray &p_Value)\n  {\n`;
      t += generate_array_to_list('p_Value', i_Field.type, 'l_List', 2);
      t += `    p_Instance.${i_Field.name} = l_List;\n`;
      t += `  }\n\n`;
    }
  }

  if (p_Struct.scripting) {
    t += `  void as_default_construct(${l_FullType} *p_Memory)\n  {\n`;
    t += `    new (p_Memory) ${l_FullType}();\n`;
    t += `  }\n\n`;
    t += `  void as_copy_construct(const ${l_FullType} &p_Other, ${l_FullType} *p_Memory)\n  {\n`;
    t += `    new (p_Memory) ${l_FullType}(p_Other);\n`;
    t += `  }\n\n`;
    t += `  void as_destruct(${l_FullType} *p_Memory)\n  {\n`;
    t += `    p_Memory->~${p_Struct.name}();\n`;
    t += `  }\n\n`;
    t += `  ${l_FullType} &as_assign(const ${l_FullType} &p_Other, ${l_FullType} *p_Self)\n  {\n`;
    t += `    *p_Self = p_Other;\n`;
    t += `    return *p_Self;\n`;
    t += `  }\n\n`;
  }

  t += `} // namespace ${p_Struct.name}Struct\n`;
  if (l_Ns) t += `} // ${l_Ns}\n`;
  return t;
}

function generate_struct_scripting_registration(p_Struct) {
  const l_HelperNs = struct_helper_namespace(p_Struct);

  let t = '';
  t += `  {\n`;
  t += `    Low::Core::Scripting::StructInfo l_ScriptingInfo;\n`;
  t += `    l_ScriptingInfo.bind_name = N(${p_Struct.bind_name});\n`;
  t += `    l_ScriptingInfo.bind_namespace = "${p_Struct.bind_namespace}";\n`;
  t += `    l_ScriptingInfo.identifier = ${l_HelperNs}::IDENTIFIER;\n`;
  t += `    l_ScriptingInfo.default_constructor = (void*)&${l_HelperNs}::as_default_construct;\n`;
  t += `    l_ScriptingInfo.copy_constructor = (void*)&${l_HelperNs}::as_copy_construct;\n`;
  t += `    l_ScriptingInfo.destructor = (void*)&${l_HelperNs}::as_destruct;\n`;
  t += `    l_ScriptingInfo.assign = (void*)&${l_HelperNs}::as_assign;\n`;
  t += `    Low::Core::Scripting::register_struct(l_ScriptingInfo);\n`;
  t += `  }\n`;
  return t;
}

function generate_struct_rtti_registration(p_Struct, p_ModuleName) {
  const l_HelperNs = struct_helper_namespace(p_Struct);
  const l_FullType = full_struct_type(p_Struct);

  let t = '';
  t += `  {\n`;
  t += `    ${l_HelperNs}::IDENTIFIER = Low::Util::TypeIdentifier(N(${p_ModuleName}), N(${p_Struct.name}));\n`;
  t += `    Low::Util::RTTI::StructInfo l_StructInfo;\n`;
  t += `    l_StructInfo.name = N(${p_Struct.name});\n`;

  for (const i_Field of p_Struct.fields) {
    const l_Resolved = db.resolve_type(i_Field.type);
    const l_PropType = g_TypeKindToPropertyType[l_Resolved.kind] || 'UNKNOWN';
    const l_AsType = is_list_type(i_Field.type) ? get_list_as_type(i_Field.type) : db.get_as_type_string(i_Field.type);

    t += `    {\n`;
    t += `      Low::Util::RTTI::StructFieldInfo l_Field;\n`;
    t += `      l_Field.name = N(${i_Field.name});\n`;
    t += `      l_Field.type = Low::Util::RTTI::PropertyType::${l_PropType};\n`;
    if (l_Resolved.entry) {
      t += `      l_Field.referenced_type = Low::Util::TypeIdentifier(N(${l_Resolved.entry.module}), N(${l_Resolved.entry.name}));\n`;
    }
    t += `      l_Field.as_type = "${l_AsType}";\n`;
    t += `      l_Field.offset = offsetof(${l_FullType}, ${i_Field.name});\n`;
    t += `      l_Field.getter = (void*)&${l_HelperNs}::get_${i_Field.name};\n`;
    t += `      l_Field.setter = (void*)&${l_HelperNs}::set_${i_Field.name};\n`;
    if (p_Struct.scripting && is_string_type(i_Field.type)) {
      t += `      l_Field.scripting_getter = (void*)&${l_HelperNs}::as_get_${i_Field.name};\n`;
      t += `      l_Field.scripting_setter = (void*)&${l_HelperNs}::as_set_${i_Field.name};\n`;
    } else if (p_Struct.scripting && is_list_type(i_Field.type)) {
      t += `      l_Field.scripting_getter = (void*)&${l_HelperNs}::as_get_${i_Field.name};\n`;
      t += `      l_Field.scripting_setter = (void*)&${l_HelperNs}::as_set_${i_Field.name};\n`;
      t += `      l_Field.scripting_get_type = "${l_AsType}@+";\n`;
      t += `      l_Field.scripting_set_type = "const ${l_AsType} &in";\n`;
    }
    t += `      l_StructInfo.fields.push_back(l_Field);\n`;
    t += `    }\n`;
  }

  t += `    l_StructInfo.size = sizeof(${l_FullType});\n`;
  t += `    ${l_HelperNs}::g_StructId = Low::Util::register_struct_info(${l_HelperNs}::IDENTIFIER, l_StructInfo);\n`;
  t += `  }\n`;

  return t;
}

// ---------------------------------------------------------------------------
// Per-header .gen.h content
// ---------------------------------------------------------------------------

function generate_enum_header_declarations(p_Enum) {
  const l_FullType = full_enum_type(p_Enum);
  const l_Ns = p_Enum.namespace ? `namespace ${p_Enum.namespace}` : '';

  let t = '';
  if (l_Ns) t += `${l_Ns} {\n`;
  t += `namespace ${p_Enum.name}Enum {\n`;
  t += `  extern Low::Util::TypeIdentifier IDENTIFIER;\n`;
  t += `  extern u16 g_EnumId;\n`;
  t += `  Low::Util::Name entry_name(${l_FullType} p_Value);\n`;
  t += `  Low::Util::Name _entry_name(u8 p_Value);\n`;
  t += `  ${l_FullType} entry_value(Low::Util::Name p_Name);\n`;
  t += `  u8 _entry_value(Low::Util::Name p_Name);\n`;
  t += `  u16 get_enum_id();\n`;
  t += `  u8 get_entry_count();\n`;
  t += `} // namespace ${p_Enum.name}Enum\n`;
  if (l_Ns) t += `} // ${l_Ns}\n`;

  return t;
}

// p_Enums/p_Structs: all from one source header
function generate_header_file(p_Enums, p_Structs) {
  let t = '';
  t += `// AUTO-GENERATED by LowLens - do not edit manually\n`;
  t += `\n`;
  t += `#pragma once\n`;
  t += `\n`;
  t += `#include "LowUtilHandle.h"\n`;
  t += `\n`;
  for (const i_Enum of p_Enums) {
    t += generate_enum_header_declarations(i_Enum);
    t += `\n`;
  }
  for (const i_Struct of p_Structs) {
    t += generate_struct_header_declarations(i_Struct);
    t += `\n`;
  }
  return t;
}

// ---------------------------------------------------------------------------
// Module .gen.cpp
// ---------------------------------------------------------------------------

function generate_module_file(p_ModuleName, p_Functions, p_Enums, p_Structs, p_Headers) {
  const l_RegFnName = `register_${p_ModuleName.toLowerCase()}`;
  const l_Functions = p_Functions.map((i_Fn, i_Index) => ({
    ...i_Fn,
    wrapper_name: needs_function_wrapper(i_Fn) ? wrapper_name(p_ModuleName, i_Fn, i_Index) : '',
  }));

  let t = '';
  t += `// AUTO-GENERATED by LowLens - do not edit manually\n`;
  t += `\n`;
  t += `#include "LowCoreScripting.h"\n`;
  t += `#include "LowUtilHandle.h"\n`;
  for (const i_Header of p_Headers) {
    t += `#include "${i_Header}"\n`;
  }
  t += `#include <string>\n`;
  if (needs_script_array_include(l_Functions, p_Structs)) {
    t += `#include <scriptarray/scriptarray.h>\n`;
  }
  t += `\n`;

  // Function wrappers are emitted only when the script-facing API changes the
  // raw C++ call shape.
  for (const i_Fn of l_Functions) {
    if (!i_Fn.scripting || !i_Fn.wrapper_name) continue;
    t += generate_function_wrapper(i_Fn);
    t += `\n`;
  }

  // Enum helper implementations
  for (const i_Enum of p_Enums) {
    t += generate_enum_impl(i_Enum, p_ModuleName);
    t += `\n`;
  }

  // Struct helper implementations
  for (const i_Struct of p_Structs) {
    t += generate_struct_impl(i_Struct, p_ModuleName);
    t += `\n`;
  }

  // RTTI registration (safe to call before scripting is initialized)
  t += `void ${l_RegFnName}()\n`;
  t += `{\n`;

  for (const i_Enum of p_Enums) {
    t += generate_enum_rtti_registration(i_Enum, p_ModuleName);
    t += `\n`;
  }

  for (const i_Struct of p_Structs) {
    t += generate_struct_rtti_registration(i_Struct, p_ModuleName);
    t += `\n`;
  }

  t += `}\n`;
  t += `\n`;

  // Scripting registration (call after scripting engine is initialized)
  t += `void ${l_RegFnName}_scripting()\n`;
  t += `{\n`;

  for (const i_Enum of p_Enums) {
    if (!i_Enum.scripting) continue;
    t += generate_enum_scripting_registration(i_Enum);
    t += `\n`;
  }

  for (const i_Struct of p_Structs) {
    if (!i_Struct.scripting) continue;
    t += generate_struct_scripting_registration(i_Struct);
    t += `\n`;
  }

  for (const i_Fn of l_Functions) {
    if (!i_Fn.scripting) continue;
    t += generate_function_registration(i_Fn);
    t += `\n`;
  }

  t += `}\n`;

  return t;
}

module.exports = { generate_header_file, generate_module_file, generate_struct_header_declarations };
