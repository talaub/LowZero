project(LowLauncher)

# Message boxes + CreateProcessW, nothing to launch elsewhere yet
if (NOT WIN32)
  return()
endif()

add_executable(LowLauncher WIN32
  src/main.cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/../Lowder/resources/Lowder.rc"
)

# No LowUtil on purpose: the launcher sits in project roots and has to
# be able to report a missing or incomplete engine folder
target_compile_features(LowLauncher PRIVATE cxx_std_20)
target_compile_definitions(LowLauncher PRIVATE UNICODE _UNICODE)

set_target_properties(LowLauncher PROPERTIES
  OUTPUT_NAME "lowlauncher"
  FOLDER "LowEngine/Tools"
)

# Packaging copies it from <engine>/bin into the project root
install(TARGETS LowLauncher RUNTIME DESTINATION bin COMPONENT editor)
