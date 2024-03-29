project(Flode)

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

add_library(Flode SHARED
   ${NODEGRAPH_SOURCES} 
   ${SOURCES} 
   ${INCLUDES}
  )

source_group("Header Files" FILES ${INCLUDES})

add_dependencies(Flode
  LowUtil
  LowMath
  LowRenderer
  LowCore
  nativefiledialog
  assimp
  physx
)

set(FLODE_INCLUDE_DIRS
  "${NODEGRAPH_PATH}"
  "${NODEGRAPH_PATH}/examples/blueprints-example"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_property(TARGET Flode PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${APP_OUTPUT_DIR}")

target_compile_definitions(Flode PRIVATE
  flode_EXPORTS
  LOW_MODULE_NAME="flode"
)

include_directories(
  ${FLODE_INCLUDE_DIRS}
)

target_include_directories(Flode PUBLIC
  ${FLODE_INCLUDE_DIRS}
)

set_target_properties(Flode PROPERTIES OUTPUT_NAME "flode")

target_link_libraries(Flode PRIVATE
  LowCore
  LowRenderer
  LowUtil
  LowMath
  nativefiledialog
  assimp
  physx
)
