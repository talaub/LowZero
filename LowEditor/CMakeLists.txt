project(LowEditor)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(LowEditor ${SOURCES})

add_dependencies(LowEditor
  LowUtil
  LowMath
  LowRenderer
  LowCore
  nativefiledialog
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
  LowCore
  LowRenderer
  LowUtil
  LowMath
  nativefiledialog
)
