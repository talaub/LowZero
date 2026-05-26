#include "LowCoreTransformSystem.h"

#include "LowRendererAnimationClipState.h"
#include "LowRendererMeshState.h"
#include "LowRendererSkeletonState.h"
#include "LowRendererSkinningInstance.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTransform.h"
#include "LowCoreTaskScheduler.h"

#include "LowRenderer.h"
#include "LowRendererAnimationClip.h"
#include "LowRendererResourceManager.h"

#include <cmath>
#include <stdint.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Low {
  namespace Core {
    namespace System {
      namespace MeshRenderer {
        float g_TestProgress = 0.0f;
        Renderer::SkinningPose g_Pose;
        Renderer::SkinningInstance g_Instance;
        Renderer::AnimationClip g_Clip;

        static Math::Vector3 sample_vector_keys(
            const Util::List<Renderer::BinSerial::VecKey> &p_Keys,
            const float p_Time, const Math::Vector3 p_Default)
        {
          if (p_Keys.empty()) {
            return p_Default;
          }
          if (p_Keys.size() == 1u || p_Time <= p_Keys[0].time) {
            return p_Keys[0].value;
          }

          for (u32 i = 0u; i < p_Keys.size() - 1u; ++i) {
            const Renderer::BinSerial::VecKey &i_Current = p_Keys[i];
            const Renderer::BinSerial::VecKey &i_Next =
                p_Keys[i + 1u];

            if (p_Time <= i_Next.time) {
              const float i_Duration = i_Next.time - i_Current.time;
              if (i_Duration <= 0.0f) {
                return i_Current.value;
              }

              const float i_Factor =
                  (p_Time - i_Current.time) / i_Duration;
              return glm::mix(i_Current.value, i_Next.value,
                              i_Factor);
            }
          }

          return p_Keys[p_Keys.size() - 1u].value;
        }

        static Math::Quaternion sample_quat_keys(
            const Util::List<Renderer::BinSerial::QuatKey> &p_Keys,
            const float p_Time, const Math::Quaternion p_Default)
        {
          if (p_Keys.empty()) {
            return p_Default;
          }
          if (p_Keys.size() == 1u || p_Time <= p_Keys[0].time) {
            return glm::normalize(p_Keys[0].value);
          }

          for (u32 i = 0u; i < p_Keys.size() - 1u; ++i) {
            const Renderer::BinSerial::QuatKey &i_Current = p_Keys[i];
            const Renderer::BinSerial::QuatKey &i_Next =
                p_Keys[i + 1u];

            if (p_Time <= i_Next.time) {
              const float i_Duration = i_Next.time - i_Current.time;
              if (i_Duration <= 0.0f) {
                return glm::normalize(i_Current.value);
              }

              const float i_Factor =
                  (p_Time - i_Current.time) / i_Duration;
              return glm::normalize(glm::slerp(
                  i_Current.value, i_Next.value, i_Factor));
            }
          }

          return glm::normalize(p_Keys[p_Keys.size() - 1u].value);
        }

        static void test_posing(const float p_Delta)
        {
          if (!g_Pose.is_alive() || !g_Clip.is_alive() ||
              g_Clip.get_state() !=
                  Renderer::AnimationClipState::LOADED) {
            return;
          }

          Renderer::Skeleton l_Skeleton = g_Clip.get_skeleton();
          if (!l_Skeleton.is_alive()) {
            l_Skeleton = g_Instance.get_mesh().get_skeleton();
          }
          if (!l_Skeleton.is_alive() ||
              l_Skeleton.get_state() !=
                  Renderer::SkeletonState::LOADED) {
            return;
          }

          const float l_Duration = g_Clip.get_duration();
          if (l_Duration <= 0.0f || l_Skeleton.get_bones().empty()) {
            return;
          }

          const float l_TicksPerSecond =
              g_Clip.get_ticks_per_second() > 0.0f
                  ? g_Clip.get_ticks_per_second()
                  : 25.0f;
          g_TestProgress += p_Delta * l_TicksPerSecond;
          g_TestProgress = std::fmod(g_TestProgress, l_Duration);
          if (g_TestProgress < 0.0f) {
            g_TestProgress += l_Duration;
          }

          g_Pose.set_skeleton(l_Skeleton);
          g_Pose.get_matrices().resize(l_Skeleton.get_bone_count());

          Util::List<Math::Matrix4x4> l_LocalTransforms;
          Util::List<Math::Matrix4x4> l_GlobalTransforms;
          l_LocalTransforms.resize(l_Skeleton.get_bone_count());
          l_GlobalTransforms.resize(l_Skeleton.get_bone_count());

          for (u32 i = 0u; i < l_Skeleton.get_bone_count(); ++i) {
            l_LocalTransforms[i] =
                l_Skeleton.get_bones()[i].local_bind_transform;
          }

          for (Renderer::AnimationChannel &i_Channel :
               g_Clip.get_channels()) {
            if (i_Channel.bone_index >= l_LocalTransforms.size()) {
              continue;
            }

            Math::Vector3 i_BindScale(1.0f);
            Math::Quaternion i_BindRotation(1.0f, 0.0f, 0.0f, 0.0f);
            Math::Vector3 i_BindPosition(0.0f);
            Math::Vector3 i_BindSkew(0.0f);
            Math::Vector4 i_BindPerspective(0.0f);
            glm::decompose(
                l_Skeleton.get_bones()[i_Channel.bone_index]
                    .local_bind_transform,
                i_BindScale, i_BindRotation, i_BindPosition,
                i_BindSkew, i_BindPerspective);
            i_BindRotation = glm::normalize(i_BindRotation);

            const Math::Vector3 i_Position = sample_vector_keys(
                i_Channel.positions, g_TestProgress, i_BindPosition);
            const Math::Quaternion i_Rotation = sample_quat_keys(
                i_Channel.rotations, g_TestProgress, i_BindRotation);
            const Math::Vector3 i_Scale = sample_vector_keys(
                i_Channel.scales, g_TestProgress, i_BindScale);

            Math::Matrix4x4 i_LocalTransform(1.0f);
            i_LocalTransform =
                glm::translate(i_LocalTransform, i_Position);
            i_LocalTransform *= glm::toMat4(i_Rotation);
            i_LocalTransform = glm::scale(i_LocalTransform, i_Scale);

            l_LocalTransforms[i_Channel.bone_index] =
                i_LocalTransform;
          }

          for (u32 i = 0u; i < l_Skeleton.get_bone_count(); ++i) {
            const Renderer::SkeletonBone &i_Bone =
                l_Skeleton.get_bones()[i];

            if (i_Bone.parent_index >= 0) {
              l_GlobalTransforms[i] =
                  l_GlobalTransforms[i_Bone.parent_index] *
                  l_LocalTransforms[i];
            } else {
              l_GlobalTransforms[i] = l_LocalTransforms[i];
            }

            g_Pose.get_matrices()[i] =
                l_GlobalTransforms[i] * i_Bone.inverse_bind_matrix;
          }

          g_Pose.mark_dirty();
        }

        static void
        test_handle_anim(const float p_Delta,
                         Component::MeshRenderer p_MeshRenderer)
        {
          if (p_MeshRenderer.get_mesh().get_state() !=
              Renderer::MeshState::LOADED) {
            return;
          }
          if (!g_Pose.is_alive() && !g_Instance.is_alive()) {
            g_Pose = Renderer::SkinningPose::make(N(TestPose));
            g_Instance = Renderer::SkinningInstance::make(
                N(TestInstance), p_MeshRenderer.get_mesh());
            g_Instance.set_pose(g_Pose);
            g_Clip = Renderer::AnimationClip::find_by_index(1);
            Renderer::ResourceManager::load_skeleton(
                p_MeshRenderer.get_mesh().get_skeleton());
            Renderer::ResourceManager::load_animation_clip(g_Clip);

            p_MeshRenderer.get_render_object().set_skinning_instance(
                g_Instance);
          }

          if (p_MeshRenderer.get_mesh().get_skeleton().get_state() !=
              Renderer::SkeletonState::LOADED) {
            return;
          }
          if (g_Clip.get_state() !=
              Renderer::AnimationClipState::LOADED) {
            return;
          }
          test_posing(p_Delta);
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          Component::MeshRenderer *l_MeshRenderers =
              Component::MeshRenderer::living_instances();

          for (uint32_t i = 0u;
               i < Component::MeshRenderer::living_count(); ++i) {
            Component::MeshRenderer i_MeshRenderer =
                l_MeshRenderers[i];

            Entity i_Entity = i_MeshRenderer.get_entity();

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
              i_MeshRenderer.get_render_object().set_object_id(
                  i_Entity.get_index());
            }

            Renderer::RenderObject i_RenderObject =
                i_MeshRenderer.get_render_object();

            if (!i_RenderObject.is_alive()) {
              continue;
            }

            if (i_MeshRenderer.get_mesh().get_name() == N(hero)) {
              test_handle_anim(p_Delta, i_MeshRenderer);
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
