SET(CMAKE_CURRENT_BINARY_DIR "${LOW_BINARY_PATH}/LowCore")

project(LowCore)

SET(CFLAT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/Cflat")

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

file(GLOB CFLAT_SOURCES "${CFLAT_PATH}/*.cpp")
file(GLOB CFLAT_INCLUDES "${CFLAT_PATH}/*.h")


add_library(LowCore SHARED
  ${CFLAT_SOURCES}
  ${CFLAT_INCLUDES}
  ${SOURCES}
  ${INCLUDES}
)

source_group("Header Files" FILES ${INCLUDES})
source_group("Cflat Header Files" FILES ${CFLAT_INCLUDES})

add_dependencies(LowCore
  LowUtil
  LowMath
  LowRenderer2
  physx
  Recast
  Detour
  DetourCrowd
  #assimp
  freetype
)

set(LOWCORE_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CFLAT_PATH}"
)

target_compile_definitions(LowCore PRIVATE
  lowcore_EXPORTS
  LOW_MODULE_NAME="lowcore"
)

#include_directories(
#  ${LOWCORE_INCLUDE_DIRS}
#)

target_include_directories(LowCore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CFLAT_PATH}>
  $<INSTALL_INTERFACE:include>
)

set_target_properties(LowCore PROPERTIES OUTPUT_NAME "lowcore")

target_compile_options(LowCore PRIVATE
  $<$<C_COMPILER_ID:MSVC>:/bigobj>
  $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

set(WIN_PLATFORM_STRING "x64")

target_link_libraries(LowCore PRIVATE
  freetype
  LowUtil
  LowMath
  LowRenderer2
  physx
  Recast
  Detour
  DetourCrowd
  angelscript
  #assimp
)
function(mark_includes_as_system consumer thirdparty)
  if (NOT TARGET "${consumer}" OR NOT TARGET "${thirdparty}")
    return()
  endif()
  get_target_property(_iface "${thirdparty}" INTERFACE_INCLUDE_DIRECTORIES)
  if (_iface)
    target_include_directories("${consumer}" SYSTEM PRIVATE ${_iface})
  endif()
endfunction()

mark_includes_as_system(LowCore physx)
