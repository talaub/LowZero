#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <string>

#define LOW_TO_STRING(x) Low::Util::String(std::to_string(x).c_str())

namespace Low {
  namespace Util {
    struct Name;

    struct LOW_EXPORT StringBuilder
    {
      StringBuilder &append(String p_String);
      StringBuilder &append(const char *p_String);
      StringBuilder &append(float p_Float);
      StringBuilder &append(uint32_t p_Content);
      StringBuilder &append(int p_Content);
      StringBuilder &append(uint64_t p_Content);
      StringBuilder &append(uint8_t p_Content);
      StringBuilder &append(Name p_Content);

      StringBuilder &endl();

      String get() const;

    private:
      String m_String;
    };
    namespace StringHelper {
      LOW_EXPORT bool ends_with(String &p_Full, String p_Test);
      LOW_EXPORT bool begins_with(String &p_Full, String p_Test);
      LOW_EXPORT bool contains(String &p_Full, String p_Test);

      LOW_EXPORT void split(String p_String, char p_Delimiter,
                            List<String> &p_Parts);

      LOW_EXPORT String replace(String p_String, char p_Replacee,
                                char p_Replacer);

      LOW_EXPORT void append(String &p_String, int p_Appendix);
    } // namespace StringHelper
    namespace PathHelper {
      String get_base_name_no_ext(const String p_Path);
    }
  } // namespace Util
} // namespace Low
