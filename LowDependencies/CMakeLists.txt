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
set(GLI_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(gli)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE) # supported by Assimp
add_subdirectory(assimp EXCLUDE_FROM_ALL)

# Make everything PIC by default (safe on non-Windows)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Or, only for EASTL targets:
add_subdirectory(EASTL EXCLUDE_FROM_ALL)

foreach(t EASTL EASTLd eastl eastld)  # handle possible target names/suffixes
  if (TARGET ${t})
    set_property(TARGET ${t} PROPERTY POSITION_INDEPENDENT_CODE ON)
    if (MSVC)
      target_compile_options(${t} PRIVATE /w /WX-)   # optional: hush warnings
    else()
      target_compile_options(${t} PRIVATE -w -Wno-error)
    endif()
  endif()
endforeach()

set(FT_DISABLE_ZLIB    ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2   ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG     ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
add_subdirectory(freetype)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

# (Some SDL2 setups also honor CMake's generic switch)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory(SDL)

set(VK_BOOTSTRAP_TEST OFF)
set(VK_BOOTSTRAP_INSTALL OFF)
add_subdirectory(vk-bootstrap)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # makes ALL targets compile with -fPIC on non-Windows

# Some FreeType trees export 'freetype' (lowercase). Provide the alias msdfgen expects:
if (NOT TARGET Freetype::Freetype AND TARGET freetype)
  add_library(Freetype::Freetype ALIAS freetype)
endif()

set_property(TARGET freetype PROPERTY POSITION_INDEPENDENT_CODE ON)

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
#set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Dependencies")
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

# project(nativefiledialog)
#
# SET(NFD_SOURCES 
#   "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_win.cpp"
# )
#
# if(UNIX)
# SET(NFD_SOURCES 
#   "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_gtk.c"
# )
# endif()
#
# add_library(nativefiledialog STATIC
#   "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_common.c"
#   ${NFD_SOURCES}
# )
#
# target_include_directories(nativefiledialog PUBLIC
#   "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/include"
#   "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src"
# )
# set_target_properties(nativefiledialog PROPERTIES FOLDER "Dependencies")
#

set(RECASTNAVIGATION_DEMO     OFF CACHE BOOL "Build demo" FORCE)
set(RECASTNAVIGATION_TESTS    OFF CACHE BOOL "Build tests" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
add_subdirectory(recastnavigation)

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

if (MSVC)
file(GLOB PHYSX_PLATFORM_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/foundation/src/windows/*.cpp"
)
else()
file(GLOB PHYSX_PLATFORM_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/source/foundation/src/unix/*.cpp"
)
endif()

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

# Keep your existing line for Release
target_compile_definitions(physx PUBLIC
  $<$<CONFIG:Release>:NDEBUG>
  $<$<CONFIG:RelWithDebInfo>:NDEBUG>
  $<$<CONFIG:MinSizeRel>:NDEBUG>
  $<$<CONFIG:Debug>:_DEBUG>
)

# Add _DEBUG for Debug builds (MSVC does this automatically, but it’s harmless to repeat)
target_compile_definitions(physx PRIVATE
  $<$<CONFIG:Debug>:_DEBUG>
)

# (Defensive) If something globally adds -DNDEBUG, undo it in Debug for Clang/GCC
target_compile_options(physx PRIVATE
  $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>>:-UNDEBUG>
  $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,Clang,GNU>>:-UNDEBUG>
)

target_include_directories(physx PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}"
  ${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/pxshared/include
${CMAKE_CURRENT_SOURCE_DIR}/${PHYSX_NAME}/physx/include
)

target_include_directories(physx PRIVATE
  ${PHYSICS_PLATFORM_INCLUDE_DIRS}
  ${PHYSX_INCLUDE_DIRS}
  ${PHYSX_SOURCE_DIRS}
)

if (MSVC)
  target_compile_options(physx PRIVATE /w /WX-)
else()
  target_compile_options(physx PRIVATE -w -Wno-error)
endif()

set_target_properties(physx PROPERTIES FOLDER "Dependencies")
