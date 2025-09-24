#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTransform.h"
#include "LowCoreTaskScheduler.h"

#include "LowRenderer.h"
#include <cmath>
#include "microprofile.h"
#include <stdint.h>

namespace Low {
  namespace Core {
    namespace System {
      namespace MeshRenderer {
        void tick(float p_Delta, Util::EngineState p_State)
        {
          MICROPROFILE_SCOPEI("Core", "MeshRendererSystem::TICK",
                              MP_RED);
          Component::MeshRenderer *l_MeshRenderers =
              Component::MeshRenderer::living_instances();

          for (uint32_t i = 0u;
               i < Component::MeshRenderer::living_count(); ++i) {
            Component::MeshRenderer i_MeshRenderer =
                l_MeshRenderers[i];

            LOCK_HANDLE(i_MeshRenderer);

            Component::Transform i_Transform =
                i_MeshRenderer.get_entity().get_transform();

            if (i_MeshRenderer.is_dirty()) {
              if (i_MeshRenderer.get_render_object().is_alive()) {
                i_MeshRenderer.get_render_object().destroy();
              }
            }

            i_MeshRenderer.set_dirty(false);

            if (!i_MeshRenderer.get_render_object().is_alive() &&
                i_MeshRenderer.get_mesh().is_alive()) {
              i_MeshRenderer.set_render_object(
                  Renderer::RenderObject::make(
                      Renderer::get_global_renderscene(),
                      i_MeshRenderer.get_mesh()));
            }

            Renderer::RenderObject i_RenderObject =
                i_MeshRenderer.get_render_object();

            LOCK_HANDLE(i_RenderObject);

            if (!i_RenderObject.is_alive()) {
              continue;
            }

            Renderer::Material l_Material =
                Renderer::get_default_material_texture();

            if (i_MeshRenderer.get_material().is_alive()) {
              l_Material = i_MeshRenderer.get_material();
            }

            i_RenderObject.set_material(l_Material);
            i_RenderObject.set_world_transform(
                i_Transform.get_world_matrix());
          }
        }
      } // namespace MeshRenderer
    } // namespace System
  } // namespace Core
} // namespace Low
