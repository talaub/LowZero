'use strict';

function node_text(p_Node, p_Src) {
  return p_Src.slice(p_Node.startIndex, p_Node.endIndex).trim();
}

function strip_declaration_macros(p_Text) {
  return p_Text.replace(/\b[A-Z][A-Z0-9_]*_API\b/g, '').replace(/\s+/g, ' ').trim();
}

function is_declaration_macro_identifier(p_Name) {
  return /^[A-Z][A-Z0-9_]*_API$/.test(p_Name);
}

function parse_macro_args_text(p_RawArgs) {
  const l_Args = {};
  if (!p_RawArgs.trim()) return l_Args;

  for (const i_Part of p_RawArgs.split(',')) {
    const [l_Key, ...l_Rest] = i_Part.split('=');
    const l_TrimmedKey = l_Key.trim();
    if (!l_TrimmedKey) continue;

    if (l_Rest.length > 0) {
      l_Args[l_TrimmedKey] = l_Rest.join('=').trim().replace(/^["']|["']$/g, '');
    } else {
      l_Args[l_TrimmedKey] = true;
    }
  }

  return l_Args;
}

function extract_low_param(p_ParamText) {
  const l_Match = p_ParamText.match(/^LOW_PARAM\s*\(([^)]*)\)\s*/);
  if (!l_Match) return { text: p_ParamText, args: {} };

  return {
    text: p_ParamText.slice(l_Match[0].length).trim(),
    args: parse_macro_args_text(l_Match[1]),
  };
}

function extract_param_from_text(p_ParamText) {
  const { text: l_Text, args: l_ParamArgs } = extract_low_param(p_ParamText);
  const l_NoDefault = l_Text.replace(/\s*=\s*.*$/, '').trim();
  const l_NameMatch = l_NoDefault.match(/([A-Za-z_][A-Za-z0-9_]*)\s*$/);

  if (!l_NameMatch) {
    return {
      type: l_NoDefault,
      name: '',
      bind_name: '',
      param_args: l_ParamArgs,
    };
  }

  const l_Name = l_NameMatch[1];
  const l_Type = l_NoDefault.slice(0, l_NameMatch.index).trim();

  return {
    type: l_Type,
    name: l_Name,
    bind_name: l_ParamArgs.bind_name || l_Name,
    param_args: l_ParamArgs,
  };
}

function split_parameter_list_text(p_ParamListText) {
  const l_Inner = p_ParamListText.trim().replace(/^\(/, '').replace(/\)$/, '');
  const l_Params = [];
  let l_Current = '';
  let l_Depth = 0;

  for (let i = 0; i < l_Inner.length; ++i) {
    const l_Char = l_Inner[i];

    if (l_Char === '(' || l_Char === '<' || l_Char === '[') {
      l_Depth++;
    } else if (l_Char === ')' || l_Char === '>' || l_Char === ']') {
      l_Depth = Math.max(0, l_Depth - 1);
    }

    if (l_Char === ',' && l_Depth === 0) {
      if (l_Current.trim()) l_Params.push(l_Current.trim());
      l_Current = '';
    } else {
      l_Current += l_Char;
    }
  }

  if (l_Current.trim()) l_Params.push(l_Current.trim());
  return l_Params;
}

function extract_declarator_name(p_Node, p_Src) {
  if (!p_Node) return '';
  if (p_Node.type === 'identifier') return node_text(p_Node, p_Src);

  for (const i_Child of p_Node.namedChildren) {
    const l_Result = extract_declarator_name(i_Child, p_Src);
    if (l_Result) return l_Result;
  }
  return node_text(p_Node, p_Src);
}

function extract_params(p_ParamListNode, p_Src) {
  return split_parameter_list_text(node_text(p_ParamListNode, p_Src))
      .map(extract_param_from_text);
}

function extract_function_from_text(p_Node, p_Src) {
  const l_Text = strip_declaration_macros(node_text(p_Node, p_Src).replace(/;\s*$/, ''));
  const l_Match = l_Text.match(/^(.*?)\s+([A-Za-z_][A-Za-z0-9_:]*)\s*\(([\s\S]*)\)\s*(?:const)?$/);
  if (!l_Match) return null;

  return {
    return_type: l_Match[1].trim(),
    name: l_Match[2].trim().split('::').pop(),
    params: split_parameter_list_text(`(${l_Match[3]})`).map(extract_param_from_text),
  };
}

function extract_function(p_Node, p_Src) {
  let l_ReturnType = '';
  let l_Name = '';
  let l_Params = [];
  let l_Qualifiers = [];

  for (const i_Child of p_Node.namedChildren) {
    switch (i_Child.type) {
      case 'type_qualifier':
      case 'storage_class_specifier':
        l_Qualifiers.push(node_text(i_Child, p_Src));
        break;

      case 'primitive_type':
      case 'type_identifier':
      case 'scoped_type_identifier':
      case 'qualified_identifier':
      case 'template_type':
      case 'sized_type_specifier':
        l_ReturnType = node_text(i_Child, p_Src);
        break;

      case 'function_declarator': {
        const l_NameNode = i_Child.namedChildren.find(
          n => n.type === 'identifier' ||
               n.type === 'scoped_identifier' ||
               n.type === 'field_identifier'
        );
        if (l_NameNode) l_Name = node_text(l_NameNode, p_Src);

        const l_ParamList = i_Child.namedChildren.find(
          n => n.type === 'parameter_list'
        );
        if (l_ParamList) l_Params = extract_params(l_ParamList, p_Src);
        break;
      }

      case 'pointer_declarator':
      case 'reference_declarator': {
        const l_FuncDecl = i_Child.namedChildren.find(
          n => n.type === 'function_declarator'
        );
        if (l_FuncDecl) {
          l_ReturnType = (l_ReturnType + '*').trim();
          const l_NameNode = l_FuncDecl.namedChildren.find(
            n => n.type === 'identifier' || n.type === 'scoped_identifier'
          );
          if (l_NameNode) l_Name = node_text(l_NameNode, p_Src);
          const l_ParamList = l_FuncDecl.namedChildren.find(
            n => n.type === 'parameter_list'
          );
          if (l_ParamList) l_Params = extract_params(l_ParamList, p_Src);
        }
        break;
      }
    }
  }

  if (!l_ReturnType || !l_Name || is_declaration_macro_identifier(l_Name) || l_Params.length === 0) {
    const l_Fallback = extract_function_from_text(p_Node, p_Src);
    if (l_Fallback) {
      l_ReturnType = l_Fallback.return_type;
      l_Name = l_Fallback.name;
      l_Params = l_Fallback.params;
    }
  }

  return { name: l_Name, return_type: l_ReturnType, params: l_Params, qualifiers: l_Qualifiers };
}

function find_enum_specifier(p_Node) {
  if (p_Node.type === 'enum_specifier') return p_Node;
  if (p_Node.type === 'declaration') {
    return p_Node.namedChildren.find(n => n.type === 'enum_specifier') || null;
  }
  return null;
}

function extract_enum(p_Node, p_Src) {
  const l_EnumSpec = find_enum_specifier(p_Node);
  if (!l_EnumSpec) return null;

  const l_NameNode = l_EnumSpec.childForFieldName
    ? l_EnumSpec.childForFieldName('name')
    : l_EnumSpec.namedChildren.find(n => n.type === 'type_identifier');
  if (!l_NameNode) return null;

  const l_Name = node_text(l_NameNode, p_Src);

  const l_Body = l_EnumSpec.namedChildren.find(n => n.type === 'enumerator_list');
  if (!l_Body) return null;

  const l_Values = [];
  let l_NextValue = 0;

  for (const i_Enumerator of l_Body.namedChildren) {
    if (i_Enumerator.type !== 'enumerator') continue;

    const l_EntryNameNode = i_Enumerator.childForFieldName
      ? i_Enumerator.childForFieldName('name')
      : i_Enumerator.namedChildren[0];
    if (!l_EntryNameNode) continue;

    const l_EntryName = node_text(l_EntryNameNode, p_Src);

    const l_ValueNode = i_Enumerator.childForFieldName
      ? i_Enumerator.childForFieldName('value')
      : i_Enumerator.namedChildren[1];

    if (l_ValueNode) {
      const l_Parsed = parseInt(node_text(l_ValueNode, p_Src));
      if (!isNaN(l_Parsed)) l_NextValue = l_Parsed;
    }

    l_Values.push({ name: l_EntryName, value: l_NextValue });
    l_NextValue++;
  }

  return { name: l_Name, values: l_Values };
}

function extract_struct(p_Node, p_Src) {
  // p_Node is a declaration or struct_specifier
  let l_StructSpec = p_Node;
  if (p_Node.type === 'declaration') {
    l_StructSpec = p_Node.namedChildren.find(
      n => n.type === 'struct_specifier'
    );
    if (!l_StructSpec) return null;
  }
  if (l_StructSpec.type !== 'struct_specifier') return null;

  const l_NameNode = l_StructSpec.childForFieldName
    ? l_StructSpec.childForFieldName('name')
    : l_StructSpec.namedChildren.find(n => n.type === 'type_identifier');
  if (!l_NameNode) return null;

  const l_Name = node_text(l_NameNode, p_Src);
  const l_Body = l_StructSpec.namedChildren.find(
    n => n.type === 'field_declaration_list'
  );
  if (!l_Body) return null;

  const l_Fields = [];
  let l_I = 0;
  const l_Children = l_Body.namedChildren;

  while (l_I < l_Children.length) {
    const l_Child = l_Children[l_I];

    // Inside a struct body, LOW_FIELD() is parsed as a declaration
    // containing a function_declarator whose name is LOW_FIELD
    if (l_Child.type === 'declaration') {
      const l_FuncDecl = l_Child.namedChildren.find(
        n => n.type === 'function_declarator'
      );
      if (l_FuncDecl) {
        const l_FnName = l_FuncDecl.namedChildren.find(
          n => n.type === 'identifier'
        );
        const l_MacroName = l_FnName ? node_text(l_FnName, p_Src) : '';
        if (l_MacroName === 'LOW_FIELD') {
          const l_Next = l_Children[l_I + 1];
          if (l_Next && l_Next.type === 'field_declaration') {
            l_Fields.push(extract_field(l_Next, p_Src));
            l_I += 2;
            continue;
          }
        }
        // LOW_BODY and other macros are skipped
      }
    }
    l_I++;
  }

  return { name: l_Name, fields: l_Fields };
}

function extract_field(p_Node, p_Src) {
  const l_Children = p_Node.namedChildren;
  if (l_Children.length === 0) return { type: '', name: '' };

  // The field name is always the field_identifier node
  const l_NameNode = l_Children.find(n => n.type === 'field_identifier');
  if (!l_NameNode) return { type: '', name: '' };

  // The type is everything before the field_identifier
  const l_NameIdx = l_Children.indexOf(l_NameNode);
  const l_Type = l_Children.slice(0, l_NameIdx).map(n => node_text(n, p_Src)).join(' ').trim();

  return { type: l_Type, name: node_text(l_NameNode, p_Src) };
}

function find_struct_specifier(p_Node) {
  if (p_Node.type === 'struct_specifier') return p_Node;
  if (p_Node.type === 'declaration') {
    return p_Node.namedChildren.find(n => n.type === 'struct_specifier') || null;
  }
  return null;
}

module.exports = { extract_function, extract_enum, find_enum_specifier, extract_struct, find_struct_specifier };
