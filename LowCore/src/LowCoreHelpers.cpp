#include "LowCore.h"

#include "LowUtilGlobals.h"

namespace Low {
  namespace Core {
    Scene get_loaded_scene()
    {
      return Scene::get_loaded_scene();
    }

    void game_dimensions(Math::UVector2 &p_Dimensions)
    {
      p_Dimensions = Util::Globals::get(N(LOW_GAME_DIMENSIONS));
    }
  } // namespace Core
} // namespace Low
