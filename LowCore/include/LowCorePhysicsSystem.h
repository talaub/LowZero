#pragma once

#include "LowCoreApi.h"

#include "LowUtilEnums.h"
#include "LowUtilContainers.h"

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

    namespace Physics {
      struct OverlapResult;

      void get_overlapping_mesh_with_box(
          physx::PxScene *p_Scene, const Math::Vector3 &p_HalfExtents,
          const Math::Vector3 &p_Position,
          const Math::Quaternion &p_Orientation, uint32_t p_CollisionMask,
          Util::List<Math::Vector3> &p_Vertices, uint32_t p_SceneQueryFlags = 0,
          uint8_t p_MaxMeshOverlaps = 4u,
          uint16_t p_MaxTriangleVertices = 1023u, bool p_IgnoreTriggers = true);

      bool overlap_box(physx::PxScene *p_Scene,
                       const Math::Vector3 &p_HalfExtents,
                       const Math::Vector3 &p_Position,
                       const Math::Quaternion &p_Orientation,
                       Util::List<OverlapResult> &p_Touches,
                       uint32_t p_CollisionMask, uint32_t p_SceneQueryFlags = 0,
                       uint8_t p_MaxResults = 4u, bool p_IgnoreTriggers = true);

      physx::PxScene *get_scene();
    }; // namespace Physics
  }    // namespace Core
} // namespace Low
