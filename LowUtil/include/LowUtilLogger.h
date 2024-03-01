#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"
#include "LowUtilName.h"

#include "LowMathVectorUtil.h"

#include <stdint.h>
#include <string>

#define LOW_LOG_DEBUG                                                \
  Low::Util::Log::begin_log(Low::Util::Log::LogLevel::DEBUG,         \
                            LOW_MODULE_NAME)
#define LOW_LOG_INFO                                                 \
  Low::Util::Log::begin_log(Low::Util::Log::LogLevel::INFO,          \
                            LOW_MODULE_NAME)
#define LOW_LOG_WARN                                                 \
  Low::Util::Log::begin_log(Low::Util::Log::LogLevel::WARN,          \
                            LOW_MODULE_NAME)
#define LOW_LOG_ERROR                                                \
  Low::Util::Log::begin_log(Low::Util::Log::LogLevel::ERROR,         \
                            LOW_MODULE_NAME)
#define LOW_LOG_PROFILE                                              \
  Low::Util::Log::begin_log(Low::Util::Log::LogLevel::PROFILE,       \
                            LOW_MODULE_NAME)

#define LOW_LOG_END Low::Util::Log::LogLineEnd::LINE_END

namespace Low {
  namespace Util {
    struct LOW_EXPORT Handle;
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

      enum class LogLineEnd
      {
        LINE_END
      };

      struct LogStream;

      struct LogEntry
      {
        uint8_t level;
        String module;
        int threadId;
        time_t time;
        String message;
        bool terminate;
      };

      typedef void (*LogCallback)(const LogEntry &);

      LOW_EXPORT LogStream &begin_log(uint8_t p_LogLevel,
                                      const char *p_Module,
                                      bool p_Terminate = false);

      struct LOW_EXPORT LogStream
      {
        friend LogStream &begin_log(uint8_t, const char *, bool);

        LogStream &operator<<(LogLineEnd p_End);
        LogStream &operator<<(String &p_Message);
        LogStream &operator<<(const char *p_Message);
        LogStream &operator<<(std::string &p_Message);

        LogStream &operator<<(int p_Message);
        LogStream &operator<<(uint32_t p_Message);
        LogStream &operator<<(uint64_t p_Message);
        LogStream &operator<<(float p_Message);
        LogStream &operator<<(bool p_Message);

        LogStream &operator<<(Math::Vector2 &p_Vec);
        LogStream &operator<<(Math::Vector3 &p_Vec);
        LogStream &operator<<(Math::Vector4 &p_Vec);
        LogStream &operator<<(Math::Quaternion &p_Quat);
        LogStream &operator<<(Math::UVector2 &p_Vec);
        LogStream &operator<<(Math::UVector3 &p_Vec);

        LogStream &operator<<(Name &p_Name);
        LogStream &operator<<(Handle &p_Message);

      private:
        LogEntry m_Entry;
      };

      LOW_EXPORT void register_log_callback(LogCallback p_Callback);

      void initialize();
      void cleanup();

    } // namespace Log
  }   // namespace Util
} // namespace Low
