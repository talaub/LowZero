SET(CMAKE_CURRENT_BINARY_DIR "${LOW_BINARY_PATH}/LowCore")

project(LowCore)

SET(CFLAT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/Cflat")

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")
file(GLOB_RECURSE ANIMATION_SOURCES "animation/src/*.cpp")
file(GLOB_RECURSE ANIMATION_INCLUDES "animation/include/*.h")
file(GLOB_RECURSE ANIMATION_PRIVATE_INCLUDES
     "animation/private_include/*.h")
file(GLOB_RECURSE PHYSICS_SOURCES "physics/src/*.cpp")
file(GLOB_RECURSE PHYSICS_INCLUDES "physics/include/*.h")
file(GLOB_RECURSE PHYSICS_PRIVATE_INCLUDES
     "physics/private_include/*.h")

file(GLOB CFLAT_SOURCES "${CFLAT_PATH}/*.cpp")
file(GLOB CFLAT_INCLUDES "${CFLAT_PATH}/*.h")

add_library(LowCoreAnimation OBJECT
  ${ANIMATION_SOURCES}
  ${ANIMATION_INCLUDES}
  ${ANIMATION_PRIVATE_INCLUDES}
)
set_target_properties(LowCoreAnimation PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )

add_dependencies(LowCoreAnimation
  LowUtil
  LowMath
  LowRenderer2
)

source_group("Animation Source Files" FILES ${ANIMATION_SOURCES})
source_group("Animation Header Files" FILES ${ANIMATION_INCLUDES})
source_group("Animation Private Header Files"
             FILES ${ANIMATION_PRIVATE_INCLUDES})

target_include_directories(LowCoreAnimation PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/animation/include
  ${CMAKE_CURRENT_SOURCE_DIR}/animation/private_include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(LowCoreAnimation PRIVATE
  lowcore_EXPORTS
  LOW_MODULE_NAME="lowcore"
  LOW_CORE_ANIMATION_INTERNAL
)

target_link_libraries(LowCoreAnimation PRIVATE
  LowUtil
  LowMath
  LowRenderer2
)

add_library(LowCorePhysics STATIC
  ${PHYSICS_SOURCES}
  ${PHYSICS_INCLUDES}
  ${PHYSICS_PRIVATE_INCLUDES}
)

set_target_properties(LowCorePhysics PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

add_dependencies(LowCorePhysics
  LowUtil
  LowMath
  LowRenderer2
  Jolt
)

source_group("Physics Source Files" FILES ${PHYSICS_SOURCES})
source_group("Physics Header Files" FILES ${PHYSICS_INCLUDES})
source_group("Physics Private Header Files"
             FILES ${PHYSICS_PRIVATE_INCLUDES})

target_include_directories(LowCorePhysics PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/physics/include
  ${CMAKE_CURRENT_SOURCE_DIR}/physics/private_include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(LowCorePhysics PRIVATE
  lowcore_EXPORTS
  LOW_MODULE_NAME="lowcore"
  LOW_CORE_PHYSICS_INTERNAL
)

target_link_libraries(LowCorePhysics PRIVATE
  LowUtil
  LowMath
  LowRenderer2
  Jolt
)


add_library(LowCore SHARED
  ${CFLAT_SOURCES}
  ${CFLAT_INCLUDES}
  ${SOURCES}
  ${INCLUDES}
  $<TARGET_OBJECTS:LowCoreAnimation>
)

source_group("Header Files" FILES ${INCLUDES})
source_group("Cflat Header Files" FILES ${CFLAT_INCLUDES})

add_dependencies(LowCore
  LowCoreAnimation
  LowCorePhysics
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
  "${CMAKE_CURRENT_SOURCE_DIR}/animation/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/physics/include"
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
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/animation/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/physics/include>
  $<BUILD_INTERFACE:${CFLAT_PATH}>
  $<INSTALL_INTERFACE:include>
)

set_target_properties(LowCore PROPERTIES OUTPUT_NAME "lowcore")

target_compile_options(LowCore PRIVATE
  $<$<C_COMPILER_ID:MSVC>:/bigobj>
  $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

target_include_directories(LowCore PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/angelscript/sdk/angelscript/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/angelscript/sdk/add_on
)

target_sources(LowCore PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/angelscript/sdk/add_on/scriptstdstring/scriptstdstring.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/angelscript/sdk/add_on/scriptbuilder/scriptbuilder.cpp
)

set(WIN_PLATFORM_STRING "x64")

target_link_libraries(LowCore PRIVATE
  freetype
  LowUtil
  LowMath
  LowRenderer2
  LowCorePhysics
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
