#include "LowUtilString.h"

namespace Low {
  namespace Util {
    namespace StringHelper {
      bool ends_with(String &p_Full, String &p_Test)
      {
        if (p_Full.length() >= p_Test.length()) {
          return (0 == p_Full.compare(p_Full.length() - p_Test.length(),
                                      p_Test.length(), p_Test));
        }
        return false;
      }
    } // namespace StringHelper
  }   // namespace Util
} // namespace Low
