SET(BUILD_SHARED_LIBS OFF)

SET(GLI_TEST_ENABLE OFF)
SET(ASSIMP_BUILD_TESTS OFF)
SET(YAML_CPP_BUILD_TOOLS OFF)
SET(YAML_CPP_FORMAT_SOURCE OFF)

add_subdirectory(glm)
add_subdirectory(yaml-cpp)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_subdirectory(gli)
add_subdirectory(assimp)
add_subdirectory(EASTL)
set(FT_DISABLE_ZLIB    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2   ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
add_subdirectory(freetype)
add_subdirectory(SDL)
set(VK_BOOTSTRAP_TEST OFF)
set(VK_BOOTSTRAP_INSTALL OFF)
add_subdirectory(vk-bootstrap)

# Some FreeType trees export 'freetype' (lowercase). Provide the alias msdfgen expects:
if (NOT TARGET Freetype::Freetype AND TARGET freetype)
  add_library(Freetype::Freetype ALIAS freetype)
endif()

# --- msdfgen (needs to be AFTER freetype) --------------------------
set(MSDFGEN_USE_VCPKG                OFF CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL                 OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_MSDFGEN_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_FREETYPE ON CACHE BOOL "" FORCE)
# Turn off optional deps that trigger find_package
set(MSDFGEN_DISABLE_SVG ON  CACHE BOOL "" FORCE)   # <- avoids tinyxml2
set(MSDFGEN_DISABLE_PNG ON  CACHE BOOL "" FORCE)  # <- avoids libpng
set(MSDFGEN_USE_SKIA OFF  CACHE BOOL "" FORCE)  # <- avoids skia

# IMPORTANT: enable FreeType; DO NOT set CORE_ONLY
set(MSDFGEN_USE_FREETYPE             ON CACHE BOOL "" FORCE)
# remove/avoid: set(MSDFGEN_CORE_ONLY ON)

add_subdirectory(msdfgen) # exports msdfgen-core, msdfgen-freetype (and msdfgen-ext)
# msdf-atlas-gen (library only)
set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_MSDFGEN_EXTERNAL ON CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "" FORCE)
add_subdirectory(msdf-atlas-gen)

set_target_properties(EASTL PROPERTIES FOLDER "Dependencies")
set_target_properties(gli_dummy PROPERTIES FOLDER "Dependencies")
set_target_properties(assimp PROPERTIES FOLDER "Dependencies")
set_target_properties(freetype PROPERTIES FOLDER "Dependencies")
set_target_properties(uninstall PROPERTIES FOLDER "Dependencies")
set_target_properties(zlibstatic PROPERTIES FOLDER "Dependencies")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Dependencies")
set_target_properties(msdfgen-core PROPERTIES FOLDER "Dependencies")
set_target_properties(msdfgen-ext PROPERTIES FOLDER "Dependencies")

set_target_properties(yaml-cpp PROPERTIES FOLDER "Dependencies")
#set_target_properties(yaml-cpp-parse PROPERTIES FOLDER "Dependencies/yaml")
#set_target_properties(yaml-cpp-read PROPERTIES FOLDER "Dependencies/yaml")
#set_target_properties(yaml-cpp-sandbox PROPERTIES FOLDER "Dependencies/yaml")
#set_target_properties(format PROPERTIES FOLDER "Dependencies/yaml")

project(tlsf)

add_library(tlsf STATIC "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c")
target_include_directories(tlsf PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tlsf>
  $<INSTALL_INTERFACE:include>
)
set_target_properties(tlsf PROPERTIES FOLDER "Dependencies")
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
set_target_properties(nativefiledialog PROPERTIES FOLDER "Dependencies")


project(RecastNavigation)

add_subdirectory(recastnavigation/DebugUtils)
add_subdirectory(recastnavigation/Detour)
add_subdirectory(recastnavigation/DetourCrowd)
add_subdirectory(recastnavigation/DetourTileCache)
add_subdirectory(recastnavigation/Recast)

set_target_properties(Detour PROPERTIES FOLDER "Dependencies/navigation")
set_target_properties(DetourTileCache PROPERTIES FOLDER "Dependencies/navigation")
set_target_properties(DebugUtils PROPERTIES FOLDER "Dependencies/navigation")
set_target_properties(DetourCrowd PROPERTIES FOLDER "Dependencies/navigation")
set_target_properties(Recast PROPERTIES FOLDER "Dependencies/navigation")

project(RecastDemo)

add_library(RecastDemo STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Include/ChunkyTriMesh.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Source/ChunkyTriMesh.cpp"
)

target_include_directories(RecastDemo PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/RecastDemo/Include"
)
set_target_properties(RecastDemo PROPERTIES FOLDER "Dependencies")



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

set_target_properties(physx PROPERTIES FOLDER "Dependencies")