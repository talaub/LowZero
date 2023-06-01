#pragma once

#include "LowCoreApi.h"

#include "LowUtilEnums.h"

#include "LowCorePhysicsObjects.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Physics {
        void initialize();
        void tick(float p_Delta, Util::EngineState p_State);
        void late_tick(float p_Delta, Util::EngineState p_State);

        void register_rigid_dynamic(PhysicsRigidDynamic &p_RigidDynamic);
        void remove_rigid_dynamic(PhysicsRigidDynamic &p_RigidDynamic);
      } // namespace Physics
    }   // namespace System
  }     // namespace Core
} // namespace Low
