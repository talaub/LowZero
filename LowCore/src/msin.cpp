#include <iostream>

#include "test.h"

#include "LowUtilTest.h"
#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"
#include "LowUtilYaml.h"

#include <stdint.h>

int main()
{
  Low::Math::Vector3 vec(0.0f, 1.0f, 0.0f);
  Low::Math::Vector3 voc;

  float mag = Low::Math::VectorUtil::magnitude_squared(vec);
  float mag2 = Low::Math::VectorUtil::magnitude_squared(voc);

  eastl::vector<int> testvec;
  testvec.push_back(8);
  testvec.push_back(15);

  return 0;
}
