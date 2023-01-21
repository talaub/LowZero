project(LowRenderer)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowRenderer SHARED ${SOURCES})

add_dependencies(LowRenderer LowUtil LowMath glfw)

set(LOWRENDERER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(LOWRENDERER_PRIV_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/backend/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/backend/vulkan/include"
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
)

target_link_libraries(LowRenderer PRIVATE
  LowUtil
  LowMath
)

target_link_libraries(LowRenderer PUBLIC
  glfw
)
