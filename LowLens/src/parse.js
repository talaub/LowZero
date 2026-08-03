'use strict';

const { extract_function, extract_enum, find_enum_specifier, extract_struct, find_struct_specifier } = require('./extract');
const { parse_args } = require('./args');

const g_LowFunctionMacro = 'LOW_FUNCTION';
const g_LowEnumMacro = 'LOW_ENUM';
const g_LowStructMacro = 'LOW_STRUCT';

function find_matching_close(p_Src, p_OpenIdx) {
  const l_Pairs = { '(': ')', '[': ']' };
  const l_Stack = [l_Pairs[p_Src[p_OpenIdx]]];

  for (let i = p_OpenIdx + 1; i < p_Src.length; i++) {
    const l_C = p_Src[i];

    if (l_C === '"' || l_C === "'") {
      const l_Quote = l_C;
      i++;
      while (i < p_Src.length && p_Src[i] !== l_Quote) {
        if (p_Src[i] === '\\') i++;
        i++;
      }
      continue;
    }

    if (l_C === '(' || l_C === '[') {
      l_Stack.push(l_Pairs[l_C]);
    } else if (l_C === ')' || l_C === ']') {
      if (l_Stack[l_Stack.length - 1] === l_C) {
        l_Stack.pop();
        if (l_Stack.length === 0) return i;
      }
    }
  }

  return -1;
}

