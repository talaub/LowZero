#include <iostream>

#include "imgui.h"

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

#include <microprofile.h>
#include <vector>

#include "LowEditorMainWindow.h"

#include "MtdPlugin.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

static void setup_scene()
{
  Low::Core::Scene l_Scene = Low::Core::Scene::find_by_name(N(TestScene));
  l_Scene.load();
};

int run_low()
{
  Low::Util::initialize();

  Low::Renderer::initialize();

  Low::Core::initialize();

  setup_scene();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  Mtd::initialize();

  Low::Editor::initialize();

  Low::Core::GameLoop::start();

  Mtd::cleanup();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}

int main()
{
  return run_low();
}
