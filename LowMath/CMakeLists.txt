project(LowMath)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowMath SHARED ${SOURCES})

set(LOWMATH_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(LowMath PROPERTIES OUTPUT_NAME "lowmath")

target_include_directories(LowMath PUBLIC
  ${LOWMATH_INCLUDE_DIRS}
)

add_dependencies(LowMath glm)

target_link_libraries(LowMath PUBLIC glm)
