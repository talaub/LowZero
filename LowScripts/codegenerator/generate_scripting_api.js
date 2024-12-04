const fs = require("fs");
const os = require("os");
const exec = require("child_process").execSync;
const YAML = require("yaml");
var CRC32 = require("crc-32");

const {
  read_file,
  collect_types_for_project,
  collect_types_for_low,
  collect_enums_for_project,
  collect_enums_for_low,
  write,
  line,
  empty,
  format,
  save_file,
  load_project_info,
  find_begin_marker_start,
  find_begin_marker_end,
  find_end_marker_start,
  get_marker_begin,
  get_marker_end,
  find_end_marker_end,
  g_Directory,
} = require("./lib.js");

function hash_name(p_String) {
  var n = CRC32.str(p_String);
  var uint32 = n >>> 0;

  return uint32;
}

function internal_get(p_Prop) {
  let l_Type = p_Prop.cs_type;

  if (p_Prop.handle) {
    return `Low.Internal.Handle.GetHandleValue<${l_Type}>`;
  }

  if (p_Prop.plain_type.endsWith("Util::Name")) {
    return `Low.Internal.HandleHelper.GetNameValue`;
  }

  if (l_Type === "ulong") {
    return `Low.Internal.HandleHelper.GetUlongValue`;
  }
  if (l_Type === "float") {
    return `Low.Internal.HandleHelper.GetFloatValue`;
  }
  if (l_Type === "Low.Vector3") {
    return `Low.Internal.HandleHelper.GetVector3Value`;
  }
  if (l_Type === "Low.Vector2") {
    return `Low.Internal.HandleHelper.GetVector2Value`;
  }
  if (l_Type === "Low.Vector4") {
    return `Low.Internal.HandleHelper.GetVector4Value`;
  }
  if (l_Type === "Low.Quaternion") {
    return `Low.Internal.HandleHelper.GetQuaternionValue`;
  }
  if (l_Type === "Low.Quaternion") {
    return `Low.Internal.HandleHelper.GetQuaternionValue`;
  }

  return `Low.Internal.HandleHelper.GetIntValue`;
}

function internal_set(p_Prop) {
  let l_Type = p_Prop.cs_type;

  if (p_Prop.handle) {
    return `Low.Internal.Handle.SetHandleValue<${l_Type}>`;
  }

  if (p_Prop.plain_type.endsWith("Util::Name")) {
    return `Low.Internal.HandleHelper.SetNameValue`;
  }

  if (l_Type === "ulong") {
    return `Low.Internal.HandleHelper.SetUlongValue`;
  }
  if (l_Type === "float") {
    return `Low.Internal.HandleHelper.SetFloatValue`;
  }
  if (l_Type === "Low.Vector3") {
    return `Low.Internal.HandleHelper.SetVector3Value`;
  }
  if (l_Type === "Low.Vector2") {
    return `Low.Internal.HandleHelper.SetVector2Value`;
  }
  if (l_Type === "Low.Vector4") {
    return `Low.Internal.HandleHelper.SetVector4Value`;
  }
  if (l_Type === "Low.Quaternion") {
    return `Low.Internal.HandleHelper.SetQuaternionValue`;
  }

  return `Low.Internal.HandleHelper.SetIntValue`;
}

