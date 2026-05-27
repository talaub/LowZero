#include "LowCoreTransformSystem.h"

#include "LowRendererAnimationClipState.h"
#include "LowRendererMeshState.h"
#include "LowRendererMeshType.h"
#include "LowRendererSkeletalRenderObject.h"
#include "LowRendererSkeletonState.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererSkinningSystem.h"
#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreMeshRenderer.h"
#include "LowCoreTransform.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreAnimator.h"

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
        static Util::List<Math::Matrix4x4> g_LocalPoseScratch;
        static Util::List<Math::Matrix4x4> g_GlobalPoseScratch;

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

        static void extract_transform_components(
            const Math::Matrix4x4 &p_Transform,
            Math::Vector3 &p_Position, Math::Quaternion &p_Rotation,
            Math::Vector3 &p_Scale)
        {
          Math::Vector3 l_Skew;
          Math::Vector4 l_Perspective;
          glm::decompose(p_Transform, p_Scale, p_Rotation, p_Position,
                         l_Skew, l_Perspective);
          p_Rotation = glm::normalize(p_Rotation);
        }

        static void
        tick_animator(const float p_Delta,
                      Component::MeshRenderer p_MeshRenderer)
        {
          Entity l_Entity = p_MeshRenderer.get_entity();

          Component::Transform l_Transform = l_Entity.get_transform();

          Component::Animator l_Animator =
              l_Entity.get_component(Component::Animator::type_id());

          if (p_MeshRenderer.get_render_object().is_alive()) {
            p_MeshRenderer.get_render_object().destroy();
          }

          if (p_MeshRenderer.is_dirty()) {
            if (l_Animator.get_render_object().is_alive()) {
              l_Animator.get_render_object().destroy();
            }
            if (l_Animator.get_pose().is_alive()) {
              // TODO: make poses reusable at some point
              l_Animator.get_pose().destroy();
            }
            if (l_Animator.get_skinning_instance().is_alive()) {
              // TODO: Potentially reuse skinning instances across
              // entities
              l_Animator.get_skinning_instance().destroy();
            }

            l_Animator.set_skeleton(Util::Handle::DEAD);
            l_Animator.set_animation_progress(0);
          }

          p_MeshRenderer.set_dirty(false);

          if (!l_Animator.get_render_object().is_alive() &&
              p_MeshRenderer.get_mesh().is_alive() &&
              p_MeshRenderer.get_mesh().get_skeleton().is_alive()) {
            l_Animator.set_render_object(
                Renderer::SkeletalRenderObject::make(
                    Renderer::get_global_renderscene(),
                    p_MeshRenderer.get_mesh()));
            l_Animator.get_render_object().set_object_id(
                l_Entity.get_index());

            l_Animator.set_animation_progress(0);
            l_Animator.set_skeleton(
                p_MeshRenderer.get_mesh().get_skeleton());

            l_Animator.set_pose(
                Renderer::SkinningPose::make(l_Entity.get_name()));
            l_Animator.get_pose().set_skeleton(
                l_Animator.get_skeleton());

            l_Animator.set_skinning_instance(
                Renderer::SkinningInstance::make(
                    l_Entity.get_name(), p_MeshRenderer.get_mesh()));
            l_Animator.get_skinning_instance().set_pose(
                l_Animator.get_pose());
            l_Animator.get_skinning_instance().set_render_object_id(
                l_Animator.get_render_object().get_id());
            l_Animator.get_render_object().set_skinning_instance(
                l_Animator.get_skinning_instance());

            {
              // TODO: THIS IS TEST CODE
              l_Animator.set_active_clip(
                  Renderer::AnimationClip::find_by_index(1));
            }
          }

          Renderer::SkeletalRenderObject l_RenderObject =
              l_Animator.get_render_object();

          if (!l_RenderObject.is_alive()) {
            return;
          }

          Renderer::Material l_Material =
              Renderer::get_default_material_texture();

          if (p_MeshRenderer.get_material().is_alive()) {
            l_Material = p_MeshRenderer.get_material();
          }

          l_RenderObject.set_material(l_Material);
          l_RenderObject.set_world_transform(
              l_Transform.get_world_matrix());

          if (!l_Animator.get_active_clip().is_alive()) {
            return;
          }
          if (l_Animator.get_active_clip().get_state() !=
              Renderer::AnimationClipState::LOADED) {
            return;
          }
          if (!l_Animator.get_skeleton().is_alive()) {
            return;
          }
          if (l_Animator.get_skeleton().get_state() !=
              Renderer::SkeletonState::LOADED) {
            return;
          }

          Renderer::AnimationClip l_Clip =
              l_Animator.get_active_clip();
          Renderer::Skeleton l_Skeleton = l_Animator.get_skeleton();
          Renderer::SkinningPose l_Pose = l_Animator.get_pose();

          const float l_NewProgress =
              std::fmod(l_Animator.get_animation_progress() +
                            p_Delta * l_Clip.get_ticks_per_second(),
                        l_Clip.get_duration());
          l_Animator.set_animation_progress(l_NewProgress);

          Util::List<Renderer::SkeletonBone> &l_Bones =
              l_Skeleton.get_bones();
          const u32 l_BoneCount = (u32)l_Bones.size();

          Util::List<Math::Matrix4x4> &l_PoseMatrices =
              l_Pose.get_matrices();
          if (l_PoseMatrices.size() != l_BoneCount) {
            l_PoseMatrices.resize(l_BoneCount);
          }

          Util::List<Math::Matrix4x4> &l_LocalPose =
              g_LocalPoseScratch;
          l_LocalPose.resize(l_BoneCount);
          for (u32 i = 0u; i < l_BoneCount; ++i) {
            l_LocalPose[i] = l_Bones[i].local_bind_transform;
          }

          for (const Renderer::AnimationChannel &i_Channel :
               l_Clip.get_channels()) {
            if (i_Channel.bone_index >= l_BoneCount) {
              continue;
            }

            Math::Vector3 i_DefaultPos(0.f);
            Math::Quaternion i_DefaultRot(1.f, 0.f, 0.f, 0.f);
            Math::Vector3 i_DefaultScale(1.f);
            if (i_Channel.positions.empty() ||
                i_Channel.rotations.empty() ||
                i_Channel.scales.empty()) {
              extract_transform_components(
                  l_Bones[i_Channel.bone_index].local_bind_transform,
                  i_DefaultPos, i_DefaultRot, i_DefaultScale);
            }

            const Math::Vector3 i_Pos = sample_vector_keys(
                i_Channel.positions, l_NewProgress, i_DefaultPos);
            const Math::Quaternion i_Rot = sample_quat_keys(
                i_Channel.rotations, l_NewProgress, i_DefaultRot);
            const Math::Vector3 i_Scale = sample_vector_keys(
                i_Channel.scales, l_NewProgress, i_DefaultScale);

            l_LocalPose[i_Channel.bone_index] =
                glm::translate(Math::Matrix4x4(1.f), i_Pos) *
                glm::mat4_cast(i_Rot) *
                glm::scale(Math::Matrix4x4(1.f), i_Scale);
          }

          Util::List<Math::Matrix4x4> &l_GlobalPose =
              g_GlobalPoseScratch;
          l_GlobalPose.resize(l_BoneCount);
          for (u32 i = 0u; i < l_BoneCount; ++i) {
            if (l_Bones[i].parent_index < 0) {
              l_GlobalPose[i] = l_LocalPose[i];
            } else {
              l_GlobalPose[i] =
                  l_GlobalPose[(u32)l_Bones[i].parent_index] *
                  l_LocalPose[i];
            }
            l_PoseMatrices[i] =
                l_GlobalPose[i] * l_Bones[i].inverse_bind_matrix;
          }

          Renderer::SkinningSystem::
              evaluate_global_pose_for_skeletal_renderobject(
                  l_RenderObject, l_Skeleton, l_GlobalPose);

          l_Pose.mark_dirty();
        }

        static void
        tick_mesh_renderer(const float p_Delta,
                           Component::MeshRenderer p_MeshRenderer)
        {
          Entity l_Entity = p_MeshRenderer.get_entity();

          Component::Transform l_Transform = l_Entity.get_transform();

          if (p_MeshRenderer.is_dirty()) {
            if (p_MeshRenderer.get_render_object().is_alive()) {
              p_MeshRenderer.get_render_object().destroy();
            }
          }

          p_MeshRenderer.set_dirty(false);

          if (!p_MeshRenderer.get_render_object().is_alive() &&
              p_MeshRenderer.get_mesh().is_alive()) {
            p_MeshRenderer.set_render_object(
                Renderer::RenderObject::make(
                    Renderer::get_global_renderscene(),
                    p_MeshRenderer.get_mesh()));
            p_MeshRenderer.get_render_object().set_object_id(
                l_Entity.get_index());
          }

          Renderer::RenderObject l_RenderObject =
              p_MeshRenderer.get_render_object();

          if (!l_RenderObject.is_alive()) {
            return;
          }

          Renderer::Material l_Material =
              Renderer::get_default_material_texture();

          if (p_MeshRenderer.get_material().is_alive()) {
            l_Material = p_MeshRenderer.get_material();
          }

          l_RenderObject.set_material(l_Material);
          l_RenderObject.set_world_transform(
              l_Transform.get_world_matrix());
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

            if (i_MeshRenderer.get_mesh().is_alive() &&
                i_MeshRenderer.get_mesh().get_type() ==
                    Renderer::MeshType::SKELETAL &&
                i_Entity.has_component(
                    Component::Animator::type_id())) {
              tick_animator(p_Delta, i_MeshRenderer);
            } else {
              tick_mesh_renderer(p_Delta, i_MeshRenderer);
            }
          }
        }
      } // namespace MeshRenderer
    } // namespace System
  } // namespace Core
} // namespace Low
