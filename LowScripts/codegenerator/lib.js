const fs = require("fs");
const os = require("os");
const exec = require("child_process").execSync;
const YAML = require("yaml");
const { assert } = require("console");
const { isArray } = require("util");
const path = require("path");

const {
  process_directory,
  parse_type_description,
  type_children,
} = require("../../LowLens/index.js");

const g_Directory = `${__dirname}/../../misteda/data/_internal/type_configs`;

let g_TypeIdMap = {};
const g_AllTypeIds = [];

let g_EnumIdMap = {};
const g_AllEnumIds = [];

const g_SupportedContainers = {
  List: "list",
  Set: "set",
  EnumMask: "enum_mask",
};

const g_ContainerPrefixes = ["Low::Util::", "Util::", ""];

const isObject = (value) => {
  return (
    typeof value === "object" &&
    value !== null &&
    !Array.isArray(value) &&
    !(value instanceof RegExp) &&
    !(value instanceof Date) &&
    !(value instanceof Set) &&
    !(value instanceof Map)
  );
};

function output_section(p_Name) {
  process.stdout.write(
    JSON.stringify({ type: "section", label: p_Name }) + "\n",
  );
}

function output_result(p_Text) {
  process.stdout.write(JSON.stringify({ type: "result", msg: p_Text }) + "\n");
}

function output_log(p_Level, p_Msg, p_Details, p_Hint) {
  const l_Obj = {
    type: "log",
    level: p_Level,
    msg: p_Msg,
  };

  if (p_Details) {
    l_Obj.details = p_Details;
  }
  if (p_Hint) {
    l_Obj.hint = p_Hint;
  }

  process.stdout.write(JSON.stringify(l_Obj) + "\n");
}
function output_info(p_Msg, p_Details, p_Hint) {
  output_log("info", p_Msg, p_Details, p_Hint);
}
function output_warn(p_Msg, p_Details, p_Hint) {
  output_log("warn", p_Msg, p_Details, p_Hint);
}
function output_error(p_Msg, p_Details, p_Hint) {
  output_log("error", p_Msg, p_Details, p_Hint);
}
function output_debug(p_Msg, p_Details, p_Hint) {
  output_log("debug", p_Msg, p_Details, p_Hint);
}
function output_detail(p_Msg, p_Details, p_Hint) {
  output_log("detail", p_Msg, p_Details, p_Hint);
}

function output_begin_task(p_Id, p_Label, p_Total) {
  const l_Obj = {
    type: "task_begin",
    id: p_Id,
    label: p_Label,
  };
  if (p_Total) {
    l_Obj.total = p_Total;
  }

  process.stdout.write(JSON.stringify(l_Obj) + "\n");

  return p_Id;
}
function output_update_task(p_Id, p_Done, p_Label) {
  const l_Obj = {
    type: "task_update",
    id: p_Id,
    done: p_Done,
  };
  if (p_Label) {
    l_Obj.label = p_Label;
  }

  process.stdout.write(JSON.stringify(l_Obj) + "\n");

  return p_Id;
}
function output_task_end(p_Id, p_Status) {
  process.stdout.write(
    JSON.stringify({ type: "task_end", id: p_Id, status: p_Status }) + "\n",
  );
}
function output_task_success(p_Id) {
  output_task_end(p_Id, "ok");
}
function output_task_failed(p_Id) {
  output_task_end(p_Id, "failed");
}
function output_task_cancelled(p_Id) {
  output_task_end(p_Id, "cancelled");
}

function write_file_if_changed(p_Path, p_Content) {
  if (!fs.existsSync(p_Path)) {
    fs.mkdirSync(path.dirname(p_Path), { recursive: true });
    fs.writeFileSync(p_Path, p_Content);
    return true;
  }

  const l_FileContent = fs.readFileSync(p_Path, {
    encoding: "utf8",
    flag: "r",
  });

  if (l_FileContent !== p_Content) {
    fs.writeFileSync(p_Path, p_Content);
    return true;
  }
  return false;
}

function strip_container_prefix(p_Name) {
  for (const i_Prefix of g_ContainerPrefixes) {
    if (i_Prefix.length > 0 && p_Name.startsWith(i_Prefix)) {
      return p_Name.substring(i_Prefix.length);
    }
  }
  return p_Name;
}

function parse_container(p_Name) {
  const l_Name = strip_container_prefix(p_Name);

  if (Object.prototype.hasOwnProperty.call(g_SupportedContainers, l_Name)) {
    return g_SupportedContainers[l_Name];
  }

  return undefined;
}

function split_template(p_Text) {
  const l_OpenIndex = p_Text.indexOf("<");

  if (l_OpenIndex < 0) {
    return { name: p_Text.trim(), args: [] };
  }

  if (!p_Text.endsWith(">")) {
    return undefined;
  }

  const l_Name = p_Text.substring(0, l_OpenIndex).trim();
  const l_Body = p_Text.substring(l_OpenIndex + 1, p_Text.length - 1);

  const l_Args = [];
  let l_Depth = 0;
  let l_Current = "";

  for (const i_Char of l_Body) {
    if (i_Char === "<" || i_Char === "(") {
      l_Depth++;
    } else if (i_Char === ">" || i_Char === ")") {
      l_Depth--;
      if (l_Depth < 0) {
        return undefined;
      }
    }

    if (i_Char === "," && l_Depth === 0) {
      l_Args.push(l_Current.trim());
      l_Current = "";
      continue;
    }

    l_Current += i_Char;
  }

  if (l_Depth !== 0) {
    return undefined;
  }

  if (l_Current.trim().length > 0) {
    l_Args.push(l_Current.trim());
  }

  return { name: l_Name, args: l_Args };
}

function parse_type(p_Text) {
  const l_TypeObject = {
    text: p_Text.trim(),
    name: undefined,
    plain: undefined,
    template_args: [],
    is_const: false,
    reference: false,
    pointer_level: 0,
    container: undefined,
    inner: undefined,
    supported: true,
  };

  let l_Text = l_TypeObject.text;

  if (l_Text.startsWith("const ")) {
    l_TypeObject.is_const = true;
    l_Text = l_Text.substring("const ".length).trim();
  }

  if (l_Text.endsWith("&")) {
    l_TypeObject.reference = true;
    l_Text = l_Text.substring(0, l_Text.length - 1).trim();
  }

  while (l_Text.endsWith("*")) {
    l_TypeObject.pointer_level++;
    l_Text = l_Text.substring(0, l_Text.length - 1).trim();
  }

  l_TypeObject.plain = l_Text;

  const l_Split = split_template(l_Text);
  if (!l_Split) {
    console.log(`WARNING: Could not parse type '${p_Text}'`);
    l_TypeObject.name = l_Text;
    l_TypeObject.supported = false;
    return l_TypeObject;
  }

  l_TypeObject.name = l_Split.name;
  l_TypeObject.template_args = l_Split.args;

  if (l_Split.args.length === 0) {
    return l_TypeObject;
  }

  l_TypeObject.container = parse_container(l_Split.name);

  if (!l_TypeObject.container || l_Split.args.length !== 1) {
    l_TypeObject.container = undefined;
    l_TypeObject.supported = false;
    return l_TypeObject;
  }

  l_TypeObject.inner = parse_type(l_Split.args[0]);

  // Only one container level is supported for now
  if (
    l_TypeObject.inner.template_args.length > 0 ||
    !l_TypeObject.inner.supported
  ) {
    l_TypeObject.supported = false;
  }

  return l_TypeObject;
}

