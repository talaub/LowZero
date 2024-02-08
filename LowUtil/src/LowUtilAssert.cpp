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
    } // namespace Assertion
  }   // namespace Util
} // namespace Low
