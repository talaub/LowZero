#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <string>

#define LOW_TO_STRING(x) Low::Util::String(std::to_string(x).c_str())

namespace Low {
  namespace Util {
    namespace StringHelper {
      LOW_EXPORT bool ends_with(String &p_Full, String p_Test);
      LOW_EXPORT bool begins_with(String &p_Full, String p_Test);

      LOW_EXPORT void split(String p_String, char p_Delimiter,
                            List<String> &p_Parts);
    } // namespace StringHelper
  }   // namespace Util
} // namespace Low
