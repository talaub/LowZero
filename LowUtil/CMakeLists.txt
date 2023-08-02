project(LowUtil)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowUtil SHARED
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile/microprofile.cpp"
  ${SOURCES}
)

set(LOWUTIL_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_dependencies(LowUtil
  LowMath
)

option(MICROPROFILE_USE_CONFIG_FILE "Use user provided configuration in microprofile.config.h file." OFF)

set(MICROPROFILE_EXPORT_FILENAME microprofile.export.h)
set(MICROPROFILE_CONFIG_HEADER ${PROJECT_SOURCE_DIR}/microprofile.config.h)
set(MICROPROFILE_PUBLIC_HEADERS
	${PROJECT_SOURCE_DIR}/microprofile.h
	${CMAKE_CURRENT_BINARY_DIR}/${MICROPROFILE_EXPORT_FILENAME}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
	EXPORT_MACRO_NAME MICROPROFILE_API
	EXPORT_FILE_NAME ${MICROPROFILE_EXPORT_FILENAME}
)

set_target_properties(LowUtil PROPERTIES OUTPUT_NAME "lowutil")

target_include_directories(LowUtil PUBLIC
  ${LOWUTIL_INCLUDE_DIRS}
)

target_compile_definitions(LowUtil PRIVATE
  LOW_MODULE_NAME="lowutil"
  lowutil_EXPORTS
)

set(EASTL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/EASTL")
target_include_directories (LowUtil PUBLIC
    ${EASTL_ROOT_DIR}/include
    ${EASTL_ROOT_DIR}/test/packages/EAAssert/include
    ${EASTL_ROOT_DIR}/test/packages/EABase/include/Common
    ${EASTL_ROOT_DIR}/test/packages/EAMain/include
    ${EASTL_ROOT_DIR}/test/packages/EAStdC/include
    ${EASTL_ROOT_DIR}/test/packages/EATest/include
    ${EASTL_ROOT_DIR}/test/packages/EAThread/include
)
add_custom_target(NatVis SOURCES ${EASTL_ROOT_DIR}/doc/EASTL.natvis)

target_link_libraries(LowUtil PRIVATE
  LowMath
)

target_link_libraries(LowUtil PUBLIC
  "${CMAKE_BINARY_DIR}/LowDependencies/EASTL/build/${CMAKE_BUILD_TYPE}/EASTL.lib"
  yaml-cpp
  gli
  tlsf
  assimp
)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(${PROJECT_NAME} PUBLIC MICROPROFILE_EXPORT)

if (MICROPROFILE_USE_CONFIG_FILE)
	target_compile_definitions(${PROJECT_NAME} PUBLIC MICROPROFILE_USE_CONFIG)
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MICROPROFILE_PUBLIC_HEADERS}")

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(stb QUIET IMPORTED_TARGET stb)
endif()
if (stb_FOUND)
	target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::stb)
	target_compile_definitions(${PROJECT_NAME} PRIVATE MICROPROFILE_SYSTEM_STB)
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME})
if (MICROPROFILE_USE_CONFIG_FILE)
	install(FILES ${MICROPROFILE_CONFIG_HEADER} TYPE INCLUDE)
endif()
