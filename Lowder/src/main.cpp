#include <iostream>

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilYaml.h"
#include "LowUtilName.h"
#include "LowUtilResource.h"
#include "LowUtilContainers.h"

#include "LowRenderer.h"
#include "LowRendererExposedObjects.h"
#include "LowRendererRenderFlow.h"

#include "LowCore.h"
#include "LowCoreGameLoop.h"
#include "LowCoreScene.h"
#include "LowCoreRegion.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"
#include "LowCoreDirectionalLight.h"

#include <stdint.h>

#include "LowEditorMainWindow.h"

#include <windows.h>

typedef int(__stdcall *f_funci)();

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

static void setup_scene()
{
  Low::Core::Scene l_Scene =
      Low::Core::Scene::find_by_name(N(TestScene));
  l_Scene.load();
};

int run_low()
{
  HINSTANCE hGetProcIDDLL =
      LoadLibrary("../../../misteda/build/Debug/app/Gameplayd.dll");

  if (!hGetProcIDDLL) {
    std::cout << "could not load the dynamic library" << std::endl;
    return 1;
  }

  // resolve function address here
  f_funci plugin_initialize =
      (f_funci)GetProcAddress(hGetProcIDDLL, "plugin_initialize");
  if (!plugin_initialize) {
    std::cout << "could not locate the initialize function"
              << std::endl;
    return 1;
  }

  f_funci plugin_cleanup =
      (f_funci)GetProcAddress(hGetProcIDDLL, "plugin_cleanup");
  if (!plugin_cleanup) {
    std::cout << "could not locate the cleanup function" << std::endl;
    return 1;
  }

  Low::Util::initialize();

  Low::Renderer::initialize();

  Low::Core::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  // Mtd::initialize();

  plugin_initialize();

  setup_scene();

  Low::Editor::initialize();

  Low::Core::GameLoop::start();

  // Mtd::cleanup();

  plugin_cleanup();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}

int main()
{
  return run_low();
}
