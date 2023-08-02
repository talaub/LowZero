#include "LowCoreLightSystem.h"

#include "LowCorePointLight.h"
#include "LowCoreDirectionalLight.h"
#include "LowCoreTransform.h"

#include "LowRenderer.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

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

            Renderer::DirectionalLight l_RendererDirectionalLight;
            l_RendererDirectionalLight.color =
                Component::DirectionalLight::living_instances()[0].get_color() *
                Component::DirectionalLight::living_instances()[0]
                    .get_intensity();

            l_RendererDirectionalLight.rotation =
                l_Transform.get_world_rotation();

            Renderer::get_main_renderflow().set_directional_light(
                l_RendererDirectionalLight);
          } else {
            Renderer::DirectionalLight l_RendererDirectionalLight;
            l_RendererDirectionalLight.color = Math::ColorRGB(0.0f);

            l_RendererDirectionalLight.rotation =
                Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

            Renderer::get_main_renderflow().set_directional_light(
                l_RendererDirectionalLight);
          }

          for (Component::PointLight i_PointLight :
               Component::PointLight::ms_LivingInstances) {
            Component::Transform i_Transform =
                i_PointLight.get_entity().get_transform();

            Renderer::PointLight i_RendererPointLight;
            i_RendererPointLight.position = i_Transform.get_world_position();
            i_RendererPointLight.color =
                i_PointLight.get_color() * i_PointLight.get_intensity();

            Renderer::get_main_renderflow().get_point_lights().push_back(
                i_RendererPointLight);
          }
        }
      } // namespace Light
    }   // namespace System
  }     // namespace Core
} // namespace Low
