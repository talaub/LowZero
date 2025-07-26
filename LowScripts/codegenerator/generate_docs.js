const fs = require("fs");
const os = require("os");
const exec = require("child_process").execSync;
const YAML = require("yaml");

const {
  read_file,
  get_plain_type,
  get_accessor_type,
  is_reference_type,
  format,
  write,
  line,
  include,
  empty,
  get_marker_begin,
  get_marker_end,
  find_begin_marker_start,
  find_begin_marker_end,
  find_end_marker_start,
  find_end_marker_end,
  save_file,
  collect_enums_for_low,
  collect_enums_for_project,
  collect_types_for_project,
  collect_types_for_low,
  is_string_type,
  is_math_type,
  is_name_type,
  is_container_type,
} = require("./lib.js");

let g_Types = 0;

function has_type(name) {
  for (let type of g_Types) {
    if (type.name == name) {
      return true;
    }
  }
}

function get_type(name) {
  for (let type of g_Types) {
    if (type.name == name) {
      return type;
    }
  }

  return undefined;
}

function anchorify(str) {
  return str
    .split("_") // Split by underscores
    .map((word) => {
      return word;
    }) // Capitalize each word
    .join("-"); // Join with spaces
}

function prettify(str) {
  return str
    .split("_") // Split by underscores
    .map((word) => {
      if (word === "id") {
        return "ID";
      }
      return word.charAt(0).toUpperCase() + word.slice(1);
    }) // Capitalize each word
    .join(" "); // Join with spaces
}

function escape_type(p_T) {
  let l_T = p_T;
  l_T = l_T.replaceAll(":", "&#58;");
  l_T = l_T.replaceAll("<", "&#60;");
  l_T = l_T.replaceAll(">", "&#62;");
  //l_T = l_T.replaceAll("::", " ");
  //l_T = l_T.replaceAll("<", " ");
  //l_T = l_T.replaceAll(">", " ");

  return l_T;
}

function get_friendly_type_name(p_Type) {
  if (is_string_type(p_Type)) {
    return "String";
  }
  if (is_name_type(p_Type)) {
    return "Name";
  }
  return escape_type(p_Type);
}

function display_type(p_Type) {
  const l_Parts = p_Type.split("::");
  const l_Type = l_Parts[l_Parts.length - 1];

  const l_RefType = get_type(l_Type);

  if (l_RefType) {
    return `<a title="${escape_type(p_Type)}" href="${l_RefType.page_url}">${get_friendly_type_name(p_Type)}</a>`;
  }

  return `<span title="${escape_type(p_Type)}">${get_friendly_type_name(p_Type)}</span>`;
}

function pin_type(p_Type, p_IsHandle = false, p_IsEnum = false) {
  if (is_string_type(p_Type)) {
    return "string";
  }
  if (is_name_type(p_Type)) {
    return "string";
  }
  if (p_IsHandle) {
    return "handle";
  }
  if (p_IsEnum) {
    return "enum";
  }
  if (p_Type === "bool") {
    return "bool";
  }
  return "number";
}

