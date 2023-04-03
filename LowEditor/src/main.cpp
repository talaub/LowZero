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

  Low::Editor::initialize();

  while (Low::Renderer::window_is_open()) {
    {
      Low::Renderer::tick(delta);

      Low::Editor::tick(delta);

      Low::Renderer::late_tick(delta);
    }

    MicroProfileFlip(nullptr);
  }

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}
