add_subdirectory(glm)
add_subdirectory(yaml-cpp)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_subdirectory(gli)
add_subdirectory(assimp)
add_subdirectory(freetype)
add_subdirectory(SDL)
add_subdirectory(vk-bootstrap)

project(tlsf)

add_library(tlsf STATIC "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c")
target_include_directories(tlsf PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/tlsf"
)
cmake_minimum_required(VERSION 3.20)

project(nativefiledialog)

add_library(nativefiledialog STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_common.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_win.cpp"
)

target_include_directories(nativefiledialog PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src"
)

project(RecastNavigation)

add_subdirectory(recastnavigation/DebugUtils)
add_subdirectory(recastnavigation/Detour)
add_subdirectory(recastnavigation/DetourCrowd)
add_subdirectory(recastnavigation/DetourTileCache)
add_subdirectory(recastnavigation/Recast)

project(RecastDemo)

add_library(RecastDemo STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Include/ChunkyTriMesh.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Source/ChunkyTriMesh.cpp"
)

target_include_directories(RecastDemo PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Include"
)

project(cflat)

add_library(cflat STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/Cflat/Cflat.cpp"
)

target_include_directories(cflat PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/Cflat"
)

project(physx)

set(PHYSX_NAME "nvidia-physx")

file(GLOB PHYSX_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/**/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/source/**/src/*.h"
)
file(GLOB PHYSX_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/**/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxextensions/src/serialization/**/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxextensions/src/serialization/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxmetadata/extensions/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxcooking/src/mesh/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxcooking/src/convex/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/lowlevel/**/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/lowlevel/**/src/pipeline/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/geomutils/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/geomutils/src/**/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxmetadata/core/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physx/src/buffering/*.cpp"
)

file(GLOB PHYSX_PLATFORM_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/foundation/src/windows/*.cpp"
)

FILE(GLOB PHYSX_INCLUDE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/**/include
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/**/src
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/geomutils/src/*
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/lowlevel/**/include
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/lowlevel/common/include/*
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physx/src/buffering
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxextensions/src/serialization/*
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxextensions/src/serialization/File
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxmetadata/**/include
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxcooking/src/convex
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxcooking/src/mesh
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/physxvehicle/src/physxmetadata/include
)

add_library(physx STATIC ${PHYSX_SOURCES} ${PHYSX_INCLUDES} ${PHYSX_PLATFORM_SOURCES})

target_include_directories(physx PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}"
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/pxshared/include
)

target_include_directories(physx PRIVATE
  ${PHYSICS_PLATFORM_INCLUDE_DIRS}
  ${PHYSX_INCLUDE_DIRS}
  ${PHYSX_SOURCE_DIRS}
)
