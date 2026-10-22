project(LowShim)

add_executable(LowShim src/main.cpp)

# No LowUtil on purpose: the shim has to keep working across engine versions
target_link_libraries(LowShim PRIVATE yaml-cpp)
target_compile_features(LowShim PRIVATE cxx_std_20)

if (MSVC)
  target_compile_definitions(LowShim PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(LowShim PROPERTIES FOLDER "LowEngine/Tools")

# <engine>/bin/low.exe is LowCli; the shim goes into the per-user directory on PATH
install(PROGRAMS $<TARGET_FILE:LowShim>
  DESTINATION .
  RENAME low${CMAKE_EXECUTABLE_SUFFIX}
  COMPONENT shim)
