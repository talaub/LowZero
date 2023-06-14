project(LowCore)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowCore SHARED ${SOURCES})

add_dependencies(LowCore
  LowUtil
  LowMath
  LowRenderer
  physx
  Recast
)

set(LOWCORE_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(LowCore PRIVATE
  lowcore_EXPORTS
  LOW_MODULE_NAME="lowcore"
)

include_directories(
  ${LOWCORE_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}LowDependencies/mono/msvc/include
)

target_include_directories(LowCore PUBLIC
  ${LOWCORE_INCLUDE_DIRS}
)

set_target_properties(LowCore PROPERTIES OUTPUT_NAME "lowcore")

set(WIN_PLATFORM_STRING "x64")

target_link_directories(LowCore PUBLIC
  "${CMAKE_SOURCE_DIR}/LowDependencies/mono/msvc/build/sgen/${WIN_PLATFORM_STRING}/lib/${CMAKE_BUILD_TYPE}"
)
  

target_link_libraries(LowCore PRIVATE
  LowRenderer
  LowUtil
  LowMath
  physx
  Recast
  mono-2.0-sgen
  MonoPosixHelper
)

add_custom_command(
        TARGET LowCore PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/LowDependencies/mono/msvc/build/sgen/${WIN_PLATFORM_STRING}/bin/${CMAKE_BUILD_TYPE}/mono-2.0-sgen.dll"
                "${APP_OUTPUT_DIR}/mono-2.0-sgen.dll")

add_custom_command(
        TARGET LowCore PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/LowDependencies/mono/msvc/build/sgen/${WIN_PLATFORM_STRING}/bin/${CMAKE_BUILD_TYPE}/MonoPosixHelper.dll
	${APP_OUTPUT_DIR}/MonoPosixHelper.dll)
