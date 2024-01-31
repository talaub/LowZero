#include "LowUtilAssert.h"

#include "LowUtilLogger.h"

#include <string>

namespace Low {
  namespace Util {
    namespace Assertion {
      void print_assert(uint8_t p_LogLevel, const char *p_Module,
                        const char *p_Message, const char *p_File, int p_Line,
                        const char *p_Function, bool p_Terminate)
      {
        std::string l_Message = "";
        l_Message += std::string("\x1B[31mASSERTION FAILED\033[0m - ") +
                     p_File + ":" + std::to_string(p_Line);

        if (p_Message) {
          l_Message += std::string(", Message: ") + p_Message;
        }

        Log::begin_log(p_LogLevel, p_Module, p_Terminate)
            << l_Message << LOW_LOG_END;
      }

      bool assert_that(bool p_Condition, const char *p_Module, char *p_Message,
                       const char *p_File, int p_Line, const char *p_Function)
      {
        if (!p_Condition) {
          print_assert(Log::LogLevel::ERROR, p_Module, p_Message, p_File,
                       p_Line, p_Function, true);
          __debugbreak();
        }

        return p_Condition;
      }

      bool assert_ignore(bool p_Condition, const char *p_Module,
                         char *p_Message, const char *p_File, int p_Line,
                         const char *p_Function)
      {
        if (!p_Condition) {
          print_assert(Log::LogLevel::WARN, p_Module, p_Message, p_File, p_Line,
                       p_Function, false);
        }

        return p_Condition;
      }

      bool assert_that(bool p_Condition, const char *p_Module,
                       const char *p_File, int p_Line, const char *p_Function)
      {
        if (!p_Condition) {
          print_assert(Log::LogLevel::ERROR, p_Module, nullptr, p_File, p_Line,
                       p_Function, true);
          __debugbreak();
        }

        return p_Condition;
      }

      bool assert_ignore(bool p_Condition, const char *p_Module,
                         const char *p_File, int p_Line, const char *p_Function)
      {
        if (!p_Condition) {
          print_assert(Log::LogLevel::WARN, p_Module, nullptr, p_File, p_Line,
                       p_Function, false);
        }

        return p_Condition;
      }
    } // namespace Assertion
  }   // namespace Util
} // namespace Low
