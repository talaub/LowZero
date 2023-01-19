#include <iostream>

#include "test.h"

#include "LowUtilTest.h"
#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include <stdint.h>

int main()
{
  std::cout << "Hello world" << std::endl;
  std::cout << hello() << std::endl;
  Low::Util::test();

  Low::Math::Vector3 vec(0.0f, 1.0f, 0.0f);
  Low::Math::Vector3 voc;

  float mag = Low::Math::VectorUtil::magnitude_squared(vec);
  float mag2 = Low::Math::VectorUtil::magnitude_squared(voc);

  eastl::vector<int> testvec;
  testvec.push_back(8);
  testvec.push_back(15);

  LOW_LOG_INFO("Startup");
  LOW_LOG_INFO("Startup");
  LOW_LOG_INFO("Startup");
  LOW_LOG_INFO("Startup");

  LOW_ASSERT_WARN(false, "Testassert");
  LOW_ASSERT(false, "Testassert");

  return 0;
}
