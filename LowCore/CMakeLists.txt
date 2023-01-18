project(LowCore)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_executable(LowCore ${SOURCES})

add_dependencies(LowCore LowUtil)

set(LOWCORE_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

include_directories(
  ${LOWCORE_INCLUDE_DIRS}
)

set_target_properties(LowCore PROPERTIES OUTPUT_NAME "game")


target_link_libraries(LowCore PRIVATE
  LowUtil
  LowMath
)
