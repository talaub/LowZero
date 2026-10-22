project(Lowder)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "include/*.h")
set(WINDOWS_RESOURCES "")
if(WIN32)
  set(WINDOWS_RESOURCES "resources/Lowder.rc")
endif()

add_executable(Lowder ${SOURCES} ${INCLUDES} ${WINDOWS_RESOURCES})

source_group("Header Files" FILES ${INCLUDES})

set(WINDOWS_SDK_PATH "C:/Program Files (x86)/Windows Kits/10")
set(DBGHELP_LIB_PATH "${WINDOWS_SDK_PATH}/Lib/10.0.22621.0/um/x64")

add_dependencies(Lowder
  LowUtil
  LowMath
  LowRenderer2
  LowCore
  #nativefiledialog
  assimp
  physx
)

add_dependencies(Lowder
  LowEditor
)

set(LOWDER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

#set_property(TARGET Lowder PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_}")

# The crash upload URL and token come from crash_reporting: in the
# project's project.yaml at runtime, see LowderCrashReporter.h
target_compile_definitions(Lowder PRIVATE
  LOW_MODULE_NAME="lowder"
  LOW_HIDE_WINDOWS_CONSOLE=$<IF:$<BOOL:${LOW_HIDE_CONSOLE}>,1,0>
)

include_directories(
  ${LOWDER_INCLUDE_DIRS}
)

set_target_properties(Lowder PROPERTIES OUTPUT_NAME "lowder")

target_link_libraries(Lowder PRIVATE
  LowCore
  LowRenderer2
  LowUtil
  LowMath
  #nativefiledialog
  assimp
  physx
)

if(WIN32)
  target_link_libraries(Lowder PRIVATE Dbghelp.lib)
  target_link_libraries(Lowder PRIVATE Gdiplus.lib)
  target_link_libraries(Lowder PRIVATE Winhttp.lib)
endif()

target_link_libraries(Lowder PRIVATE
  LowEditor
)

install(TARGETS Lowder RUNTIME DESTINATION bin COMPONENT editor)
install(FILES $<TARGET_RUNTIME_DLLS:Lowder> DESTINATION bin COMPONENT editor)
