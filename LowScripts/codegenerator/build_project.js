const { assert } = require("console");
const fs = require("fs");
const path = require("path");
const YAML = require("yaml");
const { spawnSync } = require("child_process");

const { load_project_info } = require("./lib.js");

function to_cmake_path(p_Path) {
  return path.resolve(p_Path).replace(/\\/g, "/");
}

// This script lives in <engine>/LowScripts/codegenerator, so the engine root
// is derived from its location instead of the machine-specific project.yaml
function get_engine_root() {
  return to_cmake_path(path.resolve(__dirname, "../.."));
}

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

function generate_module_cmake(p_Module, p_ProjectModuleNames) {
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
  t += `  LowRenderer2\n`;
  t += `  LowCore\n`;
  t += `)\n\n`;

  t += `target_link_libraries(${p_Module.name} PUBLIC\n`;
  t += `  LowUtil\n`;
  t += `  LowMath\n`;
  t += `  LowRenderer2\n`;
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

  // Only project modules produce a lensdb. Relies on the root CMakeLists
  // adding modules in dependency order so LOW_LENSDB_<dep> is already set.
  const l_ProjectDependencies = get_project_dependencies(
    p_Module,
    p_ProjectModuleNames,
  );

  t += `low_lens_generate(${p_Module.name} ${p_Module.name} \${CMAKE_CURRENT_SOURCE_DIR}/include\n`;
  t += '  UPSTREAM_DB "${LOW_LENSDB_LowCore}"\n';
  for (const i_Dependency of l_ProjectDependencies) {
    t += `              "\${LOW_LENSDB_${i_Dependency}}"\n`;
  }
  t += '  YAML_CONFIGS "${LOW_PATH}/" "${CMAKE_SOURCE_DIR}/" PROJECT\n';
  t += `  API_MACRO ${p_Module.api_macro}\n`;
  t += `  API_HEADER ${path.basename(p_Module.api_header_path)}\n`;
  t += `)\n\n`;

  // The build-time LowLens run has to wait for the dependency's run, since
  // it reads the dependency's lensdb
  if (l_ProjectDependencies.length > 0) {
    t += `add_dependencies(${p_Module.name}_LowLensGenerate\n`;
    for (const i_Dependency of l_ProjectDependencies) {
      t += `  ${i_Dependency}_LowLensGenerate\n`;
    }
    t += `)\n\n`;
  }

  fs.writeFileSync(p_Module.cmake_path, t);
}

function get_project_dependencies(p_Module, p_ProjectModuleNames) {
  if (!p_Module.public_dependencies) {
    return [];
  }

  return p_Module.public_dependencies.filter((i_Entry) =>
    p_ProjectModuleNames.includes(i_Entry),
  );
}

// Orders modules so every module comes after the project modules it depends
// on. CMake processes add_subdirectory() calls in order, so this is what makes
// a dependency's LOW_LENSDB_<name> visible to its dependents.
function sort_modules_by_dependencies(p_Modules) {
  const l_ModuleNames = p_Modules.map((i_Module) => i_Module.name);
  const l_Sorted = [];
  const l_Visited = new Set();
  const l_Visiting = new Set();

  function visit(p_Module) {
    if (l_Visited.has(p_Module.name)) {
      return;
    }
    if (l_Visiting.has(p_Module.name)) {
      throw new Error(
        `Circular module dependency involving ${p_Module.name}`,
      );
    }

    l_Visiting.add(p_Module.name);
    for (const i_Dependency of get_project_dependencies(
      p_Module,
      l_ModuleNames,
    )) {
      visit(p_Modules.find((i_Module) => i_Module.name === i_Dependency));
    }
    l_Visiting.delete(p_Module.name);

    l_Visited.add(p_Module.name);
    l_Sorted.push(p_Module);
  }

  for (const i_Module of p_Modules) {
    visit(i_Module);
  }

  return l_Sorted;
}