// The type that flags like handle/enum refer to: the element type for
// containers, the type itself otherwise.
function get_element_type(p_TypeObject) {
  if (p_TypeObject.container) {
    return p_TypeObject.inner;
  }
  return p_TypeObject;
}

function apply_type_object(p_TypeName, p_Prop) {
  p_Prop.type_object = parse_type(p_Prop.type);

  if (
    !p_Prop.type_object.supported &&
    (p_Prop.handle || p_Prop.enum || p_Prop.lens_enum || p_Prop.lens_struct)
  ) {
    console.log(
      `WARNING: ${p_TypeName}.${p_Prop.name} has handle/enum/struct flags but its type '${p_Prop.type}' is not supported, flags will be ignored for it`,
    );
  }
}

function read_yaml_file(p_FilePath) {
  const l_FileContent = read_file(p_FilePath);
  return YAML.parse(l_FileContent);
}

function get_scripting_type(p_RawType) {
  if (p_RawType.endsWith("Math::UVector2")) {
    return "UVector2";
  }
  if (p_RawType.endsWith("Math::Vector3")) {
    return "Vector3";
  }
  if (p_RawType.endsWith("Math::ColorRGB")) {
    return "Vector3";
  }
  if (p_RawType.endsWith("Math::Vector2")) {
    return "Vector2";
  }
  if (p_RawType.endsWith("Math::Color")) {
    return "Vector4";
  }
  if (p_RawType.endsWith("Math::Vector4")) {
    return "Vector4";
  }
  if (p_RawType.endsWith("Math::Quaternion")) {
    return "Quaternion";
  }
  if (p_RawType.endsWith("Util::Handle")) {
    return "Handle";
  }
  if (p_RawType.endsWith("Util::Name")) {
    return "Name";
  }

  if (p_RawType == "uint64_t") {
    return "u64";
  }
  if (p_RawType == "uint32_t") {
    return "u32";
  }
  if (p_RawType == "uint16_t") {
    return "u16";
  }
  if (p_RawType == "uint8_t") {
    return "u8";
  }
  if (p_RawType.endsWith("Util::String")) {
    return "string";
  }
  if (p_RawType.endsWith("Util::UniqueId")) {
    return "u64";
  }

  return p_RawType;
}

function separator() {
  let t = "// --------------------------\n";
  return t;
}

function line(l, n = 0) {
  let t = "";
  for (let i = 0; i < n; i++) {
    t += "  ";
  }
  return t + `${l}\n`;
}

function write(l, n = 0) {
  let t = "";
  for (let i = 0; i < n; i++) {
    t += "  ";
  }
  return t + `${l}`;
}

function include(p) {
  return line(`#include "${p}"`);
}

function empty() {
  return line("");
}

function format(p_FilePath, p_Content) {
  const l_TmpPath = `${p_FilePath}.tmp`;

  fs.writeFileSync(l_TmpPath, p_Content);
  const l_Formatted = exec(`clang-format ${l_TmpPath}`).toString();
  fs.unlinkSync(l_TmpPath);

  return l_Formatted;
}

function load_project_info(p_FullProjectPath) {
  const l_ProjectModulesPath = `${p_FullProjectPath}/modules`;
  const l_ProjectConfigPath = `${p_FullProjectPath}/project.yaml`;

  output_info(`Looking for project at ${p_FullProjectPath}`);
  assert(fs.existsSync(p_FullProjectPath), "Project directory does not exist");
  assert(
    fs.existsSync(l_ProjectConfigPath),
    "Project config file does not exist",
  );
  assert(
    fs.existsSync(l_ProjectModulesPath),
    "Project's modules directory does not exist",
  );

  const l_ProjectConfig = read_yaml_file(l_ProjectConfigPath);
  l_ProjectConfig.config_path = l_ProjectConfigPath;
  l_ProjectConfig.modules_path = l_ProjectModulesPath;
  l_ProjectConfig.full_path = p_FullProjectPath;

  l_ProjectConfig.root_cmake_path = `${p_FullProjectPath}/.low/cmake/CMakeLists.txt`;

  assert(l_ProjectConfig.engine, "No engine defined in project config");

  const l_ProjectModuleDirectories = fs
    .readdirSync(l_ProjectModulesPath, { withFileTypes: true })
    .filter((it) => it.isDirectory())
    .map((it) => it.name);

  const l_Modules = [];

  for (const i_ModuleDirectory of l_ProjectModuleDirectories) {
    const i_ModulePath = `${l_ProjectModulesPath}/${i_ModuleDirectory}`;
    assert(
      fs.existsSync(i_ModulePath),
      `Could not find module directory ${i_ModulePath}`,
    );

    const i_ModuleConfigPath = `${i_ModulePath}/module.yaml`;

    if (!fs.existsSync(i_ModulePath)) {
      continue;
    }

    const i_ModuleConfig = read_yaml_file(i_ModuleConfigPath);
    //console.log(`📂 Found module ${i_ModuleDirectory}`);
    const i_ModuleSettings = {
      name: i_ModuleDirectory,
      path: i_ModulePath,
      api_header_path: `${i_ModulePath}/include/${l_ProjectConfig.name}${i_ModuleDirectory}Api.h`,
      api_header_path: `${p_FullProjectPath}/.low/generated/modules/${i_ModuleDirectory}/include/${l_ProjectConfig.name}${i_ModuleDirectory}Api.h`,
      cmake_path: `${p_FullProjectPath}/.low/cmake/modules/${i_ModuleDirectory}/CMakeLists.txt`,
      exports_marker: `${l_ProjectConfig.name.toLowerCase()}_${i_ModuleDirectory.toLowerCase()}_EXPORTS`,
      static_marker: `${l_ProjectConfig.name.toLowerCase()}_${i_ModuleDirectory.toLowerCase()}_BUILD_STATIC`,
      project_name: l_ProjectConfig.name,
      project_path: p_FullProjectPath,
      api_macro: `${l_ProjectConfig.name.toUpperCase()}_${i_ModuleDirectory.toUpperCase()}_API`,
      no_export_macro: `${l_ProjectConfig.name.toUpperCase()}_${i_ModuleDirectory.toUpperCase()}_NO_EXPORT`,
      deprecated_macro: `${l_ProjectConfig.name.toUpperCase()}_${i_ModuleDirectory.toUpperCase()}_DEPRECATED`,
      deprecated_export_macro: `${l_ProjectConfig.name.toUpperCase()}_${i_ModuleDirectory.toUpperCase()}_DEPRECATED_EXPORT`,
      deprecated_no_export_macro: `${l_ProjectConfig.name.toUpperCase()}_${i_ModuleDirectory.toUpperCase()}_DEPRECATED_NO_EXPORT`,
      ...i_ModuleConfig,
    };

    l_Modules.push(i_ModuleSettings);
  }

  l_ProjectConfig.modules = l_Modules;

  return l_ProjectConfig;
}

function get_unused_type_id(p_Project) {
  let id = 1;
  if (p_Project) {
    id = 30001;
  }

  while (g_AllTypeIds.includes(id)) {
    id++;
  }

  return id;
}

function get_unused_enum_id(p_Project) {
  let id = 1;
  if (p_Project) {
    id = 30001;
  }

  while (g_AllEnumIds.includes(id)) {
    id++;
  }

  return id;
}

