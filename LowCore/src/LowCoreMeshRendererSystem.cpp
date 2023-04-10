#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTransform.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace MeshRenderer {
        void tick(float p_Delta)
        {
          Component::MeshRenderer *l_MeshRenderers =
              Component::MeshRenderer::living_instances();

          for (uint32_t i = 0u; i < Component::MeshRenderer::living_count();
               ++i) {
            Component::MeshRenderer i_MeshRenderer = l_MeshRenderers[i];

            Component::Transform i_Transform =
                i_MeshRenderer.get_entity().get_transform();

            if (!i_MeshRenderer.get_mesh().get_lod0().is_loaded()) {
              // HACK
              i_MeshRenderer.get_mesh().get_lod0().load();
            }

            Renderer::RenderObject i_RenderObject;
            i_RenderObject.entity_id = i_MeshRenderer.get_entity().get_index();
            i_RenderObject.mesh =
                i_MeshRenderer.get_mesh().get_lod0().get_renderer_mesh();
            i_RenderObject.material = Renderer::Material::ms_LivingInstances[0];
            i_RenderObject.world_position = i_Transform.get_world_position();
            i_RenderObject.world_rotation = i_Transform.get_world_rotation();
            i_RenderObject.world_scale = i_Transform.get_world_scale();

            Renderer::get_main_renderflow().register_renderobject(
                i_RenderObject);
          }
        }
      } // namespace MeshRenderer
    }   // namespace System
  }     // namespace Core
} // namespace Low
