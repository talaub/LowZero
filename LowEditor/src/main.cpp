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

#include <stdint.h>

#include <microprofile.h>

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
  Low::Util::initialize();

  Low::Renderer::initialize();

  while (Low::Renderer::window_is_open()) {
    {
      Low::Renderer::tick(0.0f);

      ImGui::Begin("Editor");
      ImGui::End();

      Low::Renderer::late_tick(0.0f);
    }
    MicroProfileFlip(nullptr);
  }

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}