function read_file(p_FilePath) {
  return fs.readFileSync(p_FilePath, { encoding: "utf8", flag: "r" });
}

function process_enum_file(p_Path, p_FileName, p_Project) {
  let l_FilePath = `${p_Path}LowData/type_configs/${p_FileName}`;
  if (p_Project) {
    l_FilePath = `${p_Path}data/_internal/type_configs/${p_FileName}`;
  }

  const l_FileContent = read_file(l_FilePath);

  const l_Config = YAML.parse(l_FileContent);

  const l_Enums = [];

  for (let [i_EnumName, i_Enum] of Object.entries(l_Config.enums)) {
    i_Enum.name = i_EnumName;
    i_Enum.module = l_Config.module;
    i_Enum.prefix = l_Config.prefix ? l_Config.prefix : l_Config.module;
    i_Enum.namespace = l_Config.namespace;
    i_Enum.scripting_namespace = "";
    if (!i_Enum.identifier_name) {
      i_Enum.identifier_name = i_Enum.name;
    }

    i_Enum.api_file = `${i_Enum.module}Api.h`;
    if (l_Config.api_file) {
      i_Enum.api_file = l_Config.api_file;
    }

    i_Enum.dll_macro = l_Config.dll_macro;

    i_Enum.namespace_string = "";

    i_Enum.identifier = `${i_Enum.module}:${i_Enum.identifier_name}`;

    for (let i = 0; i < i_Enum.namespace.length; ++i) {
      if (i) {
        i_Enum.namespace_string += "::";
      }
      i_Enum.namespace_string += i_Enum.namespace[i];
    }

    i_Enum.module_path = `${p_Path}${i_Enum.module}`;
    if (p_Project) {
      i_Enum.module_path = `${p_Path}modules/${i_Enum.module}`;
    }
    i_Enum.header_file_name = `${i_Enum.prefix}${i_Enum.name}.h`;
    i_Enum.header_file_path = `${i_Enum.module_path}/include/${i_Enum.prefix}${i_Enum.name}.h`;
    i_Enum.source_file_name = `${i_Enum.prefix}${i_Enum.name}.cpp`;
    i_Enum.source_file_path = `${i_Enum.module_path}/src/${i_Enum.prefix}${i_Enum.name}.cpp`;

    for (let i_Option of i_Enum.options) {
      i_Option.uppercase = i_Option.name.toUpperCase();
    }

    l_Enums.push(i_Enum);
  }

  return l_Enums;
}

