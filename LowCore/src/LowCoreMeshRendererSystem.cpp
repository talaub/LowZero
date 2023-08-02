#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
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
        float g_Progress = 0.0f;
        static void render_submesh(Submesh &p_Submesh,
                                   Math::Matrix4x4 &p_TransformMatrix,
                                   Renderer::Material p_Material,
                                   uint32_t p_EntityIndex,
                                   bool p_UseSkinningBuffer,
                                   uint32_t p_VertexBufferStartOverride)
        {
          Renderer::RenderObject i_RenderObject;
          i_RenderObject.entity_id = p_EntityIndex;
          i_RenderObject.mesh = p_Submesh.mesh;
          i_RenderObject.material = p_Material;
          i_RenderObject.transform =
              p_TransformMatrix * p_Submesh.transformation;
          if (p_UseSkinningBuffer) {
            i_RenderObject.transform = p_TransformMatrix;
          }
          i_RenderObject.useSkinningBuffer = p_UseSkinningBuffer;
          i_RenderObject.vertexBufferStartOverride =
              p_VertexBufferStartOverride;

          Renderer::get_main_renderflow().register_renderobject(i_RenderObject);
        }

        static void render_mesh(float p_Delta, Util::EngineState p_State,
                                MeshResource p_MeshResource,
                                Math::Matrix4x4 &p_TransformMatrix,
                                Renderer::Material p_Material,
                                uint32_t p_EntityIndex)
        {
          bool l_UsesSkeletalAnimation = false;
          uint32_t l_PoseIndex = 0;

          Renderer::SkeletalAnimation l_Animation =
              Renderer::SkeletalAnimation::find_by_name(N(04_Idle));

          if (p_MeshResource.get_skeleton().is_alive()) {
            if (p_State == Util::EngineState::PLAYING) {
              g_Progress += l_Animation.get_ticks_per_second() * p_Delta;
              g_Progress = fmod(g_Progress, l_Animation.get_duration());
            }

            l_UsesSkeletalAnimation = true;
            l_PoseIndex = Renderer::calculate_skeleton_pose(
                p_MeshResource.get_skeleton(), l_Animation, g_Progress);
          }

          for (uint32_t i = 0u; i < p_MeshResource.get_submeshes().size();
               ++i) {
            uint32_t i_VertexBufferStartOverride = 0;
            if (l_UsesSkeletalAnimation) {
              i_VertexBufferStartOverride =
                  Renderer::register_skinning_operation(
                      p_MeshResource.get_submeshes()[i].mesh,
                      p_MeshResource.get_skeleton(), l_PoseIndex,
                      p_MeshResource.get_submeshes()[i].transformation);
            }

            render_submesh(p_MeshResource.get_submeshes()[i], p_TransformMatrix,
                           p_Material, p_EntityIndex, l_UsesSkeletalAnimation,
                           i_VertexBufferStartOverride);
          }
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          MICROPROFILE_SCOPEI("Core", "MeshRendererSystem::TICK", MP_RED);
          Component::MeshRenderer *l_MeshRenderers =
              Component::MeshRenderer::living_instances();

          for (uint32_t i = 0u; i < Component::MeshRenderer::living_count();
               ++i) {
            Component::MeshRenderer i_MeshRenderer = l_MeshRenderers[i];

            Component::Transform i_Transform =
                i_MeshRenderer.get_entity().get_transform();

            if (!i_MeshRenderer.get_mesh().is_alive() ||
                !i_MeshRenderer.get_mesh().get_lod0().is_alive()) {
              continue;
            }

            if (!i_MeshRenderer.get_mesh().get_lod0().is_loaded()) {
              continue;
            }

            Math::Matrix4x4 i_TransformMatrix =
                glm::translate(glm::mat4(1.0f),
                               i_Transform.get_world_position()) *
                glm::toMat4(i_Transform.get_world_rotation()) *
                glm::scale(glm::mat4(1.0f), i_Transform.get_world_scale());

            Renderer::Material l_Material = Renderer::get_default_material();

            if (i_MeshRenderer.get_material().is_alive()) {
              l_Material =
                  i_MeshRenderer.get_material().get_renderer_material();
            }

            render_mesh(p_Delta, p_State, i_MeshRenderer.get_mesh().get_lod0(),
                        i_TransformMatrix, l_Material,
                        i_MeshRenderer.get_entity().get_index());

            // LOW_LOG_INFO << "------------" << LOW_LOG_END;
          }
        }
      } // namespace MeshRenderer
    }   // namespace System
  }     // namespace Core
} // namespace Low