function generate_type(p_Type) {
  let t = "";
  t += line(`import Tabs from '@theme/Tabs';`);
  t += line(`import TabItem from '@theme/TabItem';`);
  t += line(`import FlodeNode from '@site/src/components/FlodeNode';`);
  t += line(`import '@vscode/codicons/dist/codicon.css';`);
  t += empty();

  t += line(`# ${p_Type.name}`);
  t += empty();

  if (p_Type.description) {
    t += line(`${p_Type.description}`);
    t += empty();
  }

  t += line(`## Properties`);
  t += empty();

  t += line(`| Name | Tech name | Type |`);
  t += line(`| -------- | -------- | -------- |`);
  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    let l = `<span style={{display: "inlineBlock", marginLeft: "10px", fontSize: "15px"}}>`;
    if (i_Prop.expose_scripting) {
      l += `<span title="Accessible in scripting" class="codicon codicon-code" />`;
    }
    l += `</span>`;
    t += line(
      `| <a href="#${anchorify(i_PropName)}">${prettify(i_PropName)}</a> ${l} | ${i_PropName} | ${display_type(i_Prop.type)} |`,
    );
  }
  if (p_Type.virtual_properties) {
    for (let [i_VPropName, i_VProp] of Object.entries(
      p_Type.virtual_properties,
    )) {
      let l = `| <a href="#${anchorify(i_VPropName)}">${prettify(i_VPropName)}</a>`;
      l += `<span style={{display: "inlineBlock", marginLeft: "10px", fontSize: "15px"}}>`;
      l += `<span title="Virtual property" class="codicon codicon-live-share" />`;
      l += `</span>`;
      l += `| ${i_VPropName} | ${display_type(i_VProp.type)} |`;
      t += line(l);
    }
  }

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    t += line(`### ${prettify(i_PropName)}`);
    t += empty();
    t += line(
      `<i style={{display: "block", marginTop: "-20px"}}>${i_PropName}</i>`,
    );
    t += empty();
    t += line(
      `<span style={{display: "block", fontSize: "18px", marginTop: "10px", marginBottom: "20px"}}>${escape_type(i_Prop.type)}</span>`,
    );
    t += empty();

    if (i_Prop.description) {
      t += line(`${i_Prop.description}`);
      t += empty();
    }

    if (!i_Prop.expose_scripting) {
      t += line(`:::warning`);
      t += line(
        `This property cannot be accessed in code scripting and flode.`,
      );
      t += line(`:::`);

      t += empty();
    }

    t += empty();
    t += line(`<Tabs>`);
    t += line(`<TabItem value="get" label="Get" default>`);
    if (i_Prop.expose_scripting) {
      t += line(`<FlodeNode`);
      t += line(
        `icon={<span style={{fontSize: "35px"}} className="codicon codicon-symbol-field"/>}`,
      );
      t += line(`topColor="#4c83c3"`);
      t += line(`title="Get ${prettify(i_PropName)}"`);
      t += line(`subtitle="${prettify(p_Type.name)}"`);
      t += line(`inputs={[`);
      t += line(`{label:"${p_Type.name}", type: "handle"}`);
      t += line(`]}`);
      t += line(`outputs={[`);
      t += line(
        `{type: "${pin_type(i_Prop.type, i_Prop.handle, i_Prop.enum)}"}`,
      );
      t += line(`]}`);
      t += line("/>");
    }
    t += line("```cpp");
    t += line(
      `${get_plain_type(i_Prop.type)} l_Value = l_${p_Type.name}.${i_Prop.getter_name}();`,
    );
    t += line("```");
    t += line(`</TabItem>`);
    t += line(`<TabItem value="set" label="Set">`);
    if (i_Prop.expose_scripting) {
      t += line(`<FlodeNode`);
      t += line(
        `icon={<span style={{fontSize: "35px"}} className="codicon codicon-symbol-field"/>}`,
      );
      t += line(`topColor="#4c83c3"`);
      t += line(`title="Set ${prettify(i_PropName)}"`);
      t += line(`subtitle="${prettify(p_Type.name)}"`);
      t += line(`inputs={[`);
      t += line(`{type: "exec"},`);
      t += line(`{label:"${p_Type.name}", type: "handle"},`);
      t += line(
        `{type: "${pin_type(i_Prop.type, i_Prop.handle, i_Prop.enum)}"}`,
      );
      t += line(`]}`);
      t += line(`outputs={[`);
      t += line(`{type: "exec"}`);
      t += line(`]}`);
      t += line("/>");
    }
    t += line("```cpp");
    t += line(`l_${p_Type.name}.${i_Prop.setter_name}(l_Value);`);
    t += line("```");
    t += line(`</TabItem>`);
    t += line(`</Tabs>`);
  }

  t += line(`## Functions`);
  t += empty();

  t += line(`| Name | Tech name | Return type |`);
  t += line(`| -------- | -------- | -------- |`);
  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      if (i_Func.virtual_property) {
        continue;
      }
      let l = `<span style={{display: "inlineBlock", marginLeft: "10px", fontSize: "15px"}}>`;
      if (i_Func.expose_scripting) {
        l += `<span title="Accessible in scripting" class="codicon codicon-code" />`;
      }
      l += `</span>`;
      t += line(
        `| <a href="#${anchorify(i_FuncName)}">${prettify(i_FuncName)}</a> ${l} | ${i_FuncName} | ${display_type(i_Func.return_type)} |`,
      );
    }
  }

  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      if (i_Func.virtual_property) {
        continue;
      }
      t += line(`### ${prettify(i_FuncName)}`);
      t += empty();
      t += line(
        `<i style={{display: "block", marginTop: "-20px"}}>${i_FuncName}</i>`,
      );
      t += empty();
      t += line(
        `<span style={{display: "block", fontSize: "18px", marginTop: "10px", marginBottom: "20px"}}>${escape_type(i_Func.return_type)}</span>`,
      );
      t += empty();

      if (i_Func.description) {
        t += line(`${i_Func.description}`);
        t += empty();
      }

      if (!i_Func.expose_scripting) {
        t += line(`:::warning`);
        t += line(
          `This function cannot be accessed in code scripting and flode.`,
        );
        t += line(`:::`);
      }

      if (i_Func.parameters) {
        t += line("#### Parameters");
        t += empty();
        t += line(`| Name | Tech name | Type |`);
        t += line(`| -------- | -------- | -------- |`);
        for (const i_Param of i_Func.parameters) {
          t += line(
            `| ${prettify(i_Param.base_name)} | ${i_Param.name} | ${display_type(i_Param.type)} |`,
          );
        }
      }

      t += empty();
      let l_ParamString = "";
      let l_Count = 0;
      if (i_Func.parameters) {
        for (const i_Param of i_Func.parameters) {
          if (l_Count > 0) {
            l_ParamString += ", ";
          }
          l_ParamString += i_Param.local_name;
          l_Count++;
        }
      }
      t += line("```cpp");
      t += line(`l_${p_Type.name}.${i_FuncName}(${l_ParamString});`);
      t += line("```");
    }
  }

  save_file(p_Type.docs_path, t);
}

function main() {
  let l_Path = `${__dirname}/../../`;
  let l_Project = false;
  if (process.argv.length > 2) {
    l_Path = process.argv[2];
    l_Project = true;
  }
  if (!l_Path.endsWith("/") && !l_Path.endsWith("\\")) {
    l_Path += "/";
  }

  let l_GeneratedSomething = false;

  let l_Types = 0;
  if (l_Project) {
    l_Types = collect_types_for_project(l_Path);
  } else {
    l_Types = collect_types_for_low(l_Path);
  }

  for (let i_Type of l_Types) {
    i_Type.page_url = `/docs/types/${i_Type.name.toLowerCase()}`;
  }

  g_Types = l_Types;

  for (const i_Type of l_Types) {
    generate_type(i_Type);
  }
}

main();
