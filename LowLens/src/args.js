'use strict';

// Depth-aware comma split — respects (), [], <>
function split_args(p_Text) {
  const l_Parts = [];
  let l_Current = '';
  let l_Depth = 0;

  for (let i = 0; i < p_Text.length; i++) {
    const l_C = p_Text[i];
    if (l_C === '(' || l_C === '[' || l_C === '<') l_Depth++;
    else if (l_C === ')' || l_C === ']' || l_C === '>') l_Depth--;

    if (l_C === ',' && l_Depth === 0) {
      if (l_Current.trim()) l_Parts.push(l_Current.trim());
      l_Current = '';
    } else {
      l_Current += l_C;
    }
  }

  if (l_Current.trim()) l_Parts.push(l_Current.trim());
  return l_Parts;
}

// Parse a single value token:
//   "foo"      → string
//   (k=v, ...) → object  (recursive)
//   [v, ...]   → array   (recursive)
//   1.0 / 42   → number
//   bare word  → string (identifier)
function parse_value(p_Text) {
  const l_T = p_Text.trim();

  if ((l_T.startsWith('"') && l_T.endsWith('"')) ||
      (l_T.startsWith("'") && l_T.endsWith("'"))) {
    return l_T.slice(1, -1);
  }

  if (l_T.startsWith('(') && l_T.endsWith(')')) {
    return parse_args(l_T.slice(1, -1));
  }

  if (l_T.startsWith('[') && l_T.endsWith(']')) {
    return split_args(l_T.slice(1, -1)).map(parse_value);
  }

  const l_Num = Number(l_T);
  if (l_T !== '' && !isNaN(l_Num)) return l_Num;

  return l_T;
}

// Parse a comma-separated key[=value] argument list into a plain JS object.
//
// Examples:
//   "scripting"                                  → { scripting: true }
//   'bind_name="foo"'                            → { bind_name: "foo" }
//   'vs=(icon="route", color=[1.0, 0.0, 0.0])'  → { vs: { icon: "route", color: [1.0, 0.0, 0.0] } }
//
// Bare keys map to `true`. Values are strings, numbers, arrays, or nested objects.
function parse_args(p_Text) {
  const l_Result = {};
  for (const i_Part of split_args(p_Text)) {
    const l_EqIdx = i_Part.indexOf('=');
    if (l_EqIdx === -1) {
      l_Result[i_Part.trim()] = true;
    } else {
      const l_Key = i_Part.slice(0, l_EqIdx).trim();
      const l_Val = i_Part.slice(l_EqIdx + 1).trim();
      l_Result[l_Key] = parse_value(l_Val);
    }
  }
  return l_Result;
}

module.exports = { parse_args };