function generate_enum_scripting_api(p_Enum) {
  let t = "";

  const l_TypeString = `${p_Enum.namespace_string}::${p_Enum.name}`;

  t += line(
    `static void register_${p_Enum.module.toLowerCase()}_${p_Enum.name.toLowerCase()}(){`,
  );
  t += line(`using namespace Low;`);
  t += line(`using namespace Low::Core;`);
  if (!["Low::Core"].includes(p_Enum.namespace_string)) {
    t += line(`using namespace ::${p_Enum.namespace_string};`);
  }
  t += empty();

  t += line(
    `Cflat::Namespace *l_Namespace = Low::Core::Scripting::get_environment()->requestNamespace("${p_Enum.namespace_string}");`,
  );
  t += empty();

  /*
    t += line(`CflatRegisterStruct(l_Namespace, ${p_Type.name});`)
    t += line(`CflatStructAddBaseType(Scripting::get_environment(), ${l_TypeString}, Low::Util::Handle);`)
    */
  t += line(`CflatRegisterEnumClass(l_Namespace, ${p_Enum.name});`);
  t += empty();

  t += line("{");
  t += line(
    `Cflat::Namespace *l_UtilNamespace = Low::Core::Scripting::get_environment()->requestNamespace("Low::Util");`,
  );
  t += empty();
  t += line(
    `CflatRegisterSTLVectorCustom(Low::Core::Scripting::get_environment(), Low::Util::List, ${l_TypeString});`,
  );
  t += line("}");
  t += empty();

  for (let [i_Key, i_Option] of Object.entries(p_Enum.options)) {
    t += line(
      `CflatEnumClassAddValue(l_Namespace, ${p_Enum.name}, ${i_Option.uppercase});`,
    );
  }

  t += line(`}`);
  t += empty();

  return t;
}

function generate_scripting_api(p_Type) {
  let t = "";

  const l_TypeString = `${p_Type.namespace_string}::${p_Type.name}`;

  t += line(
    `static void register_${p_Type.module.toLowerCase()}_${p_Type.name.toLowerCase()}(){`,
  );
  t += line(`using namespace Low;`);
  t += line(`using namespace Low::Core;`);
  if (!["Low::Core"].includes(p_Type.namespace_string)) {
    t += line(`using namespace ::${p_Type.namespace_string};`);
  }
  t += empty();

  t += line(
    `Cflat::Namespace *l_Namespace = Low::Core::Scripting::get_environment()->requestNamespace("${p_Type.namespace_string}");`,
  );
  t += empty();

  /*
    t += line(`CflatRegisterStruct(l_Namespace, ${p_Type.name});`)
    t += line(`CflatStructAddBaseType(Scripting::get_environment(), ${l_TypeString}, Low::Util::Handle);`)
    */
  t += line(
    `Cflat::Struct *type = Scripting::g_CflatStructs["${l_TypeString}"];`,
  );
  t += empty();

  t += line("{");
  t += line(
    `Cflat::Namespace *l_UtilNamespace = Low::Core::Scripting::get_environment()->requestNamespace("Low::Util");`,
  );
  t += empty();
  t += line(
    `CflatRegisterSTLVectorCustom(Low::Core::Scripting::get_environment(), Low::Util::List, ${l_TypeString});`,
  );
  t += line("}");
  t += empty();

  t += line(
    `CflatStructAddConstructorParams1(Low::Core::Scripting::get_environment(), ${l_TypeString}, uint64_t);`,
  );

  t += line(
    `CflatStructAddStaticMember(Low::Core::Scripting::get_environment(), ${l_TypeString}, uint16_t, TYPE_ID);`,
  );
  t += line(
    `CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(), ${l_TypeString}, bool, is_alive);`,
  );
  t += line(
    `CflatStructAddStaticMethodReturn(Low::Core::Scripting::get_environment(), ${l_TypeString}, ${l_TypeString}*, living_instances);`,
  );

  t += line(
    `CflatStructAddStaticMethodReturn(Low::Core::Scripting::get_environment(), ${l_TypeString}, uint32_t, living_count);`,
  );

  t += line(
    `CflatStructAddMethodVoid(Low::Core::Scripting::get_environment(), ${l_TypeString}, void, destroy);`,
  );

  t += line(
    `CflatStructAddStaticMethodReturn(Low::Core::Scripting::get_environment(), ${l_TypeString}, uint32_t, get_capacity);`,
  );
  t += line(
    `CflatStructAddStaticMethodReturnParams1(Low::Core::Scripting::get_environment(), ${l_TypeString}, ${l_TypeString}, find_by_index, uint32_t);`,
  );
  if (!p_Type.component && !p_Type.ui_component) {
    t += line(
      `CflatStructAddStaticMethodReturnParams1(Low::Core::Scripting::get_environment(), ${l_TypeString}, ${l_TypeString}, find_by_name, Low::Util::Name);`,
    );
  }
  t += empty();

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (!i_Prop.expose_scripting) {
      continue;
    }
    if (!i_Prop.static) {
      if (!i_Prop.no_getter && !i_Prop.private_getter) {
        t += line(
          `CflatStructAddMethodReturn(Low::Core::Scripting::get_environment(), ${l_TypeString}, ${i_Prop.accessor_type}, ${i_Prop.getter_name});`,
        );
      }

      if (!i_Prop.no_setter && !i_Prop.private_setter) {
        t += line(
          `CflatStructAddMethodVoidParams1(Low::Core::Scripting::get_environment(), ${l_TypeString}, void, ${i_Prop.setter_name}, ${i_Prop.accessor_type});`,
        );
      }
    }
    t += empty();
  }

  if (p_Type.functions) {
    for (let [i_FuncName, i_Func] of Object.entries(p_Type.functions)) {
      if (!i_Func.expose_scripting) {
        continue;
      }
      if (i_Func.parameters && i_Func.parameters.length > 5) {
        continue;
      }

      if (i_Func.static) {
        t += write(
          `CflatStructAddStaticMethod${i_Func.return_type == "void" ? "Void" : "Return"}`,
        );
        if (i_Func.parameters) {
          t += write(`Params${i_Func.parameters.length}`);
        }
        t += write("(");
        t += write(
          `Low::Core::Scripting::get_environment(), ${l_TypeString}, ${i_Func.return_type}, ${i_FuncName}`,
        );
        if (i_Func.parameters) {
          for (let i_Param of i_Func.parameters) {
            t += write(`, ${i_Param.type}`);
          }
        }
        t += write(");\n");
      } else {
        t += write(
          `CflatStructAddMethod${i_Func.return_type == "void" ? "Void" : "Return"}`,
        );
        if (i_Func.parameters) {
          t += write(`Params${i_Func.parameters.length}`);
        }
        t += write("(");
        t += write(
          `Low::Core::Scripting::get_environment(), ${l_TypeString}, ${i_Func.return_type}, ${i_FuncName}`,
        );
        if (i_Func.parameters) {
          for (let i_Param of i_Func.parameters) {
            t += write(`, ${i_Param.type}`);
          }
        }
        t += write(");\n");
      }
    }
  }

  t += line(`}`);
  t += empty();

  return t;
}

