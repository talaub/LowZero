project(LowCli)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "src/*.h")

add_executable(LowCli ${SOURCES} ${INCLUDES})

source_group("Header Files" FILES ${INCLUDES})

target_include_directories(LowCli PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(LowCli PRIVATE
  LowCliCore
  CLI11
)

target_compile_definitions(LowCli PRIVATE
  LOW_MODULE_NAME="low"
)

install(TARGETS LowCli RUNTIME DESTINATION bin COMPONENT tools)
install(FILES $<TARGET_RUNTIME_DLLS:LowCli> DESTINATION bin COMPONENT tools)

set_target_properties(LowCli PROPERTIES OUTPUT_NAME low FOLDER "LowEngine/Tools")
