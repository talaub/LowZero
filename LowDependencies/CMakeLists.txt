add_subdirectory(glm)
add_subdirectory(yaml-cpp)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
add_subdirectory(gli)
add_subdirectory(microprofile)
add_subdirectory(assimp)

project(tlsf)

add_library(tlsf STATIC "${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c")
target_include_directories(tlsf PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/tlsf"
)
cmake_minimum_required(VERSION 3.20)

project(nativefiledialog)

add_library(nativefiledialog STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_common.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/nfd_win.cpp"
)

target_include_directories(nativefiledialog PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/nativefiledialog/src"
)

