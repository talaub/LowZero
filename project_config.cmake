SET(LOW_PATH "P:\\")

SET(LOW_DEPENDENCIES_DIR "${LOW_PATH}\\LowDependencies")
SET(EASTL_ROOT_DIR "${LOW_PATH}\\LowDependencies\\EASTL")

include_directories(
  ${EASTL_ROOT_DIR}/include
  ${EASTL_ROOT_DIR}/test/packages/EAAssert/include
  ${EASTL_ROOT_DIR}/test/packages/EABase/include/Common
  ${EASTL_ROOT_DIR}/test/packages/EAMain/include
  ${EASTL_ROOT_DIR}/test/packages/EAStdC/include
  ${EASTL_ROOT_DIR}/test/packages/EATest/include
  ${EASTL_ROOT_DIR}/test/packages/EAThread/include

  ${LOW_DEPENDENCIES_DIR}/glm
  ${LOW_DEPENDENCIES_DIR}/tlsf
  ${LOW_DEPENDENCIES_DIR}/Cflat
  ${LOW_DEPENDENCIES_DIR}/yaml-cpp/include
  ${LOW_DEPENDENCIES_DIR}/microprofile
  ${LOW_DEPENDENCIES_DIR}/imgui

  ${LOW_DEPENDENCIES_DIR}/glfw/include
  $ENV{VK_SDK_PATH}/Include
)

include_directories(
  "${LOW_PATH}LowMath\\include"
  "${LOW_PATH}LowUtil\\include"
  "${LOW_PATH}LowRenderer\\include"
  "${LOW_PATH}LowCore\\include"
)

target_link_directories(${PROJ_NAME} PRIVATE
  "${LOW_PATH}build\\Debug\\lib"
  "${LOW_PATH}build\\LowDependencies\\EASTL\\build\\Debug"
)

#target_link_libraries(${PROJ_NAME} PRIVATE
#"${LOW_PATH}build\\Debug\\app\\lowmathd.dll"
#"${LOW_PATH}build\\Debug\\app\\lowutild.dll"
#"${LOW_PATH}build\\Debug\\app\\lowrendererd.dll"
#"${LOW_PATH}build\\Debug\\app\\lowcored.dll"
#)

target_link_libraries(${PROJ_NAME} PRIVATE
  "D:\\projects\\lengine\\build\\Debug\\lib\\yaml-cppd.lib"
)

target_link_libraries(${PROJ_NAME} PRIVATE
  lowmathd
  lowutild
  lowrendererd
  lowcored
  zlibstaticd
  EASTL
)

add_compile_definitions(
  YAML_CPP_STATIC_DEFINE
)
