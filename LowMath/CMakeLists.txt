SET(CMAKE_CURRENT_BINARY_DIR "${LOW_BINARY_PATH}/LowMath")

project(LowMath)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowMath SHARED
  ${SOURCES}
  ${INCLUDES}
)

source_group("Header Files" FILES ${INCLUDES})

set(LOWMATH_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(LowMath PROPERTIES OUTPUT_NAME "lowmath")

target_compile_definitions(LowMath PRIVATE
  LOW_MODULE_NAME="lowmath"
  lowmath_EXPORTS
)

target_include_directories(LowMath PUBLIC
  #${LOWMATH_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_dependencies(LowMath glm)

target_link_libraries(LowMath PUBLIC glm)