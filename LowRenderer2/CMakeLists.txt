project(LowRenderer2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")
file(GLOB_RECURSE PRIVATE_INCLUDES "private_include/*.h")

file(GLOB_RECURSE VULKAN_SOURCES "vulkan/src/*.cpp")
file(GLOB_RECURSE VULKAN_INCLUDES "vulkan/include/*.h")

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
  
add_library(LowRenderer2 SHARED
  ${IMGUI_SOURCES}
  ${IMGUIZMO_SOURCES}
  ${VULKAN_SOURCES}
  ${VULKAN_INCLUDES}
  ${SOURCES}
  ${INCLUDES}
  ${PRIVATE_INCLUDES}
)

source_group("Header Files" FILES "${INCLUDES}")

add_dependencies(LowRenderer2 
  LowUtil 
  LowMath 
  vk-bootstrap
  freetype
  assimp
  msdfgen-core
  msdfgen-ext
  msdf-atlas-gen
  #Freetype::Freetype
)

if (WIN32)
STRING(REGEX REPLACE "\\\\" "/" VULKAN_SDK_PATH $ENV{VK_SDK_PATH}) 

target_link_directories(LowRenderer2 PUBLIC
  ${VULKAN_SDK_PATH}/Lib
)

target_link_libraries(LowRenderer2 PUBLIC
  vulkan-1.lib
)
else()
find_package(Vulkan REQUIRED)
target_link_libraries(LowRenderer2 PRIVATE Vulkan::Vulkan)
endif()

set(LOWRENDERER2_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(LowRenderer2 PROPERTIES OUTPUT_NAME "lowrenderer2")

target_compile_definitions(LowRenderer2 PRIVATE
  LOW_MODULE_NAME="lowrenderer2"
  lowrenderer2_EXPORTS
  LOW_RENDERER_API_VULKAN
  IMGUI_USER_CONFIG="${LOW_SOURCE_PATH}/LowRenderer2/include/LowRendererImguiConfig.h"
)

target_include_directories(LowRenderer2 PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include
)

target_include_directories(LowRenderer2 SYSTEM PRIVATE
  "${VMA_PATH}/include"
)

target_include_directories(LowRenderer2 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/private_include
)

target_include_directories(LowRenderer2 PUBLIC
  ${LOWRENDERER2_INCLUDE_DIRS}
)

target_include_directories(LowRenderer2 SYSTEM PUBLIC
  $ENV{VK_SDK_PATH}/Include
  "${IMGUI_PATH}"
  "${IMGUI_PATH}/backends"
  "${IMGUIZMO_PATH}"
  "${ICON_PATH}"
)

target_link_libraries(LowRenderer2 PRIVATE
  LowUtil
  LowMath
  vk-bootstrap
  freetype
  assimp
  msdfgen-core
  msdfgen-ext
  msdf-atlas-gen
  #Freetype::Freetype
)

function(mark_includes_as_system consumer thirdparty)
  if (NOT TARGET "${consumer}" OR NOT TARGET "${thirdparty}")
    return()
  endif()
  get_target_property(_iface "${thirdparty}" INTERFACE_INCLUDE_DIRECTORIES)
  if (_iface)
    target_include_directories("${consumer}" SYSTEM PRIVATE ${_iface})
  endif()
endfunction()

mark_includes_as_system(LowRenderer2 assimp)
mark_includes_as_system(LowRenderer2 vk-bootstrap)
mark_includes_as_system(LowRenderer2 msdfgen-core)
mark_includes_as_system(LowRenderer2 msdfgen-ext)
mark_includes_as_system(LowRenderer2 msdf-atlas-gen)
mark_includes_as_system(LowRenderer2 freetype)
