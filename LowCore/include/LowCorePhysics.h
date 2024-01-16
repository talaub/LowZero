#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace Physics {
      struct RaycastHit
      {
        Math::Vector3 position;
      };

      LOW_CORE_API bool raycast(Math::Vector3 p_Origin,
                                Math::Vector3 p_Direction, float p_MaxDistance,
                                RaycastHit &p_Hit);

    } // namespace Physics
  }   // namespace Core
} // namespace Low
