project(LowAssetManager)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE INCLUDES "include/*.h")

add_executable(LowAssetManager ${SOURCES})

add_dependencies(LowAssetManager
  LowUtil
  LowMath
)

set(LOWASSETMANAGER_INCLUDE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(LowAssetManager PRIVATE
  LOW_MODULE_NAME="lowassetprep"
)

include_directories(
  ${LOWASSETMANAGER_INCLUDE_DIRS}
)

set_target_properties(LowAssetManager PROPERTIES OUTPUT_NAME "assetprep")

target_link_libraries(LowAssetManager PRIVATE
  LowUtil
  LowMath
  gli
)
 
