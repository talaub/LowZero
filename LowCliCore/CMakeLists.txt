project(LowCliCore)

# Reusable CLI building blocks (output, terminal, child processes), so other
# command line tools like LowShaderCompiler or a headless lowder can share them
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "include/*.h")

add_library(LowCliCore STATIC ${SOURCES} ${INCLUDES})

source_group("Header Files" FILES ${INCLUDES})

target_include_directories(LowCliCore PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# LowUtil brings EASTL and nlohmann/json (through inja)
target_link_libraries(LowCliCore PUBLIC
  LowUtil
)

target_compile_definitions(LowCliCore PRIVATE
  LOW_MODULE_NAME="lowclicore"
)

set_target_properties(LowCliCore PROPERTIES FOLDER "LowEngine/Tools")

