const { assert } = require("console");
const fs = require("fs");
const path = require("path");
const YAML = require("yaml");
const { spawnSync } = require("child_process");

const {
  load_project_info,
  write_file_if_changed,
  normalize_string_path_and_make_absolute,
  output_section,
  output_error,
  output_info,
  output_warn,
  output_task_success,
  output_task_failed,
  output_task_cancelled,
  output_begin_task,
  output_update_task,
  output_detail,
  output_debug,
  output_result,
} = require("./lib.js");

const CMAKE_DIR = "${LOW_PROJECT_ROOT}/.low/cmake";

function parse_arguments(p_Argv) {
  const l_Result = {
    project: undefined,
    buildDir: undefined,
    buildId: undefined,
    defines: [],
  };
  for (let i = 0; i < p_Argv.length; ++i) {
    const i_Arg = p_Argv[i];
    if (i_Arg === "--build-dir") {
      l_Result.buildDir = p_Argv[++i];
    } else if (i_Arg === "--build-id") {
      l_Result.buildId = p_Argv[++i];
    } else if (i_Arg.startsWith("-D")) {
      l_Result.defines.push(i_Arg);
    } else if (!l_Result.project) {
      l_Result.project = i_Arg;
    } else {
      throw new Error(`Unknown argument ${i_Arg}`);
    }
  }
  return l_Result;
}

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

function generate_module_dll_api(p_Module) {
  output_debug("Generating module API DLL");
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

  write_file_if_changed(p_Module.api_header_path, t);

  output_debug("Done generating module API DLL");
}

// git describe of the project repo, e.g. misteda-v0.1.0-3-gabc1234-dirty.
// Only evaluated on configure, like the engine's LOW_ENGINE_BUILD_ID.
function get_project_build_id(p_ProjectConfig) {
  const l_Result = spawnSync(
    "git",
    [
      "describe",
      "--tags",
      "--match",
      `${p_ProjectConfig.name.toLowerCase()}-v*`,
      "--always",
      "--dirty",
    ],
    { cwd: p_ProjectConfig.root, encoding: "utf8" },
  );

  if (l_Result.error || l_Result.status !== 0) {
    output_warn("Could not determine the project build id via git");
    return "unknown";
  }

  return l_Result.stdout.trim();
}

function to_cpp_string_literal(p_Value) {
  return `"${String(p_Value).replace(/\\/g, "\\\\").replace(/"/g, '\\"')}"`;
}

// Reads a top-level field exactly as written. The normal parse turns 1.10
// into the number 1.1, the failsafe schema keeps every scalar as a string.
function read_yaml_as_string(p_YamlPath, p_Key) {
  if (!fs.existsSync(p_YamlPath)) {
    return undefined;
  }
  const l_Root = YAML.parse(read_file(p_YamlPath), { schema: "failsafe" });
  if (!l_Root || typeof l_Root[p_Key] !== "string") {
    return undefined;
  }
  return l_Root[p_Key];
}

function generate_module_info_source(p_Module, p_ProjectConfig) {
  output_debug("Generating module info source");

  const l_Version =
    read_yaml_as_string(`${p_Module.path}/module.yaml`, "version") ??
    read_yaml_as_string(p_ProjectConfig.config_path, "version") ??
    "0.0.0";

  let t = "";
  t +=
    "// THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";
  t += `#include "${path.basename(p_Module.api_header_path)}"\n\n`;
  t += '#include "LowUtilModuleInfo.h"\n';
  t += '#include "LowUtilVersion.h"\n\n';

  t += `extern "C" ${p_Module.api_macro} const Low::Util::ModuleInfo *\n`;
  t += "low_get_module_info()\n";
  t += "{\n";
  t += "  static const Low::Util::ModuleInfo l_Info = {\n";
  t += "      LOW_MODULE_INFO_STRUCT_VERSION,\n";
  t += `      ${to_cpp_string_literal(p_Module.name)},\n`;
  t += `      ${to_cpp_string_literal(l_Version)},\n`;
  t += `      LOW_PROJECT_BUILD_ID,\n`;
  t += "      LOW_VERSION_STRING,\n";
  t += "      LOW_ENGINE_BUILD_ID,\n";
  t += "  };\n";
  t += "  return &l_Info;\n";
  t += "}\n";

  write_file_if_changed(p_Module.module_info_source_path, t);

  output_debug("Done generating module info source");
}

