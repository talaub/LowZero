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

#include <stdint.h>

#include "LowUtilTestType.h"

int main()
{
  Low::Util::initialize();

  Low::Util::TestType toast = Low::Util::TestType::make(N(Hiiii));

  bool o = toast.is_happy();

  toast.set_age(52.12f);

  LOW_LOG_DEBUG(toast.get_name().c_str());

  float x = toast.get_age();

  return 0;
}
