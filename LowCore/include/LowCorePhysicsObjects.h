#pragma once

#include "LowCoreApi.h"

#include "physx/include/PxPhysics.h"
#include "physx/include/PxPhysicsAPI.h"

#include "LowMath.h"

#define LOW_PHYSICS_MAX_RESULT_BUFFER_SIZE 1023

namespace Low {
  namespace Core {
    struct PhysicsShape;
    struct PhysicsRigidDynamic;

    namespace System {
      namespace Physics {
        void register_rigid_dynamic(PhysicsRigidDynamic &);
        void remove_rigid_dynamic(PhysicsRigidDynamic &);
      } // namespace Physics
    }   // namespace System

    struct LOW_CORE_API PhysicsRigidDynamic
    {
      friend void
      System::Physics::register_rigid_dynamic(PhysicsRigidDynamic &);
      friend void System::Physics::remove_rigid_dynamic(PhysicsRigidDynamic &);

      static void create(PhysicsRigidDynamic &p_RigidDynamic,
                         PhysicsShape &p_Shape, Math::Vector3 p_Position,
                         Math::Quaternion p_Rotation, void *p_UserData);

      void set_mass(float p_Mass);
      void set_gravity(bool p_Gravity);
      void set_fixed(bool p_Fixed);

      Math::Vector3 get_position() const;
      Math::Quaternion get_rotation() const;

      void update_transform(Math::Vector3 &p_Position,
                            Math::Quaternion &p_Rotation);

      void update_shape(PhysicsShape &p_PhysicsShape);
      void destroy();

      const physx::PxShape *get_physx_shape() const;
      physx::PxTransform get_physx_transform() const;

    private:
      physx::PxRigidDynamic *m_RigidDynamic;
      physx::PxShape *m_CurrentShape;
    };

    struct PhysicsShape
    {
      friend PhysicsRigidDynamic;

      static void create(PhysicsShape &p_PhysicsShape, Math::Shape &p_Shape);

      void update(Math::Shape &p_Shape);
      void destroy();

    private:
      physx::PxShape *m_Shape;
    };

    namespace Physics {
      struct OverlapResult
      {
        physx::PxShape *shape;
        physx::PxRigidActor *actor;
      };
    } // namespace Physics

  } // namespace Core
} // namespace Low
