#pragma once

#include "LowUtilApi.h"

#include "LowUtilLogger.h"

#if defined(_MSC_VER)
// MSVC / clang-cl on Windows
#include <intrin.h>
#define DEBUG_BREAK() __debugbreak()

#elif defined(__has_builtin)
// Clang: prefer the explicit debugtrap if available
#if __has_builtin(__builtin_debugtrap)
#define DEBUG_BREAK() __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define DEBUG_BREAK() __builtin_trap()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#elif defined(__GNUC__)
// GCC: use inline asm per-arch, fall back to SIGTRAP
#if defined(__i386__) || defined(__x86_64__)
#define DEBUG_BREAK() __asm__ __volatile__("int3")
#elif defined(__aarch64__)
#define DEBUG_BREAK() __asm__ __volatile__("brk #0")
#elif defined(__arm__)
#define DEBUG_BREAK()                                                \
  __asm__ __volatile__(".inst 0xe7f001f0") /* BKPT 0 */
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#else
// Very old/unknown compilers
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace Low {
  namespace Util {
    namespace Assertion {
      LOW_EXPORT
      void print_assert(u8 p_LogLevel, const char *p_Module,
                        const char *p_Message, const char *p_File,
                        int p_Line, const char *p_Function,
                        bool p_Terminate);

    } // namespace Assertion
  } // namespace Util
} // namespace Low

#define LOW_ASSERT(cond, msg)                                        \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::FATAL, LOW_MODULE_NAME, msg,     \
          __FILE__, __LINE__, __FUNCTION__, true);                   \
      DEBUG_BREAK();                                                 \
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

#define LOW_ASSERT_ERROR_RETURN_FALSE(cond, msg)                     \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::ERROR, LOW_MODULE_NAME, msg,     \
          __FILE__, __LINE__, __FUNCTION__, false);                  \
      return false;                                                  \
    }                                                                \
  }

#define LOW_ASSERT_ERROR_RETURN(cond, msg)                           \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Util::Assertion::print_assert(                            \
          Low::Util::Log::LogLevel::ERROR, LOW_MODULE_NAME, msg,     \
          __FILE__, __LINE__, __FUNCTION__, false);                  \
      return;                                                        \
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
      DEBUG_BREAK();                                                 \
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
