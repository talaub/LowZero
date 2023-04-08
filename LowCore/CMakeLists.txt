project(LowCore)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowCore SHARED ${SOURCES})

add_dependencies(LowCore
  LowUtil
  LowMath
  LowRenderer
)

set(LOWCORE_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
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

target_link_libraries(LowCore PRIVATE
  LowRenderer
  LowUtil
  LowMath
)
