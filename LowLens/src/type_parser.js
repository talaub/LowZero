// Parses C++ type strings as they appear in YAML type configs and LowLens
// output, e.g. "const Math::Vector3&", "Util::List<Util::Map<Util::Name, Entity>>*",
// "unsigned int", "QueryHit *", "Util::Function<bool(RenderStep)>",
// "void(*)(float)". Not a full C++ parser: C arrays and nested names after
// templates (Foo<T>::Bar) are not supported.
//
// Every parsed node has a `kind`:
//   "type"     - name, args (template arguments), const/pointer/reference, container
//   "function" - returns, params (function signatures such as bool(RenderStep))
//   "value"    - an integer template argument (the 4 in Util::Array<float, 4>)
//   "variadic" - "..." in a parameter list
// Every node has an `args` array (empty for non-"type" nodes). Use
// type_children() to walk all nested types, including function signatures.

const QUALIFIERS = ["const", "volatile"];
const PRIMITIVE_WORDS = [
  "unsigned",
  "signed",
  "long",
  "short",
  "int",
  "char",
  "double",
];
const CONTAINERS = {
  List: "list",
  Set: "set",
  Map: "map",
  UnorderedMap: "map",
  EnumMask: "enum_mask",
};

function tokenize_type(p_String) {
  const l_Tokens = [];
  const l_Pattern = /\s*(::|&&|\.\.\.|[A-Za-z_]\w*|\d+|[<>,*&()])/y;
  while (l_Pattern.lastIndex < p_String.length) {
    const l_Start = l_Pattern.lastIndex;
    const l_Match = l_Pattern.exec(p_String);
    if (!l_Match) {
      if (/^\s*$/.test(p_String.slice(l_Start))) {
        break;
      }
      const l_Rest = p_String.slice(l_Start).trimStart();
      throw new Error(
        `Cannot parse type '${p_String}': unexpected '${l_Rest[0]}' at ${p_String.length - l_Rest.length}`,
      );
    }
    l_Tokens.push(l_Match[1]);
  }
  return l_Tokens;
}

function finish_type(p_Type) {
  const l_Segments = p_Type.name.split("::");
  p_Type.short_name = l_Segments[l_Segments.length - 1];
  p_Type.namespace = l_Segments.slice(0, -1).filter((i_Part) => i_Part);
  p_Type.container =
    p_Type.args.length > 0 ? CONTAINERS[p_Type.short_name] : undefined;

  // Canonical spelling without cv/ref/pointer, e.g. "Util::List<Entity>"
  p_Type.base =
    p_Type.args.length > 0
      ? `${p_Type.name}<${p_Type.args.map((i_Arg) => i_Arg.string).join(", ")}>`
      : p_Type.name;

  p_Type.string = `${p_Type.const ? "const " : ""}${p_Type.base}${suffix_string(p_Type)}`;
  return p_Type;
}

function suffix_string(p_Type) {
  let l_Suffix = "*".repeat(p_Type.pointer);
  if (p_Type.reference === "lvalue") {
    l_Suffix += "&";
  } else if (p_Type.reference === "rvalue") {
    l_Suffix += "&&";
  }
  return l_Suffix;
}

function finish_function(p_Type) {
  const l_Params = p_Type.params.map((i_Param) => i_Param.string).join(", ");
  p_Type.base = `${p_Type.returns.string}(${l_Params})`;
  p_Type.string =
    p_Type.pointer > 0
      ? `${p_Type.returns.string}(*)(${l_Params})`
      : p_Type.base;
  return p_Type;
}

// All nested types: template arguments, or return and parameter types of a
// function signature. Values and "..." have no children.
function type_children(p_Type) {
  if (p_Type.kind === "function") {
    return [p_Type.returns, ...p_Type.params];
  }
  return p_Type.args ?? [];
}

