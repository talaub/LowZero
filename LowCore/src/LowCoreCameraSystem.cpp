#include "LowCoreCameraSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreTransform.h"
#include "LowCoreCamera.h"

#include "LowUtilProfiler.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Camera {
        static void apply_camera(Component::Camera p_Camera)
        {
          Component::Transform i_Transform =
              p_Camera.get_entity().get_transform();

          Renderer::RenderFlow l_RenderFlow =
              Renderer::get_main_renderflow();

          l_RenderFlow.set_camera_fov(p_Camera.get_fov());
          l_RenderFlow.set_camera_position(
              i_Transform.get_world_position());
          l_RenderFlow.set_camera_direction(
              Math::VectorUtil::direction(
                  i_Transform.get_world_rotation()));
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          if (p_State != Util::EngineState::PLAYING) {
            return;
          }

          LOW_PROFILE_CPU("Core", "CameraSystem::TICK");

          Component::Camera *l_Cameras =
              Component::Camera::living_instances();

          Component::Camera l_ActiveCamera = 0;

          for (uint32_t i = 0u; i < Component::Camera::living_count();
               ++i) {
            Component::Camera i_Camera = l_Cameras[i];

            if (!l_ActiveCamera.is_alive()) {
              l_ActiveCamera = i_Camera;
            }

            if (i_Camera.is_active()) {
              l_ActiveCamera = i_Camera;
              break;
            }
          }

          if (l_ActiveCamera.is_alive()) {
            apply_camera(l_ActiveCamera);
          }
        }

      } // namespace Camera
    }   // namespace System
  }     // namespace Core
} // namespace Low
