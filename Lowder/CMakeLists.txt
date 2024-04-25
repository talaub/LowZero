project(Lowder)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_executable(Lowder ${SOURCES} ${INCLUDES})

source_group("Header Files" FILES ${INCLUDES})

add_dependencies(Lowder
  LowUtil
  LowMath
  LowRenderer
  LowCore
  nativefiledialog
  assimp
  physx
)

add_dependencies(Lowder
  LowEditor
)

set(LOWDER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#set_property(TARGET Lowder PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_}")

target_compile_definitions(Lowder PRIVATE
  LOW_MODULE_NAME="lowder"
)

include_directories(
  ${LOWDER_INCLUDE_DIRS}
)

set_target_properties(Lowder PROPERTIES OUTPUT_NAME "lowder")

target_link_libraries(Lowder PRIVATE
  LowCore
  LowRenderer
  LowUtil
  LowMath
  nativefiledialog
  assimp
  physx
)

target_link_libraries(Lowder PRIVATE
  LowEditor
)