function process_file(p_Path, p_FileName, p_Project = false) {
  let l_FilePath = `${p_Path}LowData/type_configs/${p_FileName}`;
  if (p_Project) {
    l_FilePath = `${p_Path}data/_internal/type_configs/${p_FileName}`;
  }
  const l_FileContent = read_file(l_FilePath);

  const l_Config = YAML.parse(l_FileContent);

  if (!g_TypeIdMap[l_Config.module]) {
    g_TypeIdMap[l_Config.module] = {};
  }

  const l_Types = [];

  for (let [i_TypeName, i_Type] of Object.entries(l_Config.types)) {
    i_Type.name = i_TypeName;
    i_Type.module = l_Config.module;
    i_Type.prefix = l_Config.prefix ? l_Config.prefix : l_Config.module;
    i_Type.api_file = `${i_Type.module}Api.h`;
    if (!i_Type.identifier_name) {
      i_Type.identifier_name = i_Type.name;
    }
    if (!i_Type.scripting_name) {
      i_Type.scripting_name = i_TypeName;
    }
    if (!i_Type.scripting_namespace) {
      i_Type.scripting_namespace = "";
    }

    i_Type.scripting_static_namespace = i_Type.scripting_namespace;
    if (i_Type.scripting_namespace.length > 0) {
      i_Type.scripting_static_namespace += "::";
    }
    i_Type.scripting_static_namespace += `${i_Type.name}s`;

    i_Type.identifier = `${i_Type.module}:${i_Type.identifier_name}`;

    i_Type.full_scripting_string = i_Type.scripting_name;
    if (i_Type.scripting_namespace) {
      i_Type.full_scripting_string = `${i_Type.scripting_namespace}::${i_Type.scripting_name}`;
    }

    if (!i_Type.observables) {
      i_Type.observables = [];
    }

    i_Type.observables.push({ name: "destroy" });

    if (l_Config.api_file) {
      i_Type.api_file = l_Config.api_file;
    }
    i_Type.namespace = l_Config.namespace;

    i_Type.typeId = 0;
    if (g_TypeIdMap[l_Config.module][i_TypeName]) {
      i_Type.typeId = g_TypeIdMap[l_Config.module][i_TypeName];
    } else {
      i_Type.typeId = get_unused_type_id(p_Project);
      g_AllTypeIds.push(i_Type.typeId);
      g_TypeIdMap[l_Config.module][i_TypeName] = i_Type.typeId;
    }

    if (i_Type.dynamic_increase === undefined) {
      i_Type.dynamic_increase = true;
    }

    if (i_Type.reference_counted) {
      i_Type.properties.references = {
        type: "Low::Util::Set<u64>",
        no_setter: true,
        private_getter: true,
        expose_scripting: false,
      };
    }

    i_Type.namespace_string = "";

    if (i_Type.component) {
      i_Type.properties["entity"] = {
        type: "Low::Core::Entity",
        handle: true,
        skip_serialization: true,
        skip_deserialization: true,
        expose_scripting: true,
        scripting_hide_setter: true,
        skip_duplication: true,
      };

      i_Type.unique_id = true;
    } else if (i_Type.ui_component) {
      i_Type.properties["element"] = {
        type: "Low::Core::UI::Element",
        handle: true,
        skip_serialization: true,
        skip_deserialization: true,
        expose_scripting: true,
        scripting_hide_setter: true,
        skip_duplication: true,
      };

      i_Type.unique_id = true;
    }

    if (i_Type.unique_id) {
      i_Type.properties["unique_id"] = {
        type: "Low::Util::UniqueId",
        private_setter: true,
        skip_duplication: true,
        description:
          "A persistent unique id that defines this exact instance of the type.",
      };
    }

    const l_DirtyFlags = [];
    if (i_Type.dirty_flags) {
      if (Array.isArray(i_Type.dirty_flags)) {
        for (let flag of i_Type.dirty_flags) {
          l_DirtyFlags.push(flag);
        }
      } else {
        for (let flag in i_Type.dirty_flags) {
          l_DirtyFlags.push(flag);
        }
      }
    }
    for (let [i_PropName, i_Prop] of Object.entries(i_Type.properties)) {
      i_Type.observables.push({
        name: i_PropName,
      });
      if (!i_Prop.dirty_flag) {
        continue;
      }
      if (Array.isArray(i_Prop.dirty_flag)) {
        for (let i_Flag of i_Prop.dirty_flag) {
          if (!l_DirtyFlags.includes(i_Flag)) {
            l_DirtyFlags.push(i_Flag);
          }
        }
      } else if (!l_DirtyFlags.includes(i_Prop.dirty_flag)) {
        l_DirtyFlags.push(i_Prop.dirty_flag);
      }
    }

    let l_TempDirtyFlags = [];

    for (let i_Flag of l_DirtyFlags) {
      let i_F = { name: i_Flag };
      if (i_Type.dirty_flags) {
        if (i_Type.dirty_flags[i_Flag]) {
          i_F = { name: i_Flag, ...i_Type.dirty_flags[i_Flag] };
        }
      }
      l_TempDirtyFlags.push(i_F);
    }

    i_Type.dirty_flags = l_TempDirtyFlags;

    for (let i_Flag of i_Type.dirty_flags) {
      i_Type.properties[i_Flag.name] = {
        type: "bool",
        is_dirty_flag: true,
        skip_serialization: true,
        skip_deserialization: true,
        dirty_flag_info: i_Flag,
      };
      if (i_Flag.no_data) {
        i_Type.properties[i_Flag.name].no_data = true;
      }
    }

    if (!i_Type.component && !i_Type.ui_component) {
      i_Type.properties["name"] = {
        type: "Low::Util::Name",
        expose_scripting: true,
        scripting_expose: true,
        skip_duplication: true,
        description:
          "Name of the instance. Should in most cases be unique but not always necessary.",
      };
      if (i_Type.name_editable) {
        i_Type.properties.name.editor_editable = true;
      }
      if (i_Type.skip_name_serialization) {
        i_Type.properties["name"]["skip_serialization"] = true;
      }
      if (i_Type.skip_name_deserialization) {
        i_Type.properties["name"]["skip_deserialization"] = true;
      }
    }

    for (let i = 0; i < i_Type.namespace.length; ++i) {
      if (i) {
        i_Type.namespace_string += "::";
      }
      i_Type.namespace_string += i_Type.namespace[i];
    }

    for (let [i_PropName, i_Prop] of Object.entries(i_Type.properties)) {
      i_Prop.name = i_PropName;

      if (i_Prop.enum) {
        i_Prop.no_ref = true;
      }

      if (!i_Prop.getter_name) {
        i_Prop.getter_name = `get_${i_PropName}`;

        if (["bool", "boolean"].includes(i_Prop.type)) {
          i_Prop.getter_name = `is_${i_PropName}`;
        }
      }
      if (!i_Prop.setter_name) {
        i_Prop.setter_name = `set_${i_PropName}`;
      }

      if (i_Prop.dirty_flag && !Array.isArray(i_Prop.dirty_flag)) {
        i_Prop.dirty_flag = [i_Prop.dirty_flag];
      }

      apply_type_object(i_Type.name, i_Prop);

      i_Prop.plain_type = get_plain_type(i_Prop.type);
      i_Prop.soa_type = i_Prop.plain_type;
      i_Prop.cs_type = get_cs_type(i_Prop);
      i_Prop.cs_name = get_cs_field_name(i_PropName);
      if (i_Prop.soa_type.includes(",")) {
        i_Prop.soa_type = `SINGLE_ARG(${i_Prop.soa_type})`;
      }
      i_Prop.accessor_type = get_accessor_type(i_Prop.type, i_Prop.handle);
      if (i_Prop.no_ref) {
        i_Prop.accessor_type = i_Prop.plain_type;
      }
    }

    if (i_Type.functions) {
      for (let [i_FuncName, i_Func] of Object.entries(i_Type.functions)) {
        i_Func.accessor_type = get_accessor_type(
          i_Func.return_type,
          i_Func.return_handle,
        );
        if (i_Func.no_ref) {
          i_Func.accessor_type = i_Func.return_type;
        }
        i_Func.name = i_FuncName;
        if (i_Func.parameters) {
          for (let i_Param of i_Func.parameters) {
            i_Param.accessor_type = get_accessor_type(
              i_Param.type,
              i_Param.handle || i_Param.enum || i_Param.no_ref,
            );
            i_Param.base_name = i_Param.name;
            i_Param.local_name = `l_${capitalize_first_letter(i_Param.name)}`;
            i_Param.name = `p_${capitalize_first_letter(i_Param.name)}`;
          }
        }
      }
    }

    if (!i_Type.functions) {
      i_Type.functions = {};
    }

    if (i_Type.virtual_properties) {
      for (let [i_VPropName, i_VProp] of Object.entries(
        i_Type.virtual_properties,
      )) {
        if (i_VProp.enum) {
          i_VProp.no_ref = true;
        }

        if (!i_VProp.getter_name) {
          i_VProp.getter_name = `get_${i_VPropName}`;

          if (["bool", "boolean"].includes(i_VProp.type)) {
            i_VProp.getter_name = `is_${i_VPropName}`;
          }
        }

        if (!i_VProp.setter_name) {
          i_VProp.setter_name = `set_${i_VPropName}`;
        }
        i_VProp.accessor_type = get_accessor_type(i_VProp.type, i_VProp.handle);
        i_VProp.name = i_VPropName;

        i_VProp.hide_setter_flode =
          i_VProp.hide_flode || i_VProp.hide_setter_flode;

        apply_type_object(i_Type.name, i_VProp);

        i_VProp.plain_type = get_plain_type(i_VProp.type);
        i_VProp.soa_type = i_VProp.plain_type;
        i_VProp.cs_type = get_cs_type(i_VProp);
        i_VProp.cs_name = get_cs_field_name(i_VProp.name);
        if (i_VProp.soa_type.includes(",")) {
          i_VProp.soa_type = `SINGLE_ARG(${i_VProp.soa_type})`;
        }
        i_VProp.accessor_type = get_accessor_type(i_VProp.type, i_VProp.handle);
        if (i_VProp.no_ref) {
          i_VProp.accessor_type = i_VProp.plain_type;
        }

        if (!i_VProp.no_getter) {
          i_Type.functions[i_VProp.getter_name] = {
            name: i_VProp.getter_name,
            return_type: i_VProp.plain_type,
            return_handle: i_VProp.handle,
            accessor_type: i_VProp.accessor_type,
            expose_scripting: i_VProp.expose_scripting,
            hide_flode: i_VProp.hide_flode || i_VProp.hide_getter_flode,
            virtual_property: true,
          };
        }

        if (!i_VProp.no_setter) {
          i_Type.functions[i_VProp.setter_name] = {
            name: i_VProp.setter_name,
            return_type: "void",
            return_handle: false,
            accessor_type: "void",
            expose_scripting: i_VProp.expose_scripting,
            hide_flode: i_VProp.hide_flode || i_VProp.hide_setter_flode,
            virtual_property: true,
            parameters: [
              {
                name: "p_Value",
                type: i_VProp.plain_type,
                handle: i_VProp.handle,
                accessor_type: i_VProp.accessor_type,
              },
            ],
          };
        }
      }
    }

    if (!i_Type.source_path && l_Config.source_path) {
      i_Type.source_path = l_Config.source_path;
    }
    if (!i_Type.header_path && l_Config.header_path) {
      i_Type.header_path = l_Config.header_path;
    }

    i_Type.module_path = `${p_Path}${i_Type.module}`;
    if (p_Project) {
      i_Type.module_path = `${p_Path}modules/${i_Type.module}`;
    }
    i_Type.docs_path = `${p_Path}docs/docs/types/${i_Type.name.toLowerCase()}.mdx`;
    i_Type.header_file_name = `${i_Type.prefix}${i_Type.name}.h`;
    i_Type.header_file_path = `${i_Type.module_path}/include/${i_Type.prefix}${i_Type.name}.h`;
    if (i_Type.header_path) {
      i_Type.header_file_path = `${i_Type.module_path}/${i_Type.header_path}/${i_Type.prefix}${i_Type.name}.h`;
    }
    i_Type.source_file_path = `${i_Type.module_path}/src/${i_Type.prefix}${i_Type.name}.cpp`;
    if (i_Type.source_path) {
      i_Type.source_file_path = `${i_Type.module_path}/${i_Type.source_path}/${i_Type.prefix}${i_Type.name}.cpp`;
    }
    i_Type.scripting_api_file_path = `${__dirname}/../../LowScriptingApi/${i_Type.prefix}${i_Type.name}.cs`;
    if (i_Type.module == "MistedaPlugin") {
      i_Type.scripting_api_file_path = `${__dirname}/../../LowScriptingApi/${i_Type.prefix}${i_Type.name}.cs`;
    }
    i_Type.dll_macro = l_Config.dll_macro;

    l_Types.push(i_Type);
  }

  return l_Types;
}

