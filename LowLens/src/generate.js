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
  { kinds: ['uint32_t', 'u32', 'unsigned int'],               kind: 'UInt32' },
  { kinds: ['uint64_t', 'u64'],                               kind: 'UInt64' },
  { kinds: ['float'],                                         kind: 'Float'  },
  { kinds: ['Low::Util::String', 'Util::String', 'String'],   kind: 'String' },
  { kinds: ['Low::Util::Name', 'Util::Name', 'Name'],         kind: 'Name'   },
];

function get_type_kind(p_Type) {
  const l_Clean = p_Type.replace(/\s*[*&]+\s*$/, '').trim();
  for (const i_Entry of g_TypeKindMap) {
    if (i_Entry.kinds.includes(l_Clean)) return i_Entry.kind;
  }
  return 'Handle';
}

function get_pointer_level(p_Type) {
  return (p_Type.match(/\*/g) || []).length;
}

function is_const(p_Type) {
  return p_Type.startsWith('const ');
}

function is_reference(p_Type) {
  return p_Type.trimEnd().endsWith('&');
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

function generate_type_info(p_VarName, p_Type, p_Indent) {
  const l_I = '  '.repeat(p_Indent);
  let t = '';
  t += `${l_I}${p_VarName}.kind = Low::Core::Scripting::TypeKind::${get_type_kind(p_Type)};\n`;
  const l_PtrLevel = get_pointer_level(p_Type);
  if (l_PtrLevel > 0) t += `${l_I}${p_VarName}.pointer_level = ${l_PtrLevel};\n`;
  if (is_const(p_Type))  t += `${l_I}${p_VarName}.constant = true;\n`;
  if (is_reference(p_Type)) t += `${l_I}${p_VarName}.reference = true;\n`;
  return t;
}

function generate_function_registration(p_Fn) {
  const l_FullName = p_Fn.namespace ? `${p_Fn.namespace}::${p_Fn.name}` : p_Fn.name;

  let t = '';
  t += `  {\n`;
  t += `    Low::Core::Scripting::FunctionInfo l_FunctionInfo;\n`;
  t += `    l_FunctionInfo.name = N(${p_Fn.name});\n`;
  t += `    l_FunctionInfo.bind_name = N(${p_Fn.bind_name});\n`;
  t += `    l_FunctionInfo.bind_namespace = "${p_Fn.bind_namespace}";\n`;
  t += `    l_FunctionInfo.is_property = ${p_Fn.is_property};\n`;
  t += `    l_FunctionInfo.ptr = (void*)&${l_FullName};\n`;
  t += `\n`;
  t += generate_type_info('l_FunctionInfo.return_type', p_Fn.return_type, 2);

  for (const i_Param of p_Fn.params) {
    t += `\n`;
    t += `    {\n`;
    t += `      Low::Core::Scripting::FunctionParameterInfo l_Param;\n`;
    t += `      l_Param.name = N(${i_Param.name});\n`;
    t += generate_type_info('l_Param.type', i_Param.type, 3);
    t += `      l_FunctionInfo.parameters.push_back(l_Param);\n`;
    t += `    }\n`;
  }

  t += `\n`;
  t += `    Low::Core::Scripting::register_function(l_FunctionInfo);\n`;
  t += `  }\n`;
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
    const l_AsType = db.get_as_type_string(i_Field.type);

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

  let t = '';
  t += `// AUTO-GENERATED by LowLens - do not edit manually\n`;
  t += `\n`;
  t += `#include "LowCoreScripting.h"\n`;
  t += `#include "LowUtilHandle.h"\n`;
  for (const i_Header of p_Headers) {
    t += `#include "${i_Header}"\n`;
  }
  t += `\n`;

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

  for (const i_Fn of p_Functions) {
    if (!i_Fn.scripting) continue;
    t += generate_function_registration(i_Fn);
    t += `\n`;
  }

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

  t += `}\n`;

  return t;
}

module.exports = { generate_header_file, generate_module_file, generate_struct_header_declarations };
