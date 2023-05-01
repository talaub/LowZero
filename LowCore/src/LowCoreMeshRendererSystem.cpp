#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTransform.h"
#include "LowCoreTaskScheduler.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace MeshRenderer {
        static void render_submesh(Submesh &p_Submesh,
                                   Math::Matrix4x4 &p_TransformMatrix,
                                   Renderer::Material p_Material,
                                   uint32_t p_EntityIndex)
        {
          Renderer::RenderObject i_RenderObject;
          i_RenderObject.entity_id = p_EntityIndex;
          i_RenderObject.mesh = p_Submesh.mesh;
          i_RenderObject.material = p_Material;
          i_RenderObject.transform =
              p_TransformMatrix * p_Submesh.transformation;

          Renderer::get_main_renderflow().register_renderobject(i_RenderObject);
        }

        static void render_mesh(MeshResource p_MeshResource,
                                Math::Matrix4x4 &p_TransformMatrix,
                                Renderer::Material p_Material,
                                uint32_t p_EntityIndex)
        {
          for (uint32_t i = 0u; i < p_MeshResource.get_submeshes().size();
               ++i) {
            render_submesh(p_MeshResource.get_submeshes()[i], p_TransformMatrix,
                           p_Material, p_EntityIndex);
          }
        }

        void tick(float p_Delta)
        {
          Component::MeshRenderer *l_MeshRenderers =
              Component::MeshRenderer::living_instances();

          for (uint32_t i = 0u; i < Component::MeshRenderer::living_count();
               ++i) {
            Component::MeshRenderer i_MeshRenderer = l_MeshRenderers[i];

            Component::Transform i_Transform =
                i_MeshRenderer.get_entity().get_transform();

            if (!i_MeshRenderer.get_mesh().get_lod0().is_alive() ||
                !i_MeshRenderer.get_material().is_alive()) {
              continue;
            }

            if (!i_MeshRenderer.get_material().is_loaded()) {
              i_MeshRenderer.get_material().load();
            }
            if (!i_MeshRenderer.get_mesh().get_lod0().is_loaded()) {
              TaskScheduler::schedule_mesh_resource_load(
                  i_MeshRenderer.get_mesh().get_lod0());
              continue;
            }

            Math::Matrix4x4 i_TransformMatrix =
                glm::translate(glm::mat4(1.0f),
                               i_Transform.get_world_position()) *
                glm::toMat4(i_Transform.get_world_rotation()) *
                glm::scale(glm::mat4(1.0f), i_Transform.get_world_scale());

            render_mesh(i_MeshRenderer.get_mesh().get_lod0(), i_TransformMatrix,
                        i_MeshRenderer.get_material().get_renderer_material(),
                        i_MeshRenderer.get_entity().get_index());
          }
        }
      } // namespace MeshRenderer
    }   // namespace System
  }     // namespace Core
} // namespace Low
