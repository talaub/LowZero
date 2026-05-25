#include "LowRendererSkinningSystem.h"

#include "LowMath.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererVulkan.h"
#include "LowRendererSkinningCommand.h"

namespace Low {
  namespace Renderer {
    namespace SkinningSystem {
      static bool upload_pose(SkinningPose p_Pose)
      {
        const size_t l_NeededUploadSize =
            sizeof(Math::Matrix4x4) * p_Pose.get_matrices().size();

        size_t l_StagingOffset = 0;
        const u64 l_FrameUploadSpace =
            Vulkan::Global::get_current_frame_staging_buffer()
                .request_space(l_NeededUploadSize, &l_StagingOffset);

        if (l_FrameUploadSpace < l_NeededUploadSize) {
          // We don't have enough space on the staging buffer to
          // upload this pose
          return false;
        }

        LOW_ASSERT(
            Vulkan::Global::get_current_frame_staging_buffer().write(
                p_Pose.get_matrices().data(), l_NeededUploadSize,
                l_StagingOffset),
            "Failed to write pose data to staging buffer");

        VkBufferCopy l_CopyRegion;
        l_CopyRegion.srcOffset = l_StagingOffset;
        l_CopyRegion.dstOffset = p_Pose.get_pose_palette_offset() *
                                 sizeof(Math::Matrix4x4);
        l_CopyRegion.size = l_FrameUploadSpace;

        vkCmdCopyBuffer(
            Vulkan::Global::get_current_command_buffer(),
            Vulkan::Global::get_current_frame_staging_buffer()
                .buffer.buffer,
            Vulkan::Global::get_pose_palette_buffer().m_Buffer.buffer,
            1, &l_CopyRegion);

        return true;
      }

      static void poses_tick(const float p_Delta)
      {
        Util::Set<SkinningPose> l_Reschedules;

        for (SkinningPose i_Pose : SkinningPose::ms_Dirty) {
          if (!i_Pose.is_alive()) {
            continue;
          }
          if (!i_Pose.get_skeleton().is_alive()) {
            i_Pose.set_dirty(false);
            continue;
          }

          Skeleton i_Skeleton = i_Pose.get_skeleton();

          if (i_Skeleton.get_bone_count() !=
              i_Pose.get_matrices().size()) {
            i_Pose.set_dirty(false);
            continue;
          }

          if (i_Pose.is_uploaded() && !i_Pose.is_referenced()) {
            i_Pose.set_dirty(false);

            // Not used anymore, so we free it
            Vulkan::Global::get_pose_palette_buffer().free(
                i_Pose.get_pose_palette_offset(),
                i_Pose.get_matrices().size());

            continue;
          }

          if (!i_Pose.is_uploaded() && i_Pose.is_referenced()) {
            u32 l_PoseStart = 0;
            LOW_ASSERT(
                Vulkan::Global::get_pose_palette_buffer().reserve(
                    i_Skeleton.get_bone_count(), &l_PoseStart),
                "Failed to allocate pose buffer space.");

            i_Pose.set_pose_palette_offset(l_PoseStart);
          }

          if (!upload_pose(i_Pose)) {
            l_Reschedules.insert(i_Pose);
          }

          i_Pose.set_dirty(false);
        }

        SkinningPose::ms_Dirty.clear();

        for (SkinningPose i_Pose : l_Reschedules) {
          i_Pose.mark_dirty();
        }
      }

      static void skinning_instances_tick(const float p_Delta)
      {
        Util::Set<SkinningInstance> l_Reschedules;

        for (SkinningInstance i_Instance :
             SkinningInstance::ms_NeedInitialization) {
          Mesh i_Mesh = i_Instance.get_mesh();

          if (!i_Mesh.is_alive()) {
            LOW_LOG_WARN << "Wanted to initialize skinning instance "
                            "with dead mesh. Will destroy instance."
                         << LOW_LOG_END;
            i_Instance.destroy();
            continue;
          }

          if (i_Mesh.get_state() != MeshState::LOADED) {
            l_Reschedules.insert(i_Instance);
            continue;
          }

          for (SkinningCommand i_Command :
               i_Instance.get_skinning_commands()) {
            if (i_Command.is_alive()) {
              i_Command.destroy();
            }
          }

          i_Instance.get_skinning_commands().clear();

          if (!i_Instance.get_pose().is_alive()) {
            continue;
          }

          GpuMesh i_GpuMesh = i_Mesh.get_gpu();

          for (auto sit = i_GpuMesh.get_submeshes().begin();
               sit != i_GpuMesh.get_submeshes().end(); ++sit) {
            GpuSubmesh i_GpuSubmesh = *sit;

            SkinningCommand::make(i_Instance, i_GpuSubmesh);
          }
        }

        SkinningInstance::ms_NeedInitialization.clear();

        for (SkinningInstance i_Instance : l_Reschedules) {
          i_Instance.mark_needs_initialization();
        }
      }

      void tick(const float p_Delta)
      {
        poses_tick(p_Delta);

        skinning_instances_tick(p_Delta);
      }
    } // namespace SkinningSystem
  } // namespace Renderer
} // namespace Low
