#include <iostream>

#include "test.h"

#include "LowUtilTest.h"
#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"
#include "LowUtilFileIO.h"

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

  LOW_ASSERT(false, "This is a test");

  {
    using namespace Low::Util::FileIO;

    Low::Util::FileIO::File l_File = Low::Util::FileIO::open(
        "P:\\.gitignore", Low::Util::FileIO::FileMode::READ_BYTES);

    char text[1024];
    read_sync(l_File, text);

    LOW_LOG_DEBUG(text);
  }

  return 0;
}
