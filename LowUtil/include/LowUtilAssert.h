#pragma once

#include "LowUtilApi.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Util {
    namespace Assertion {
      LOW_EXPORT
      void print_assert(u8 p_LogLevel, const char *p_Module,
                        const char *p_Message, const char *p_File,
                        int p_Line, const char *p_Function,
                        bool p_Terminate);

    } // namespace Assertion
  }   // namespace Util
} // namespace Low

#define LOW_ASSERT(cond, msg)                                        \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::FATAL, LOW_MODULE_NAME, msg,     \
          __FILE__, __LINE__, __FUNCTION__, true);                   \
      __debugbreak();                                                \
    }                                                                \
  }

#define LOW_ASSERT_ERROR(cond, msg)                                  \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::ERROR, LOW_MODULE_NAME, msg,     \
          __FILE__, __LINE__, __FUNCTION__, false);                  \
    }                                                                \
  }

#define LOW_ASSERT_WARN(cond, msg)                                   \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::WARN, LOW_MODULE_NAME, msg,      \
          __FILE__, __LINE__, __FUNCTION__, false);                  \
    }                                                                \
  }

#define _LOW_ASSERT(cond)                                            \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::FATAL, LOW_MODULE_NAME, nullptr, \
          __FILE__, __LINE__, __FUNCTION__, true);                   \
      __debugbreak();                                                \
    }                                                                \
  }

#define _LOW_ASSERT_WARN(cond)                                       \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::WARN, LOW_MODULE_NAME, nullptr,  \
          __FILE__, __LINE__, __FUNCTION__, false);                  \
    }                                                                \
  }
