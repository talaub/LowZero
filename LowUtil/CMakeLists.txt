project(LowUtil)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowUtil SHARED
  ${SOURCES}
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile/microprofile.cpp"
)

set(LOWUTIL_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile"
)

add_dependencies(LowUtil
  LowMath
)

set_target_properties(LowUtil PROPERTIES OUTPUT_NAME "lowutil")

target_include_directories(LowUtil PUBLIC
  ${LOWUTIL_INCLUDE_DIRS}
)

target_compile_definitions(LowUtil PRIVATE
  LOW_MODULE_NAME="lowutil"
  lowutil_EXPORTS
)

set(EASTL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/EASTL")
target_include_directories (LowUtil PUBLIC
    ${EASTL_ROOT_DIR}/include
    ${EASTL_ROOT_DIR}/test/packages/EAAssert/include
    ${EASTL_ROOT_DIR}/test/packages/EABase/include/Common
    ${EASTL_ROOT_DIR}/test/packages/EAMain/include
    ${EASTL_ROOT_DIR}/test/packages/EAStdC/include
    ${EASTL_ROOT_DIR}/test/packages/EATest/include
    ${EASTL_ROOT_DIR}/test/packages/EAThread/include
)
add_custom_target(NatVis SOURCES ${EASTL_ROOT_DIR}/doc/EASTL.natvis)

target_link_libraries(LowUtil PRIVATE
  LowMath
)

target_link_libraries(LowUtil PUBLIC
  microprofile
  "${CMAKE_BINARY_DIR}/LowDependencies/EASTL/build/${CMAKE_BUILD_TYPE}/EASTL.lib"
  yaml-cpp
  gli
  tlsf
  assimp
)
