SET(CMAKE_CURRENT_BINARY_DIR "${LOW_BINARY_PATH}/LowUtil")

project(LowUtil)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_library(LowUtil SHARED
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile/microprofile.cpp"
  ${SOURCES}
  ${INCLUDES}
)

set(LOWUTIL_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_dependencies(LowUtil
  EASTL
  yaml-cpp
  gli
  tlsf
  assimp
  LowMath
  SDL2
)

MESSAGE("LOWUTILBUILD: ${CMAKE_CURRENT_BINARY_DIR}")

#option(MICROPROFILE_USE_CONFIG_FILE "Use user provided configuration in microprofile.config.h file." OFF)

#set(MICROPROFILE_EXPORT_FILENAME microprofile.export.h)
#set(MICROPROFILE_CONFIG_HEADER ${PROJECT_SOURCE_DIR}/microprofile.config.h)
#set(MICROPROFILE_PUBLIC_HEADERS
	#${PROJECT_SOURCE_DIR}/microprofile.h
	#${CMAKE_CURRENT_BINARY_DIR}/${MICROPROFILE_EXPORT_FILENAME}
#)

#set(THREADS_PREFER_PTHREAD_FLAG ON)

set_target_properties(LowUtil PROPERTIES OUTPUT_NAME "lowutil")

source_group("Header Files" FILES ${INCLUDES})

target_compile_definitions(LowUtil PRIVATE
  LOW_MODULE_NAME="lowutil"
  lowutil_EXPORTS
)

target_compile_definitions(LowUtil PUBLIC
MICROPROFILE_EXPORT
)

target_link_libraries(LowUtil PRIVATE
  assimp
)

target_link_libraries(LowUtil PUBLIC
  SDL2
  EASTL
  yaml-cpp
  tlsf
  gli
  LowMath
)

target_include_directories(LowUtil PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../LowDependencies/microprofile>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

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