function generate_root_cmake(p_ProjectConfig) {
  let t = "";
  t +=
    "# THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";
  t += `project(${p_ProjectConfig.name})\n\n`;
  t += `cmake_minimum_required(VERSION 3.10)\n\n`;
  t += `option(RELEASE_BUILD "Enable release build")\n\n`;
  t += 'set(ENGINE_BUILD "${RELEASE_BUILD}")\n\n';
  //t += `set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "RelWithDebInfo")\n`;
  t += `set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Supported configurations" FORCE)\n`;
  t += `set(TARGET_BUILD_PLATFORM "windows")\n`;
  t += `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n`;
  t += `set(CMAKE_CXX_STANDARD 20)\n`;
  t += `set(CMAKE_CXX_STANDARD_REQUIRED ON)\n`;
  t += `set(CMAKE_CXX_EXTENSIONS OFF)\n\n`;
  t += "add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)\n\n";

  t += `set(LOW_PATH ${get_engine_root()})\n\n`;

  t += "add_subdirectory(${LOW_PATH} EXTERNAL)\n\n";

  t += 'set(CMAKE_DEBUG_POSTFIX "")\n\n';

  t += 'set_target_properties(LowCore PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowUtil PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowRenderer PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowRenderer2 PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(Lowder PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowEditor PROPERTIES FOLDER "LowEngine")\n';
  t += 'set_target_properties(LowMath PROPERTIES FOLDER "LowEngine")\n\n';

  for (const i_Module of p_ProjectConfig.modules) {
    t += `add_subdirectory(modules/${i_Module.name})\n`;
    t += `set_target_properties(${i_Module.name} PROPERTIES FOLDER "${p_ProjectConfig.name}Modules")\n\n`;
  }

  const l_VisualizersVar = `${p_ProjectConfig.name.toUpperCase()}_DEBUG_VISUALIZERS`;

  t += `set(${l_VisualizersVar}\n`;
  t += '  "${LOW_PATH}/LowDependencies/EASTL/doc/EASTL.natvis"\n';
  t += '  "${LOW_PATH}/LowDebug.natvis"\n';
  t += ")\n\n";

  t += "if (MSVC)\n";
  for (const i_Module of p_ProjectConfig.modules) {
    t += `  target_sources(${i_Module.name} PRIVATE \${${l_VisualizersVar}})\n`;
  }
  t += `  source_group("Debug Visualizers" FILES \${${l_VisualizersVar}})\n`;
  t += "endif()\n\n";

  t += `add_custom_target(${p_ProjectConfig.name}Project SOURCES \${${l_VisualizersVar}})\n\n`;

  t += `add_dependencies(${p_ProjectConfig.name}Project\n`;
  for (const i_Module of p_ProjectConfig.modules) {
    t += `  ${i_Module.name}\n`;
  }
  t += `  Lowder\n`;
  t += `)\n\n`;

  t += `set_target_properties(${p_ProjectConfig.name}Project PROPERTIES\n`;
  t += '  VS_DEBUGGER_COMMAND "${LOW_PATH}/build/Debug/app/lowder.exe"\n';
  t += '  VS_DEBUGGER_COMMAND_ARGUMENTS "${CMAKE_SOURCE_DIR}"\n';
  t += '  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"\n';
  t += `)\n\n`;

  fs.writeFileSync(p_ProjectConfig.root_cmake_path, t);
}

function build_project(p_FullProjectPath) {
  console.log(`⚙️ Building ${p_FullProjectPath}`);

  const l_ProjectConfig = load_project_info(p_FullProjectPath);
  l_ProjectConfig.modules = sort_modules_by_dependencies(
    l_ProjectConfig.modules,
  );
  const l_ProjectModuleNames = l_ProjectConfig.modules.map(
    (i_Module) => i_Module.name,
  );

  generate_root_cmake(l_ProjectConfig);

  for (const i_Module of l_ProjectConfig.modules) {
    generate_module_dll_api(i_Module);
    generate_module_cmake(i_Module, l_ProjectModuleNames);
  }

  process.chdir(p_FullProjectPath);
  console.log("⚙️ Running CMake...");
  const child = spawnSync("cmake", ["-B", "build", "-DRELEASE_BUILD=OFF", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"]);

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

// Usage: node build_project.js <project_path>
if (process.argv.length < 3) {
  console.error("Usage: node build_project.js <project_path>");
  process.exit(1);
}

build_project(to_cmake_path(process.argv[2]));
