#pragma once

#include "LowMath.h"
#include "LowCorePhysicsBodyMotionType.h"

#include <stdint.h>

namespace Low {
  namespace Core {
    namespace Physics {
      struct WorldBackend;

      struct ShapeBackendHandle
      {
        uint64_t id = 0u;

        bool is_valid() const
        {
          return id != 0u;
        }
      };

      struct BodyBackendHandle
      {
        uint64_t id = 0u;

        bool is_valid() const
        {
          return id != 0u;
        }
      };

      struct CapsuleControllerBackendHandle
      {
        uint64_t id = 0u;

        bool is_valid() const
        {
          return id != 0u;
        }
      };

      struct ShapeCreateInfo
      {
        Math::Shape shape;
      };

      struct BodyCreateInfo
      {
        ShapeBackendHandle shape;
        Math::Vector3 position = Math::Vector3(0.0f);
        Math::Quaternion rotation = Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        BodyMotionType motion_type = BodyMotionType::STATIC;
        float mass = 1.0f;
        bool gravity = true;
        void *user_data = nullptr;
      };

      struct CapsuleControllerCreateInfo
      {
        Math::Vector3 position = Math::Vector3(0.0f);
        Math::Quaternion rotation =
            Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        float height = 2.0f;
        float radius = 0.5f;
        float slope_limit = 45.0f;
        float step_offset = 0.3f;
        float skin_width = 0.08f;
        void *user_data = nullptr;
      };

      WorldBackend *create_world_backend();
      void destroy_world_backend(WorldBackend *p_World);

      void simulate_world(WorldBackend *p_World, float p_Delta);
      void set_world_gravity(WorldBackend *p_World,
                             const Math::Vector3 &p_Gravity);
      Math::Vector3 get_world_gravity(WorldBackend *p_World);

      ShapeBackendHandle
      create_shape(WorldBackend *p_World,
                   const ShapeCreateInfo &p_CreateInfo);
      void destroy_shape(WorldBackend *p_World,
                         ShapeBackendHandle p_Shape);

      BodyBackendHandle create_body(WorldBackend *p_World,
                                    const BodyCreateInfo &p_CreateInfo);
      void destroy_body(WorldBackend *p_World, BodyBackendHandle p_Body);

      void set_body_transform(WorldBackend *p_World,
                              BodyBackendHandle p_Body,
                              const Math::Vector3 &p_Position,
                              const Math::Quaternion &p_Rotation);
      Math::Vector3 get_body_position(WorldBackend *p_World,
                                      BodyBackendHandle p_Body);
      Math::Quaternion get_body_rotation(WorldBackend *p_World,
                                         BodyBackendHandle p_Body);
      void set_body_linear_velocity(WorldBackend *p_World,
                                    BodyBackendHandle p_Body,
                                    const Math::Vector3 &p_Velocity);
      Math::Vector3 get_body_linear_velocity(WorldBackend *p_World,
                                             BodyBackendHandle p_Body);
      void set_body_angular_velocity(WorldBackend *p_World,
                                     BodyBackendHandle p_Body,
                                     const Math::Vector3 &p_Velocity);
      Math::Vector3 get_body_angular_velocity(WorldBackend *p_World,
                                              BodyBackendHandle p_Body);

      CapsuleControllerBackendHandle create_capsule_controller(
          WorldBackend *p_World,
          const CapsuleControllerCreateInfo &p_CreateInfo);
      void destroy_capsule_controller(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller);
      void move_capsule_controller(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Vector3 &p_Delta, float p_DeltaTime);
      bool is_capsule_controller_grounded(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller);
      Math::Vector3 get_capsule_controller_position(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller);
      void set_capsule_controller_position(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Vector3 &p_Position);
      Math::Quaternion get_capsule_controller_rotation(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller);
      void set_capsule_controller_rotation(
          WorldBackend *p_World,
          CapsuleControllerBackendHandle p_Controller,
          const Math::Quaternion &p_Rotation);
    } // namespace Physics
  }   // namespace Core
} // namespace Low