function parse_type_description(p_String) {
  const l_Tokens = tokenize_type(String(p_String));
  let l_Index = 0;

  const peek = () => l_Tokens[l_Index];
  const next = () => l_Tokens[l_Index++];
  const expect = (p_Token) => {
    const l_Token = next();
    if (l_Token !== p_Token) {
      throw new Error(
        `Cannot parse type '${p_String}': expected '${p_Token}', got '${l_Token}'`,
      );
    }
  };

  const is_identifier = (p_Token) =>
    p_Token !== undefined &&
    /^[A-Za-z_]\w*$/.test(p_Token) &&
    !QUALIFIERS.includes(p_Token);

  // "(A, B p_Name, ...)" -> parameter types; parameter names are dropped.
  function parse_parameter_list() {
    const l_Params = [];
    expect("(");
    if (peek() === "void" && l_Tokens[l_Index + 1] === ")") {
      next();
    }
    while (peek() !== ")") {
      if (peek() === "...") {
        next();
        l_Params.push({
          kind: "variadic",
          args: [],
          string: "...",
          base: "...",
        });
      } else {
        l_Params.push(parse_one(true));
      }
      if (peek() !== ",") {
        break;
      }
      next();
    }
    expect(")");
    return l_Params;
  }

  // Template arguments are types, or integer values ("Util::Array<float, 4>").
  function parse_template_arg() {
    if (/^\d+$/.test(peek())) {
      const l_Value = next();
      return { kind: "value", args: [], string: l_Value, base: l_Value };
    }
    return parse_one();
  }

  function parse_one(p_AllowName = false) {
    const l_Type = {
      kind: "type",
      const: false,
      volatile: false,
      const_pointer: false,
      name: "",
      args: [],
      pointer: 0,
      reference: null,
    };

    while (QUALIFIERS.includes(peek())) {
      l_Type[next()] = true;
    }

    if (PRIMITIVE_WORDS.includes(peek())) {
      // "unsigned int", "long long", "unsigned char", ...
      const l_Words = [];
      while (PRIMITIVE_WORDS.includes(peek())) {
        l_Words.push(next());
      }
      l_Type.name = l_Words.join(" ");
    } else {
      const l_Parts = [];
      if (peek() === "::") {
        next();
        l_Parts.push("");
      }
      if (!is_identifier(peek())) {
        throw new Error(
          `Cannot parse type '${p_String}': expected a type name, got '${peek()}'`,
        );
      }
      l_Parts.push(next());
      while (peek() === "::") {
        next();
        l_Parts.push(next());
      }
      l_Type.name = l_Parts.join("::");

      if (peek() === "<") {
        next();
        if (peek() !== ">") {
          l_Type.args.push(parse_template_arg());
          while (peek() === ",") {
            next();
            l_Type.args.push(parse_template_arg());
          }
        }
        expect(">");
      }
    }

    for (;;) {
      const l_Token = peek();
      if (QUALIFIERS.includes(l_Token)) {
        next();
        // "T const" is east const; "T* const" makes the pointer const.
        if (l_Type.pointer > 0) {
          l_Type.const_pointer = true;
        } else {
          l_Type[l_Token] = true;
        }
      } else if (l_Token === "*") {
        next();
        l_Type.pointer++;
      } else if (l_Token === "&") {
        next();
        l_Type.reference = "lvalue";
      } else if (l_Token === "&&") {
        next();
        l_Type.reference = "rvalue";
      } else {
        break;
      }
    }
    finish_type(l_Type);

    // Function signature: "bool(RenderStep)" or pointer "bool(*)(RenderStep)"
    if (peek() === "(") {
      let l_Pointer = 0;
      if (l_Tokens[l_Index + 1] === "*" && l_Tokens[l_Index + 2] === ")") {
        l_Index += 3;
        l_Pointer = 1;
      }
      return finish_function({
        kind: "function",
        args: [],
        returns: l_Type,
        params: parse_parameter_list(),
        pointer: l_Pointer,
      });
    }

    // Parameter names inside function signatures ("RenderStep p_Step").
    if (p_AllowName && is_identifier(peek())) {
      next();
    }
    return l_Type;
  }

  const l_Result = parse_one();
  if (l_Index < l_Tokens.length) {
    throw new Error(
      `Cannot parse type '${p_String}': unexpected '${l_Tokens[l_Index]}'`,
    );
  }
  return l_Result;
}

module.exports = { parse_type_description, tokenize_type, type_children };
