#include "LowUtilString.h"

#include "LowUtilLogger.h"
#include <cstdint>

namespace Low {
  namespace Util {
    namespace StringHelper {
      bool contains(String &p_Full, String p_Test)
      {
        return p_Full.find(p_Test) != eastl::string::npos;
      }

      bool ends_with(String &p_Full, String p_Test)
      {
        if (p_Full.length() >= p_Test.length()) {
          return (0 ==
                  p_Full.compare(p_Full.length() - p_Test.length(),
                                 p_Test.length(), p_Test));
        }
        return false;
      }

      bool begins_with(String &p_Full, String p_Test)
      {
        if (p_Full.length() >= p_Test.length()) {
          return (0 == p_Full.compare(0, p_Test.length(), p_Test));
        }
        return false;
      }

      void split(String p_String, char p_Delimiter,
                 List<String> &p_Parts)
      {
        String l_Current = "";
        for (uint32_t i = 0; i < p_String.size(); ++i) {
          if (p_String[i] == p_Delimiter) {
            p_Parts.push_back(l_Current);
            l_Current = "";
          } else {
            l_Current += p_String[i];
          }
        }
        if (!l_Current.empty()) {
          p_Parts.push_back(l_Current);
        }
      }

      String replace(String p_String, char p_Replacee,
                     char p_Replacer)
      {
        std::string l_String = p_String.c_str();
        std::replace(l_String.begin(), l_String.end(), p_Replacee,
                     p_Replacer);
        String l_Output = l_String.c_str();

        return l_Output;
      }

      void append(String &p_String, int p_Appendix)
      {
        p_String += std::to_string(p_Appendix).c_str();
      }

      String prettify_name(String p_String)
      {
        String l_String = p_String;
        if (StringHelper::begins_with(l_String, "p_")) {
          l_String = l_String.substr(2);
        }
        l_String = StringHelper::replace(p_String, '_', ' ');
        l_String[0] = toupper(l_String[0]);
        {
          Util::String l_Friendly;
          for (u32 i = 0; i < l_String.size(); ++i) {
            if (i) {
              if (islower(l_String[i - 1]) && !islower(l_String[i])) {
                l_Friendly += " ";
              }
            }

            l_Friendly += l_String[i];
          }
          l_String = l_Friendly;
        }
        return l_String;
      }

      String prettify_name(Name p_Name)
      {
        return prettify_name(String(p_Name.c_str()));
      }

      String technify_string(String p_String)
      {
        String output;
        for (char c : p_String) {
          if (std::isalnum(c)) {
            output += c; // keep letters and digits
          } else if (c == ' ' || c == '-' || c == '_') {
            output += '_'; // normalize common safe separators
          } else if (c == '.' || c == '/' || c == '\\' || c == ':' ||
                     c == '*' || c == '?' || c == '"' || c == '<' ||
                     c == '>' || c == '|') {
            // skip forbidden or unsafe characters
            continue;
          } else {
            output +=
                '_'; // fallback: replace unknowns with underscore
          }
        }
        return output;
      }
    } // namespace StringHelper

    namespace PathHelper {
      String get_base_name_no_ext(const String p_Path)
      {
        // Find the last slash or backslash
        size_t lastSlash = p_Path.find_last_of("\\/");
        size_t start =
            (lastSlash == eastl::string::npos) ? 0 : lastSlash + 1;

        // Extract just the filename (with extensions)
        eastl::string filename = p_Path.substr(start);

        // Find the first dot in the filename
        size_t dotPos = filename.find('.');
        if (dotPos != eastl::string::npos) {
          // Keep only the part before the first dot
          filename = filename.substr(0, dotPos);
        }

        return filename;
      }

      String get_file_subtype(const String p_Path)
      {
        // Find last slash or backslash
        size_t l_LastSlash = p_Path.find_last_of("/\\");
        size_t l_FileStart =
            (l_LastSlash == String::npos) ? 0 : l_LastSlash + 1;

        // Extract just the filename
        String l_Filename = p_Path.substr(l_FileStart);

        // Find dots
        size_t l_FirstDot = l_Filename.find('.');
        size_t l_LastDot = l_Filename.find_last_of('.');

        // If there aren’t at least 2 dots -> return empty
        if (l_FirstDot == String::npos || l_LastDot == String::npos ||
            l_FirstDot == l_LastDot) {
          return "";
        }

        // Extract between first and last dot
        return l_Filename.substr(l_FirstDot + 1,
                                 l_LastDot - l_FirstDot - 1);
      }

      String get_file_extension(const String p_Path)
      {
        // Find last slash or backslash
        size_t l_LastSlash = p_Path.find_last_of("/\\");
        size_t l_FileStart =
            (l_LastSlash == String::npos) ? 0 : l_LastSlash + 1;

        // Extract just the filename
        String l_Filename = p_Path.substr(l_FileStart);

        // Find last dot
        size_t l_LastDot = l_Filename.find_last_of('.');

        // If no dot -> return empty
        if (l_LastDot == String::npos) {
          return "";
        }

        // Return everything after last dot
        return l_Filename.substr(l_LastDot + 1);
      }

      String normalize(const String p_Path)
      {
        String l_Result = p_Path;
        for (size_t i = 0; i < l_Result.size(); ++i) {
          if (l_Result[i] == '\\') {
            l_Result[i] = '/';
          }
        }
        return l_Result;
      }
    } // namespace PathHelper

    String StringBuilder::get() const
    {
      return m_String;
    }

    StringBuilder &StringBuilder::endl()
    {
      return append("\n");
    }

    StringBuilder &StringBuilder::append(String p_String)
    {
      m_String += p_String;

      return *this;
    }

    StringBuilder &StringBuilder::append(const char *p_String)
    {
      String l_String = p_String;

      return append(l_String);
    }

    StringBuilder &StringBuilder::append(float p_Float)
    {
      return append(LOW_TO_STRING(p_Float));
    }

    StringBuilder &StringBuilder::append(uint8_t p_Content)
    {
      return append(LOW_TO_STRING(p_Content));
    }

    StringBuilder &StringBuilder::append(uint32_t p_Content)
    {
      return append(LOW_TO_STRING(p_Content));
    }

    StringBuilder &StringBuilder::append(uint64_t p_Content)
    {
      return append(LOW_TO_STRING(p_Content));
    }

    StringBuilder &StringBuilder::append(int p_Content)
    {
      return append(LOW_TO_STRING(p_Content));
    }

    StringBuilder &StringBuilder::append(Name p_Content)
    {
      return append(p_Content.c_str());
    }
  } // namespace Util
} // namespace Low
