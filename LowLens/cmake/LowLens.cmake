find_program(LOW_NODE_EXECUTABLE node REQUIRED)
set(LOW_LENSGEN_SCRIPT "${LOW_SOURCE_PATH}/LowLens/index.js")

# low_lens_generate(<target> <module_name> <include_dir>
#   [UPSTREAM_DB <path>...]
#   [YAML_CONFIGS <path> [PROJECT]...]
# )
#
# UPSTREAM_DB:   lensdb.yaml files from modules this target depends on
# YAML_CONFIGS:  engine/project root paths to read YAML type configs from.
#                Append PROJECT after a path to treat it as a game project root.
function(low_lens_generate p_Target p_ModuleName p_IncludeDir)
  cmake_parse_arguments(LENS "" "" "UPSTREAM_DB;YAML_CONFIGS" ${ARGN})

  set(l_Output "${CMAKE_CURRENT_BINARY_DIR}/${p_ModuleName}.gen.cpp")
  set(l_DbOutput "${CMAKE_CURRENT_BINARY_DIR}/${p_ModuleName}.lensdb.yaml")
  set(l_Stamp "${CMAKE_CURRENT_BINARY_DIR}/${p_ModuleName}.lowlens.stamp")
  set(l_GenDir "${CMAKE_CURRENT_BINARY_DIR}/gen")

  file(GLOB_RECURSE l_Headers
    "${p_IncludeDir}/*.h"
    "${p_IncludeDir}/*.hpp"
  )

  # Build extra args list
  set(l_ExtraArgs "")
  foreach(i_Db IN LISTS LENS_UPSTREAM_DB)
    list(APPEND l_ExtraArgs "--upstream-db" "${i_Db}")
  endforeach()

  set(l_YamlIsProject FALSE)
  foreach(i_Cfg IN LISTS LENS_YAML_CONFIGS)
    if(i_Cfg STREQUAL "PROJECT")
      list(APPEND l_ExtraArgs "--yaml-project")
    else()
      list(APPEND l_ExtraArgs "--yaml-configs" "${i_Cfg}")
    endif()
  endforeach()

  # Run at configure time so .gen.h files exist before compilation
  file(MAKE_DIRECTORY "${l_GenDir}")
  execute_process(
    COMMAND "${LOW_NODE_EXECUTABLE}" "${LOW_LENSGEN_SCRIPT}"
            "${p_IncludeDir}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${p_ModuleName}"
            ${l_ExtraArgs}
  )

  # Re-run at build time for incremental rebuilds. The stamp is the actual
  # custom-command output so generated files can keep stable timestamps when
  # their content has not changed.
  add_custom_command(
    OUTPUT  "${l_Stamp}"
    BYPRODUCTS "${l_Output}" "${l_DbOutput}"
    COMMAND "${LOW_NODE_EXECUTABLE}" "${LOW_LENSGEN_SCRIPT}"
            "${p_IncludeDir}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${p_ModuleName}"
            ${l_ExtraArgs}
    COMMAND "${CMAKE_COMMAND}" -E touch "${l_Stamp}"
    DEPENDS "${LOW_LENSGEN_SCRIPT}" ${l_Headers} ${LENS_UPSTREAM_DB}
    COMMENT "LowLens: generating ${p_ModuleName}.gen.cpp"
    VERBATIM
  )

  add_custom_target(${p_Target}_LowLensGenerate DEPENDS "${l_Stamp}")
  add_dependencies(${p_Target} ${p_Target}_LowLensGenerate)

  # Expose the lensdb path so downstream targets can reference it
  set(LOW_LENSDB_${p_ModuleName} "${l_DbOutput}" CACHE INTERNAL "")

  set_source_files_properties("${l_Output}" PROPERTIES GENERATED TRUE)
  target_sources(${p_Target} PRIVATE "${l_Output}")
  target_include_directories(${p_Target} PUBLIC "${l_GenDir}")
endfunction()
