#include "LowCorePhysicsSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilMemory.h"

#include "LowCoreSystem.h"
#include "LowCorePhysicsObjects.h"
#include "LowCoreRigidbody.h"
#include "LowCoreTransform.h"
#include "LowCoreDebugGeometry.h"
#include "LowCorePhysics.h"

#include "physx/include/extensions/PxTriangleMeshExt.h"

#define PHYSICS_VISUALIZATION false
#define PHYSICS_GRAVITY 9.81f

#define EPSILON 0.000f
#define RESULT_BUFFER_SIZE 128u

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
          LOW_PROFILE_CPU("Core", "PhysicsSystem::TICK");
          for (auto it = Component::Rigidbody::ms_LivingInstances.begin();
               it != Component::Rigidbody::ms_LivingInstances.end(); ++it) {
            Component::Transform i_Transform = it->get_entity().get_transform();

            Math::Matrix4x4 i_LocalMatrix(1.0f);
            i_LocalMatrix =
                glm::translate(i_LocalMatrix, it->get_shape().box.position);
            i_LocalMatrix *= glm::toMat4(it->get_shape().box.rotation);

            Math::Matrix4x4 i_WorldMatrix =
                i_Transform.get_world_matrix() * i_LocalMatrix;

            Math::Vector3 i_WorldScale;
            Math::Quaternion i_WorldRotation;
            Math::Vector3 i_WorldPosition;
            Math::Vector3 i_WorldSkew;
            Math::Vector4 i_WorldPerspective;

            glm::decompose(i_WorldMatrix, i_WorldScale, i_WorldRotation,
                           i_WorldPosition, i_WorldSkew, i_WorldPerspective);

            it->get_rigid_dynamic().update_transform(i_WorldPosition,
                                                     i_WorldRotation);
          }
        }

        void late_tick(float p_Delta, Util::EngineState p_State)
        {
          if (p_State != Util::EngineState::PLAYING) {
            return;
          }
          LOW_PROFILE_CPU("Core", "PhysicsSystem::LATETICK");
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

          // Writeback disabled for now because we just don't need it
          /*
                for (auto it = Component::Rigidbody::ms_LivingInstances.begin();
                     it != Component::Rigidbody::ms_LivingInstances.end(); ++it)
             { if (it->is_fixed()) {
                    // continue;
                  }

                  Component::Transform i_Transform =
             it->get_entity().get_transform();
                  // TODO: Hack for top level - does not work for transform
             hierarchy
                  i_Transform.position(it->get_rigid_dynamic().get_position());
                  i_Transform.rotation(it->get_rigid_dynamic().get_rotation());
                }
          */
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
                                     Math::Quaternion p_Rotation,
                                     void *p_UserData)
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

      p_RigidDynamic.m_RigidDynamic->userData = p_UserData;
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

    physx::PxTransform PhysicsRigidDynamic::get_physx_transform() const
    {
      return m_RigidDynamic->getGlobalPose();
    }

    const physx::PxShape *PhysicsRigidDynamic::get_physx_shape() const
    {
      return m_CurrentShape;
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
      namespace Helper {
        using namespace physx;
        inline PxVec3 convert(const Math::Vector3 &p_Vector)
        {
          return PxVec3(p_Vector.x, p_Vector.y, p_Vector.z);
        }
        inline Math::Vector3 convert(const PxVec3 &p_Vec)
        {
          return Math::Vector3(p_Vec.x, p_Vec.y, p_Vec.z);
        }
        inline Math::Vector3 convert(const PxExtendedVec3 &p_Vec)
        {
          return Math::Vector3(p_Vec.x, p_Vec.y,
                               p_Vec.z); // WARNING: Loss of precision
        }
        inline PxVec4 convert(const Math::Vector4 &p_Vector)
        {
          return PxVec4(p_Vector.x, p_Vector.y, p_Vector.z, p_Vector.w);
        }
        inline Math::Vector4 convert(const PxVec4 &p_Vec)
        {
          return Math::Vector4(p_Vec.x, p_Vec.y, p_Vec.z, p_Vec.w);
        }
        inline PxQuat convert(const Math::Quaternion &p_Quat)
        {
          PxQuat quat(p_Quat.x, p_Quat.y, p_Quat.z, p_Quat.w);
          quat.normalize();
          return quat;
        }
        inline Math::Quaternion convert(const PxQuat &p_Quat)
        {
          return Math::Quaternion(p_Quat.w, p_Quat.x, p_Quat.y, p_Quat.z);
        }
        inline void get_hit_info(const PxOverlapHit &p_Hit,
                                 OverlapResult &p_Result)
        {
          p_Result.shape = p_Hit.shape;
          p_Result.actor = p_Hit.actor;
        }

      } // namespace Helper

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

      inline bool get_overlapping_tringle_mesh(
          const PxGeometry &p_ShapeGeometry, const PxTransform &p_ShapePose,
          const PxTriangleMeshGeometry &p_MeshGeometry,
          const PxTransform &p_MeshPose, Util::List<Math::Vector3> &p_Vertices,
          uint16_t p_MaxTriangleVertices)
      {
        PxMeshOverlapUtil l_OverlapUtil;
        const uint32_t l_TriangleCount = l_OverlapUtil.findOverlap(
            p_ShapeGeometry, p_ShapePose, p_MeshGeometry, p_MeshPose);

        const uint32_t *__restrict l_Indices = l_OverlapUtil.getResults();

        p_Vertices.reserve(p_Vertices.size() + l_TriangleCount * 3);
        for (uint32_t i = 0; i < l_TriangleCount; ++i) {
          uint32_t i_TriangleId = l_Indices[i];

          LOW_ASSERT(i_TriangleId <
                         p_MeshGeometry.triangleMesh->getNbTriangles(),
                     "Invalid triangle ID");

          PxTriangle i_Triangle;
          PxMeshQuery::getTriangle(p_MeshGeometry, p_MeshPose, i_TriangleId,
                                   i_Triangle);

          for (uint8_t j = 0; j < 3u; ++j) {
            p_Vertices.push_back(Helper::convert(i_Triangle.verts[j]));
          }
        }

        return false;
      }

      inline void get_overlapping_heightfield(
          const PxGeometry &p_ShapeGeometry, const PxTransform &p_ShapePose,
          const PxHeightFieldGeometry &p_HeightFieldGeometry,
          const PxTransform &p_MeshPose, Util::List<Math::Vector3> &p_Vertices,
          uint16_t p_MaxTriangleVertices)
      {
        PxMeshOverlapUtil l_OverlapUtil;
        const uint32_t l_TriangleCount = l_OverlapUtil.findOverlap(
            p_ShapeGeometry, p_ShapePose, p_HeightFieldGeometry, p_MeshPose);

        const uint32_t *__restrict l_Indices = l_OverlapUtil.getResults();

        p_Vertices.reserve(p_Vertices.size() + l_TriangleCount * 3);
        for (uint32_t i = 0; i < l_TriangleCount; ++i) {
          uint32_t i_TriangleId = l_Indices[i];

          LOW_ASSERT(i_TriangleId <
                         p_HeightFieldGeometry.heightField->getNbRows() *
                             p_HeightFieldGeometry.heightField->getNbColumns() *
                             2u,
                     "Invalid triangle ID");

          PxTriangle i_Triangle;
          PxMeshQuery::getTriangle(p_HeightFieldGeometry, p_MeshPose,
                                   i_TriangleId, i_Triangle);

          for (uint8_t j = 0; j < 3u; ++j) {
            p_Vertices.push_back(Helper::convert(i_Triangle.verts[j]));
          }
        }
      }

      bool overlap_box(
          physx::PxScene *p_Scene, const Math::Vector3 &p_HalfExtents,
          const Math::Vector3 &p_Position,
          const Math::Quaternion &p_Orientation,
          Util::List<OverlapResult> &p_Touches, uint32_t p_CollisionMask,
          uint32_t p_SceneQueryFlags /*= SceneQueryFlags::Default*/,
          uint8_t p_MaxResults /*= 4u*/, bool p_IgnoreTriggers /*= true*/)
      {
        LOW_ASSERT(p_MaxResults <= RESULT_BUFFER_SIZE,
                   "Max results exceeds static buffer size");

        if (p_HalfExtents.x < EPSILON || p_HalfExtents.y < EPSILON ||
            p_HalfExtents.z < EPSILON) {
          LOW_LOG_WARN << "Half extents must be positive" << LOW_LOG_END;
        }

        bool l_Hit = false;
        PxScene *l_Scene = g_Scene;
        // SceneReadLock l_Lock(l_Scene);

        PxQueryFlags l_Flags;
        // l_Flags |= PxQueryFlag::eSTATIC;
        l_Flags |= PxQueryFlag::eDYNAMIC;

        // l_Flags |= PxQueryFlag::eANY_HIT;
        if (p_IgnoreTriggers) {
          // l_Flags |= PxQueryFlag::ePREFILTER;
        }
        l_Flags |= PxQueryFlag::eNO_BLOCK;

        PxQueryFilterData l_FilterData(l_Flags);

        PxQueryFilterCallback *l_FilterCallback = nullptr;

        uint32_t l_MemSize = sizeof(PxOverlapHit) * RESULT_BUFFER_SIZE;

        // TODO: Replace with scratch allocator!!
        void *l_Memory = Util::Memory::main_allocator()->allocate(l_MemSize);
        PxOverlapHit *l_Buffer = static_cast<PxOverlapHit *>(l_Memory);

        PxOverlapBuffer l_HitBuffer;
        l_HitBuffer = PxOverlapBuffer(l_Buffer, p_MaxResults);

        PxBoxGeometry l_BoxGeom(Helper::convert(p_HalfExtents));
        PxTransform l_BoxTrans(Helper::convert(p_Position));

        PxQueryFilterCallback *l_Callback(nullptr);

        l_Hit = l_Scene->overlap(PxBoxGeometry(Helper::convert(p_HalfExtents)),
                                 PxTransform(Helper::convert(p_Position),
                                             Helper::convert(p_Orientation)),
                                 l_HitBuffer, l_FilterData, l_FilterCallback);

        if (l_Hit && l_HitBuffer.hasAnyHits()) {
          const uint32_t l_TouchCount = l_HitBuffer.getNbTouches();
          p_Touches.resize(l_TouchCount);
          for (uint32_t i = 0; i < l_TouchCount; ++i) {
            Helper::get_hit_info(l_HitBuffer.getTouch(i), p_Touches[i]);
          }
        }

        // TODO: Remove when replaced with scrap allocator
        Util::Memory::main_allocator()->deallocate(l_Memory);

        return l_Hit;
      }

      void get_overlapping_mesh_with_box(
          PxScene *p_Scene, const Math::Vector3 &p_HalfExtents,
          const Math::Vector3 &p_Position,
          const Math::Quaternion &p_Orientation, uint32_t p_CollisionMask,
          Util::List<Math::Vector3> &p_Vertices,
          uint32_t p_SceneQueryFlags /*= SceneQueryFlags::Static*/,
          uint8_t p_MaxMeshOverlaps /*= 4u*/,
          uint16_t p_MaxTriangleVertices /*= 1023u*/,
          bool p_IgnoreTriggers /*= true*/)
      {
        _LOW_ASSERT(p_MaxTriangleVertices % 3 == 0);

        PxScene *l_Scene = g_Scene;

        Util::List<OverlapResult> l_OverlappingActors;
        overlap_box(l_Scene, p_HalfExtents, p_Position, p_Orientation,
                    l_OverlappingActors, p_CollisionMask, p_SceneQueryFlags,
                    p_MaxMeshOverlaps, p_IgnoreTriggers);

        for (const auto &i_Overlap : l_OverlappingActors) {
          if (p_Vertices.size() < p_MaxTriangleVertices && i_Overlap.shape &&
              i_Overlap.shape->getConcreteType() == PxConcreteType::eSHAPE &&
              i_Overlap.actor &&
              (i_Overlap.actor->getConcreteType() ==
                   PxConcreteType::eRIGID_STATIC ||
               i_Overlap.actor->getConcreteType() ==
                   PxConcreteType::eRIGID_DYNAMIC)) {
            if (i_Overlap.shape->getGeometryType() ==
                PxGeometryType::eTRIANGLEMESH) {
              PxTriangleMeshGeometry triangleMesh;
              i_Overlap.shape->getTriangleMeshGeometry(triangleMesh);

              get_overlapping_tringle_mesh(
                  PxBoxGeometry(Helper::convert(p_HalfExtents)),
                  PxTransform(Helper::convert(p_Position),
                              Helper::convert(p_Orientation)),
                  triangleMesh, i_Overlap.actor->getGlobalPose(), p_Vertices,
                  p_MaxTriangleVertices - p_Vertices.size());
            } else if (i_Overlap.shape->getGeometryType() ==
                       PxGeometryType::eHEIGHTFIELD) {
              PxHeightFieldGeometry i_HeightField;
              i_Overlap.shape->getHeightFieldGeometry(i_HeightField);

              get_overlapping_heightfield(
                  PxBoxGeometry(Helper::convert(p_HalfExtents)),
                  PxTransform(Helper::convert(p_Position),
                              Helper::convert(p_Orientation)),
                  i_HeightField, i_Overlap.actor->getGlobalPose(), p_Vertices,
                  p_MaxTriangleVertices - p_Vertices.size());
            } else {
              // Not a triangle mesh or heightfield, continue...
              continue;
            }
          }

          if (p_Vertices.size() == p_MaxTriangleVertices) {
            LOW_LOG_WARN << "Maximum number of triangles retrieved with an "
                            "overlap query ("
                         << p_MaxTriangleVertices
                         << "). Some might have been skipped." << LOW_LOG_END;
          }
        }
      }

      physx::PxScene *get_scene()
      {
        return g_Scene;
      }
    } // namespace Physics
  }   // namespace Core
} // namespace Low
