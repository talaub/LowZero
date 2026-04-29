project(Lowder)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_executable(Lowder ${SOURCES} ${INCLUDES})

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
)

#set_property(TARGET Lowder PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_}")

set(LOW_CRASH_REPORT_UPLOAD_URL "" CACHE STRING "Crash reporter submit URL")
set(LOW_CRASH_REPORT_UPLOAD_TOKEN "" CACHE STRING "Crash reporter upload token")

if(DEFINED ENV{LOW_CRASH_REPORT_UPLOAD_URL} AND NOT "$ENV{LOW_CRASH_REPORT_UPLOAD_URL}" STREQUAL "")
  set(LOW_CRASH_REPORT_UPLOAD_URL "$ENV{LOW_CRASH_REPORT_UPLOAD_URL}")
endif()

if(DEFINED ENV{LOW_CRASH_REPORT_UPLOAD_TOKEN} AND NOT "$ENV{LOW_CRASH_REPORT_UPLOAD_TOKEN}" STREQUAL "")
  set(LOW_CRASH_REPORT_UPLOAD_TOKEN "$ENV{LOW_CRASH_REPORT_UPLOAD_TOKEN}")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/CrashReporter.local.cmake" OPTIONAL)

target_compile_definitions(Lowder PRIVATE
  LOW_MODULE_NAME="lowder"
  LOW_HIDE_WINDOWS_CONSOLE=$<IF:$<BOOL:${ENGINE_BUILD}>,1,0>
  "LOW_CRASH_REPORT_UPLOAD_URL=\"${LOW_CRASH_REPORT_UPLOAD_URL}\""
  "LOW_CRASH_REPORT_UPLOAD_TOKEN=\"${LOW_CRASH_REPORT_UPLOAD_TOKEN}\""
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