function get_plain_type(p_Type) {
  if (p_Type == "u32") {
    return "uint32_t";
  } else if (p_Type == "u64") {
    return "uint64_t";
  } else if (p_Type == "u16") {
    return "uint16_t";
  } else if (p_Type == "u8") {
    return "uint8_t";
  }
  return p_Type;
}

function get_cs_type(p_Prop) {
  let l_Type = p_Prop.type;

  if (p_Prop.handle) {
    l_Type = l_Type.replace(/::/g, ".");
    let l_TypeName = l_Type.split(".");
    l_TypeName = l_TypeName[l_TypeName.length - 1];
    if (l_Type.includes("Core.Component.") || l_Type.includes("Component.")) {
      return `Low.${l_TypeName}`;
    }
    if (l_TypeName == "Entity") {
      return "Low.Entity";
    }

    return l_Type;
  }

  if (l_Type.endsWith("Util::Name")) {
    return "string";
  }

  if (l_Type === "uint64_t") {
    return "ulong";
  }
  if (l_Type.endsWith("Math::ColorRGB")) {
    return "Low.Vector3";
  }
  if (l_Type.endsWith("Math::Vector3")) {
    return "Low.Vector3";
  }
  if (l_Type.endsWith("Math::Vector4")) {
    return "Low.Vector4";
  }
  if (l_Type.endsWith("Math::Vector2")) {
    return "Low.Vector2";
  }
  if (l_Type.endsWith("Math::Color")) {
    return "Low.Vector4";
  }
  if (l_Type.endsWith("Math::Quaternion")) {
    return "Low.Quaternion";
  }
  return get_plain_type(l_Type);
}

function get_cs_field_name(p_Name) {
  let s = "";
  let cap = false;

  for (let i = 0; i < p_Name.length; ++i) {
    if (p_Name[i] === "_") {
      cap = true;
      continue;
    }

    if (cap) {
      s += p_Name[i].toUpperCase();
    } else {
      s += p_Name[i];
    }

    cap = false;
  }

  return s;
}

function get_cs_method_name(p_Name) {
  let s = "";
  let cap = true;

  for (let i = 0; i < p_Name.length; ++i) {
    if (p_Name[i] === "_") {
      cap = true;
      continue;
    }

    if (cap) {
      s += p_Name[i].toUpperCase();
    } else {
      s += p_Name[i];
    }

    cap = false;
  }

  return s;
}

function is_reference_type(t) {
  if (t.endsWith("Math::Color")) {
    return false;
  }
  if (t.endsWith("Math::ColorRGB")) {
    return false;
  }
  if (t.endsWith("Math::Vector3")) {
    return false;
  }
  if (t.endsWith("Math::Vector2")) {
    return false;
  }
  if (t.endsWith("Math::UVector3")) {
    return false;
  }
  if (t.endsWith("Math::UVector2")) {
    return false;
  }
  if (t.endsWith("Math::Vector4")) {
    return false;
  }
  if (t.endsWith("Math::Quaternion")) {
    return false;
  }
  if (t.endsWith("Util::String")) {
    return false;
  }
  return (
    ![
      "void",
      "int",
      "char",
      "float",
      "Name",
      "Low::Util::Name",
      "Util::Name",
      "bool",
      "uint8_t",
      "uint16_t",
      "uint32_t",
      "uint64_t",
      "int8_t",
      "int16_t",
      "int32_t",
      "int64_t",
      "size_t",
      "u64",
      "u32",
      "u16",
      "u8",
      "Util::UniqueId",
      "Low::Util::UniqueId",
    ].includes(t) && !t.endsWith("*")
  );
}

function capitalize_first_letter(p_String) {
  return p_String.charAt(0).toUpperCase() + p_String.slice(1);
}

function get_accessor_type(p_Type, p_Handle) {
  const l_PlainType = get_plain_type(p_Type);
  let l_AccessorType = l_PlainType + " ";

  if (is_container_type(p_Type)) {
    l_AccessorType += "&";
  } else if (is_reference_type(p_Type) && !p_Handle) {
    l_AccessorType += "&";
  }

  return l_AccessorType;
}

function removeItemOnce(arr, value) {
  var index = arr.indexOf(value);
  if (index > -1) {
    arr.splice(index, 1);
  }
  return arr;
}

function collect_enums_for_project(p_Path, p_WriteIds = true) {
  const l_TypeConfigsPath = `${p_Path}data/_internal/type_configs`;
  const l_FileList = fs.readdirSync(l_TypeConfigsPath);

  const l_Enums = [];

  const l_EnumFiles = [];

  for (let i_FileName of l_FileList) {
    if (i_FileName.endsWith(".enums.yaml")) {
      l_EnumFiles.push(i_FileName);
    }
  }

  for (let i_FileName of l_EnumFiles) {
    l_Enums.push(...process_enum_file(p_Path, i_FileName, true));
  }

  return l_Enums;
}

function collect_enums_for_low(p_Path, p_WriteIds = true) {
  const l_TypeConfigsPath = `${p_Path}LowData/type_configs`;
  const l_FileList = fs.readdirSync(l_TypeConfigsPath);

  const l_Enums = [];

  const l_EnumFiles = [];

  for (let i_FileName of l_FileList) {
    if (i_FileName.endsWith(".enums.yaml")) {
      l_EnumFiles.push(i_FileName);
    }
  }

  for (let i_FileName of l_EnumFiles) {
    l_Enums.push(...process_enum_file(p_Path, i_FileName, false));
  }

  return l_Enums;
}

function path_contains_file(p_Path, p_FileName) {
  const fullPath = path.join(p_Path, p_FileName);
  return fs.existsSync(fullPath) && fs.statSync(fullPath).isFile();
}

function is_plugin(p_Path) {
  return (
    path_contains_file(p_Path, "project.yaml") ||
    path_contains_file(p_Path, "plugin.yaml")
  );
}

function get_engine_root_for_plugin(p_Path) {
  if (p_Path.endsWith("/") || p_Path.endsWith("\\")) {
    return `${p_Path}../`;
  }
  return `${p_Path}/../`;
}

function type_desc(p_Desc, p_Handle) {
  const l_Parsed = parse_type_description(p_Desc);

  let l_Desc = {
    handle: p_Handle,
    string: p_Desc,
    container: l_Parsed.container,
    parsed: l_Parsed,
  };

  return l_Desc;
}

