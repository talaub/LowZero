project(LowEditor)

set(NODEGRAPH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/node-editor")

SET(NODEGRAPH_SOURCES
  "${NODEGRAPH_PATH}/crude_json.cpp"
  "${NODEGRAPH_PATH}/imgui_canvas.cpp"
  "${NODEGRAPH_PATH}/imgui_node_editor.cpp"
  "${NODEGRAPH_PATH}/imgui_node_editor_api.cpp"
)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(LowEditor ${NODEGRAPH_SOURCES} ${SOURCES})

add_dependencies(LowEditor
  LowUtil
  LowMath
  LowRenderer
  LowCore
  nativefiledialog
  assimp
  physx
  MistedaPlugin
)

set(LOWEDITOR_INCLUDE_DIRS
  "${NODEGRAPH_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_property(TARGET LowEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${APP_OUTPUT_DIR}")

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
  assimp
  physx
  MistedaPlugin
)
