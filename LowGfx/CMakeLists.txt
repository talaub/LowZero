project(LowGfx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "include/*.h")
file(GLOB_RECURSE PRIVATE_INCLUDES CONFIGURE_DEPENDS "private_include/*.h")

file(GLOB_RECURSE VULKAN_SOURCES CONFIGURE_DEPENDS "vulkan/src/*.cpp")
file(GLOB_RECURSE VULKAN_INCLUDES CONFIGURE_DEPENDS "vulkan/include/*.h")

set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/imgui3")
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/IconFontCppHeaders")

set(IMGUIZMO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/ImGuizmo")

set(VMA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/VulkanMemoryAllocator")

SET(IMGUI_SOURCES
  "${IMGUI_PATH}/imgui.cpp"
  "${IMGUI_PATH}/imgui_draw.cpp"
  "${IMGUI_PATH}/imgui_demo.cpp"
  "${IMGUI_PATH}/imgui_tables.cpp"
  "${IMGUI_PATH}/imgui_widgets.cpp"
  "${IMGUI_PATH}/imgui_stacklayout.cpp"
  "${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp"
  "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp"
)

SET(IMGUIZMO_SOURCES
  "${IMGUIZMO_PATH}/GraphEditor.cpp"
  "${IMGUIZMO_PATH}/ImCurveEdit.cpp"
  "${IMGUIZMO_PATH}/ImGradient.cpp"
  "${IMGUIZMO_PATH}/ImGuizmo.cpp"
  "${IMGUIZMO_PATH}/ImSequencer.cpp"
)

add_executable(LowGfx
  ${IMGUI_SOURCES}
  ${IMGUIZMO_SOURCES}
  ${VULKAN_SOURCES}
  ${VULKAN_INCLUDES}
  ${SOURCES}
  ${INCLUDES}
  ${PRIVATE_INCLUDES}
)

source_group("Header Files" FILES "${INCLUDES}")

if (WIN32)
  STRING(REGEX REPLACE "\\\\" "/" VULKAN_SDK_PATH $ENV{VK_SDK_PATH})

  target_link_directories(LowGfx PUBLIC
  ${VULKAN_SDK_PATH}/Lib
)

  target_link_libraries(LowGfx PUBLIC
  vulkan-1.lib
)
else()
  find_package(Vulkan REQUIRED)
  target_link_libraries(LowGfx PRIVATE Vulkan::Vulkan)
endif()

set(LOWGFX_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(LowGfx PROPERTIES OUTPUT_NAME "lowgfx")

target_compile_definitions(LowGfx PRIVATE
  LOW_MODULE_NAME="lowgfx"
  lowgfx_EXPORTS
  LOW_GFX_API_VULKAN
  IMGUI_USER_CONFIG="${LOW_SOURCE_PATH}/LowGfx/include/LowGfxImguiConfig.h"
)

if (MSVC)
  target_compile_options(LowGfx PRIVATE /FS)
endif()

target_include_directories(LowGfx PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include
)

target_include_directories(LowGfx SYSTEM PRIVATE
  "${VMA_PATH}/include"
)

target_include_directories(LowGfx PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/private_include
)

target_include_directories(LowGfx PUBLIC
  ${LOWGFX_INCLUDE_DIRS}
)

target_include_directories(LowGfx SYSTEM PUBLIC
  $ENV{VK_SDK_PATH}/Include
  "${IMGUI_PATH}"
  "${IMGUI_PATH}/backends"
  "${IMGUIZMO_PATH}"
  "${ICON_PATH}"
)

target_link_libraries(LowGfx PRIVATE
  LowUtil
  LowMath
  vk-bootstrap
)
