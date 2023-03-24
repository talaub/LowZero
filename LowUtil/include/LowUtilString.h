#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    namespace StringHelper {
      LOW_EXPORT bool ends_with(String &p_Full, String &p_Test);
      LOW_EXPORT bool begins_with(String &p_Full, String &p_Test);
    } // namespace StringHelper
  }   // namespace Util
} // namespace Low
