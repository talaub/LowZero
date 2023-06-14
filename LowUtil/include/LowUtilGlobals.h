#pragma once

#include "LowUtilApi.h"
#include "LowUtilVariant.h"
#include "LowUtilName.h"

#define G(x) Low::Util::Globals::get(Low::Util::Name(#x));

namespace Low {
  namespace Util {
    namespace Globals {
      LOW_EXPORT void set(Name p_Name, Variant p_Value);
      LOW_EXPORT Variant get(Name p_Name);
    } // namespace Globals
  }   // namespace Util
} // namespace Low
