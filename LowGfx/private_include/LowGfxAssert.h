#pragma once

#include "LowGfxLog.h"
#include "LowGfxLogInternal.h"

#include <string>

#if defined(_MSC_VER)
#include <intrin.h>
#define GFX_DEBUG_BREAK() __debugbreak()

#elif defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define GFX_DEBUG_BREAK() __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define GFX_DEBUG_BREAK() __builtin_trap()
#else
#include <signal.h>
#define GFX_DEBUG_BREAK() raise(SIGTRAP)
#endif

#elif defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
#define GFX_DEBUG_BREAK() __asm__ __volatile__("int3")
#elif defined(__aarch64__)
#define GFX_DEBUG_BREAK() __asm__ __volatile__("brk #0")
#elif defined(__arm__)
#define GFX_DEBUG_BREAK() __asm__ __volatile__(".inst 0xe7f001f0")
#else
#include <signal.h>
#define GFX_DEBUG_BREAK() raise(SIGTRAP)
#endif

#else
#include <signal.h>
#define GFX_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace Low {
  namespace Gfx {
    namespace Detail {
      inline void assert_fail(const char *p_Message,
                              const char *p_File, int p_Line,
                              const char *p_Function)
      {
        std::string l_Message = "Assertion failed";
        if (p_Message && p_Message[0]) {
          l_Message += ": ";
          l_Message += p_Message;
        }
        l_Message += " (";
        l_Message += p_File ? p_File : "";
        l_Message += ":";
        append_formatted_value(l_Message, p_Line);
        l_Message += ", ";
        l_Message += p_Function ? p_Function : "";
        l_Message += ")";

        log(LogLevel::Fatal, l_Message.c_str());
      }
    } // namespace Detail
  } // namespace Gfx
} // namespace Low

#define GFX_ASSERT(cond, msg)                                        \
  {                                                                  \
    if (!(cond)) {                                                   \
      Low::Gfx::Detail::assert_fail(msg, __FILE__, __LINE__,         \
                                    __FUNCTION__);                   \
      GFX_DEBUG_BREAK();                                             \
    }                                                                \
  }
