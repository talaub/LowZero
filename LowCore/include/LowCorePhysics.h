#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"
#include "LowUtilHandle.h"

#include <cstdint>

namespace Low {
  namespace Core {
    namespace Physics {
      LOW_ENUM(scripting, bind_name = "HitObjectFamily",
               bind_namespace = "Physics")
      enum class HitObjectFamily
      {
        None,
        Body,
        Rigidbody,
        StaticCollider,
        CharacterController,
        Unknown
      };

      LOW_STRUCT(scripting, bind_name = "QueryHit",
                 bind_namespace = "Physics")
      struct QueryHit
      {
        LOW_FIELD()
        Math::Vector3 position = Math::Vector3(0.0f);

        LOW_FIELD()
        Math::Vector3 normal = Math::Vector3(0.0f);

        LOW_FIELD()
        float fraction = 0.0f;

        LOW_FIELD()
        float distance = 0.0f;

        LOW_FIELD()
        Low::Util::Handle body;

        LOW_FIELD()
        Low::Util::Handle owner;

        LOW_FIELD()
        HitObjectFamily family = HitObjectFamily::None;
      };

      LOW_FUNCTION(scripting, bind_name = "raycast",
                   bind_namespace = "Physics")
      bool raycast(const Math::Vector3 &p_Origin,
                   const Math::Vector3 &p_Direction,
                   float p_MaxDistance,
                   LOW_PARAM(out) QueryHit *p_Hit);

    } // namespace Physics
  } // namespace Core
} // namespace Low

#include "LowCorePhysics.gen.h"
