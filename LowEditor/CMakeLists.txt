project(LowEditor)

set(NODEGRAPH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/node-editor")

SET(NODEGRAPH_SOURCES
  "${NODEGRAPH_PATH}/crude_json.cpp"
  "${NODEGRAPH_PATH}/imgui_canvas.cpp"
  "${NODEGRAPH_PATH}/imgui_node_editor.cpp"
  "${NODEGRAPH_PATH}/imgui_node_editor_api.cpp"
  "${NODEGRAPH_PATH}/examples/blueprints-example/utilities/builders.cpp"
  "${NODEGRAPH_PATH}/examples/blueprints-example/utilities/drawing.cpp"
  "${NODEGRAPH_PATH}/examples/blueprints-example/utilities/widgets.cpp"
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

file(GLOB_RECURSE FLODE_SOURCES "../Flode/src/*.cpp")
file(GLOB_RECURSE FLODE_INCLUDES "../Flode/include/*.h")

add_library(LowEditor SHARED
  ${NODEGRAPH_SOURCES} 
  ${SOURCES} 
  ${INCLUDES} 
  ${FLODE_SOURCES} 
  ${FLODE_INCLUDES} 
)

source_group("Header Files" FILES ${INCLUDES})
source_group("Flode Header Files" FILES ${FLODE_INCLUDES})
source_group("Flode Source Files" FILES ${FLODE_SOURCES})

add_dependencies(LowEditor
  LowUtil
  LowMath
  LowRenderer2
  LowCore
  nativefiledialog
)

set(LOWEDITOR_INCLUDE_DIRS
  "${NODEGRAPH_PATH}"
  "${NODEGRAPH_PATH}/examples/blueprints-example"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Flode/include"
)

set_property(TARGET LowEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${APP_OUTPUT_DIR}")

target_compile_definitions(LowEditor PRIVATE
  loweditor_EXPORTS
  flode_EXPORTS
  LOW_MODULE_NAME="lowed"
)

include_directories(
  ${LOWEDITOR_INCLUDE_DIRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/private_include"
)

set_target_properties(LowEditor PROPERTIES OUTPUT_NAME "lowed")

target_link_libraries(LowEditor PRIVATE
  LowCore
  LowRenderer2
  LowUtil
  LowMath
  nativefiledialog
  assimp
  physx
)

target_include_directories(LowEditor PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  "${CMAKE_CURRENT_SOURCE_DIR}/../Flode/include"
  "${NODEGRAPH_PATH}"
  "${NODEGRAPH_PATH}/examples/blueprints-example"
)

target_include_directories(LowEditor PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/private_include"
)
