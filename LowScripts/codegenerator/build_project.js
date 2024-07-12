const g_FullProjectPath = "P:/misteda";

const { assert } = require("console");
const fs = require("fs");
const YAML = require("yaml");
const { spawnSync } = require("child_process");

const { load_project_info } = require("./lib.js");

function read_file(p_FilePath) {
  return fs.readFileSync(p_FilePath, { encoding: "utf8", flag: "r" });
}

function cleanup_cmake_files(p_ProjectConfig) {
  fs.unlinkSync(p_ProjectConfig.root_cmake_path);

  for (const i_Module of p_ProjectConfig.modules) {
    fs.unlinkSync(i_Module.cmake_path);
  }
}

function generate_module_dll_api(p_Module) {
  let t = "";
  t += `#ifndef ${p_Module.project_name.toUpperCase()}_${p_Module.name.toUpperCase()}_EXPORT_H\n`;
  t += `#define ${p_Module.project_name.toUpperCase()}_${p_Module.name.toUpperCase()}_EXPORT_H\n\n`;
  t +=
    "// THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";

  t += `#ifdef ${p_Module.static_marker}\n`;
  t += `#define ${p_Module.api_macro}\n`;
  t += `#define ${p_Module.no_export_macro}\n`;
  t += `#else\n`;
  t += `#ifndef ${p_Module.api_macro}\n`;
  t += `#ifdef ${p_Module.exports_marker}\n`;
  t += `/* We are building this library */\n`;
  t += `#define ${p_Module.api_macro} __declspec(dllexport)\n`;
  t += `#else\n`;
  t += `/* We are using this library */\n`;
  t += `#define ${p_Module.api_macro} __declspec(dllimport)\n`;
  t += `#endif\n`;
  t += `#endif\n`;
  t += `#endif\n\n`;

  t += `#ifndef ${p_Module.no_export_macro}\n`;
  t += `#define ${p_Module.no_export_macro}\n`;
  t += `#endif\n\n`;

  t += `#ifndef ${p_Module.deprecated_macro}\n`;
  t += `#define ${p_Module.deprecated_macro} __declspec(deprecated)\n`;
  t += `#endif\n\n`;

  t += `#ifndef ${p_Module.deprecated_export_macro}\n`;
  t += `#define ${p_Module.deprecated_export_macro} ${p_Module.api_macro} ${p_Module.deprecated_macro}\n`;
  t += `#endif\n\n`;

  t += `#ifndef ${p_Module.deprecated_no_export_macro}\n`;
  t += `#define ${p_Module.deprecated_no_export_macro} ${p_Module.no_export_macro} ${p_Module.deprecated_macro}\n`;
  t += `#endif\n\n`;

  t += "#endif /* EXPORT_H */\n";

  fs.writeFileSync(p_Module.api_header_path, t);
}

function generate_module_cmake(p_Module) {
  let t = "";
  t +=
    "# THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";

  t += `project(${p_Module.name})\n\n`;

  t += `file(GLOB_RECURSE SOURCES "src/*.cpp")\n`;
  t += `file(GLOB_RECURSE INCLUDES "include/*.h")\n\n`;

  t += `add_library(${p_Module.name} SHARED\n`;
  t += "  ${SOURCES}\n  ${INCLUDES}\n)\n\n";

  t += 'source_group("Header Files" FILES ${INCLUDES})\n\n';

  t += "foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})\n";
  t += "  string(TOUPPER ${OUTPUTCONFIG} UOUTPUTCONFIG)\n";
  t += `  set_target_properties(${p_Module.name} PROPERTIES `;
  /*
  t +=
    "RUNTIME_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/app)\n";
    */
  t +=
    "RUNTIME_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG})\n";
  t += `  set_target_properties(${p_Module.name} PROPERTIES `;
  t +=
    "LIBRARY_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/lib)\n";
  t += `  set_target_properties(${p_Module.name} PROPERTIES `;
  t +=
    "ARCHIVE_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/lib)\n";
  t += `  set_target_properties(${p_Module.name} PROPERTIES `;
  t +=
    "PDB_OUTPUT_DIRECTORY_${UOUTPUTCONFIG} ${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/pdb)\n";
  t += "endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)\n\n";

  t += `target_compile_definitions(${p_Module.name} PRIVATE\n  ${p_Module.exports_marker}\n  LOW_MODULE_NAME="${p_Module.name}"\n)\n\n`;

  t += "include_directories(\n  ${CMAKE_CURRENT_SOURCE_DIR}/include\n)\n";
  t +=
    `target_include_directories(${p_Module.name} PUBLIC` +
    "\n  ${CMAKE_CURRENT_SOURCE_DIR}/include\n)\n\n";
  t += `set_target_properties(${p_Module.name} PROPERTIES OUTPUT_NAME "${p_Module.name}")\n\n`;

  t += `add_dependencies(${p_Module.name}\n`;
  t += `  LowUtil\n`;
  t += `  LowMath\n`;
  t += `  LowRenderer\n`;
  t += `  LowCore\n`;
  t += `)\n\n`;

  t += `target_link_libraries(${p_Module.name} PUBLIC\n`;
  t += `  LowUtil\n`;
  t += `  LowMath\n`;
  t += `  LowRenderer\n`;
  t += `  LowCore\n`;
  t += `)\n\n`;

  if (p_Module.public_dependencies) {
    t += `add_dependencies(${p_Module.name}\n`;
    for (const i_Entry of p_Module.public_dependencies) {
      t += `  ${i_Entry}\n`;
    }
    t += `)\n\n`;
  }

  if (p_Module.public_dependencies) {
    t += `target_link_libraries(${p_Module.name} PUBLIC\n`;
    for (const i_Entry of p_Module.public_dependencies) {
      t += `  ${i_Entry}\n`;
    }
    t += `)\n\n`;
  }

  fs.writeFileSync(p_Module.cmake_path, t);
}

