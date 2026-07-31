#include "LowCorePhysics.h"

#include "LowCore.h"
#include "LowCoreScene.h"

namespace Low {
  namespace Core {
    namespace Physics {
      bool raycast(const Math::Vector3 &p_Origin,
                   const Math::Vector3 &p_Direction,
                   float p_MaxDistance, QueryHit *p_Hit)
      {
        Scene l_Scene = get_loaded_scene();
        World l_World = l_Scene.get_physics_world();

        return l_World.raycast(p_Origin, p_Direction, p_MaxDistance,
                               p_Hit);
      }
    } // namespace Physics
  } // namespace Core
} // namespace Low