function sanitize_macro_arg_nesting(p_Src) {
  const l_Pattern = /\b(?:LOW_FUNCTION|LOW_ENUM|LOW_STRUCT)\s*\(/g;
  let l_Chars = null;
  let l_Match;

  while ((l_Match = l_Pattern.exec(p_Src))) {
    const l_OpenIdx = l_Match.index + l_Match[0].length - 1;
    const l_CloseIdx = find_matching_close(p_Src, l_OpenIdx);
    if (l_CloseIdx === -1) continue;

    let i = l_OpenIdx + 1;
    while (i < l_CloseIdx) {
      const l_C = p_Src[i];

      if (l_C === '"' || l_C === "'") {
        const l_Quote = l_C;
        i++;
        while (i < l_CloseIdx && p_Src[i] !== l_Quote) {
          if (p_Src[i] === '\\') i++;
          i++;
        }
        i++;
        continue;
      }

      if (l_C === '(' || l_C === '[') {
        const l_NestedClose = find_matching_close(p_Src, i);
        if (l_NestedClose === -1 || l_NestedClose > l_CloseIdx) {
          i++;
          continue;
        }

        if (!l_Chars) l_Chars = p_Src.split('');
        for (let k = i; k <= l_NestedClose; k++) {
          if (l_Chars[k] === '\n') continue;
          l_Chars[k] = (k === i || k === l_NestedClose) ? '"' : '_';
        }

        i = l_NestedClose + 1;
        continue;
      }

      i++;
    }

    l_Pattern.lastIndex = l_CloseIdx + 1;
  }

  return l_Chars ? l_Chars.join('') : p_Src;
}

const g_FunctionNodeTypes = new Set([
  'function_definition',
  'declaration',
]);

function get_namespace_name(p_Node, p_Src) {
  const l_NameNode = p_Node.namedChildren.find(n => n.type === 'namespace_identifier');
  return l_NameNode ? p_Src.slice(l_NameNode.startIndex, l_NameNode.endIndex).trim() : null;
}

function is_macro_call(p_Node, p_Src, p_MacroName) {
  return find_macro_call(p_Node, p_Src, p_MacroName) !== null;
}

function find_macro_call(p_Node, p_Src, p_MacroName) {
  let l_Node = p_Node;

  if (l_Node.type === 'expression_statement') {
    l_Node = l_Node.namedChildren[0];
    if (!l_Node) return null;
  }

  if (l_Node.type === 'call_expression') {
    const l_Fn = l_Node.namedChildren.find(n => n.type === 'identifier');
    if (l_Fn && p_Src.slice(l_Fn.startIndex, l_Fn.endIndex).trim() === p_MacroName) {
      return l_Node;
    }
  }

  for (const i_Child of l_Node.namedChildren) {
    const l_Result = find_macro_call(i_Child, p_Src, p_MacroName);
    if (l_Result) return l_Result;
  }

  return null;
}

function get_macro_args(p_Node, p_Src) {
  let l_Node =
      find_macro_call(p_Node, p_Src, g_LowFunctionMacro) ||
      find_macro_call(p_Node, p_Src, g_LowEnumMacro) ||
      find_macro_call(p_Node, p_Src, g_LowStructMacro) ||
      p_Node;
  if (l_Node.type === 'expression_statement') l_Node = l_Node.namedChildren[0];
  if (!l_Node || l_Node.type !== 'call_expression') return {};

  const l_ArgList = l_Node.namedChildren.find(n => n.type === 'argument_list');
  if (!l_ArgList) return {};

  const l_Raw = p_Src.slice(l_ArgList.startIndex + 1, l_ArgList.endIndex - 1).trim();
  return parse_args(l_Raw);
}

function walk_children(p_Children, p_Src, p_NamespaceStack, p_Functions, p_Enums, p_Structs) {
  let l_I = 0;
  while (l_I < p_Children.length) {
    const l_Node = p_Children[l_I];

    if (l_Node.type === 'namespace_definition') {
      const l_NsName = get_namespace_name(l_Node, p_Src);
      const l_BodyNode = l_Node.namedChildren.find(n => n.type === 'declaration_list');
      if (l_BodyNode) {
        const l_NextStack = l_NsName
          ? [...p_NamespaceStack, l_NsName]
          : p_NamespaceStack;
        walk_children(l_BodyNode.namedChildren, p_Src, l_NextStack, p_Functions, p_Enums, p_Structs);
      }
      l_I++;
      continue;
    }

    if (is_macro_call(l_Node, p_Src, g_LowFunctionMacro)) {
      const l_MacroArgs = get_macro_args(l_Node, p_Src);
      const l_Next = p_Children[l_I + 1];
      if (l_Next && g_FunctionNodeTypes.has(l_Next.type)) {
        const l_Fn = extract_function(l_Next, p_Src);
        p_Functions.push({
          ...l_Fn,
          namespace: p_NamespaceStack.join('::'),
          macro_args: l_MacroArgs,
          bind_name: l_MacroArgs.bind_name || l_Fn.name,
          bind_namespace: l_MacroArgs.bind_namespace || '',
          is_property: l_MacroArgs.property === true,
          scripting: l_MacroArgs.scripting === true,
        });
        l_I += 2;
        continue;
      }
    }

    if (is_macro_call(l_Node, p_Src, g_LowEnumMacro)) {
      const l_MacroArgs = get_macro_args(l_Node, p_Src);
      const l_Next = p_Children[l_I + 1];
      if (l_Next && find_enum_specifier(l_Next)) {
        const l_Enum = extract_enum(l_Next, p_Src);
        if (l_Enum) {
          p_Enums.push({
            ...l_Enum,
            namespace: p_NamespaceStack.join('::'),
            macro_args: l_MacroArgs,
            bind_name: l_MacroArgs.bind_name || l_Enum.name,
            bind_namespace: l_MacroArgs.bind_namespace || '',
            scripting: l_MacroArgs.scripting === true,
          });
          l_I += 2;
          continue;
        }
      }
    }

    if (is_macro_call(l_Node, p_Src, g_LowStructMacro)) {
      const l_MacroArgs = get_macro_args(l_Node, p_Src);
      const l_Next = p_Children[l_I + 1];
      if (l_Next && find_struct_specifier(l_Next)) {
        const l_Struct = extract_struct(l_Next, p_Src);
        if (l_Struct) {
          p_Structs.push({
            ...l_Struct,
            namespace: p_NamespaceStack.join('::'),
            macro_args: l_MacroArgs,
            bind_name: l_MacroArgs.bind_name || l_Struct.name,
            bind_namespace: l_MacroArgs.bind_namespace || '',
            scripting: l_MacroArgs.scripting === true,
          });
          l_I += 2;
          continue;
        }
      }
    }

    l_I++;
  }
}

function parse_file(p_Src, p_Tree) {
  const l_Functions = [];
  const l_Enums = [];
  const l_Structs = [];
  walk_children(p_Tree.rootNode.namedChildren, p_Src, [], l_Functions, l_Enums, l_Structs);
  return { functions: l_Functions, enums: l_Enums, structs: l_Structs };
}

module.exports = { parse_file, sanitize_macro_arg_nesting };