function insert_type_into_db(p_Type, db) {
  let l_Type = {
    id: p_Type.identifier,
    identifier_name: p_Type.identifier_name,
    identifier: p_Type.identifier,
    name: p_Type.name,
    namespace: p_Type.namespace,
    namespace_string: p_Type.namespace_string,
    full_string: `${p_Type.namespace_string}::${p_Type.name}`,
    module: p_Type.module,
    properties: {},
    functions: {},
    scripting_expose: !!p_Type.scripting_expose,
    full_scripting_string: p_Type.full_scripting_string,
    scripting_name: p_Type.scripting_name ? p_Type.scripting_name : l_Type.name,
    scripting_namespace: p_Type.scripting_namespace,
    header_file_name: p_Type.header_file_name,
    include_line: `#include "${p_Type.header_file_name}"`,
    component: !!p_Type.component,
    private_make: !!p_Type.private_make,
    ui_component: !!p_Type.ui_component,
    any_component_type: p_Type.component || p_Type.ui_component,
    scripting_static_namespace: p_Type.scripting_static_namespace,
    description: p_Type.description || "",
  };

  for (let i_Prop of Object.values(p_Type.properties)) {
    l_Type.properties[i_Prop.name] = {
      name: i_Prop.name,
      type: type_desc(i_Prop.type, !!i_Prop.handle),
      getter_name: i_Prop.getter_name,
      setter_name: i_Prop.setter_name,
      private_setter: !!i_Prop.private_setter,
      no_setter: !!i_Prop.no_setter,
      private_getter: !!i_Prop.private_setter,
      no_getter: !!i_Prop.no_setter,
      getter_exposed_scripting: !i_Prop.no_getter && !i_Prop.private_getter,
      setter_exposed_scripting: !i_Prop.no_setter && !i_Prop.private_setter,
      expose_scripting: !!i_Prop.expose_scripting,
      visual_script_expose: !i_Prop.flode_hide && !!i_Prop.expose_scripting,
    };
  }
  for (let i_Func of Object.values(p_Type.functions)) {
    l_Type.functions[i_Func.name] = {
      static: i_Func.static,
      name: i_Func.name,
      return_type: type_desc(i_Func.return_type, !!i_Func.return_hamdle),
      expose_scripting: !!i_Func.expose_scripting,
      scripting: i_Func.scripting ? i_Func.scripting : {},
      visual_script_expose: !i_Func.flode_hide && !!i_Func.expose_scripting,
    };

    l_Type.functions[i_Func.name].parameters = [];
    if (i_Func.parameters) {
      for (let i_Param of i_Func.parameters) {
        l_Type.functions[i_Func.name].parameters.push({
          name: i_Param.name,
          type: type_desc(i_Param.type, !!i_Param.handle),
        });
      }
    }
  }

  if (!db.types) {
    db.types = {};
  }
  if (!db.modules) {
    db.modules = {};
  }
  if (!db.modules[l_Type.module]) {
    db.modules[l_Type.module] = [];
  }

  db.types[l_Type.id] = l_Type;

  db.modules[l_Type.module].push({ f: "type", id: l_Type.id });
}

function resolve_type_args(p_Parsed, p_ContextNamespace, db) {
  for (const i_Child of type_children(p_Parsed)) {
    if (i_Child.kind === "type") {
      const i_ReferencedType = db.find_by_type_string(
        i_Child.name,
        p_ContextNamespace,
      );
      if (i_ReferencedType) {
        if (i_ReferencedType.f === "type") {
          i_Child.handle_id = i_ReferencedType.id;
        }
        i_Child.reference_type = i_ReferencedType.f;
        i_Child.reference_id = i_ReferencedType.id;
      }
    }
    resolve_type_args(i_Child, p_ContextNamespace, db);
  }
}

function get_full_type_description(p_Desc, p_ContextNamespace, db) {
  let l_Desc = JSON.parse(JSON.stringify(p_Desc));
  const l_Name = l_Desc.parsed.name;

  l_Desc.reference_type = undefined;
  l_Desc.reference_id = undefined;

  const l_ReferencedType = db.find_by_type_string(l_Name, p_ContextNamespace);
  if (l_ReferencedType) {
    l_Desc.reference_id = l_ReferencedType.id;
    l_Desc.reference_type = l_ReferencedType.f;
    if (l_ReferencedType.f === "type") {
      l_Desc.handle_id = l_ReferencedType.id;
    }
  }

  let l_IsHandle = p_Desc.handle;
  if (!l_IsHandle && !p_Desc.container) {
    if (l_ReferencedType) {
      if (l_ReferencedType.f === "type") {
        l_IsHandle = true;
        l_Desc.handle_id = l_ReferencedType.id;
      }
      l_Desc.reference_type = l_ReferencedType.f;
      l_Desc.reference_id = l_ReferencedType.id;
    }
  }

  if (l_IsHandle && !l_Desc.handle_id) {
    if (l_Name.endsWith("Util::Handle")) {
      l_IsHandle = false;
      l_Desc.base_handle = true;
    } else if (!l_ReferencedType || l_ReferencedType.f !== "type") {
      l_IsHandle = false;
      console.log(
        `WARNING: Expected handle but could not find type for it: '${p_Desc.string}'`,
      );
    }
  }

  resolve_type_args(l_Desc.parsed, p_ContextNamespace, db);

  if (l_IsHandle) {
    l_Desc.full_string = db.types[l_Desc.handle_id].full_string;
    l_Desc.scripting = db.types[l_Desc.handle_id].full_scripting_string;
  } else {
    l_Desc.full_string = l_Desc.parsed.string;
    l_Desc.scripting = get_scripting_type(l_Name);
  }

  return l_Desc;
}

function fill_types(db) {
  for (let i_Type of Object.values(db.types)) {
    for (let i_Prop of Object.values(i_Type.properties)) {
      //console.log(`T: ${i_Type.id} - P: ${i_Prop.name}`)
      i_Prop.type = get_full_type_description(
        i_Prop.type,
        i_Type.namespace,
        db,
      );
    }
    if (i_Type.functions) {
      for (let i_Func of Object.values(i_Type.functions)) {
        i_Func.return_type = get_full_type_description(
          i_Func.return_type,
          i_Type.namespace,
          db,
        );
        if (i_Func.parameters) {
          for (let i_Param of i_Func.parameters) {
            i_Param.type = get_full_type_description(
              i_Param.type,
              i_Type.namespace,
              db,
            );
          }
        }
      }
    }
  }

  for (let i_Func of Object.values(db.functions)) {
    i_Func.type = get_full_type_description(i_Func.type, i_Func.namespace, db);
    for (const i_Param of i_Func.params) {
      i_Param.type = get_full_type_description(
        i_Param.type,
        i_Func.namespace,
        db,
      );
    }
  }

  for (let i_Struct of Object.values(db.structs)) {
    for (const i_Field of i_Struct.fields) {
      i_Field.type = get_full_type_description(
        i_Field.type,
        i_Struct.namespace,
        db,
      );
    }
  }
}

