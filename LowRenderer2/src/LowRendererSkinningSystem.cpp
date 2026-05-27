#include "LowRendererSkinningSystem.h"

#include "LowMath.h"
#include "LowRendererSkeletalRenderObject.h"
#include "LowRendererMeshInstanceNode.h"
#include "LowRendererMeshGeometry.h"
#include "LowRendererSkeleton.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererVulkan.h"
#include "LowRendererSkinningCommand.h"
#include "LowRendererVulkanPipeline.h"
#include "LowRendererDrawCommand.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Renderer {
    namespace SkinningSystem {
      struct SkinningPushConstants
      {
        u32 sourceVertexStart;
        u32 weightsStart;
        u32 posePaletteStart;
        u32 outputVertexStart;
        u32 vertexCount;
        u32 outputBufferIndex;
      };

      struct SkinningPipelineState
      {
        bool initialized = false;
        Vulkan::PipelineLayout pipelineLayout;
        Vulkan::Pipeline pipeline;
      };

      static SkinningPipelineState g_SkinningPipelineState;
      static Util::List<bool> g_NodeUpdateScratch;
      static Util::List<SkinningCommand> g_ExecutableSkinningCommands;

      static void initialize_skinning_pipeline()
      {
        if (g_SkinningPipelineState.initialized) {
          return;
        }

        Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
        l_DescriptorSetLayouts.push_back(
            Vulkan::Global::get_global_descriptor_set_layout());
        l_DescriptorSetLayouts.push_back(
            Vulkan::Global::get_skinning_descriptor_set_layout());

        VkPushConstantRange l_PushConstant{};
        l_PushConstant.offset = 0;
        l_PushConstant.size = sizeof(SkinningPushConstants);
        l_PushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.pSetLayouts = l_DescriptorSetLayouts.data();
        l_LayoutInfo.setLayoutCount = l_DescriptorSetLayouts.size();
        l_LayoutInfo.pPushConstantRanges = &l_PushConstant;
        l_LayoutInfo.pushConstantRangeCount = 1;

        g_SkinningPipelineState.pipelineLayout =
            Vulkan::PipelineUtil::create_layout(N(Skinning),
                                                l_LayoutInfo);

        Vulkan::PipelineUtil::ComputePipelineBuilder l_Builder;
        l_Builder.set_shader("skinning.comp");
        l_Builder.set_pipeline_layout(
            g_SkinningPipelineState.pipelineLayout);
        g_SkinningPipelineState.pipeline =
            l_Builder.register_pipeline();

        g_SkinningPipelineState.initialized = true;
      }

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

      bool evaluate_global_pose_for_skeletal_renderobject(
          SkeletalRenderObject p_SRO, Skeleton p_Skeleton,
          const Util::List<Math::Matrix4x4> &p_GlobalPose)
      {
        if (!p_SRO.is_alive() || !p_Skeleton.is_alive()) {
          return false;
        }

        Mesh l_Mesh = p_SRO.get_mesh();
        if (!l_Mesh.is_alive()) {
          return false;
        }

        MeshGeometry l_MeshGeometry = l_Mesh.get_geometry();
        if (!l_MeshGeometry.is_alive()) {
          return false;
        }

        Util::List<MeshNode> &l_MeshNodes =
            l_MeshGeometry.get_nodes();
        Util::List<MeshInstanceNode> &l_InstanceNodes =
            p_SRO.get_nodes();

        if (l_MeshNodes.size() != l_InstanceNodes.size()) {
          return false;
        }

        Util::List<SkeletonBone> &l_Bones = p_Skeleton.get_bones();
        if (l_Bones.size() != p_GlobalPose.size()) {
          return false;
        }

        const Math::Matrix4x4 &l_SROWorld =
            p_SRO.get_world_transform();

        const u32 l_NodeCount = (u32)l_MeshNodes.size();
        Util::List<bool> &l_Updated = g_NodeUpdateScratch;
        l_Updated.resize(l_NodeCount);
        for (u32 i = 0u; i < l_NodeCount; ++i) {
          l_Updated[i] = false;
        }

        for (u32 i = 0u; i < l_NodeCount; ++i) {
          const MeshNode &i_MeshNode = l_MeshNodes[i];
          MeshInstanceNode i_InstanceNode = l_InstanceNodes[i];

          if (!i_InstanceNode.is_alive()) {
            continue;
          }

          Math::Matrix4x4 i_WorldTransform;

          DrawCommand i_DrawCommand =
              i_InstanceNode.get_draw_command();

          if (i_MeshNode.bone_index >= 0) {
            const u32 i_BoneIdx = (u32)i_MeshNode.bone_index;
            if (i_BoneIdx < (u32)p_GlobalPose.size()) {
              i_WorldTransform = l_SROWorld * p_GlobalPose[i_BoneIdx];
              l_Updated[i] = true;
            }
          } else if (i_MeshNode.parent_index >= 0) {
            const u32 i_ParentIdx = (u32)i_MeshNode.parent_index;
            if (l_Updated[i_ParentIdx]) {
              MeshInstanceNode i_ParentNode =
                  l_InstanceNodes[i_ParentIdx];
              if (i_ParentNode.is_alive()) {
                i_WorldTransform =
                    i_ParentNode.get_world_transform() *
                    i_MeshNode.local_transform;
                l_Updated[i] = true;
              }
            }
          }

          if (!l_Updated[i]) {
            continue;
          }

          i_InstanceNode.set_world_transform(i_WorldTransform);

          if (i_DrawCommand.is_alive() &&
              i_DrawCommand.get_submesh().get_bone_weight_count() ==
                  0u) {
            i_DrawCommand.set_world_transform(i_WorldTransform);
          }
        }

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
            i_Pose.set_uploaded(false);

            continue;
          }

          if (!i_Pose.is_uploaded() && i_Pose.is_referenced()) {
            u32 l_PoseStart = 0;
            LOW_ASSERT(
                Vulkan::Global::get_pose_palette_buffer().reserve(
                    i_Skeleton.get_bone_count(), &l_PoseStart),
                "Failed to allocate pose buffer space.");

            i_Pose.set_pose_palette_offset(l_PoseStart);
            i_Pose.set_uploaded(true);
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

          SkeletalRenderObject i_SRO(
              i_Instance.get_render_object_id());
          if (!i_SRO.is_alive()) {
            continue;
          }

          if (i_SRO.get_draw_commands().empty()) {
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

          bool l_AttachedSkinningCommand = false;
          for (auto sit = i_GpuMesh.get_submeshes().begin();
               sit != i_GpuMesh.get_submeshes().end(); ++sit) {
            GpuSubmesh i_GpuSubmesh = *sit;

            if (i_GpuSubmesh.get_bone_weight_count() == 0u) {
              continue;
            }

            SkinningCommand i_Command =
                SkinningCommand::make(i_Instance, i_GpuSubmesh);

            for (DrawCommand i_DrawCommand :
                 i_SRO.get_draw_commands()) {
              if (i_DrawCommand.get_submesh() == i_GpuSubmesh) {
                i_DrawCommand.set_skinning_command(i_Command);
                l_AttachedSkinningCommand = true;

                break;
              }
            }
          }

          if (l_AttachedSkinningCommand) {
            i_SRO.mark_dirty();
          }
        }

        SkinningInstance::ms_NeedInitialization.clear();

        for (SkinningInstance i_Instance : l_Reschedules) {
          i_Instance.mark_needs_initialization();
        }
      }

      static void execute_skinning_commands(const float p_Delta)
      {
        g_ExecutableSkinningCommands.clear();
        for (SkinningInstance i_Instance :
             SkinningInstance::ms_LivingInstances) {
          if (!i_Instance.is_alive() ||
              !i_Instance.get_pose().is_alive()) {
            continue;
          }

          for (SkinningCommand i_Command :
               i_Instance.get_skinning_commands()) {
            if (i_Command.is_alive() &&
                i_Command.get_submesh().is_alive() &&
                i_Command.get_vertex_count() > 0u) {
              g_ExecutableSkinningCommands.push_back(i_Command);
            }
          }
        }

        if (g_ExecutableSkinningCommands.empty()) {
          return;
        }

        initialize_skinning_pipeline();

        VkCommandBuffer l_Cmd =
            Vulkan::Global::get_current_command_buffer();

        {
          VkBufferMemoryBarrier l_Barriers[3]{};

          l_Barriers[0].sType =
              VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          l_Barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          l_Barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
          l_Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[0].buffer =
              Vulkan::Global::get_mesh_vertex_buffer()
                  .m_Buffer.buffer;
          l_Barriers[0].offset = 0;
          l_Barriers[0].size = VK_WHOLE_SIZE;

          l_Barriers[1].sType =
              VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          l_Barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          l_Barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
          l_Barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[1].buffer =
              Vulkan::Global::get_mesh_bone_weight_buffer()
                  .m_Buffer.buffer;
          l_Barriers[1].offset = 0;
          l_Barriers[1].size = VK_WHOLE_SIZE;

          l_Barriers[2].sType =
              VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          l_Barriers[2].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          l_Barriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
          l_Barriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barriers[2].buffer =
              Vulkan::Global::get_pose_palette_buffer()
                  .m_Buffer.buffer;
          l_Barriers[2].offset = 0;
          l_Barriers[2].size = VK_WHOLE_SIZE;

          vkCmdPipelineBarrier(l_Cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               0, 0, nullptr, 3, l_Barriers, 0,
                               nullptr);
        }

        vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          g_SkinningPipelineState.pipeline.get());

        {
          VkDescriptorSet l_GlobalSet =
              Vulkan::Global::get_global_descriptor_set();
          vkCmdBindDescriptorSets(
              l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
              g_SkinningPipelineState.pipelineLayout.get(), 0, 1,
              &l_GlobalSet, 0, nullptr);
        }

        {
          VkDescriptorSet l_SkinningSet =
              Vulkan::Global::get_skinning_descriptor_set();
          vkCmdBindDescriptorSets(
              l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
              g_SkinningPipelineState.pipelineLayout.get(), 1, 1,
              &l_SkinningSet, 0, nullptr);
        }

        for (SkinningCommand i_Command :
             g_ExecutableSkinningCommands) {
          SkinningInstance i_Instance = i_Command.get_instance();
          SkinningPose i_Pose = i_Instance.get_pose();

          GpuSubmesh i_Submesh = i_Command.get_submesh();

          SkinningPushConstants l_PushConstants{};
          l_PushConstants.sourceVertexStart =
              i_Submesh.get_vertex_start();
          l_PushConstants.weightsStart =
              i_Command.get_weights_start();
          l_PushConstants.posePaletteStart =
              i_Pose.get_pose_palette_offset();
          l_PushConstants.outputVertexStart =
              i_Command.get_skinned_vertex_start();
          l_PushConstants.vertexCount = i_Command.get_vertex_count();
          const u32 l_OutputBufferIndex =
              Vulkan::Global::get_skinned_vertex_output_buffer()
                  .get_current_buffer_index();
          l_PushConstants.outputBufferIndex = l_OutputBufferIndex;

          i_Command.set_active_vertex_buffer(
              l_OutputBufferIndex == 0u ? VertexBuffer::SkinnedA
                                        : VertexBuffer::SkinnedB);

          vkCmdPushConstants(
              l_Cmd, g_SkinningPipelineState.pipelineLayout.get(),
              VK_SHADER_STAGE_COMPUTE_BIT, 0,
              sizeof(SkinningPushConstants), &l_PushConstants);

          const u32 l_WorkgroupCount =
              (i_Command.get_vertex_count() + 63u) / 64u;
          vkCmdDispatch(l_Cmd, l_WorkgroupCount, 1, 1);
        }

        {
          VkBufferMemoryBarrier l_Barrier{};
          l_Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          l_Barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
          l_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
          l_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          l_Barrier.buffer =
              Vulkan::Global::get_skinned_vertex_output_buffer()
                  .current()
                  .buffer;
          l_Barrier.offset = 0;
          l_Barrier.size = VK_WHOLE_SIZE;

          vkCmdPipelineBarrier(l_Cmd,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
                               0, nullptr, 1, &l_Barrier, 0, nullptr);
        }
      }

      void tick(const float p_Delta)
      {
        poses_tick(p_Delta);

        skinning_instances_tick(p_Delta);

        execute_skinning_commands(p_Delta);
      }
    } // namespace SkinningSystem
  } // namespace Renderer
} // namespace Low
