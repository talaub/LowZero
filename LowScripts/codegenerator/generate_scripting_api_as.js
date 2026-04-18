
const fs = require("fs");
const os = require("os");
const exec = require("child_process").execSync;
const YAML = require("yaml");
var CRC32 = require("crc-32");

let l_OldCode = "";

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
  separator,
  get_scripting_type,
  build_database_for
} = require("./lib.js");

function hash_name(p_String) {
  var n = CRC32.str(p_String);
  var uint32 = n >>> 0;

  return uint32;
}

function generate_type_api(p_Type, db) {
  const TYPE_PREFIX = `${p_Type.module}_${p_Type.name}`;

  //console.log(p_Type.properties)

  const l_TypeString = `${p_Type.namespace_string}::${p_Type.name}`;
  let t = ""
  t += separator();

  t += line(`static void ${TYPE_PREFIX}_default_construct(${l_TypeString}* p_Memory){`)
  t += line(`new (p_Memory) ${l_TypeString};`);
  t += line(`}`)
  t += line(`static void ${TYPE_PREFIX}_copy_construct(const ${l_TypeString}& p_Other, ${l_TypeString}* p_Memory){`)
  t += line(`new (p_Memory) ${l_TypeString}(p_Other);`);
  t += line(`}`)
  t += line(`static void ${TYPE_PREFIX}_destruct(${l_TypeString}* p_Memory){`)
  t += line(`using namespace ${p_Type.namespace_string};`)
  t += line(`p_Memory->~${p_Type.name}();`);
  t += line(`}`)

  if (p_Type.any_component_type) {

  }
  else if (!p_Type.private_make) {
    t += line(`static ${l_TypeString} ${TYPE_PREFIX}_genmake(Low::Util::Name p_Name){`)
    t += line(`return ${l_TypeString}::make(p_Name);`);
    t += line(`}`)
    t += line(`static ${l_TypeString} ${TYPE_PREFIX}_genfindbyname(Low::Util::Name p_Name){`)
    t += line(`return ${l_TypeString}::find_by_name(p_Name);`);
    t += line(`}`)
  }

  t += line(`static u32 ${TYPE_PREFIX}_living_count(){`)
  t += line(`return ${l_TypeString}::living_count();`);
  t += line(`}`)

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (!i_Prop.expose_scripting) {
      continue;
    }
    if (i_Prop.type.scripting == 'string') {
      if (i_Prop.getter_exposed_scripting) {
        t += line(`static std::string ${TYPE_PREFIX}_get_${i_Prop.name}(${l_TypeString} p_This) {`)
        t += line(`Low::Util::String l_Value = p_This.${i_Prop.getter_name}();`);
        t += line(`return std::string(l_Value.c_str());`)
        t += line('}')
      }
      if (i_Prop.setter_exposed_scripting) {
        t += line(`static void ${TYPE_PREFIX}_set_${i_Prop.name}(${l_TypeString} p_This, const std::string& p_Value) {`)
        t += line(`p_This.${i_Prop.setter_name}(Low::Util::String(p_Value.c_str()));`)
        t += line('}')
      }
    }
  }

  if (true) {
    const l_MarkerName = `CUSTOM:${p_Type.identifier.toUpperCase()}:HELPERS`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  t += line(`static void expose_${TYPE_PREFIX}(asIScriptEngine* p_Engine) {`)
  t += line(`int r = 0;`);
  t += line(`r = p_Engine->RegisterObjectType("${p_Type.scripting_name}", sizeof(${l_TypeString}), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose ${l_TypeString} type.");`)
  t += empty();
  t += line(`r = p_Engine->RegisterObjectBehaviour("${p_Type.scripting_name}", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(${TYPE_PREFIX}_default_construct), asCALL_CDECL_OBJLAST);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose default constructor of ${l_TypeString}.");`)
  t += empty();
  t += line(`r = p_Engine->RegisterObjectBehaviour("${p_Type.scripting_name}", asBEHAVE_CONSTRUCT, "void f(const ${p_Type.scripting_name}& in)", asFUNCTION(${TYPE_PREFIX}_copy_construct), asCALL_CDECL_OBJLAST);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose copy constructor of ${l_TypeString}.");`)
  t += empty();
  t += line(`r = p_Engine->RegisterObjectBehaviour("${p_Type.scripting_name}", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(${TYPE_PREFIX}_destruct), asCALL_CDECL_OBJLAST);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose destructor of ${l_TypeString}.");`)

  t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "bool get_alive() const property", asMETHODPR(${l_TypeString}, is_alive, () const, bool), asCALL_THISCALL);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose is_alive getter for ${l_TypeString}.");`)
  t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "void destroy()", asMETHODPR(${l_TypeString}, destroy, () , void), asCALL_THISCALL);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose destroy for ${l_TypeString}.");`)

  /*
  t += empty();
  t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "Name get_name() const property", asMETHOD(${l_TypeString}, get_name), asCALL_THISCALL);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose property getter for name of ${l_TypeString}.");`)

  t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "void set_name(Name) property", asMETHOD(${l_TypeString}, set_name), asCALL_THISCALL);`)
  t += line(`LOW_ASSERT(r >= 0, "Failed to expose property setter for name of ${l_TypeString}.");`)
  */

  for (let [i_PropName, i_Prop] of Object.entries(p_Type.properties)) {
    if (!i_Prop.expose_scripting) {
      continue;
    }
    if (i_Prop.type.container){
      continue
    }
    t += empty();

    if (i_Prop.getter_name == i_Prop.setter_name) {
      if (i_Prop.getter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "${i_Prop.type.scripting} get_${i_PropName}() const property", asMETHODPR(${l_TypeString}, ${i_Prop.getter_name}, () const, ${i_Prop.type.string}), asCALL_THISCALL);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property getter for ${i_PropName} of ${l_TypeString}.");`)
      }

      if (i_Prop.setter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "void set_${i_PropName}(${i_Prop.type.scripting}) property", asMETHODPR(${l_TypeString}, ${i_Prop.setter_name}, (${i_Prop.type.string}), void), asCALL_THISCALL);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property setter for ${i_PropName} of ${l_TypeString}.");`)
      }
    }
    else if (i_Prop.type.scripting == 'string') {
      if (i_Prop.getter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "${i_Prop.type.scripting} get_${i_PropName}() const property", asFUNCTION(${TYPE_PREFIX}_get_${i_Prop.name}), asCALL_CDECL_OBJFIRST);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property getter for ${i_PropName} of ${l_TypeString}.");`)
      }

      if (i_Prop.setter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "void set_${i_PropName}(${i_Prop.type.scripting}) property", asFUNCTION(${TYPE_PREFIX}_set_${i_Prop.name}), asCALL_CDECL_OBJFIRST);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property setter for ${i_PropName} of ${l_TypeString}.");`)
      }
    } else {
      if (i_Prop.getter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "${i_Prop.type.scripting} get_${i_PropName}() const property", asMETHOD(${l_TypeString}, ${i_Prop.getter_name}), asCALL_THISCALL);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property getter for ${i_PropName} of ${l_TypeString}.");`)
      }

      if (i_Prop.setter_exposed_scripting){
        t += line(`r = p_Engine->RegisterObjectMethod("${p_Type.scripting_name}", "void set_${i_PropName}(${i_Prop.type.scripting}) property", asMETHOD(${l_TypeString}, ${i_Prop.setter_name}), asCALL_THISCALL);`)
        t += line(`LOW_ASSERT(r >= 0, "Failed to expose property setter for ${i_PropName} of ${l_TypeString}.");`)
      }
    }
  }
  t += empty();

  if (p_Type.scripting_namespace.length > 0) {
    t += line(`r = p_Engine->SetDefaultNamespace("${p_Type.full_scripting_string}");`);
  }
  else {
    t += line(`r = p_Engine->SetDefaultNamespace("${p_Type.scripting_name}");`);
  }
  t += line(`LOW_ASSERT(r >= 0, "Failed to set namespace for ${l_TypeString}.");`)

  if (p_Type.any_component_type) {

  } else if (!p_Type.private_make){
    t += line(`r = p_Engine->RegisterGlobalFunction("${p_Type.scripting_name} make(Name)", asFUNCTION(${TYPE_PREFIX}_genmake), asCALL_CDECL);`)
    t += line(`LOW_ASSERT(r >= 0, "Failed to expose generic make function for ${l_TypeString}.");`)

    t += line(`r = p_Engine->RegisterGlobalFunction("${p_Type.scripting_name} find_by_name(Name)", asFUNCTION(${TYPE_PREFIX}_genfindbyname), asCALL_CDECL);`)
    t += line(`LOW_ASSERT(r >= 0, "Failed to expose generic find by name function for ${l_TypeString}.");`)
  }


  t += line(`r = p_Engine->SetDefaultNamespace("");`);
  t += line(`LOW_ASSERT(r >= 0, "Failed to reset default namespace.");`)

  if (true) {
    const l_MarkerName = `CUSTOM:${p_Type.identifier.toUpperCase()}:EXPOSE`;

    const l_CustomBeginMarker = get_marker_begin(l_MarkerName);
    const l_CustomEndMarker = get_marker_end(l_MarkerName);

    const l_BeginMarkerIndex = find_begin_marker_end(l_OldCode, l_MarkerName);

    let l_CustomCode = "";

    if (l_BeginMarkerIndex >= 0) {
      const l_EndMarkerIndex = find_end_marker_start(l_OldCode, l_MarkerName);

      l_CustomCode = l_OldCode.substring(l_BeginMarkerIndex, l_EndMarkerIndex);
    }
    t += line(l_CustomBeginMarker);
    t += l_CustomCode;
    t += line(l_CustomEndMarker);
    t += empty();
  }

  t += line("}")
  t += empty()

  return t;

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

  const db = build_database_for(l_Path)

  const l_FilePath = `../LowCore/src/LowCoreScriptingRegister.gen.cpp`;

  if (fs.existsSync(l_FilePath)) {
    l_OldCode = read_file(l_FilePath);
  }

  let l_IncludeCode = ''
  l_IncludeCode += line(`#include <angelscript.h>`)
  l_IncludeCode += empty()
  let l_MainCode = ''

  let l_RegisterCode = 'namespace Low::Core { void expose_types(asIScriptEngine* p_Engine) {\n'

  for (const s of l_Types) {
    const i_Type = db.types[s.identifier]
    if (!i_Type.scripting_expose) {
      continue
    }
    const TYPE_PREFIX = `${i_Type.module}_${i_Type.name}`;
    l_IncludeCode += line(i_Type.include_line)
    l_MainCode += generate_type_api(i_Type, db);
    l_RegisterCode += line(`expose_${TYPE_PREFIX}(p_Engine);`)
  }
  l_RegisterCode += '}}'

  const l_FileContent = `${l_IncludeCode}\n${l_MainCode}\n\n${l_RegisterCode}`


  const l_Formatted = format(l_FilePath, l_FileContent);
  save_file(l_FilePath, l_Formatted);
  console.log("✔️ Finished generating scripting API");

}

main();
