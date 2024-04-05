SET EASTL_BUILD_DIR=..\build\LowDependencies\EASTL\build
mkdir %EASTL_BUILD_DIR%
pushd %EASTL_BUILD_DIR%
cmake ..\..\..\..\LowDependencies\EASTL -DEASTL_BUILD_TESTS:BOOL=OFF -DEASTL_BUILD_BENCHMARK:BOOL=OFF
cmake --build . --config Release
cmake --build . --config Debug
cmake --build . --config RelWithDebInfo
cmake --build . --config MinSizeRel
popd
