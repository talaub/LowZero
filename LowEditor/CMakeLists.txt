project(LowEditor)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_executable(LowEditor ${SOURCES})

add_dependencies(LowEditor
  LowUtil
  LowMath
  LowRenderer
)

set(LOWEDITOR_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(LowEditor PRIVATE
  LOW_MODULE_NAME="lowed"
)

include_directories(
  ${LOWEDITOR_INCLUDE_DIRS}
)

set_target_properties(LowEditor PROPERTIES OUTPUT_NAME "lowed")

target_link_libraries(LowEditor PRIVATE
  LowUtil
  LowMath
  LowRenderer
)
