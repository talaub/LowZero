#include "LowCorePhysicsBackend.h"

#include "LowCoreDebugGeometry.h"
#include "LowUtilAssert.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

JPH_SUPPRESS_WARNINGS

namespace {
  namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
  } // namespace Layers

  namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
  } // namespace BroadPhaseLayers

  class ObjectLayerPairFilter final
      : public JPH::ObjectLayerPairFilter
  {
  public:
    bool ShouldCollide(JPH::ObjectLayer p_Object1,
                       JPH::ObjectLayer p_Object2) const override
    {
      if (p_Object1 == Layers::NON_MOVING) {
        return p_Object2 == Layers::MOVING;
      }
      if (p_Object1 == Layers::MOVING) {
        return true;
      }
      LOW_ASSERT(false, "Invalid Jolt object layer");
      return false;
    }
  };

  class BroadPhaseLayerInterface final
      : public JPH::BroadPhaseLayerInterface
  {
  public:
    BroadPhaseLayerInterface()
    {
      m_ObjectToBroadPhase[Layers::NON_MOVING] =
          BroadPhaseLayers::NON_MOVING;
      m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    JPH::uint GetNumBroadPhaseLayers() const override
    {
      return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer
    GetBroadPhaseLayer(JPH::ObjectLayer p_Layer) const override
    {
      LOW_ASSERT(p_Layer < Layers::NUM_LAYERS,
                 "Invalid Jolt object layer");
      return m_ObjectToBroadPhase[p_Layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(
        JPH::BroadPhaseLayer p_Layer) const override
    {
      if (p_Layer == BroadPhaseLayers::NON_MOVING) {
        return "NON_MOVING";
      }
      if (p_Layer == BroadPhaseLayers::MOVING) {
        return "MOVING";
      }
      return "INVALID";
    }
#endif

  private:
    JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
  };

  class ObjectVsBroadPhaseLayerFilter final
      : public JPH::ObjectVsBroadPhaseLayerFilter
  {
  public:
    bool ShouldCollide(JPH::ObjectLayer p_Layer1,
                       JPH::BroadPhaseLayer p_Layer2) const override
    {
      if (p_Layer1 == Layers::NON_MOVING) {
        return p_Layer2 == BroadPhaseLayers::MOVING;
      }
      if (p_Layer1 == Layers::MOVING) {
        return true;
      }
      LOW_ASSERT(false, "Invalid Jolt object layer");
      return false;
    }
  };

  struct JoltRuntime
  {
    std::mutex mutex;
    uint32_t world_count = 0u;
    bool initialized = false;
  };

  JoltRuntime &runtime()
  {
    static JoltRuntime g_Runtime;
    return g_Runtime;
  }

  void initialize_jolt_runtime()
  {
    JoltRuntime &l_Runtime = runtime();
    std::lock_guard<std::mutex> l_Lock(l_Runtime.mutex);

    if (!l_Runtime.initialized) {
      JPH::RegisterDefaultAllocator();
      JPH::Factory::sInstance = new JPH::Factory();
      JPH::RegisterTypes();
      l_Runtime.initialized = true;
    }
    ++l_Runtime.world_count;
  }

  void shutdown_jolt_runtime()
  {
    JoltRuntime &l_Runtime = runtime();
    std::lock_guard<std::mutex> l_Lock(l_Runtime.mutex);

    LOW_ASSERT(l_Runtime.world_count > 0u,
               "Jolt runtime shutdown without active world");
    --l_Runtime.world_count;

    if (l_Runtime.world_count == 0u && l_Runtime.initialized) {
      JPH::UnregisterTypes();
      delete JPH::Factory::sInstance;
      JPH::Factory::sInstance = nullptr;
      l_Runtime.initialized = false;
    }
  }

  JPH::Vec3 to_jolt(const Low::Math::Vector3 &p_Value)
  {
    return JPH::Vec3(p_Value.x, p_Value.y, p_Value.z);
  }

  JPH::RVec3 to_jolt_position(const Low::Math::Vector3 &p_Value)
  {
    return JPH::RVec3(p_Value.x, p_Value.y, p_Value.z);
  }

  JPH::Quat to_jolt(const Low::Math::Quaternion &p_Value)
  {
    return JPH::Quat(p_Value.x, p_Value.y, p_Value.z, p_Value.w);
  }

  Low::Math::Vector3 from_jolt(JPH::Vec3Arg p_Value)
  {
    return Low::Math::Vector3(p_Value.GetX(), p_Value.GetY(),
                              p_Value.GetZ());
  }

  template <typename VectorType>
  Low::Math::Vector3 from_jolt_position(const VectorType &p_Value)
  {
    return Low::Math::Vector3(static_cast<float>(p_Value.GetX()),
                              static_cast<float>(p_Value.GetY()),
                              static_cast<float>(p_Value.GetZ()));
  }

  Low::Math::Quaternion from_jolt(JPH::QuatArg p_Value)
  {
    return Low::Math::Quaternion(p_Value.GetW(), p_Value.GetX(),
                                 p_Value.GetY(), p_Value.GetZ());
  }

  JPH::EMotionType
  to_jolt(Low::Core::Physics::BodyMotionType p_MotionType)
  {
    switch (p_MotionType) {
    case Low::Core::Physics::BodyMotionType::STATIC:
      return JPH::EMotionType::Static;
    case Low::Core::Physics::BodyMotionType::KINEMATIC:
      return JPH::EMotionType::Kinematic;
    case Low::Core::Physics::BodyMotionType::DYNAMIC:
      return JPH::EMotionType::Dynamic;
    }

    LOW_ASSERT(false, "Invalid physics body motion type");
    return JPH::EMotionType::Static;
  }

  JPH::ObjectLayer get_layer_for_motion_type(
      Low::Core::Physics::BodyMotionType p_MotionType)
  {
    return p_MotionType == Low::Core::Physics::BodyMotionType::STATIC
               ? Layers::NON_MOVING
               : Layers::MOVING;
  }
} // namespace

namespace Low {
  namespace Core {
    namespace Physics {
      struct WorldBackend
      {
        struct CapsuleControllerBackend
        {
          JPH::Ref<JPH::CharacterVirtual> controller;
          float center_offset = 0.0f;
          float step_offset = 0.0f;
          float skin_width = 0.0f;
        };

        BroadPhaseLayerInterface broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilter
            object_vs_broadphase_layer_filter;
        ObjectLayerPairFilter object_vs_object_layer_filter;
        JPH::PhysicsSystem physics_system;
        JPH::TempAllocatorImpl temp_allocator;
        JPH::JobSystemThreadPool job_system;
        std::unordered_map<uint64_t, JPH::ShapeRefC> shapes;
        std::unordered_map<uint64_t, JPH::BodyID> bodies;
        std::unordered_map<uint64_t, CapsuleControllerBackend>
            capsule_controllers;
        uint64_t next_shape_id = 1u;
        uint64_t next_body_id = 1u;
        uint64_t next_capsule_controller_id = 1u;

        WorldBackend()
            : temp_allocator(10u * 1024u * 1024u),
              job_system(
                  JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                  std::max(1u, std::thread::hardware_concurrency()) -
                      1u)
        {
          const JPH::uint l_MaxBodies = 65536u;
          const JPH::uint l_NumBodyMutexes = 0u;
          const JPH::uint l_MaxBodyPairs = 65536u;
          const JPH::uint l_MaxContactConstraints = 10240u;

          physics_system.Init(l_MaxBodies, l_NumBodyMutexes,
                              l_MaxBodyPairs, l_MaxContactConstraints,
                              broad_phase_layer_interface,
                              object_vs_broadphase_layer_filter,
                              object_vs_object_layer_filter);
        }
      };

      static uint64_t get_backend_body_id(WorldBackend *p_World,
                                          const JPH::BodyID &p_BodyId)
      {
        for (const auto &i_Body : p_World->bodies) {
          if (i_Body.second == p_BodyId) {
            return i_Body.first;
          }
        }

        return 0u;
      }

      static void fill_body_query_hit(WorldBackend *p_World,
                                      const JPH::BodyID &p_BodyId,
                                      BackendQueryHit &p_Hit)
      {
        p_Hit.body_backend_id =
            get_backend_body_id(p_World, p_BodyId);
        p_Hit.user_data = nullptr;

        JPH::BodyLockRead l_Lock(
            p_World->physics_system.GetBodyLockInterface(), p_BodyId);
        if (l_Lock.Succeeded()) {
          p_Hit.user_data = reinterpret_cast<void *>(
              l_Lock.GetBody().GetUserData());
        }
      }

      static JPH::ShapeRefC
      create_sphere_query_shape(const float p_Radius)
      {
        return new JPH::SphereShape(std::max(p_Radius, 0.001f));
      }

      static JPH::ShapeRefC
      create_box_query_shape(const Math::Vector3 &p_HalfExtents)
      {
        const JPH::Vec3 l_HalfExtents =
            to_jolt(glm::max(p_HalfExtents, Math::Vector3(0.001f)));
        const float l_MinHalfExtent =
            std::min({l_HalfExtents.GetX(), l_HalfExtents.GetY(),
                      l_HalfExtents.GetZ()});
        const float l_ConvexRadius =
            std::min(l_MinHalfExtent * 0.5f, 0.05f);
        return new JPH::BoxShape(l_HalfExtents, l_ConvexRadius);
      }

      static bool fill_raycast_hit(WorldBackend *p_World,
                                   const JPH::RRayCast &p_Ray,
                                   const JPH::RayCastResult &p_Result,
                                   const float p_MaxDistance,
                                   BackendQueryHit &p_Hit)
      {
        const JPH::RVec3 l_Position =
            p_Ray.GetPointOnRay(p_Result.mFraction);
        p_Hit.position = from_jolt_position(l_Position);
        p_Hit.fraction = p_Result.mFraction;
        p_Hit.distance = p_MaxDistance * p_Result.mFraction;
        p_Hit.normal = Math::Vector3(0.0f);
        p_Hit.user_data = nullptr;

        JPH::BodyLockRead l_Lock(
            p_World->physics_system.GetBodyLockInterface(),
            p_Result.mBodyID);
        if (l_Lock.Succeeded()) {
          p_Hit.normal =
              from_jolt(l_Lock.GetBody().GetWorldSpaceSurfaceNormal(
                  p_Result.mSubShapeID2, l_Position));
          p_Hit.user_data = reinterpret_cast<void *>(
              l_Lock.GetBody().GetUserData());
        }

        p_Hit.body_backend_id =
            get_backend_body_id(p_World, p_Result.mBodyID);
        return true;
      }

      static bool
      fill_shape_cast_hit(WorldBackend *p_World,
                          const JPH::ShapeCastResult &p_Result,
                          const float p_MaxDistance, BackendQueryHit &p_Hit)
      {
        p_Hit.position = from_jolt(p_Result.mContactPointOn2);
        p_Hit.normal =
            from_jolt(-p_Result.mPenetrationAxis.Normalized());
        p_Hit.fraction = p_Result.mFraction;
        p_Hit.distance = p_MaxDistance * p_Result.mFraction;
        fill_body_query_hit(p_World, p_Result.mBodyID2, p_Hit);
        return true;
      }

      static bool
      fill_overlap_hit(WorldBackend *p_World,
                       const JPH::CollideShapeResult &p_Result,
                       BackendQueryHit &p_Hit)
      {
        p_Hit.position = from_jolt(p_Result.mContactPointOn2);
        p_Hit.normal =
            from_jolt(-p_Result.mPenetrationAxis.Normalized());
        p_Hit.fraction = 0.0f;
        p_Hit.distance = 0.0f;
        fill_body_query_hit(p_World, p_Result.mBodyID2, p_Hit);
        return true;
      }

      static bool cast_query_shape(WorldBackend *p_World,
                                   const JPH::Shape *p_Shape,
                                   const Math::Vector3 &p_Origin,
                                   const Math::Quaternion &p_Rotation,
                                   const Math::Vector3 &p_Direction,
                                   const float p_MaxDistance,
                                   BackendQueryHit &p_Hit)
      {
        LOW_ASSERT(p_World,
                   "Cannot cast shape in null physics world");

        const float l_DirectionLength = glm::length(p_Direction);
        if (l_DirectionLength <= LOW_MATH_EPSILON ||
            p_MaxDistance <= 0.0f) {
          return false;
        }

        const JPH::Vec3 l_Direction = to_jolt(
            (p_Direction / l_DirectionLength) * p_MaxDistance);
        const JPH::RMat44 l_Start = JPH::RMat44::sRotationTranslation(
            to_jolt(p_Rotation), to_jolt_position(p_Origin));
        const JPH::RShapeCast l_Cast =
            JPH::RShapeCast::sFromWorldTransform(
                p_Shape, JPH::Vec3::sReplicate(1.0f), l_Start,
                l_Direction);

        JPH::ShapeCastSettings l_Settings;
        JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector>
            l_Collector;
        p_World->physics_system.GetNarrowPhaseQuery().CastShape(
            l_Cast, l_Settings, JPH::RVec3::sZero(), l_Collector);

        if (!l_Collector.HadHit()) {
          return false;
        }

        return fill_shape_cast_hit(p_World, l_Collector.mHit,
                                   p_MaxDistance, p_Hit);
      }

      static bool overlap_query_shape(
          WorldBackend *p_World, const JPH::Shape *p_Shape,
          const Math::Vector3 &p_Position,
          const Math::Quaternion &p_Rotation, BackendQueryHit *p_Hit)
      {
        LOW_ASSERT(p_World,
                   "Cannot overlap shape in null physics world");

        const JPH::RMat44 l_Transform =
            JPH::RMat44::sRotationTranslation(
                to_jolt(p_Rotation), to_jolt_position(p_Position));

        JPH::CollideShapeSettings l_Settings;
        JPH::ClosestHitCollisionCollector<JPH::CollideShapeCollector>
            l_Collector;
        p_World->physics_system.GetNarrowPhaseQuery().CollideShape(
            p_Shape, JPH::Vec3::sReplicate(1.0f), l_Transform,
            l_Settings, JPH::RVec3::sZero(), l_Collector);

        if (!l_Collector.HadHit()) {
          return false;
        }

        if (p_Hit) {
          fill_overlap_hit(p_World, l_Collector.mHit, *p_Hit);
        }
        return true;
      }

      WorldBackend *create_world_backend()
      {
        initialize_jolt_runtime();
        return new WorldBackend();
      }

      void destroy_world_backend(WorldBackend *p_World)
      {
        if (!p_World) {
          return;
        }

        JPH::BodyInterface &l_BodyInterface =
            p_World->physics_system.GetBodyInterface();
        p_World->capsule_controllers.clear();
        for (auto &i_Body : p_World->bodies) {
          l_BodyInterface.RemoveBody(i_Body.second);
          l_BodyInterface.DestroyBody(i_Body.second);
        }
        p_World->bodies.clear();
        p_World->shapes.clear();

        delete p_World;
        shutdown_jolt_runtime();
      }

      void simulate_world(WorldBackend *p_World, float p_Delta)
      {
        LOW_ASSERT(p_World, "Cannot simulate null physics world");

        constexpr int l_CollisionSteps = 1;
        p_World->physics_system.Update(p_Delta, l_CollisionSteps,
                                       &p_World->temp_allocator,
                                       &p_World->job_system);
      }

      void set_world_gravity(WorldBackend *p_World,
                             const Math::Vector3 &p_Gravity)
      {
        LOW_ASSERT(p_World,
                   "Cannot set gravity on null physics world");
        p_World->physics_system.SetGravity(to_jolt(p_Gravity));
      }

      Math::Vector3 get_world_gravity(WorldBackend *p_World)
      {
        LOW_ASSERT(p_World,
                   "Cannot get gravity from null physics world");
        return from_jolt(p_World->physics_system.GetGravity());
      }

      ShapeBackendHandle
      create_shape(WorldBackend *p_World,
                   const ShapeCreateInfo &p_CreateInfo)
      {
        LOW_ASSERT(p_World,
                   "Cannot create shape in null physics world");

        JPH::ShapeRefC l_Shape;
        switch (p_CreateInfo.shape.type) {
        case Math::ShapeType::BOX: {
          const Math::Box &l_Box = p_CreateInfo.shape.box;
          const JPH::Vec3 l_HalfExtents = to_jolt(l_Box.halfExtents);
          const float l_MinHalfExtent =
              std::min({l_HalfExtents.GetX(), l_HalfExtents.GetY(),
                        l_HalfExtents.GetZ()});
          const float l_ConvexRadius =
              std::min(l_MinHalfExtent * 0.5f, 0.05f);
          l_Shape = new JPH::BoxShape(l_HalfExtents, l_ConvexRadius);
          break;
        }
        case Math::ShapeType::SPHERE: {
          const Math::Sphere &l_Sphere = p_CreateInfo.shape.sphere;
          l_Shape = new JPH::SphereShape(l_Sphere.radius);
          break;
        }
        case Math::ShapeType::CYLINDER: {
          const Math::Cylinder &l_Cylinder =
              p_CreateInfo.shape.cylinder;
          l_Shape = new JPH::CylinderShape(l_Cylinder.height * 0.5f,
                                           l_Cylinder.radius);
          break;
        }
        case Math::ShapeType::CONE:
          LOW_ASSERT(false,
                     "Jolt cone physics shape is not supported yet");
          return ShapeBackendHandle();
        }

        LOW_ASSERT(l_Shape != nullptr,
                   "Could not create Jolt physics shape");

        ShapeBackendHandle l_Handle;
        l_Handle.id = p_World->next_shape_id++;
        p_World->shapes[l_Handle.id] = l_Shape;
        return l_Handle;
      }

      ShapeBackendHandle
      create_convex_hull_shape(WorldBackend *p_World,
                               const Math::Vector3 *p_Points,
                               uint32_t p_PointCount)
      {
        LOW_ASSERT(p_World,
                   "Cannot create shape in null physics world");
        LOW_ASSERT(p_Points && p_PointCount >= 4,
                   "Convex hull requires at least 4 points");

        JPH::Array<JPH::Vec3> l_JoltPoints;
        l_JoltPoints.reserve(p_PointCount);
        for (uint32_t i = 0; i < p_PointCount; ++i) {
          l_JoltPoints.push_back(to_jolt(p_Points[i]));
        }

        JPH::ConvexHullShapeSettings l_Settings(
            l_JoltPoints.data(), (int)l_JoltPoints.size());
        JPH::Shape::ShapeResult l_Result = l_Settings.Create();
        LOW_ASSERT(l_Result.IsValid(),
                   "Could not create Jolt convex hull shape");

        ShapeBackendHandle l_Handle;
        l_Handle.id = p_World->next_shape_id++;
        p_World->shapes[l_Handle.id] = l_Result.Get();
        return l_Handle;
      }

      ShapeBackendHandle create_convex_hull_shape(
          WorldBackend *p_World,
          const Low::Util::List<Math::Vector3> &p_Points)
      {
        return create_convex_hull_shape(p_World, p_Points.data(),
                                        (uint32_t)p_Points.size());
      }

      void visualize_convex_hull(WorldBackend *p_World,
                                 ShapeBackendHandle p_Shape,
                                 Renderer::RenderView p_RenderView,
                                 const Math::Vector3 &p_Position,
                                 const Math::Quaternion &p_Rotation,
                                 const Math::Color &p_Color,
                                 bool p_Wireframe, bool p_DepthTest)
      {
        LOW_ASSERT(p_World,
                   "Cannot visualize shape in null physics world");

        auto l_ShapeIt = p_World->shapes.find(p_Shape.id);
        LOW_ASSERT(l_ShapeIt != p_World->shapes.end(),
                   "Unknown physics shape handle");
        LOW_ASSERT(l_ShapeIt->second->GetSubType() ==
                       JPH::EShapeSubType::ConvexHull,
                   "Physics shape is not a convex hull");

        const JPH::ConvexHullShape *l_Hull =
            static_cast<const JPH::ConvexHullShape *>(
                l_ShapeIt->second.GetPtr());
        const Math::Vector3 l_CenterOfMass =
            from_jolt(l_Hull->GetCenterOfMass());

        auto l_GetPoint = [&](uint32_t p_Index) {
          const Math::Vector3 l_LocalPoint =
              from_jolt(l_Hull->GetPoint(p_Index)) + l_CenterOfMass;
          return p_Position + (p_Rotation * l_LocalPoint);
        };

        std::unordered_set<uint64_t> l_RenderedEdges;
        for (uint32_t i = 0u; i < l_Hull->GetNumFaces(); ++i) {
          const uint32_t i_VertexCount =
              l_Hull->GetNumVerticesInFace(i);
          if (i_VertexCount < 3u) {
            continue;
          }

          JPH::Array<JPH::uint> i_Indices;
          i_Indices.resize(i_VertexCount);
          l_Hull->GetFaceVertices(i, i_VertexCount, i_Indices.data());

          if (!p_Wireframe) {
            const Math::Vector3 i_FirstPoint =
                l_GetPoint(i_Indices[0]);
            for (uint32_t j = 1u; j + 1u < i_VertexCount; ++j) {
              DebugGeometry::render_triangle(
                  p_RenderView, i_FirstPoint,
                  l_GetPoint(i_Indices[j]),
                  l_GetPoint(i_Indices[j + 1u]), p_Color, p_DepthTest,
                  false);
            }
          }

          else {
            for (uint32_t j = 0u; j < i_VertexCount; ++j) {
              const uint32_t i_Index0 = i_Indices[j];
              const uint32_t i_Index1 =
                  i_Indices[(j + 1u) % i_VertexCount];
              const uint32_t i_MinIndex =
                  std::min(i_Index0, i_Index1);
              const uint32_t i_MaxIndex =
                  std::max(i_Index0, i_Index1);
              const uint64_t i_EdgeKey =
                  (static_cast<uint64_t>(i_MinIndex) << 32u) |
                  i_MaxIndex;

              if (l_RenderedEdges.insert(i_EdgeKey).second) {
                DebugGeometry::render_line(
                    p_RenderView, l_GetPoint(i_Index0),
                    l_GetPoint(i_Index1), p_Color, p_DepthTest);
              }
            }
          }
        }
      }

      void destroy_shape(WorldBackend *p_World,
                         ShapeBackendHandle p_Shape)
      {
        LOW_ASSERT(p_World,
                   "Cannot destroy shape in null physics world");
        if (!p_Shape.is_valid()) {
          return;
        }
        p_World->shapes.erase(p_Shape.id);
      }

      BodyBackendHandle
      create_body(WorldBackend *p_World,
                  const BodyCreateInfo &p_CreateInfo)
      {
        LOW_ASSERT(p_World,
                   "Cannot create body in null physics world");
        LOW_ASSERT(p_CreateInfo.shape.is_valid(),
                   "Cannot create body without a valid shape");

        auto l_ShapeIt = p_World->shapes.find(p_CreateInfo.shape.id);
        LOW_ASSERT(l_ShapeIt != p_World->shapes.end(),
                   "Unknown physics shape handle");

        JPH::BodyCreationSettings l_Settings(
            l_ShapeIt->second,
            to_jolt_position(p_CreateInfo.position),
            to_jolt(p_CreateInfo.rotation),
            to_jolt(p_CreateInfo.motion_type),
            get_layer_for_motion_type(p_CreateInfo.motion_type));
        l_Settings.mOverrideMassProperties =
            JPH::EOverrideMassProperties::CalculateInertia;
        l_Settings.mMassPropertiesOverride.mMass = p_CreateInfo.mass;
        l_Settings.mGravityFactor =
            p_CreateInfo.gravity ? 1.0f : 0.0f;
        l_Settings.mUserData =
            reinterpret_cast<uint64_t>(p_CreateInfo.user_data);

        const JPH::EActivation l_Activation =
            p_CreateInfo.motion_type == BodyMotionType::DYNAMIC
                ? JPH::EActivation::Activate
                : JPH::EActivation::DontActivate;

        JPH::BodyID l_BodyId =
            p_World->physics_system.GetBodyInterface()
                .CreateAndAddBody(l_Settings, l_Activation);
        LOW_ASSERT(!l_BodyId.IsInvalid(),
                   "Could not create Jolt physics body");

        BodyBackendHandle l_Handle;
        l_Handle.id = p_World->next_body_id++;
        p_World->bodies[l_Handle.id] = l_BodyId;
        return l_Handle;
      }

      void destroy_body(WorldBackend *p_World,
                        BodyBackendHandle p_Body)
      {
        LOW_ASSERT(p_World,
                   "Cannot destroy body in null physics world");
        if (!p_Body.is_valid()) {
          return;
        }

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        if (l_BodyIt == p_World->bodies.end()) {
          return;
        }

        JPH::BodyInterface &l_BodyInterface =
            p_World->physics_system.GetBodyInterface();
        l_BodyInterface.RemoveBody(l_BodyIt->second);
        l_BodyInterface.DestroyBody(l_BodyIt->second);
        p_World->bodies.erase(l_BodyIt);
      }

      void set_body_transform(WorldBackend *p_World,
                              BodyBackendHandle p_Body,
                              const Math::Vector3 &p_Position,
                              const Math::Quaternion &p_Rotation)
      {
        LOW_ASSERT(p_World,
                   "Cannot set transform in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        p_World->physics_system.GetBodyInterface()
            .SetPositionAndRotation(
                l_BodyIt->second, to_jolt_position(p_Position),
                to_jolt(p_Rotation), JPH::EActivation::Activate);
      }

      Math::Vector3 get_body_position(WorldBackend *p_World,
                                      BodyBackendHandle p_Body)
      {
        LOW_ASSERT(p_World,
                   "Cannot get body position in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        return from_jolt_position(
            p_World->physics_system.GetBodyInterface().GetPosition(
                l_BodyIt->second));
      }

      Math::Quaternion get_body_rotation(WorldBackend *p_World,
                                         BodyBackendHandle p_Body)
      {
        LOW_ASSERT(p_World,
                   "Cannot get body rotation in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        return from_jolt(
            p_World->physics_system.GetBodyInterface().GetRotation(
                l_BodyIt->second));
      }

      void set_body_linear_velocity(WorldBackend *p_World,
                                    BodyBackendHandle p_Body,
                                    const Math::Vector3 &p_Velocity)
      {
        LOW_ASSERT(
            p_World,
            "Cannot set body linear velocity in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        p_World->physics_system.GetBodyInterface().SetLinearVelocity(
            l_BodyIt->second, to_jolt(p_Velocity));
      }

      Math::Vector3 get_body_linear_velocity(WorldBackend *p_World,
                                             BodyBackendHandle p_Body)
      {
        LOW_ASSERT(
            p_World,
            "Cannot get body linear velocity in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        return from_jolt(p_World->physics_system.GetBodyInterface()
                             .GetLinearVelocity(l_BodyIt->second));
      }

      void set_body_angular_velocity(WorldBackend *p_World,
                                     BodyBackendHandle p_Body,
                                     const Math::Vector3 &p_Velocity)
      {
        LOW_ASSERT(
            p_World,
            "Cannot set body angular velocity in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        p_World->physics_system.GetBodyInterface().SetAngularVelocity(
            l_BodyIt->second, to_jolt(p_Velocity));
      }

      Math::Vector3
      get_body_angular_velocity(WorldBackend *p_World,
                                BodyBackendHandle p_Body)
      {
        LOW_ASSERT(
            p_World,
            "Cannot get body angular velocity in null physics world");

        auto l_BodyIt = p_World->bodies.find(p_Body.id);
        LOW_ASSERT(l_BodyIt != p_World->bodies.end(),
                   "Unknown physics body handle");

        return from_jolt(p_World->physics_system.GetBodyInterface()
                             .GetAngularVelocity(l_BodyIt->second));
      }

      bool raycast_world(WorldBackend *p_World,
                         const Math::Vector3 &p_Origin,
                         const Math::Vector3 &p_Direction,
                         float p_MaxDistance, BackendQueryHit &p_Hit)
      {
        LOW_ASSERT(p_World, "Cannot raycast in null physics world");

        const float l_DirectionLength = glm::length(p_Direction);
        if (l_DirectionLength <= LOW_MATH_EPSILON ||
            p_MaxDistance <= 0.0f) {
          return false;
        }

        const JPH::RRayCast l_Ray(
            to_jolt_position(p_Origin),
            to_jolt((p_Direction / l_DirectionLength) *
                    p_MaxDistance));

        JPH::RayCastResult l_Result;
        if (!p_World->physics_system.GetNarrowPhaseQuery().CastRay(
                l_Ray, l_Result)) {
          return false;
        }

        return fill_raycast_hit(p_World, l_Ray, l_Result,
                                p_MaxDistance, p_Hit);
      }

      bool sphere_cast_world(WorldBackend *p_World,
                             const Math::Vector3 &p_Origin,
                             float p_Radius,
                             const Math::Vector3 &p_Direction,
                             float p_MaxDistance, BackendQueryHit &p_Hit)
      {
        JPH::ShapeRefC l_Shape = create_sphere_query_shape(p_Radius);
        return cast_query_shape(
            p_World, l_Shape.GetPtr(), p_Origin,
            Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f), p_Direction,
            p_MaxDistance, p_Hit);
      }

      bool box_cast_world(WorldBackend *p_World,
                          const Math::Vector3 &p_Origin,
                          const Math::Vector3 &p_HalfExtents,
                          const Math::Quaternion &p_Rotation,
                          const Math::Vector3 &p_Direction,
                          float p_MaxDistance, BackendQueryHit &p_Hit)
      {
        JPH::ShapeRefC l_Shape =
            create_box_query_shape(p_HalfExtents);
        return cast_query_shape(p_World, l_Shape.GetPtr(), p_Origin,
                                p_Rotation, p_Direction,
                                p_MaxDistance, p_Hit);
      }

      bool overlap_sphere_world(WorldBackend *p_World,
                                const Math::Vector3 &p_Position,
                                float p_Radius, BackendQueryHit *p_Hit)
      {
        JPH::ShapeRefC l_Shape = create_sphere_query_shape(p_Radius);
        return overlap_query_shape(
            p_World, l_Shape.GetPtr(), p_Position,
            Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f), p_Hit);
      }

      bool overlap_box_world(WorldBackend *p_World,
                             const Math::Vector3 &p_Position,
                             const Math::Vector3 &p_HalfExtents,
                             const Math::Quaternion &p_Rotation,
                             BackendQueryHit *p_Hit)
      {
        JPH::ShapeRefC l_Shape =
            create_box_query_shape(p_HalfExtents);
        return overlap_query_shape(p_World, l_Shape.GetPtr(),
                                   p_Position, p_Rotation, p_Hit);
      }

      CapsuleControllerBackendHandle create_capsule_controller(
          WorldBackend *p_World,
          const CapsuleControllerCreateInfo &p_CreateInfo)
      {
        LOW_ASSERT(
            p_World,
            "Cannot create capsule controller in null physics world");

        const float l_Radius = std::max(p_CreateInfo.radius, 0.001f);
        const float l_TotalHeight =
            std::max(p_CreateInfo.height, l_Radius * 2.0f + 0.002f);
        const float l_HalfCylinderHeight = std::max(
            (l_TotalHeight - (l_Radius * 2.0f)) * 0.5f, 0.001f);
        const float l_CenterOffset = l_HalfCylinderHeight + l_Radius;

        JPH::Ref<JPH::CharacterVirtualSettings> l_Settings =
            new JPH::CharacterVirtualSettings();
        l_Settings->mMaxSlopeAngle =
            glm::radians(std::max(p_CreateInfo.slope_limit, 0.0f));
        l_Settings->mCharacterPadding =
            std::max(p_CreateInfo.skin_width, 0.001f);
        l_Settings->mPredictiveContactDistance =
            std::max(p_CreateInfo.skin_width * 2.0f, 0.05f);
        l_Settings->mPenetrationRecoverySpeed = 1.0f;
        l_Settings->mInnerBodyLayer = Layers::MOVING;

        JPH::RefConst<JPH::Shape> l_CapsuleShape =
            new JPH::CapsuleShape(l_HalfCylinderHeight, l_Radius);
        JPH::ShapeSettings::ShapeResult l_ShapeResult =
            JPH::RotatedTranslatedShapeSettings(
                JPH::Vec3(0.0f, l_CenterOffset, 0.0f),
                JPH::Quat::sIdentity(), l_CapsuleShape)
                .Create();
        LOW_ASSERT(l_ShapeResult.IsValid(),
                   "Could not create Jolt capsule controller shape");

        l_Settings->mShape = l_ShapeResult.Get();
        l_Settings->mSupportingVolume =
            JPH::Plane(JPH::Vec3::sAxisY(), -l_Radius);

        const JPH::Quat l_Rotation = to_jolt(p_CreateInfo.rotation);
        const JPH::Vec3 l_Up = l_Rotation.RotateAxisY();
        const JPH::RVec3 l_BottomPosition =
            to_jolt_position(p_CreateInfo.position) -
            JPH::RVec3(l_Up * l_CenterOffset);

        JPH::Ref<JPH::CharacterVirtual> l_Controller =
            new JPH::CharacterVirtual(
                l_Settings, l_BottomPosition, l_Rotation,
                reinterpret_cast<uint64_t>(p_CreateInfo.user_data),
                &p_World->physics_system);
        l_Controller->SetUp(l_Up);

        CapsuleControllerBackendHandle l_Handle;
        l_Handle.id = p_World->next_capsule_controller_id++;
        WorldBackend::CapsuleControllerBackend l_Backend;
        l_Backend.controller = l_Controller;
        l_Backend.center_offset = l_CenterOffset;
        l_Backend.step_offset =
            std::max(p_CreateInfo.step_offset, 0.0f);
        l_Backend.skin_width =
            std::max(p_CreateInfo.skin_width, 0.0f);
        p_World->capsule_controllers[l_Handle.id] = l_Backend;

        l_Controller->RefreshContacts(
            p_World->physics_system.GetDefaultBroadPhaseLayerFilter(
                Layers::MOVING),
            p_World->physics_system.GetDefaultLayerFilter(
                Layers::MOVING),
            {}, {}, p_World->temp_allocator);

        return l_Handle;
      }

      void destroy_capsule_controller(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller)
      {
        LOW_ASSERT(p_World, "Cannot destroy capsule controller in "
                            "null physics world");
        if (!p_Controller.is_valid()) {
          return;
        }

        p_World->capsule_controllers.erase(p_Controller.id);
      }

      static WorldBackend::CapsuleControllerBackend &
      get_capsule_controller_backend(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller)
      {
        LOW_ASSERT(
            p_World,
            "Cannot access capsule controller in null physics world");
        auto l_ControllerIt =
            p_World->capsule_controllers.find(p_Controller.id);
        LOW_ASSERT(l_ControllerIt !=
                       p_World->capsule_controllers.end(),
                   "Unknown capsule controller handle");
        return l_ControllerIt->second;
      }

      void move_capsule_controller(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Vector3 &p_Delta, float p_DeltaTime)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);

        if (p_DeltaTime <= 0.0f) {
          set_capsule_controller_position(
              p_World, p_Controller,
              get_capsule_controller_position(p_World, p_Controller) +
                  p_Delta);
          return;
        }

        const JPH::Vec3 l_DesiredVelocity =
            to_jolt(p_Delta) / p_DeltaTime;
        l_Backend.controller->SetLinearVelocity(l_DesiredVelocity);

        JPH::CharacterVirtual::ExtendedUpdateSettings
            l_UpdateSettings;
        const JPH::Vec3 l_Up = l_Backend.controller->GetUp();
        l_UpdateSettings.mWalkStairsStepUp =
            l_Up * l_Backend.step_offset;
        l_UpdateSettings.mStickToFloorStepDown =
            -l_Up *
            std::max(l_Backend.step_offset + l_Backend.skin_width,
                     0.0f);

        l_Backend.controller->ExtendedUpdate(
            p_DeltaTime, p_World->physics_system.GetGravity(),
            l_UpdateSettings,
            p_World->physics_system.GetDefaultBroadPhaseLayerFilter(
                Layers::MOVING),
            p_World->physics_system.GetDefaultLayerFilter(
                Layers::MOVING),
            {}, {}, p_World->temp_allocator);
      }

      bool is_capsule_controller_grounded(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);
        return l_Backend.controller->GetGroundState() ==
               JPH::CharacterBase::EGroundState::OnGround;
      }

      Math::Vector3 get_capsule_controller_position(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);
        const JPH::Vec3 l_Up =
            l_Backend.controller->GetRotation().RotateAxisY();
        return from_jolt_position(
            l_Backend.controller->GetPosition() +
            JPH::RVec3(l_Up * l_Backend.center_offset));
      }

      void set_capsule_controller_position(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Vector3 &p_Position)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);
        const JPH::Vec3 l_Up =
            l_Backend.controller->GetRotation().RotateAxisY();
        l_Backend.controller->SetPosition(
            to_jolt_position(p_Position) -
            JPH::RVec3(l_Up * l_Backend.center_offset));
        l_Backend.controller->RefreshContacts(
            p_World->physics_system.GetDefaultBroadPhaseLayerFilter(
                Layers::MOVING),
            p_World->physics_system.GetDefaultLayerFilter(
                Layers::MOVING),
            {}, {}, p_World->temp_allocator);
      }

      Math::Quaternion get_capsule_controller_rotation(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);
        return from_jolt(l_Backend.controller->GetRotation());
      }

      void set_capsule_controller_rotation(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Quaternion &p_Rotation)
      {
        WorldBackend::CapsuleControllerBackend &l_Backend =
            get_capsule_controller_backend(p_World, p_Controller);
        const Math::Vector3 l_Position =
            get_capsule_controller_position(p_World, p_Controller);
        const JPH::Quat l_Rotation = to_jolt(p_Rotation);
        l_Backend.controller->SetRotation(l_Rotation);
        l_Backend.controller->SetUp(l_Rotation.RotateAxisY());
        set_capsule_controller_position(p_World, p_Controller,
                                        l_Position);
      }
    } // namespace Physics
  } // namespace Core
} // namespace Low
