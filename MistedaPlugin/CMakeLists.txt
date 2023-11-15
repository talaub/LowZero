project(MistedaPlugin)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(MistedaPlugin SHARED
  ${SOURCES}
  ${INCLUDES}
)

source_group("Header Files" FILES ${INCLUDES})

add_dependencies(MistedaPlugin
  LowUtil
  LowMath
  LowCore
  LowRenderer
)

set(MISTEDA_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(MistedaPlugin PRIVATE
  mistedaplugin_EXPORTS
  LOW_MODULE_NAME="misteda"
)

include_directories(
  ${MISTEDA_INCLUDE_DIRS}
)

target_include_directories(MistedaPlugin PUBLIC
  ${MISTEDA_INCLUDE_DIRS}
)

set_target_properties(MistedaPlugin PROPERTIES OUTPUT_NAME "mistedaplugin")

target_link_libraries(MistedaPlugin PRIVATE
  LowUtil
  LowMath
  LowCore
  LowRenderer
)
