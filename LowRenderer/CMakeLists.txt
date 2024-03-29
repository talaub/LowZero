project(LowRenderer)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/imgui")
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/IconFontCppHeaders")

set(IMGUIZMO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/ImGuizmo")

SET(IMGUI_SOURCES
  "${IMGUI_PATH}/imgui.cpp"
  "${IMGUI_PATH}/imgui_draw.cpp"
  "${IMGUI_PATH}/imgui_demo.cpp"
  "${IMGUI_PATH}/imgui_tables.cpp"
  "${IMGUI_PATH}/imgui_widgets.cpp"
  "${IMGUI_PATH}/imgui_stacklayout.cpp"
  "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
  "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp"
)

SET(IMGUIZMO_SOURCES
  "${IMGUIZMO_PATH}/GraphEditor.cpp"
  "${IMGUIZMO_PATH}/ImCurveEdit.cpp"
  "${IMGUIZMO_PATH}/ImGradient.cpp"
  "${IMGUIZMO_PATH}/ImGuizmo.cpp"
  "${IMGUIZMO_PATH}/ImSequencer.cpp"
)
  
add_library(LowRenderer SHARED
  ${IMGUI_SOURCES}
  ${IMGUIZMO_SOURCES}
  ${SOURCES}
  ${INCLUDES}
)

source_group("Header Files" FILES "${INCLUDES}")

add_dependencies(LowRenderer LowUtil LowMath glfw)

STRING(REGEX REPLACE "\\\\" "/" VULKAN_SDK_PATH $ENV{VK_SDK_PATH}) 

set(LOWRENDERER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(LowRenderer PROPERTIES OUTPUT_NAME "lowrenderer")

target_compile_definitions(LowRenderer PRIVATE
  LOW_MODULE_NAME="lowrenderer"
  lowrenderer_EXPORTS
  LOW_RENDERER_WINDOW_API_GLFW
  LOW_RENDERER_API_VULKAN
)

target_include_directories(LowRenderer PUBLIC
  ${LOWRENDERER_INCLUDE_DIRS}
  $ENV{VK_SDK_PATH}/Include
  "${IMGUI_PATH}"
  "${IMGUI_PATH}/backends"
  "${IMGUIZMO_PATH}"
  "${ICON_PATH}"
)

target_link_libraries(LowRenderer PRIVATE
  LowUtil
  LowMath
)

target_link_directories(LowRenderer PUBLIC
  ${VULKAN_SDK_PATH}/Lib
)

target_link_libraries(LowRenderer PUBLIC
  glfw
  vulkan-1
)
