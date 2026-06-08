#include "LowCorePhysicsSystem.h"

#include "LowCoreBoxCollider.h"
#include "LowCoreRigidbody.h"
#include "LowCoreScene.h"
#include "LowCoreSphereCollider.h"
#include "LowCoreTransform.h"
#include "LowCoreCharacterController.h"

#include "LowMath.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Physics {
        static Low::Math::Vector3
        get_box_center(Component::BoxCollider p_Collider)
        {
          return p_Collider.get_center();
        }

        static Low::Math::Vector3
        get_sphere_center(Component::SphereCollider p_Collider)
        {
          return p_Collider.get_center();
        }

        static void
        set_body_from_transform(Low::Core::Physics::Body p_Body,
                                Component::Transform p_Transform,
                                const Low::Math::Vector3 &p_Center)
        {
          if (!p_Body.is_alive() || !p_Transform.is_alive()) {
            return;
          }

          p_Body.set_position(
              p_Transform.get_world_position() +
              (p_Transform.get_world_rotation() * p_Center));
          p_Body.set_rotation(p_Transform.get_world_rotation());
        }

        static void
        write_transform_from_body(Component::Transform p_Transform,
                                  Low::Core::Physics::Body p_Body,
                                  const Low::Math::Vector3 &p_Center)
        {
          if (!p_Body.is_alive() || !p_Transform.is_alive()) {
            return;
          }

          const Low::Math::Quaternion l_WorldRotation =
              p_Body.get_rotation();
          const Low::Math::Vector3 l_WorldPosition =
              p_Body.get_position() - (l_WorldRotation * p_Center);

          Component::Transform l_Parent = p_Transform.get_parent();
          if (l_Parent.is_alive()) {
            Low::Math::Matrix4x4 l_WorldMatrix(1.0f);
            l_WorldMatrix =
                glm::translate(l_WorldMatrix, l_WorldPosition);
            l_WorldMatrix *= glm::toMat4(l_WorldRotation);

            Low::Math::Matrix4x4 l_LocalMatrix =
                glm::inverse(l_Parent.get_world_matrix()) *
                l_WorldMatrix;

            Low::Math::Vector3 l_LocalScale;
            Low::Math::Quaternion l_LocalRotation;
            Low::Math::Vector3 l_LocalPosition;
            Low::Math::Vector3 l_LocalSkew;
            Low::Math::Vector4 l_LocalPerspective;

            glm::decompose(l_LocalMatrix, l_LocalScale,
                           l_LocalRotation, l_LocalPosition,
                           l_LocalSkew, l_LocalPerspective);

            p_Transform.position(l_LocalPosition);
            p_Transform.rotation(l_LocalRotation);
            return;
          }

          p_Transform.position(l_WorldPosition);
          p_Transform.rotation(l_WorldRotation);
        }

        static bool get_rigidbody_center(Entity p_Entity,
                                         Low::Math::Vector3 &p_Center)
        {
          if (p_Entity.has_component(
                  Component::BoxCollider::type_id())) {
            Component::BoxCollider l_Collider =
                p_Entity.get_component(
                    Component::BoxCollider::type_id());
            p_Center = get_box_center(l_Collider);
            return true;
          }

          if (p_Entity.has_component(
                  Component::SphereCollider::type_id())) {
            Component::SphereCollider l_Collider =
                p_Entity.get_component(
                    Component::SphereCollider::type_id());
            p_Center = get_sphere_center(l_Collider);
            return true;
          }

          p_Center = Low::Math::Vector3(0.0f);
          return false;
        }

        static void sync_static_colliders_to_physics()
        {
          for (u32 i = 0u; i < Component::BoxCollider::living_count();
               ++i) {
            Component::BoxCollider i_Collider =
                Component::BoxCollider::living_instances()[i];
            if (!i_Collider.is_alive() ||
                !i_Collider.get_static_body().is_alive()) {
              continue;
            }

            Entity i_Entity = i_Collider.get_entity();
            if (!i_Entity.is_alive() ||
                i_Entity.has_component(
                    Component::Rigidbody::type_id())) {
              continue;
            }

            set_body_from_transform(i_Collider.get_static_body(),
                                    i_Entity.get_transform(),
                                    get_box_center(i_Collider));
          }

          for (u32 i = 0u;
               i < Component::SphereCollider::living_count(); ++i) {
            Component::SphereCollider i_Collider =
                Component::SphereCollider::living_instances()[i];
            if (!i_Collider.is_alive() ||
                !i_Collider.get_static_body().is_alive()) {
              continue;
            }

            Entity i_Entity = i_Collider.get_entity();
            if (!i_Entity.is_alive() ||
                i_Entity.has_component(
                    Component::Rigidbody::type_id())) {
              continue;
            }

            set_body_from_transform(i_Collider.get_static_body(),
                                    i_Entity.get_transform(),
                                    get_sphere_center(i_Collider));
          }
        }

        static void sync_rigidbodies_to_physics()
        {
          for (u32 i = 0u; i < Component::Rigidbody::living_count();
               ++i) {
            Component::Rigidbody i_Rigidbody =
                Component::Rigidbody::living_instances()[i];
            if (!i_Rigidbody.is_alive() ||
                !i_Rigidbody.get_body().is_alive() ||
                i_Rigidbody.get_motion_type() ==
                    Low::Core::Physics::BodyMotionType::DYNAMIC) {
              continue;
            }

            Entity i_Entity = i_Rigidbody.get_entity();
            if (!i_Entity.is_alive()) {
              continue;
            }

            Low::Math::Vector3 l_Center(0.0f);
            get_rigidbody_center(i_Entity, l_Center);
            set_body_from_transform(i_Rigidbody.get_body(),
                                    i_Entity.get_transform(),
                                    l_Center);
          }
        }

        static void write_dynamic_bodies_to_transforms()
        {
          for (u32 i = 0u; i < Component::Rigidbody::living_count();
               ++i) {
            Component::Rigidbody i_Rigidbody =
                Component::Rigidbody::living_instances()[i];
            if (!i_Rigidbody.is_alive() ||
                !i_Rigidbody.get_body().is_alive() ||
                i_Rigidbody.get_motion_type() !=
                    Low::Core::Physics::BodyMotionType::DYNAMIC) {
              continue;
            }

            Entity i_Entity = i_Rigidbody.get_entity();
            if (!i_Entity.is_alive()) {
              continue;
            }

            Low::Math::Vector3 l_Center(0.0f);
            get_rigidbody_center(i_Entity, l_Center);
            write_transform_from_body(i_Entity.get_transform(),
                                      i_Rigidbody.get_body(),
                                      l_Center);
          }
        }

        static void write_character_controller_positions_to_transforms()
        {
          for (u32 i = 0u;
               i < Component::CharacterController::living_count(); ++i) {
            Component::CharacterController i_Controller =
                Component::CharacterController::living_instances()[i];
            if (!i_Controller.is_alive() ||
                !i_Controller.get_capsule_controller().is_alive()) {
              continue;
            }

            Entity i_Entity = i_Controller.get_entity();
            if (!i_Entity.is_alive()) {
              continue;
            }

            Component::Transform l_Transform =
                i_Entity.get_transform();
            if (!l_Transform.is_alive()) {
              continue;
            }

            const Low::Math::Quaternion l_WorldRotation =
                l_Transform.get_world_rotation();
            const Low::Math::Vector3 l_WorldPosition =
                i_Controller.get_capsule_controller().get_position() -
                (l_WorldRotation * i_Controller.get_center());

            Component::Transform l_Parent = l_Transform.get_parent();
            if (l_Parent.is_alive()) {
              Low::Math::Vector4 l_LocalPosition4 =
                  glm::inverse(l_Parent.get_world_matrix()) *
                  Low::Math::Vector4(l_WorldPosition, 1.0f);
              l_Transform.position(
                  Low::Math::Vector3(l_LocalPosition4));
            } else {
              l_Transform.position(l_WorldPosition);
            }
          }
        }

        static void simulate_loaded_worlds(float p_Delta)
        {
          for (u32 i = 0u; i < Scene::living_count(); ++i) {
            Scene i_Scene = Scene::living_instances()[i];
            if (!i_Scene.is_loaded()) {
              continue;
            }

            Low::Core::Physics::World l_PhysicsWorld =
                i_Scene.get_physics_world();
            if (l_PhysicsWorld.is_alive()) {
              l_PhysicsWorld.simulate(p_Delta);
            }
          }
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          LOW_PROFILE_CPU("Core", "PhysicsSystem::TICK");

          for (Component::BoxCollider i_Collider :
               Component::BoxCollider::ms_Dirty) {
            if (i_Collider.is_alive()) {
              i_Collider.rebuild();
              i_Collider.set_dirty(false);
            }
          }
          Component::BoxCollider::ms_Dirty.clear();

          for (Component::SphereCollider i_Collider :
               Component::SphereCollider::ms_Dirty) {
            if (i_Collider.is_alive()) {
              i_Collider.rebuild();
              i_Collider.set_dirty(false);
            }
          }
          Component::SphereCollider::ms_Dirty.clear();

          for (Component::CharacterController i_CController :
               Component::CharacterController::ms_Dirty) {
            if (i_CController.is_alive()) {
              i_CController.rebuild();
              i_CController.set_dirty(false);
            }
          }
          Component::CharacterController::ms_Dirty.clear();

          for (Component::Rigidbody i_Rigidbody :
               Component::Rigidbody::ms_Dirty) {
            if (i_Rigidbody.is_alive()) {
              i_Rigidbody.rebuild();
              i_Rigidbody.set_dirty(false);
            }
          }
          Component::Rigidbody::ms_Dirty.clear();
        }

        void late_tick(float p_Delta, Util::EngineState p_State)
        {
          if (p_State != Util::EngineState::PLAYING) {
            return;
          }

          LOW_PROFILE_CPU("Core", "PhysicsSystem::LATETICK");

          sync_static_colliders_to_physics();
          sync_rigidbodies_to_physics();
          simulate_loaded_worlds(p_Delta);
          write_dynamic_bodies_to_transforms();
          write_character_controller_positions_to_transforms();
        }

      } // namespace Physics
    } // namespace System
  } // namespace Core
} // namespace Low
