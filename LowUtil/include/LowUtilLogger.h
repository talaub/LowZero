#pragma once

#include "LowUtilApi.h"

#include <stdint.h>

#define LOW_LOG_DEBUG(x) Low::Util::Log::debug(LOW_MODULE_NAME, x)
#define LOW_LOG_INFO(x) Low::Util::Log::info(LOW_MODULE_NAME, x)
#define LOW_LOG_WARN(x) Low::Util::Log::warn(LOW_MODULE_NAME, x)
#define LOW_LOG_ERROR(x) Low::Util::Log::error(LOW_MODULE_NAME, x)
#define LOW_LOG_PROFILE(x) Low::Util::Log::profile(LOW_MODULE_NAME, x)

namespace Low {
  namespace Util {
    namespace Log {
      namespace LogLevel {
        enum Enum
        {
          INFO,
          DEBUG,
          WARN,
          ERROR,
          PROFILE
        };
      }

      LOW_EXPORT void log(uint8_t p_LogLevel, const char *p_Module,
                          const char *p_Message);

      LOW_EXPORT void info(const char *p_Module, const char *p_Message);
      LOW_EXPORT void debug(const char *p_Module, const char *p_Message);
      LOW_EXPORT void warn(const char *p_Module, const char *p_Message);
      LOW_EXPORT void error(const char *p_Module, const char *p_Message);
      LOW_EXPORT void profile(const char *p_Module, const char *p_Message);
    } // namespace Log
  }   // namespace Util
} // namespace Low
