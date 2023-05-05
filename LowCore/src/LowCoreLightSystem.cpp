#include "LowCoreLightSystem.h"

#include "LowCoreDirectionalLight.h"
#include "LowCoreTransform.h"

#include "LowRenderer.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Light {
        void tick(float p_Delta)
        {
          if (Component::DirectionalLight::living_count() > 0u) {
            Component::Transform l_Transform =
                Component::DirectionalLight::living_instances()[0]
                    .get_entity()
                    .get_transform();

            Renderer::DirectionalLight l_RendererDirectionalLight;
            l_RendererDirectionalLight.color =
                Component::DirectionalLight::living_instances()[0].get_color();

            l_RendererDirectionalLight.rotation =
                l_Transform.get_world_rotation();

            Renderer::get_main_renderflow().set_directional_light(
                l_RendererDirectionalLight);
          }
        }
      } // namespace Light
    }   // namespace System
  }     // namespace Core
} // namespace Low
