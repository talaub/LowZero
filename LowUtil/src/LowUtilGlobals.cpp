#include "LowUtilGlobals.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    namespace Globals {
      Map<Name, Variant> g_GlobalValues;

      void set(Name p_Name, Variant p_Value)
      {
        g_GlobalValues[p_Name] = p_Value;
      }

      Variant get(Name p_Name)
      {
        LOW_ASSERT(g_GlobalValues.find(p_Name) !=
                       g_GlobalValues.end(),
                   "Could not find global");

        return g_GlobalValues[p_Name];
      }
    } // namespace Globals
  }   // namespace Util
} // namespace Low
