project(LowRenderer)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowRenderer SHARED ${SOURCES})

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