function generate_scripting_api_for(
  p_FilePath,
  p_Types,
  p_Enums,
  p_ModuleConfig,
) {
  if (p_ModuleConfig === undefined) {
    console.log("🚧 Type: LowEngine");
  } else {
    console.log("🚧 Type: Project");
  }
  console.log(`📂 Scripting filepath: ${p_FilePath}`);

  const l_OriginalContent = read_file(p_FilePath);

  const l_IncludeIndexStart = l_OriginalContent.indexOf(
    `// REGISTER_CFLAT_INCLUDES_BEGIN`,
  );
  const l_IncludeIndexEnd = l_OriginalContent.indexOf(
    `// REGISTER_CFLAT_INCLUDES_END`,
  );

  const l_IndexStart = l_OriginalContent.indexOf(`// REGISTER_CFLAT_BEGIN`);
  const l_IndexEnd = l_OriginalContent.indexOf(`// REGISTER_CFLAT_END`);

  const l_IncludePreContent = l_OriginalContent.substring(
    0,
    l_IncludeIndexStart,
  );
  const l_IncludePostContent = l_OriginalContent.substring(
    l_IncludeIndexEnd + `// REGISTER_CFLAT_INCLUDES_END`.length,
    l_IndexStart,
  );

  const l_PostContent = l_OriginalContent.substring(
    l_IndexEnd + `// REGISTER_CFLAT_END`.length,
  );

  let l_IncludeCode = "";
  let l_RegisterCode = "";
  let l_EntryMethodCode = line(`static void register_types() {`);

  let l_PreRegisterCode = line(`static void preregister_types() {`);
  l_PreRegisterCode += line(`using namespace Low::Core;`);
  l_PreRegisterCode += empty();

  for (const i_Enum of p_Enums) {
    console.log(`⚙️ Generating enum scripting API for: ${i_Enum.name}`);

    l_IncludeCode += line(`#include "${i_Enum.header_file_name}"`);

    l_EntryMethodCode += line(
      `register_${i_Enum.module.toLowerCase()}_${i_Enum.name.toLowerCase()}();`,
    );
    l_RegisterCode += generate_enum_scripting_api(i_Enum);
  }

  for (const i_Type of p_Types) {
    if (i_Type.scripting_expose) {
      console.log(`⚙️ Generating scripting API for: ${i_Type.name}`);

      l_IncludeCode += line(`#include "${i_Type.header_file_name}"`);

      l_RegisterCode += generate_scripting_api(i_Type);
      l_EntryMethodCode += line(
        `register_${i_Type.module.toLowerCase()}_${i_Type.name.toLowerCase()}();`,
      );
      l_PreRegisterCode += line(`{`);
      l_PreRegisterCode += line(`using namespace ${i_Type.namespace_string};`);
      l_PreRegisterCode += line(
        `Cflat::Namespace* l_Namespace = Low::Core::Scripting::get_environment()->requestNamespace("${i_Type.namespace_string}");`,
      );
      l_PreRegisterCode += empty();
      l_PreRegisterCode += line(
        `CflatRegisterStruct(l_Namespace, ${i_Type.name});`,
      );
      l_PreRegisterCode += line(
        `CflatStructAddBaseType(Low::Core::Scripting::get_environment(), ${i_Type.namespace_string}::${i_Type.name}, Low::Util::Handle);`,
      );
      l_PreRegisterCode += empty();
      l_PreRegisterCode += line(
        `Scripting::g_CflatStructs["${i_Type.namespace_string}::${i_Type.name}"] = type;`,
      );
      l_PreRegisterCode += line(`}`);
      l_PreRegisterCode += empty();
    }
  }

  l_EntryMethodCode += line("}");
  l_PreRegisterCode += line("}");

  let l_FinalContent =
    l_IncludePreContent + `// REGISTER_CFLAT_INCLUDES_BEGIN\n`;
  l_FinalContent += l_IncludeCode;
  l_FinalContent += `// REGISTER_CFLAT_INCLUDES_END\n`;
  l_FinalContent += l_IncludePostContent;
  l_FinalContent += `// REGISTER_CFLAT_BEGIN\n`;
  l_FinalContent += l_RegisterCode;
  l_FinalContent += l_PreRegisterCode;
  l_FinalContent += l_EntryMethodCode;
  l_FinalContent += `// REGISTER_CFLAT_END` + l_PostContent;

  const l_Formatted = format(p_FilePath, l_FinalContent);
  save_file(p_FilePath, l_Formatted);
  console.log("✔️ Finished generating scripting API");
}

