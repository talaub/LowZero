SET(LOW_PATH "P:\\")

SET(LOW_DEPENDENCIES_DIR "${LOW_PATH}\\LowDependencies")
SET(EASTL_ROOT_DIR "${LOW_PATH}\\LowDependencies\\EASTL")

target_include_directories(${PROJ_NAME} PRIVATE
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

add_compile_definitions(
  YAML_CPP_STATIC_DEFINE
)


target_include_directories(${PROJ_NAME} PRIVATE
  "${LOW_PATH}LowMath\\include"
  "${LOW_PATH}LowUtil\\include"
  "${LOW_PATH}LowRenderer\\include"
  "${LOW_PATH}LowCore\\include"
)

target_link_directories(${PROJ_NAME} PRIVATE
  "${LOW_PATH}build\\Debug\\lib"
  "${LOW_PATH}build\\LowDependencies\\EASTL\\build\\Release"
)

target_link_libraries(${PROJ_NAME} PRIVATE
)

target_link_libraries(${PROJ_NAME} PRIVATE
  lowmath
  lowutil
  lowrenderer
  lowcore
  zlibstaticd
  EASTL
  yaml-cpp
)

# ===============================================
if(CMAKE_CONFIGURATION_TYPES)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)

        find_library(LowUtil_LIBRARY_${CONFIG_TYPE_UPPER}
            NAMES lowutil
            HINTS "${LOW_PATH}build/${CONFIG_TYPE}/lib"
        )
      find_library(LowMath_LIBRARY_${CONFIG_TYPE_UPPER}
            NAMES lowmath
            HINTS "${LOW_PATH}build/${CONFIG_TYPE}/lib"
        )
      find_library(LowRenderer_LIBRARY_${CONFIG_TYPE_UPPER}
            NAMES lowrenderer
            HINTS "${LOW_PATH}build/${CONFIG_TYPE}/lib"
        )
        find_library(LowCore_LIBRARY_${CONFIG_TYPE_UPPER}
            NAMES lowcore
            HINTS "${LOW_PATH}build/${CONFIG_TYPE}/lib"
        )

    endforeach()
endif()

find_path(LowUtil_INCLUDE_DIR
  NAMES LowUtil.h
  HINTS "${LOW_PATH}LowUtil/include"
)
find_path(LowMath_INCLUDE_DIR
  NAMES LowMath.h
  HINTS "${LOW_PATH}LowMath/include"
)
find_path(LowRenderer_INCLUDE_DIR
  NAMES LowRenderer.h
  HINTS "${LOW_PATH}LowRenderer/include"
)
find_path(LowCore_INCLUDE_DIR
  NAMES LowCore.h
  HINTS "${LOW_PATH}LowCore/include"
)

include_directories(${LowUtil_INCLUDE_DIR})
include_directories(${LowMath_INCLUDE_DIR})
include_directories(${LowRenderer_INCLUDE_DIR})
include_directories(${LowCore_INCLUDE_DIR})

add_dependencies(MistedaPlugin
  #LowDependencies
  #LowUtil
  #LowMath
  #LowRenderer
  #LowCore
  LowZero
)

#target_link_libraries(MistedaPlugin PUBLIC
  #LowUtil
  # LowMath
  #  LowRenderer
  #LowCore
  #)

 foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
        target_link_libraries(MistedaPlugin 
          ${LowUtil_LIBRARY_${CONFIG_TYPE_UPPER}} 
          ${LowMath_LIBRARY_${CONFIG_TYPE_UPPER}} 
          ${LowRenderer_LIBRARY_${CONFIG_TYPE_UPPER}} 
          ${LowCore_LIBRARY_${CONFIG_TYPE_UPPER}} 
          )
    endforeach()
