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
  LowRenderer
  physx
  Recast
)

set(LOWCORE_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CFLAT_PATH}"
)

target_compile_definitions(LowCore PRIVATE
  lowcore_EXPORTS
  LOW_MODULE_NAME="lowcore"
)

include_directories(
  ${LOWCORE_INCLUDE_DIRS}
)

target_include_directories(LowCore PUBLIC
  ${LOWCORE_INCLUDE_DIRS}
)

set_target_properties(LowCore PROPERTIES OUTPUT_NAME "lowcore")

set(WIN_PLATFORM_STRING "x64")

target_link_libraries(LowCore PRIVATE
  LowRenderer
  LowUtil
  LowMath
  physx
  Recast
)