function generate_module_cmake(p_Module, p_ProjectModuleNames) {
  output_debug("Generating module CMakeLists.txt");
  let t = "";
  t +=
    "# THIS FILE HAS BEEN GENERATED AUTOMATICALLY BY THE LOWENGINE BUILD SYSTEM\n\n";

  t += `project(${p_Module.name})\n\n`;

  t += `file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "\${LOW_PROJECT_ROOT}/modules/${p_Module.name}/src/*.cpp")\n`;
  t += `file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "\${LOW_PROJECT_ROOT}/modules/${p_Module.name}/include/*.h")\n`;

  t += `add_library(${p_Module.name} SHARED\n`;
  t += "  ${SOURCES}\n  ${INCLUDES}\n";
  t += `  "${to_cmake_path(p_Module.module_info_source_path)}"\n)\n\n`;

  t += 'source_group("Header Files" FILES ${INCLUDES})\n\n';

  t += `target_compile_definitions(${p_Module.name} PRIVATE\n  ${p_Module.exports_marker}\n  LOW_MODULE_NAME="${p_Module.name}"\n)\n\n`;

  t +=
    `target_include_directories(${p_Module.name} PUBLIC` +
    `\n  \${LOW_PROJECT_ROOT}/modules/${p_Module.name}/include` +
    `\n  \${LOW_PROJECT_ROOT}/.low/generated/modules/${p_Module.name}/include\n)\n\n`;
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

  t += `set_source_files_properties("${to_cmake_path(p_Module.module_info_source_path)}" PROPERTIES\n`;
  t +=
    '  COMPILE_DEFINITIONS "LOW_PROJECT_BUILD_ID=\\"${LOW_PROJECT_BUILD_ID}\\""\n';
  t += ")\n\n";

  // Only project modules produce a lensdb. Relies on the root CMakeLists
  // adding modules in dependency order so LOW_LENSDB_<dep> is already set.
  const l_ProjectDependencies = get_project_dependencies(
    p_Module,
    p_ProjectModuleNames,
  );

  t += `low_lens_generate(${p_Module.name} ${p_Module.name} \${LOW_PROJECT_ROOT}/modules/${p_Module.name}/include\n`;
  t += '  UPSTREAM_DB "${LOW_LENSDB_LowCore}"\n';
  for (const i_Dependency of l_ProjectDependencies) {
    t += `              "\${LOW_LENSDB_${i_Dependency}}"\n`;
  }
  t += '  YAML_CONFIGS "${LOW_PATH}/" "${LOW_PROJECT_ROOT}/" PROJECT\n';
  t += `  API_MACRO ${p_Module.api_macro}\n`;
  t += `  API_HEADER ${path.basename(p_Module.api_header_path)}\n`;
  t += `)\n\n`;

  // The build-time LowLens run has to wait for the dependency's run, since
  // it reads the dependency's lensdb
  if (l_ProjectDependencies.length > 0) {
    t += `add_dependencies(${p_Module.name}.Lens\n`;
    for (const i_Dependency of l_ProjectDependencies) {
      t += `  ${i_Dependency}.Lens\n`;
    }
    t += `)\n\n`;
  }

  t += `install(TARGETS ${p_Module.name} RUNTIME DESTINATION bin COMPONENT modules)\n\n`;

  write_file_if_changed(p_Module.cmake_path, t);

  output_debug("Done generating module CMakeLists.txt");
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
      throw new Error(`Circular module dependency involving ${p_Module.name}`);
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
  //t += `set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "RelWithDebInfo")\n`;
  t += `set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Supported configurations" FORCE)\n`;
  t += `set(TARGET_BUILD_PLATFORM "windows")\n`;
  t += `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n`;
  t += `set(CMAKE_CXX_STANDARD 20)\n`;
  t += `set(CMAKE_CXX_STANDARD_REQUIRED ON)\n`;
  t += `set(CMAKE_CXX_EXTENSIONS OFF)\n\n`;
  t += "add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)\n\n";

  t += `set(LOW_PATH ${get_engine_root()})\n`;
  t += `set(LOW_PROJECT_ROOT "${p_ProjectConfig.root}")\n\n`;

  // Engine and project modules inherit these through directory scope, so
  // lowder, all engine DLLs and the module DLLs end up next to each other
  t +=
    'set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LOW_PROJECT_ROOT}/.low/bin/$<CONFIG>")\n';
  t +=
    'set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")\n';
  t +=
    'set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")\n';
  t +=
    'set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/pdb")\n\n';

  t += 'set(CMAKE_DEBUG_POSTFIX "")\n\n';

  // The engine sorts its own targets into the LowEngine solution folder
  t += "add_subdirectory(${LOW_PATH} engine)\n\n";

  for (const i_Module of p_ProjectConfig.modules) {
    t += `add_subdirectory("${CMAKE_DIR}/modules/${i_Module.name}")\n`;
    t += `low_set_folder("${CMAKE_DIR}/modules/${i_Module.name}" "Modules")\n\n`;
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

  // Launch target: builds everything and runs lowder on this project (F5)
  t += `add_custom_target(${p_ProjectConfig.name} SOURCES \${${l_VisualizersVar}})\n\n`;

  t += `add_dependencies(${p_ProjectConfig.name}\n`;
  for (const i_Module of p_ProjectConfig.modules) {
    t += `  ${i_Module.name}\n`;
  }
  t += `  Lowder\n`;
  t += `)\n\n`;

  t += `set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${p_ProjectConfig.name})\n\n`;

  t += `set_target_properties(${p_ProjectConfig.name} PROPERTIES\n`;
  t += '  VS_DEBUGGER_COMMAND "$<TARGET_FILE:Lowder>"\n';
  t += '  VS_DEBUGGER_COMMAND_ARGUMENTS "${LOW_PROJECT_ROOT}"\n';
  t += '  VS_DEBUGGER_WORKING_DIRECTORY "${LOW_PROJECT_ROOT}"\n';
  t += `)\n\n`;

  write_file_if_changed(p_ProjectConfig.root_cmake_path, t);
}

function build_project(p_Arguments) {
  if (!p_Arguments.project) {
    output_error("Did not pass project.");
    process.exitCode = 1;
    return;
  }
  output_section("Configuring project");
  const l_FullProjectPath = to_cmake_path(p_Arguments.project);
  output_detail(l_FullProjectPath);

  const l_ProjectConfig = load_project_info(l_FullProjectPath);
  l_ProjectConfig.root =
    normalize_string_path_and_make_absolute(l_FullProjectPath);
  l_ProjectConfig.modules = sort_modules_by_dependencies(
    l_ProjectConfig.modules,
  );
  const l_ProjectModuleNames = l_ProjectConfig.modules.map(
    (i_Module) => i_Module.name,
  );

  const l_BuildId =
    p_Arguments.buildId ?? get_project_build_id(l_ProjectConfig);

  const l_BuildDir = to_cmake_path(
    path.resolve(l_FullProjectPath, p_Arguments.buildDir ?? ".low/build"),
  );

  generate_root_cmake(l_ProjectConfig);

  for (const i_Module of l_ProjectConfig.modules) {
    i_Module.module_info_source_path = `${l_ProjectConfig.root}/.low/generated/modules/${i_Module.name}/src/${i_Module.name}ModuleInfo.cpp`;
    generate_module_dll_api(i_Module);
    generate_module_info_source(i_Module, l_ProjectConfig);
    generate_module_cmake(i_Module, l_ProjectModuleNames);
  }

  process.chdir(l_FullProjectPath);
  output_begin_task("cmake", "Running CMake");
  const child = spawnSync("cmake", [
    "-S",
    `${l_FullProjectPath}/.low/cmake`,
    "-B",
    l_BuildDir,
    `-DLOW_PROJECT_BUILD_ID=${l_BuildId}`,
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    ...p_Arguments.defines,
  ]);

  const l_ErrorString = child.stderr.toString();
  if (l_ErrorString.includes("CMake Error")) {
    output_task_failed("cmake");
    const l_ErrorLines = child.stderr.toString().split("\n");
    output_error("CMake configure failed", l_ErrorLines);
    process.exitCode = 2;
    return;
  }

  output_task_success("cmake");
}

build_project(parse_arguments(process.argv.slice(2)));
