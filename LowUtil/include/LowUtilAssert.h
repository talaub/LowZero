#pragma once

#include "LowUtilApi.h"

#define LOW_ASSERT(cond, msg)                                                  \
  Low::Util::Assertion::assert_that(cond, LOW_MODULE_NAME, msg, __FILE__,      \
                                    __LINE__, __FUNCTION__)

#define LOW_ASSERT_WARN(cond, msg)                                             \
  Low::Util::Assertion::assert_ignore(cond, LOW_MODULE_NAME, msg, __FILE__,    \
                                      __LINE__, __FUNCTION__)

#define _LOW_ASSERT(cond)                                                      \
  Low::Util::Assertion::assert_that(cond, LOW_MODULE_NAME, __FILE__, __LINE__, \
                                    __FUNCTION__)

#define _LOW_ASSERT_WARN(cond)                                                 \
  Low::Util::Assertion::assert_ignore(cond, LOW_MODULE_NAME, __FILE__,         \
                                      __LINE__, __FUNCTION__)

namespace Low {
  namespace Util {
    namespace Assertion {
      LOW_EXPORT bool assert_that(bool p_Condition, const char *p_Module,
                                  char *p_Message, const char *p_File,
                                  int p_Line, const char *p_Function);

      LOW_EXPORT bool assert_ignore(bool p_Condition, const char *p_Module,
                                    char *p_Message, const char *p_File,
                                    int p_Line, const char *p_Function);

      LOW_EXPORT bool assert_that(bool p_Condition, const char *p_Module,
                                  const char *p_File, int p_Line,
                                  const char *p_Function);

      LOW_EXPORT bool assert_ignore(bool p_Condition, const char *p_Module,
                                    const char *p_File, int p_Line,
                                    const char *p_Function);

    } // namespace Assertion
  }   // namespace Util
} // namespace Low
