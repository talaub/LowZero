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

      Util::String LOW_EXPORT prettify_name(Util::Name p_Name);
      Util::String LOW_EXPORT prettify_name(Util::String p_String);
      Util::String LOW_EXPORT technify_string(Util::String p_String);

      inline String to_upper(const String &p_Input)
      {
        String l_Result = p_Input;
        for (char &l_C : l_Result) {
          l_C = static_cast<char>(
              std::toupper(static_cast<unsigned char>(l_C)));
        }
        return l_Result;
      }
    } // namespace StringHelper
    namespace PathHelper {
      LOW_EXPORT String get_base_name_no_ext(const String p_Path);
      LOW_EXPORT String get_file_subtype(const String p_Path);
      LOW_EXPORT String get_file_extension(const String p_Path);
      LOW_EXPORT String normalize(const String p_Path);
    } // namespace PathHelper
  } // namespace Util
} // namespace Low