function insert_enum_into_db(p_Enum, p_Module, p_ModuleId, db) {
  let l_Enum = {
    id: `${p_ModuleId}:${p_Enum.namespace}::${p_Enum.name}`,
    name: p_Enum.name,
    scripting_name: p_Enum.bind_name,
    full_string: `${p_Enum.namespace}::${p_Enum.name}`,
    namespace_string: p_Enum.namespace,
    namespace: p_Enum.namespace.split("::"),
    scripting_namespace_string: p_Enum.bind_namespace,
    scripting_namespace: p_Enum.bind_namespace.split("::"),
    scripting_expose: p_Enum.scripting,
    description: "",
    visual_script_expose: false,
    visual_script_icon: "",
    visual_script_category: "",
    values: [],
  };

  if (p_Enum.macro_args && p_Enum.macro_args.vs) {
    l_Enum.visual_script_expose = true;
    if (isObject(p_Enum.macro_args.vs)) {
      if (p_Enum.macro_args.vs.category) {
        l_Enum.visual_script_category = p_Enum.macro_args.vs.category;
      }
      if (p_Enum.macro_args.vs.icon) {
        l_Enum.visual_script_icon = p_Enum.macro_args.vs.icon;
      }
    }
  }

  for (const i_Val of p_Enum.values) {
    let i_Value = {
      value: i_Val.value,
      name: i_Val.name,
    };

    l_Enum.values.push(i_Val);
  }

  if (!db.enums) {
    db.enums = {};
  }
  if (!db.modules) {
    db.modules = {};
  }
  if (!db.modules[p_ModuleId]) {
    db.modules[p_ModuleId] = [];
  }

  db.enums[l_Enum.id] = l_Enum;

  db.modules[p_ModuleId].push({ f: "enum", id: l_Enum.id });
}

function insert_struct_into_db(p_Struct, p_Module, p_ModuleId, db) {
  let l_Struct = {
    id: `${p_ModuleId}:${p_Struct.namespace}::${p_Struct.name}`,
    name: p_Struct.name,
    scripting_name: p_Struct.bind_name,
    full_string: `${p_Struct.namespace}::${p_Struct.name}`,
    namespace_string: p_Struct.namespace,
    namespace: p_Struct.namespace.split("::"),
    scripting_namespace_string: p_Struct.bind_namespace,
    scripting_namespace: p_Struct.bind_namespace.split("::"),
    scripting_expose: p_Struct.scripting,
    description: "",
    visual_script_expose: false,
    visual_script_icon: "",
    visual_script_category: "",
    fields: [],
  };

  if (p_Struct.macro_args && p_Struct.macro_args.vs) {
    l_Struct.visual_script_expose = true;
    if (isObject(p_Struct.macro_args.vs)) {
      if (p_Struct.macro_args.vs.category) {
        l_Struct.visual_script_category = p_Struct.macro_args.vs.category;
      }
      if (p_Struct.macro_args.vs.icon) {
        l_Struct.visual_script_icon = p_Struct.macro_args.vs.icon;
      }
    }
  }

  for (const i_Field of p_Struct.fields) {
    let i_F = {
      name: i_Field.name,
      type: type_desc(i_Field.type, false),
    };
    l_Struct.fields.push(i_F);
  }

  if (!db.structs) {
    db.structs = {};
  }
  if (!db.modules) {
    db.modules = {};
  }
  if (!db.modules[p_ModuleId]) {
    db.modules[p_ModuleId] = [];
  }

  db.structs[l_Struct.id] = l_Struct;

  db.modules[p_ModuleId].push({ f: "struct", id: l_Struct.id });
}

function insert_function_into_db(p_Function, p_Module, p_ModuleId, db) {
  let l_Function = {
    id: `${p_ModuleId}:${p_Function.namespace}::${p_Function.name}`,
    name: p_Function.name,
    scripting_name: p_Function.bind_name,
    full_string: `${p_Function.namespace}::${p_Function.name}`,
    namespace_string: p_Function.namespace,
    namespace: p_Function.namespace.split("::"),
    scripting_namespace_string: p_Function.bind_namespace,
    scripting_namespace: p_Function.bind_namespace.split("::"),
    scripting_expose: p_Function.scripting,
    visual_script_expose: false,
    visual_script_icon: "",
    visual_script_category: "",
    type: type_desc(p_Function.return_type, false),
    description: "",
    params: [],
  };

  for (const i_Param of p_Function.params) {
    const i_Parameter = {
      name: i_Param.name,
      scripting_name: i_Param.bind_name,
      type: type_desc(i_Param.type, false),
    };
    l_Function.params.push(i_Parameter);
  }

  if (!db.functions) {
    db.functions = {};
  }
  if (!db.modules) {
    db.modules = {};
  }
  if (!db.modules[p_ModuleId]) {
    db.modules[p_ModuleId] = [];
  }

  db.functions[l_Function.id] = l_Function;

  db.modules[p_ModuleId].push({ f: "function", id: l_Function.id });
}

function build_database_for_low(p_Path, db) {
  const l_Types = collect_types_for_low(p_Path);

  for (let i_Type of l_Types) {
    insert_type_into_db(i_Type, db);
  }

  if (true) {
    const l_Module = "LowCore";
    const l_ModulePath = `${p_Path}/LowCore`;
    const l_IncludePath = `${p_Path}/LowCore/include`;
    const l_LensOutput = process_directory(l_IncludePath);

    for (const i_File of l_LensOutput) {
      for (const i_Function of i_File.functions) {
        insert_function_into_db(i_Function, l_Module, l_Module, db);
      }

      for (const i_Struct of i_File.structs) {
        insert_struct_into_db(i_Struct, l_Module, l_Module, db);
      }

      for (const i_Enum of i_File.enums) {
        insert_enum_into_db(i_Enum, l_Module, l_Module, db);
      }
    }
  }
  if (true) {
    const l_Module = "LowCore";
    const l_ModuleId = "LowCoreNavigation";
    const l_IncludePath = `${p_Path}/LowCore/navigation/include`;
    const l_LensOutput = process_directory(l_IncludePath);

    for (const i_File of l_LensOutput) {
      for (const i_Function of i_File.functions) {
        insert_function_into_db(i_Function, l_Module, l_ModuleId, db);
      }

      for (const i_Struct of i_File.structs) {
        insert_struct_into_db(i_Struct, l_Module, l_ModuleId, db);
      }

      for (const i_Enum of i_File.enums) {
        insert_enum_into_db(i_Enum, l_Module, l_ModuleId, db);
      }
    }
  }
  if (true) {
    const l_Module = "LowCore";
    const l_ModuleId = "LowCorePhysics";
    const l_IncludePath = `${p_Path}/LowCore/physics/include`;
    const l_LensOutput = process_directory(l_IncludePath);

    for (const i_File of l_LensOutput) {
      for (const i_Function of i_File.functions) {
        insert_function_into_db(i_Function, l_Module, l_ModuleId, db);
      }

      for (const i_Struct of i_File.structs) {
        insert_struct_into_db(i_Struct, l_Module, l_ModuleId, db);
      }

      for (const i_Enum of i_File.enums) {
        insert_enum_into_db(i_Enum, l_Module, l_ModuleId, db);
      }
    }
  }

  if (true) {
    const l_Module = "LowCore";
    const l_ModuleId = "LowCoreAnimation";
    const l_IncludePath = `${p_Path}/LowCore/animation/include`;
    const l_LensOutput = process_directory(l_IncludePath);

    for (const i_File of l_LensOutput) {
      for (const i_Function of i_File.functions) {
        insert_function_into_db(i_Function, l_Module, l_ModuleId, db);
      }

      for (const i_Struct of i_File.structs) {
        insert_struct_into_db(i_Struct, l_Module, l_ModuleId, db);
      }

      for (const i_Enum of i_File.enums) {
        insert_enum_into_db(i_Enum, l_Module, l_ModuleId, db);
      }
    }
  }
}

function build_database_for_plugin(p_Path, db) {}

