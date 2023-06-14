#include "LowUtilLogger.h"

#include "LowCoreMonoUtils.h"

namespace Low {
  namespace Core {
    namespace Mono {
      static void log(uint8_t p_LogLevel, MonoString *p_MonoString)
      {
        Util::String l_ParameterString = from_mono_string(p_MonoString);

        Util::Log::begin_log(p_LogLevel, "scripting")
            << l_ParameterString << LOW_LOG_END;
      }

      void log_debug(MonoString *p_MonoString)
      {
        log(Util::Log::LogLevel::DEBUG, p_MonoString);
      }
      void log_info(MonoString *p_MonoString)
      {
        log(Util::Log::LogLevel::INFO, p_MonoString);
      }
      void log_warn(MonoString *p_MonoString)
      {
        log(Util::Log::LogLevel::WARN, p_MonoString);
      }
      void log_error(MonoString *p_MonoString)
      {
        log(Util::Log::LogLevel::ERROR, p_MonoString);
      }
    } // namespace Mono
  }   // namespace Core
} // namespace Low
