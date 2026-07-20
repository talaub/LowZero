#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"
#include "LowUtilHandle.h"

#include <cstdint>

namespace Low {
  namespace Core {
    namespace Physics {
      enum class HitObjectFamily
      {
        NONE,
        BODY,
        RIGIDBODY,
        STATIC_COLLIDER,
        CHARACTER_CONTROLLER,
        UNKNOWN
      };

      struct QueryHit
      {
        Math::Vector3 position = Math::Vector3(0.0f);
        Math::Vector3 normal = Math::Vector3(0.0f);
        float fraction = 0.0f;
        float distance = 0.0f;
        Low::Util::Handle body;
        Low::Util::Handle owner;
        HitObjectFamily family = HitObjectFamily::NONE;
      };

    } // namespace Physics
  }   // namespace Core
} // namespace Low
