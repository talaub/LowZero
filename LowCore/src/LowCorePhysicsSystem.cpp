#include "LowCorePhysicsSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreSystem.h"
#include "LowCorePhysicsObjects.h"
#include "LowCoreRigidbody.h"
#include "LowCoreTransform.h"
#include "LowCoreDebugGeometry.h"
#include "LowCorePhysics.h"

#define PHYSICS_VISUALIZATION false
#define PHYSICS_GRAVITY 9.81f

using namespace physx;

namespace Low {
  namespace Core {
    struct PErrorCallback : public PxErrorCallback
    {
      void reportError(PxErrorCode::Enum code, const char *message,
                       const char *file, int line) override
      {
        LOW_LOG_ERROR << message << LOW_LOG_END;
      }
    };

    PxPhysics *g_Base = 0;
    PxDefaultErrorCallback g_ErrorCallback;
    PErrorCallback g_CustomErrorCallback;
    PxDefaultAllocator g_Allocator;
    PxFoundation *g_Foundation = 0;
    PxDefaultCpuDispatcher *g_Dispatcher = 0;
    PxTolerancesScale g_ToleranceScale;

    PxMaterial *g_DefaultMaterial;

    PxScene *g_Scene;

    namespace System {
      namespace Physics {

        void initialize()
        {
          g_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_Allocator,
                                            g_CustomErrorCallback);
          LOW_ASSERT(g_Foundation, "Could not create physx foundation");

          g_ToleranceScale.length = 100; // typical length of an object
          g_ToleranceScale.speed = 981;  // typical speed of an object,
                                         // gravity*1s is a reasonable choice

          g_Base = PxCreatePhysics(PX_PHYSICS_VERSION, *g_Foundation,
                                   g_ToleranceScale);
          LOW_ASSERT(g_Base, "Could not create physx base");

          g_Dispatcher = PxDefaultCpuDispatcherCreate(2);
          LOW_ASSERT(g_Dispatcher, "Could not create physx dispachter");

          g_DefaultMaterial = g_Base->createMaterial(0.5f, 0.5f, 0.5f);
          LOW_ASSERT(g_DefaultMaterial, "Could not create physx material");

          PxSceneDesc l_Desc(g_Base->getTolerancesScale());
          l_Desc.gravity = PxVec3(0.0f, -PHYSICS_GRAVITY, 0.0f);
          l_Desc.cpuDispatcher = g_Dispatcher;
          l_Desc.filterShader = PxDefaultSimulationFilterShader;

          g_Scene = g_Base->createScene(l_Desc);

          if (PHYSICS_VISUALIZATION) {
            g_Scene->setVisualizationParameter(PxVisualizationParameter::eSCALE,
                                               1.0f);
            /*
            g_Scene->setVisualizationParameter(
                PxVisualizationParameter::eCOLLISION_AABBS, 2.0f);
            g_Scene->setVisualizationParameter(
                PxVisualizationParameter::eWORLD_AXES, 2.0f);
            */
            g_Scene->setVisualizationParameter(
                PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);
          }
        }

        static float to_float(uint8_t p_Value)
        {
          return ((float)p_Value) / 255.0f;
        }

        static Math::Color get_color_for_physx_color(physx::PxU32 p_Color)
        {
          using namespace physx;

          uint8_t *l_Channels = (uint8_t *)&p_Color;
          return Math::Color(to_float(l_Channels[0]), to_float(l_Channels[1]),
                             to_float(l_Channels[2]), to_float(l_Channels[3]));
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          for (auto it = Component::Rigidbody::ms_LivingInstances.begin();
               it != Component::Rigidbody::ms_LivingInstances.end(); ++it) {
            Component::Transform i_Transform = it->get_entity().get_transform();

            it->get_rigid_dynamic().update_transform(
                i_Transform.get_world_position(),
                i_Transform.get_world_rotation());
          }
        }

        void late_tick(float p_Delta, Util::EngineState p_State)
        {
          if (p_State != Util::EngineState::PLAYING) {
            return;
          }
          g_Scene->simulate(p_Delta);
          g_Scene->fetchResults(true);

          if (PHYSICS_VISUALIZATION) {
            const PxRenderBuffer &rb = g_Scene->getRenderBuffer();
            for (PxU32 i = 0; i < rb.getNbLines(); i++) {
              const PxDebugLine &line = rb.getLines()[i];
              Math::Box l_Box;
              l_Box.position =
                  Math::Vector3(line.pos0.x, line.pos0.y, line.pos0.z);
              l_Box.rotation = Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
              l_Box.halfExtents = Math::Vector3(0.05f);
              DebugGeometry::render_box(
                  l_Box, get_color_for_physx_color(line.color0), true, false);
            }
          }

          for (auto it = Component::Rigidbody::ms_LivingInstances.begin();
               it != Component::Rigidbody::ms_LivingInstances.end(); ++it) {
            if (it->is_fixed()) {
              // continue;
            }

            Component::Transform i_Transform = it->get_entity().get_transform();
            // TODO: Hack for top level - does not work for transform hierarchy
            i_Transform.position(it->get_rigid_dynamic().get_position());
            i_Transform.rotation(it->get_rigid_dynamic().get_rotation());
          }
        }

        void register_rigid_dynamic(PhysicsRigidDynamic &p_RigidDynamic)
        {
          g_Scene->addActor(*p_RigidDynamic.m_RigidDynamic);
        }

        void remove_rigid_dynamic(PhysicsRigidDynamic &p_RigidDynamic)
        {
          g_Scene->removeActor(*p_RigidDynamic.m_RigidDynamic);
        }

      } // namespace Physics
    }   // namespace System

