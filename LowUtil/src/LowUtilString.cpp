#include "LowUtilString.h"

namespace Low {
  namespace Util {
    namespace StringHelper {
      bool ends_with(String &p_Full, String p_Test)
      {
        if (p_Full.length() >= p_Test.length()) {
          return (0 == p_Full.compare(p_Full.length() - p_Test.length(),
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

      void split(String p_String, char p_Delimiter, List<String> &p_Parts)
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
      }
    } // namespace StringHelper
  }   // namespace Util
} // namespace Low