function generate_root_cmake(p_ProjectConfig) {
  let t = "";
  t +=
    "# THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";
  t += `project(${p_ProjectConfig.name})\n\n`;
  t += `cmake_minimum_required(VERSION 3.10)\n\n`;
  t += `set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")\n`;
  t += `set(TARGET_BUILD_PLATFORM "windows")\n`;
  t += `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n`;
  t += `set(CMAKE_CXX_STANDARD 17)\n`;
  t += `set(CMAKE_CXX_STANDARD_REQUIRED ON)\n`;
  t += `set(CMAKE_CXX_EXTENSIONS OFF)\n\n`;

  t += `set(LOW_PATH ${p_ProjectConfig.engine_root})\n\n`;

  t += "add_subdirectory(${LOW_PATH} EXTERNAL)\n\n";

  t += 'set(CMAKE_DEBUG_POSTFIX "")\n\n';

  t += 'set_target_properties(LowCore PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowUtil PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowRenderer PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(Lowder PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowEditor PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowMath PROPERTIES FOLDER "LowEngine")\n\n';

  for (const i_Module of p_ProjectConfig.modules) {
    t += `add_subdirectory(modules/${i_Module.name})\n`;
    t += `set_target_properties(${i_Module.name} PROPERTIES FOLDER "${p_ProjectConfig.name}Modules")\n\n`;
  }

  t += `add_custom_target(${p_ProjectConfig.name}Project)\n\n`;

  t += `add_dependencies(${p_ProjectConfig.name}Project\n`;
  for (const i_Module of p_ProjectConfig.modules) {
    t += `  ${i_Module.name}\n`;
  }
  t += `  Lowder\n`;
  t += `)\n\n`;

  t += `set_target_properties(${p_ProjectConfig.name}Project PROPERTIES\n`;
  t += '  VS_DEBUGGER_COMMAND "${LOW_PATH}/build/Debug/app/lowder.exe"\n';
  t += `  VS_DEBUGGER_COMMAND_ARGUMENTS "${p_ProjectConfig.full_path}"\n`;
  t += `  VS_DEBUGGER_WORKING_DIRECTORY "${p_ProjectConfig.full_path}"\n`;
  t += `)\n\n`;

  fs.writeFileSync(p_ProjectConfig.root_cmake_path, t);
}

function build_project(p_FullProjectPath) {
  console.log(`⚙️ Building ${p_FullProjectPath}`);

  const l_ProjectConfig = load_project_info(p_FullProjectPath);

  generate_root_cmake(l_ProjectConfig);

  for (const i_Module of l_ProjectConfig.modules) {
    generate_module_dll_api(i_Module);
    generate_module_cmake(i_Module);
  }

  process.chdir(p_FullProjectPath);
  console.log("⚙️ Running CMake...");
  const child = spawnSync("cmake", ["-B", "build"]);

  const l_ErrorString = child.stderr.toString();
  if (l_ErrorString.includes("CMake Error")) {
    //console.log("🗑️ Cleaning up CMake files");
    //cleanup_cmake_files(l_ProjectConfig);
    console.log("❌ An error occured");
    console.log(child.stderr.toString());
    return;
  }

  //console.log("🗑️ Cleaning up CMake files");
  //cleanup_cmake_files(l_ProjectConfig);

  console.log("✔️ Configuration successful");
}

build_project(g_FullProjectPath);
