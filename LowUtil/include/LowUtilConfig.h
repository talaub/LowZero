#pragma once

#include "LowUtilName.h"
#include "LowUtilApi.h"

namespace Low {
  namespace Util {
    namespace Config {
      LOW_EXPORT void initialize();

      LOW_EXPORT uint32_t get_capacity(Name p_TypeName);
    } // namespace Config
  }   // namespace Util
} // namespace Low
