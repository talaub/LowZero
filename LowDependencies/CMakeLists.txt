add_subdirectory(glm)
add_subdirectory(yaml-cpp)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_subdirectory(gli)

project(tlsf)

add_library(tlsf STATIC "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c")
target_include_directories(tlsf PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/tlsf"
)
