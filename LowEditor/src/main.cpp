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

#include "LowRenderer.h"
#include "LowRendererRenderFlow.h"

#include "LowCore.h"
#include "LowCoreGameLoop.h"
#include "LowCoreEntity.h"
#include "LowCoreTransform.h"

#include <stdint.h>

#include <microprofile.h>

#include "LowEditorMainWindow.h"

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

int main()
{
  float delta = 0.004f;

  Low::Util::initialize();

  Low::Renderer::initialize();

  Low::Core::initialize();

  Low::Core::Entity l_Entity = Low::Core::Entity::make(N(Entity1));
  Low::Core::Component::Transform l_Transform =
      Low::Core::Component::Transform::make(l_Entity);

  Low::Editor::initialize();

  Low::Core::GameLoop::register_tick_callback(&Low::Editor::tick);

  Low::Core::GameLoop::start();

  Low::Core::cleanup();

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}