function main() {
  let l_Path = `${__dirname}/../../`;
  let l_Project = false;
  if (process.argv.length > 2) {
    l_Path = process.argv[2];
    l_Project = true;
  }

  let l_Types = 0;
  let l_Enums = 0;
  if (l_Project) {
    let l_LocalPath = l_Path;
    if (!l_LocalPath.endsWith("/") && !l_LocalPath.endsWith("\\")) {
      l_LocalPath += "/";
    }
    l_Types = collect_types_for_project(l_LocalPath);
    l_Enums = collect_enums_for_project(l_Path);
  } else {
    if (!l_Path.endsWith("/") && !l_Path.endsWith("\\")) {
      l_Path += "/";
    }
    l_Types = collect_types_for_low(l_Path);
    l_Enums = collect_enums_for_low(l_Path);
  }

  let l_FilePath = "";
  if (l_Project) {
    const l_ProjectConfig = load_project_info(l_Path);
    for (const i_Module of l_ProjectConfig.modules) {
      if (!i_Module.scripting_api_file) {
        continue;
      }
      l_FilePath = `${i_Module.path}/${i_Module.scripting_api_file}`;
      generate_scripting_api_for(
        l_FilePath,
        l_Types.filter((it) => it.module === i_Module.name),
        l_Enums,
        i_Module,
      );
    }
  } else {
    l_FilePath = `../../LowCore/src/LowCoreCflatScripting.cpp`;
    generate_scripting_api_for(l_FilePath, l_Types, l_Enums, undefined);
  }
}

main();