function build_database_for(p_Path) {
  const IS_PLUGIN = is_plugin(p_Path);

  let db = {
    fits: function (p_Str, p_CurNamespace, p_Obj) {
      if (p_Str == p_Obj.full_string) {
        return true;
      }
      let l_DStr = p_Str;
      for (let i = p_CurNamespace.length - 1; i >= 0; --i) {
        l_DStr = `${p_CurNamespace[i]}::${l_DStr}`;
        if (l_DStr == p_Obj.full_string) {
          return true;
        }
      }
      let l_Ns = "";
      for (let i_N of p_CurNamespace) {
        l_Ns = `${l_Ns}${i_N}::`;
        if (`${l_Ns}${p_Str}` == p_Obj.full_string) {
          return true;
        }
      }

      return false;
    },

    find_by_type_string: function (p_Str, p_CurNamespace) {
      for (let i_Type of Object.values(db.types)) {
        if (this.fits(p_Str, p_CurNamespace, i_Type)) {
          return { f: "type", id: i_Type.id };
        }
      }
      for (let i_Enum of Object.values(db.enums)) {
        if (this.fits(p_Str, p_CurNamespace, i_Enum)) {
          return { f: "enum", id: i_Enum.id };
        }
      }
      for (let i_Struct of Object.values(db.structs)) {
        if (this.fits(p_Str, p_CurNamespace, i_Struct)) {
          return { f: "struct", id: i_Struct.id };
        }
      }

      return undefined;
    },
  };

  if (IS_PLUGIN) {
    build_database_for_low(get_engine_root_for_plugin(p_Path), db);
    build_database_for_plugin(p_Path, db);
  } else {
    build_database_for_low(p_Path, db);
  }

  fill_types(db);

  return db;
}

function collect_types_for_low(p_Path, p_WriteIds = true) {
  let l_Path = p_Path;
  if (!l_Path.endsWith("/")) {
    l_Path += "/";
  }
  const l_FileList = fs.readdirSync(`${l_Path}LowData/type_configs`);

  const l_Types = [];

  const l_TypeFiles = [];

  for (let i_FileName of l_FileList) {
    if (i_FileName.endsWith(".types.yaml")) {
      l_TypeFiles.push(i_FileName);
    }
  }

  for (let i_FileName of l_TypeFiles) {
    l_Types.push(...process_file(l_Path, i_FileName, false));
  }

  return l_Types;
}

function collect_types_for_project(p_Path, p_WriteIds = true) {
  let l_Path = p_Path;
  if (!l_Path.endsWith("/")) {
    l_Path += "/";
  }
  const l_FileList = fs.readdirSync(`${l_Path}data/_internal/type_configs`);

  const l_Types = [];

  const l_TypeFiles = [];

  for (let i_FileName of l_FileList) {
    if (i_FileName.endsWith(".types.yaml")) {
      l_TypeFiles.push(i_FileName);
    }
  }

  for (let i_FileName of l_TypeFiles) {
    l_Types.push(...process_file(p_Path, i_FileName, true));
  }

  return l_Types;
}

function save_file(p_FilePath, p_Content) {
  fs.writeFileSync(p_FilePath, p_Content);
}

function get_marker_begin(p_Name) {
  return `// LOW_CODEGEN:BEGIN:${p_Name}`;
}

function get_marker_end(p_Name) {
  return `// LOW_CODEGEN::END::${p_Name}`;
}

function find_begin_marker_start(p_Text, p_Name) {
  const l_Marker = get_marker_begin(p_Name);

  return p_Text.indexOf(l_Marker);
}

function find_begin_marker_end(p_Text, p_Name) {
  const l_Marker = get_marker_begin(p_Name);
  const l_Index = p_Text.indexOf(l_Marker);

  if (l_Index < 0) {
    return -1;
  }

  return l_Index + l_Marker.length + 1;
}

function find_end_marker_start(p_Text, p_Name) {
  const l_Marker = get_marker_end(p_Name);

  return p_Text.indexOf(l_Marker);
}

function find_end_marker_end(p_Text, p_Name) {
  const l_Marker = get_marker_end(p_Name);
  const l_Index = p_Text.indexOf(l_Marker);

  if (l_Index < 0) {
    return -1;
  }

  return l_Index + l_Marker.length;
}

function is_math_type(p_Type) {
  const l_Name = [
    "Vector2",
    "Vector3",
    "Vector4",
    "Color",
    "ColorRGB",
    "Quaternion",
    "Shape",
    "Box",
    "Sphere",
    "Cylinder",
    "Cone",
  ];

  const l_Prefixes = ["Low::Math::", "Math::", ""];

  for (const i_Name of l_Name) {
    for (const i_Prefix of l_Prefixes) {
      if (p_Type === `${i_Prefix}${i_Name}`) {
        return true;
      }
    }
  }

  return false;
}

function is_name_type(p_Type) {
  const l_Name = ["Name"];

  const l_Prefixes = ["Low::Util::", "Util::", ""];

  for (const i_Name of l_Name) {
    for (const i_Prefix of l_Prefixes) {
      if (p_Type === `${i_Prefix}${i_Name}`) {
        return true;
      }
    }
  }

  return false;
}

function is_string_type(p_Type) {
  const l_Name = ["String"];

  const l_Prefixes = ["Low::Util::", "Util::", ""];

  for (const i_Name of l_Name) {
    for (const i_Prefix of l_Prefixes) {
      if (p_Type === `${i_Prefix}${i_Name}`) {
        return true;
      }
    }
  }

  return false;
}

function is_container_type(p_Type) {
  const l_Containers = ["List", "Array", "Map", "Set", "Queue"];

  const l_Prefixes = ["Low::Util::", "Util::", ""];

  for (const i_Container of l_Containers) {
    for (const i_Prefix of l_Prefixes) {
      if (p_Type.startsWith(`${i_Prefix}${i_Container}<`)) {
        return true;
      }
    }
  }

  return false;
}

function normalize_string_path_and_make_absolute(p_Input) {
  let l_Path = p_Input;

  if (l_Path.endsWith("/")) {
    l_Path = l_Path.substring(0, l_Path.length - 1);
  }

  if (l_Path.startsWith("\\") || l_Path.startsWith("/")) {
    l_Path = l_Path.replace(/\\/g, "/");
  } else {
    l_Path = path.resolve(l_Path);
    l_Path = l_Path.replace(/\\/g, "/");
  }

  return l_Path;
}

function resolve_engine_for_project(p_Path) {}

module.exports = {
  read_file,
  collect_types_for_project,
  collect_types_for_low,
  collect_enums_for_project,
  collect_enums_for_low,
  get_plain_type,
  get_accessor_type,
  is_reference_type,
  capitalize_first_letter,
  removeItemOnce,
  format,
  line,
  separator,
  write,
  include,
  empty,
  save_file,
  get_marker_begin,
  get_marker_end,
  find_begin_marker_start,
  find_begin_marker_end,
  find_end_marker_start,
  find_end_marker_end,
  load_project_info,
  g_Directory,
  is_math_type,
  is_name_type,
  is_container_type,
  is_string_type,
  get_scripting_type,
  build_database_for,
  parse_type,
  get_element_type,
  write_file_if_changed,
  normalize_string_path_and_make_absolute,
  output_task_end,
  output_log,
  output_section,
  output_begin_task,
  output_info,
  output_error,
  output_task_success,
  output_task_cancelled,
  output_task_failed,
  output_task_end,
  output_debug,
  output_detail,
  output_update_task,
  output_result,
  path_contains_file,
  is_plugin,
};
