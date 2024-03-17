#include "LowCore.h"

#include "LowUtilGlobals.h"

namespace Low {
  namespace Core {
    void game_dimensions(Math::UVector2 &p_Dimensions)
    {
      p_Dimensions = Util::Globals::get(N(LOW_GAME_DIMENSIONS));
    }
  } // namespace Core
} // namespace Low