    static PxShape *generate_pxshape(Math::Shape &p_Shape)
    {

      if (p_Shape.type == Math::ShapeType::BOX) {
        Math::Box &l_Box = p_Shape.box;
        return g_Base->createShape(PxBoxGeometry(l_Box.halfExtents.x * 2.0f,
                                                 l_Box.halfExtents.y * 2.0f,
                                                 l_Box.halfExtents.z * 2.0f),
                                   *g_DefaultMaterial);
      } else {
        LOW_ASSERT(false, "Physics shape type not supported");
      }

      return 0;
    }

    void PhysicsShape::create(PhysicsShape &p_PhysicsShape,
                              Math::Shape &p_Shape)
    {
      p_PhysicsShape.m_Shape = generate_pxshape(p_Shape);

      p_PhysicsShape.m_Shape->setFlag(PxShapeFlag::eVISUALIZATION,
                                      PHYSICS_VISUALIZATION);
    }

    void PhysicsShape::update(Math::Shape &p_Shape)
    {
      PxShapeFlags l_ShapeFlags = m_Shape->getFlags();

      m_Shape->release();

      m_Shape = generate_pxshape(p_Shape);
    }

    void PhysicsShape::destroy()
    {
      m_Shape->release();
    }

    void PhysicsRigidDynamic::destroy()
    {
      m_RigidDynamic->release();
    }

    void PhysicsRigidDynamic::create(PhysicsRigidDynamic &p_RigidDynamic,
                                     PhysicsShape &p_Shape,
                                     Math::Vector3 p_Position,
                                     Math::Quaternion p_Rotation)
    {
      PxQuat l_Quat;
      l_Quat.x = p_Rotation.x;
      l_Quat.y = p_Rotation.y;
      l_Quat.z = p_Rotation.z;
      l_Quat.w = p_Rotation.w;

      p_RigidDynamic.m_RigidDynamic = PxCreateDynamic(
          *g_Base,
          PxTransform(PxVec3(p_Position.x, p_Position.y, p_Position.z), l_Quat),
          *(p_Shape.m_Shape), 1.0f);

      p_RigidDynamic.m_RigidDynamic->setActorFlag(PxActorFlag::eVISUALIZATION,
                                                  PHYSICS_VISUALIZATION);

      p_RigidDynamic.m_CurrentShape = p_Shape.m_Shape;
    }

    void PhysicsRigidDynamic::set_mass(float p_Mass)
    {
      m_RigidDynamic->setMass(p_Mass);
      m_RigidDynamic->wakeUp();
    }
    void PhysicsRigidDynamic::set_gravity(bool p_Gravity)
    {
      m_RigidDynamic->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !p_Gravity);
      m_RigidDynamic->wakeUp();
    }
    void PhysicsRigidDynamic::set_fixed(bool p_Fixed)
    {
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_LINEAR_X, p_Fixed);
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, p_Fixed);
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, p_Fixed);
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, p_Fixed);
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, p_Fixed);
      m_RigidDynamic->setRigidDynamicLockFlag(
          PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, p_Fixed);
      m_RigidDynamic->wakeUp();
    }

    Math::Vector3 PhysicsRigidDynamic::get_position() const
    {
      physx::PxTransform l_Transform = m_RigidDynamic->getGlobalPose();

      return Math::Vector3(l_Transform.p.x, l_Transform.p.y, l_Transform.p.z);
    }

    Math::Quaternion PhysicsRigidDynamic::get_rotation() const
    {
      physx::PxTransform l_Transform =
          m_RigidDynamic->getGlobalPose().getNormalized();

      Math::Quaternion l_Quat;
      l_Quat.w = l_Transform.q.w;
      l_Quat.x = l_Transform.q.x;
      l_Quat.y = l_Transform.q.y;
      l_Quat.z = l_Transform.q.z;

      return l_Quat;
    }

    void PhysicsRigidDynamic::update_transform(Math::Vector3 &p_Position,
                                               Math::Quaternion &p_Rotation)
    {
      PxVec3 l_Pos(p_Position.x, p_Position.y, p_Position.z);
      PxQuat l_Rot(p_Rotation.x, p_Rotation.y, p_Rotation.z, p_Rotation.w);

      PxTransform l_Transform(l_Pos, l_Rot);

      m_RigidDynamic->setGlobalPose(l_Transform);
      m_RigidDynamic->wakeUp();
    }

    void PhysicsRigidDynamic::update_shape(PhysicsShape &p_PhysicsShape)
    {
      m_RigidDynamic->detachShape(*m_CurrentShape);
      m_RigidDynamic->attachShape(*p_PhysicsShape.m_Shape);
      m_CurrentShape = p_PhysicsShape.m_Shape;
    }

    namespace Physics {
      bool raycast(Math::Vector3 p_Origin, Math::Vector3 p_Direction,
                   float p_MaxDistance, RaycastHit &p_Hit)
      {
        PxVec3 l_Origin;
        l_Origin.x = p_Origin.x;
        l_Origin.y = p_Origin.y;
        l_Origin.z = p_Origin.z;
        PxVec3 l_Direction;
        l_Direction.x = p_Direction.x;
        l_Direction.y = p_Direction.y;
        l_Direction.z = p_Direction.z;
        PxReal l_MaxDistance = p_MaxDistance;
        PxRaycastBuffer l_Hit;

        bool l_Status =
            g_Scene->raycast(l_Origin, l_Direction, l_MaxDistance, l_Hit);

        p_Hit.position.x = l_Hit.block.position.x;
        p_Hit.position.y = l_Hit.block.position.y;
        p_Hit.position.z = l_Hit.block.position.z;

        return l_Status;
      }
    } // namespace Physics
  }   // namespace Core
} // namespace Low
