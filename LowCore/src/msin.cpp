#include <iostream>

#include "test.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilYaml.h"
#include "LowUtilName.h"

#include "LowRendererWindow.h"

#include <stdint.h>

#include "LowUtilTestType.h"

int main()
{
  Low::Util::initialize();

  Low::Renderer::Window l_Window;
  Low::Renderer::WindowInit l_WindowInit;
  l_WindowInit.dimensions.x = 1280;
  l_WindowInit.dimensions.y = 820;
  l_WindowInit.title = "Low Editor";
  Low::Renderer::window_initialize(l_Window, l_WindowInit);

  while (l_Window.is_open()) {
    l_Window.tick();
  }

  l_Window.cleanup();

  Low::Util::cleanup();

  return 0;
}
