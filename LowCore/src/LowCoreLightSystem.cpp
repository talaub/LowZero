#include "LowCoreLightSystem.h"

#include "LowCorePointLight.h"
#include "LowCoreDirectionalLight.h"
#include "LowCoreTransform.h"

#include "LowRenderer.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Light {
        void tick(float p_Delta, Util::EngineState p_State)
        {
          LOW_PROFILE_CPU("Core", "LightSystem tick");
          if (Component::DirectionalLight::living_count() > 0u) {
            Component::Transform l_Transform =
                Component::DirectionalLight::living_instances()[0]
                    .get_entity()
                    .get_transform();

            Component::DirectionalLight l_DirectioalLight =
                Component::DirectionalLight::living_instances()[0];

            Renderer::get_global_renderscene()
                .set_directional_light_color(
                    l_DirectioalLight.get_color());
            Renderer::get_global_renderscene()
                .set_directional_light_direction(
                    Math::VectorUtil::direction(
                        l_Transform.get_world_rotation()));
            Renderer::get_global_renderscene()
                .set_directional_light_intensity(
                    l_DirectioalLight.get_intensity());
          } else {
            Renderer::get_global_renderscene()
                .set_directional_light_intensity(0.0f);
          }

          for (Component::PointLight i_PointLight :
               Component::PointLight::ms_LivingInstances) {
            Component::Transform i_Transform =
                i_PointLight.get_entity().get_transform();

            if (!i_PointLight.get_renderer_point_light().is_alive()) {
              Renderer::PointLight i_Renderer =
                  Renderer::PointLight::make(
                      Renderer::get_global_renderscene());
              i_PointLight.set_renderer_point_light(i_Renderer);
            }

            LOCK_HANDLE(i_PointLight.get_renderer_point_light());

            // TODO: Only update if the point light is actually dirty
            if (i_PointLight.get_renderer_point_light().is_alive()) {
              i_PointLight.get_renderer_point_light().set_color(
                  i_PointLight.get_color());
              i_PointLight.get_renderer_point_light().set_intensity(
                  i_PointLight.get_intensity());
              i_PointLight.get_renderer_point_light().set_range(
                  i_PointLight.get_range());
              i_PointLight.get_renderer_point_light()
                  .set_world_position(
                      i_Transform.get_world_position());
            }
          }
        }
      } // namespace Light
    } // namespace System
  } // namespace Core
} // namespace Low
