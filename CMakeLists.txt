cmake_minimum_required(VERSION 3.5)

project(LowEngine VERSION 2026.1.0)

# Feature flags. Defaults are for local development, `low package` sets
# its own values in a separate build dir
option(LOW_RUNTIME_SHADER_COMPILATION
       "Compile shaders at runtime instead of only loading SPIR-V" ON)
option(LOW_VULKAN_VALIDATION "Enable the Vulkan validation layers" ON)
option(LOW_HIDE_CONSOLE "Hide the console window of lowder" OFF)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

SET(LOW_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# A project that pulls the engine in via add_subdirectory() decides where
# binaries go; standalone engine builds keep their own layout
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/app")
endif()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  if (UNIX)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/app")
  else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
  endif()
endif()

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
endif()

if (NOT CMAKE_PDB_OUTPUT_DIRECTORY)
  set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/pdb")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Supported configurations" FORCE)
set(TARGET_BUILD_PLATFORM "windows")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)

if (MSVC)
  add_compile_options(/FS)
endif()

find_package(Git QUIET)

set(LOW_ENGINE_BUILD_ID "unknown")
if (GIT_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --match "lowengine-v*" --always --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE LOW_ENGINE_BUILD_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
endif()

set(EASTL_BUILD_TESTS OFF)
set(EASTL_BUILD_BENCHMARK OFF)

set(CMAKE_DEBUG_POSTFIX "")


# First for the generic no-config case (e.g. with mingw)
#set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
#set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
#set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${APP_OUTPUT_DIR} )
#set(CMAKE_PDB_OUTPUT_DIRECTORY ${PDB_OUTPUT_DIR})
# Second, for multi-config builds (e.g. msvc)


FILE(GLOB SUBFOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
SET(SUBPROJECTS "")
SET(FOUND "")

add_compile_definitions(
  GLM_ENABLE_EXPERIMENTAL
)

# The feature flags above are applied per target (LowRenderer2, Lowder)
# so toggling one only rebuilds the target that uses it
add_compile_definitions(
  LOW_EDITOR_BUILD
  IMGUI_DEFINE_MATH_OPERATORS
  PX_PHYSX_STATIC_LIB
  DISABLE_CUDA_PHYSX
  PX_COOKING
  LOW_RENDERER_RENDEROBJECT_COUNT=1024
  CFLAT_ENABLED=1
)

# Excluded directories
# These directorories are not part of the build tree
SET(SUB_EXCEPTIONS
  LowScripts
  LowDocs
  LowData
  LowCrashOpener
)

# Adds all subfolders that start with the prefix "Low" and are not
# listet as excluded to the SUBPROJECTS list
FOREACH(child ${SUBFOLDERS})
	      IF (child MATCHES "^Low")
	      	 IF (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
		    LIST(FIND SUB_EXCEPTIONS ${child} FOUND)

		    IF (FOUND LESS 0)
		    		    LIST(APPEND SUBPROJECTS ${child})
		    ENDIF()
		 ENDIF()
	ENDIF()
ENDFOREACH()

SET(LINK_DIRS "")
SET(SUBPROJECTS_LOWERCASE "")
SET(PROJECT_LINK_LIBS "")

# Adds subproject as subdirectory to cmake buildtree
FOREACH(project ${SUBPROJECTS})
  #add_subdirectory(${project})
ENDFOREACH()
option(LOW_ENABLE_ASAN "Enable AddressSanitizer" OFF)

if (LOW_ENABLE_ASAN)
  if (MSVC)
    add_compile_options(/fsanitize=address)
    add_link_options(/INCREMENTAL:NO)
  endif()
endif()

option(LOW_JOBMANAGER_TRACKING
       "Track JobManager jobs for editor/debug tooling" ON)

if (LOW_JOBMANAGER_TRACKING)
  add_compile_definitions(LOW_JOBMANAGER_TRACKING=1)
else()
  add_compile_definitions(LOW_JOBMANAGER_TRACKING=0)
endif()

include(${LOW_SOURCE_PATH}/LowLens/cmake/LowLens.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Sorts all targets defined in p_Dir into the solution folder p_Folder.
# LowLens codegen targets go into a Codegen subfolder. Targets that already
# have a folder keep it unless FORCE is given. RECURSIVE also handles all
# subdirectories, which is what third-party code needs.
function(low_set_folder p_Dir p_Folder)
  cmake_parse_arguments(LOW_FOLDER "RECURSIVE;FORCE" "" "" ${ARGN})

  get_property(l_Targets DIRECTORY "${p_Dir}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(i_Target IN LISTS l_Targets)
    get_target_property(l_Folder ${i_Target} FOLDER)
    if (l_Folder AND NOT LOW_FOLDER_FORCE)
      continue()
    endif()

    if (i_Target MATCHES "\\.Lens$")
      set_target_properties(${i_Target} PROPERTIES FOLDER "${p_Folder}/Codegen")
    else()
      set_target_properties(${i_Target} PROPERTIES FOLDER "${p_Folder}")
    endif()
  endforeach()

  if (LOW_FOLDER_RECURSIVE)
    get_property(l_SubDirs DIRECTORY "${p_Dir}" PROPERTY SUBDIRECTORIES)
    foreach(i_Dir IN LISTS l_SubDirs)
      low_set_folder("${i_Dir}" "${p_Folder}" ${ARGN})
    endforeach()
  endif()
endfunction()

add_subdirectory(LowDependencies)
add_subdirectory(LowMath)
add_subdirectory(LowUtil)
add_subdirectory(LowRenderer)
add_subdirectory(LowRenderer2)
add_subdirectory(LowCore)
add_subdirectory(LowCliCore)
add_subdirectory(LowCli)
add_subdirectory(LowShim)
add_subdirectory(LowLauncher)
add_subdirectory(LowEditor)
add_subdirectory(Lowder)

foreach(i_Dir LowMath LowUtil LowRenderer LowRenderer2 LowCore LowCliCore LowCli LowShim LowLauncher LowEditor Lowder)
  low_set_folder("${CMAKE_CURRENT_SOURCE_DIR}/${i_Dir}" "LowEngine")
endforeach()

set(LOW_DEBUG_VISUALIZERS
  "${CMAKE_CURRENT_SOURCE_DIR}/LowDependencies/EASTL/doc/EASTL.natvis"
  "${CMAKE_CURRENT_SOURCE_DIR}/LowDebug.natvis"
)

function(low_add_debug_visualizers target_name)
  if (MSVC AND TARGET ${target_name})
    target_sources(${target_name} PRIVATE ${LOW_DEBUG_VISUALIZERS})
    source_group("Debug Visualizers" FILES ${LOW_DEBUG_VISUALIZERS})
  endif()
endfunction()

foreach(LOW_VISUALIZER_TARGET
  LowMath
  LowUtil
  LowRenderer
  LowRenderer2
  LowCore
  LowEditor
  Lowder
)
  low_add_debug_visualizers(${LOW_VISUALIZER_TARGET})
endforeach()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/LowData/"
  DESTINATION LowData COMPONENT editor)
