SET(CMAKE_CURRENT_BINARY_DIR "${LOW_BINARY_PATH}/LowUtil")

project(LowUtil)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

set(LOW_GENERATED_INCLUDE_DIR "${LOW_BINARY_PATH}/generated/include")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/LowUtilVersion.h.in"
  "${LOW_GENERATED_INCLUDE_DIR}/LowUtilVersion.h"
  @ONLY
)

add_library(LowUtil SHARED
  ${SOURCES}
  ${INCLUDES}
)

set_target_properties(LowUtil PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

set(LOWUTIL_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_dependencies(LowUtil
  EASTL
  yaml-cpp
  gli
  tlsf
  assimp
  LowMath
  SDL2
  inja
)

MESSAGE("LOWUTILBUILD: ${CMAKE_CURRENT_BINARY_DIR}")


set_target_properties(LowUtil PROPERTIES OUTPUT_NAME "lowutil")

source_group("Header Files" FILES ${INCLUDES})

target_compile_definitions(LowUtil PRIVATE
  LOW_MODULE_NAME="lowutil"
  lowutil_EXPORTS
)

target_link_libraries(LowUtil PRIVATE
  assimp
)

function(mark_includes_as_system consumer thirdparty)
  if (NOT TARGET "${consumer}" OR NOT TARGET "${thirdparty}")
    return()
  endif()

  # Some libs are ALIAS (e.g., assimp::assimp); resolve to real target
  get_target_property(_aliased "${thirdparty}" ALIASED_TARGET)
  if (_aliased)
    set(thirdparty "${_aliased}")
  endif()

  # Copy both INTERFACE include props if present
  get_target_property(_iface "${thirdparty}" INTERFACE_INCLUDE_DIRECTORIES)
  if (_iface)
    target_include_directories("${consumer}" SYSTEM PUBLIC ${_iface})
  endif()

  # CMake 3.25+: INTERFACE_SYSTEM_INCLUDE_DIRECTORIES (if the lib already flagged them)
  get_target_property(_iface_sys "${thirdparty}" INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  if (_iface_sys)
    target_include_directories("${consumer}" SYSTEM PUBLIC ${_iface_sys})
  endif()
endfunction()

mark_includes_as_system(LowUtil assimp)


target_link_libraries(LowUtil PUBLIC
  SDL2
  EASTL
  yaml-cpp
  tlsf
  gli
  inja
  LowMath
)

mark_includes_as_system(LowUtil EASTL)
mark_includes_as_system(LowUtil gli)
mark_includes_as_system(LowUtil yaml-cpp)

target_include_directories(LowUtil PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${LOW_GENERATED_INCLUDE_DIR}>
	$<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)


find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(stb QUIET IMPORTED_TARGET stb)
endif()
if (stb_FOUND)
	target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::stb)
endif()
