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

#include "LowRenderer.h"

#include <stdint.h>

#include "LowUtilTestType.h"

int main()
{
  Low::Util::initialize();

  Low::Renderer::initialize();

  while (Low::Renderer::window_is_open()) {
    Low::Renderer::tick(0.0f);
  }

  Low::Renderer::cleanup();

  Low::Util::cleanup();

  return 0;
}
