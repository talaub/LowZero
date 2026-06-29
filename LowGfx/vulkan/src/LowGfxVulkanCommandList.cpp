#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"

#include "LowGfxAssert.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      static const VulkanQueue &
      get_queue(const VulkanContextState &p_State,
                QueueRole p_QueueRole)
      {
        switch (p_QueueRole) {
        case QueueRole::Graphics:
          return p_State.graphics_queue;
        case QueueRole::Compute:
          return p_State.compute_queue;
        case QueueRole::Transfer:
          return p_State.transfer_queue;
        }

        return p_State.graphics_queue;
      }

      static VulkanFrameCommandPool &
      get_frame_command_pool(FrameState &p_Frame,
                             QueueRole p_QueueRole)
      {
        switch (p_QueueRole) {
        case QueueRole::Graphics:
          return p_Frame.graphics;
        case QueueRole::Compute:
          return p_Frame.compute;
        case QueueRole::Transfer:
          return p_Frame.transfer;
        }

        return p_Frame.graphics;
      }

      VkCommandBuffer
      acquire_frame_command_buffer(VulkanContextState &p_State,
                                   VulkanFrameCommandPool &p_Pool)
      {
        GFX_ASSERT(
            p_Pool.pool != VK_NULL_HANDLE,
            "Cannot acquire Vulkan command buffer without pool");

        if (p_Pool.next_command_buffer <
            p_Pool.command_buffers.size()) {
          VkCommandBuffer l_CommandBuffer =
              p_Pool.command_buffers[p_Pool.next_command_buffer];
          ++p_Pool.next_command_buffer;
          return l_CommandBuffer;
        }

        VkCommandBufferAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocateInfo.commandPool = p_Pool.pool;
        l_AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocateInfo.commandBufferCount = 1;

        VkCommandBuffer l_CommandBuffer = VK_NULL_HANDLE;
        VkResult l_AllocateResult = vkAllocateCommandBuffers(
            p_State.device, &l_AllocateInfo, &l_CommandBuffer);
        GFX_ASSERT(l_AllocateResult == VK_SUCCESS,
                   "Failed to allocate Vulkan frame command buffer");

        p_Pool.command_buffers.push_back(l_CommandBuffer);
        ++p_Pool.next_command_buffer;
        return l_CommandBuffer;
      }

      struct VulkanImageStateAccess
      {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;
      };

      static VulkanImageStateAccess
      to_vulkan_image_state_access(ImageState p_State)
      {
        switch (p_State) {
        case ImageState::Undefined:
          return {VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
        case ImageState::ShaderRead:
          return {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
                      VK_ACCESS_2_SHADER_STORAGE_READ_BIT};
        case ImageState::ShaderWrite:
          return {VK_IMAGE_LAYOUT_GENERAL,
                  VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
                      VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT};
        case ImageState::ColorAttachment:
          return {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                  VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT};
        case ImageState::DepthWrite:
          return {VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                  VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                      VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                      VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
        case ImageState::DepthRead:
          return {VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
                  VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                      VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                      VK_ACCESS_2_SHADER_SAMPLED_READ_BIT};
        case ImageState::TransferSrc:
          return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                  VK_ACCESS_2_TRANSFER_READ_BIT};
        case ImageState::TransferDst:
          return {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                  VK_ACCESS_2_TRANSFER_WRITE_BIT};
        case ImageState::Present:
          return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
        }

        GFX_ASSERT(false, "Unsupported LowGfx image state");
        return {VK_IMAGE_LAYOUT_UNDEFINED,
                VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
      }

      static VkImageAspectFlags
      to_vulkan_aspect(ImageAspect p_Aspect)
      {
        switch (p_Aspect) {
        case ImageAspect::Color:
          return VK_IMAGE_ASPECT_COLOR_BIT;
        case ImageAspect::Depth:
          return VK_IMAGE_ASPECT_DEPTH_BIT;
        case ImageAspect::Stencil:
          return VK_IMAGE_ASPECT_STENCIL_BIT;
        case ImageAspect::DepthStencil:
          return VK_IMAGE_ASPECT_DEPTH_BIT |
                 VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        GFX_ASSERT(false, "Unsupported LowGfx image aspect");
        return VK_IMAGE_ASPECT_COLOR_BIT;
      }

      Detail::BackendCommandList
      request_command_list(Detail::ContextImpl &p_Context,
                           const FrameContext &p_Frame,
                           QueueRole p_QueueRole)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot create Vulkan command list "
                            "without context state");
        GFX_ASSERT(
            l_State->device != VK_NULL_HANDLE,
            "Cannot create Vulkan command list without device");

        const VulkanQueue &l_Queue = get_queue(*l_State, p_QueueRole);
        GFX_ASSERT(l_Queue.queue != VK_NULL_HANDLE,
                   "Cannot create Vulkan command list without queue");
        GFX_ASSERT(
            l_Queue.family_index != LOW_UINT32_MAX,
            "Cannot create Vulkan command list without queue family");

        VulkanCommandListState *l_CommandListState =
            new VulkanCommandListState();
        l_CommandListState->queue_role = p_QueueRole;
        l_CommandListState->queue_family_index = l_Queue.family_index;
        l_CommandListState->owns_command_pool = false;

        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);
        GFX_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot allocate frame command list with invalid "
                   "frame index");

        VulkanFrameCommandPool &l_FramePool = get_frame_command_pool(
            l_State->frames[l_FrameIndex], p_QueueRole);
        l_CommandListState->command_pool = l_FramePool.pool;
        l_CommandListState->command_buffer =
            acquire_frame_command_buffer(*l_State, l_FramePool);

        Detail::BackendCommandList l_CommandList;
        l_CommandList.queue_role = p_QueueRole;
        l_CommandList.state = CommandListState::Initial;
        l_CommandList.backend_state = l_CommandListState;
        return l_CommandList;
      }

      Detail::BackendCommandList
      request_immediate_command_list(Detail::ContextImpl &p_Context,
                                     QueueRole p_QueueRole)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot create Vulkan command list "
                            "without context state");
        GFX_ASSERT(
            l_State->device != VK_NULL_HANDLE,
            "Cannot create Vulkan command list without device");

        const VulkanQueue &l_Queue = get_queue(*l_State, p_QueueRole);
        GFX_ASSERT(l_Queue.queue != VK_NULL_HANDLE,
                   "Cannot create Vulkan command list without queue");
        GFX_ASSERT(
            l_Queue.family_index != LOW_UINT32_MAX,
            "Cannot create Vulkan command list without queue family");

        VulkanCommandListState *l_CommandListState =
            new VulkanCommandListState();
        l_CommandListState->queue_role = p_QueueRole;
        l_CommandListState->queue_family_index = l_Queue.family_index;
        l_CommandListState->owns_command_pool = true;

        VkCommandPoolCreateInfo l_CommandPoolInfo{};
        l_CommandPoolInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        l_CommandPoolInfo.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        l_CommandPoolInfo.queueFamilyIndex = l_Queue.family_index;

        VkResult l_PoolResult = vkCreateCommandPool(
            l_State->device, &l_CommandPoolInfo, nullptr,
            &l_CommandListState->command_pool);
        GFX_ASSERT(l_PoolResult == VK_SUCCESS,
                   "Failed to create Vulkan immediate command pool");

        VkCommandBufferAllocateInfo l_AllocateInfo{};
        l_AllocateInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        l_AllocateInfo.commandPool = l_CommandListState->command_pool;
        l_AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        l_AllocateInfo.commandBufferCount = 1;

        VkResult l_AllocateResult = vkAllocateCommandBuffers(
            l_State->device, &l_AllocateInfo,
            &l_CommandListState->command_buffer);
        if (l_AllocateResult != VK_SUCCESS) {
          vkDestroyCommandPool(l_State->device,
                               l_CommandListState->command_pool,
                               nullptr);
          delete l_CommandListState;
          GFX_ASSERT(false,
                     "Failed to allocate Vulkan command buffer");
        }

        Detail::BackendCommandList l_CommandList;
        l_CommandList.queue_role = p_QueueRole;
        l_CommandList.state = CommandListState::Initial;
        l_CommandList.backend_state = l_CommandListState;
        return l_CommandList;
      }

      void
      destroy_command_list(Detail::ContextImpl &p_Context,
                           Detail::BackendCommandList &p_CommandList)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot destroy Vulkan command list "
                            "without context state");

        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        if (l_CommandListState) {
          if (l_State->device != VK_NULL_HANDLE &&
              l_CommandListState->command_pool != VK_NULL_HANDLE &&
              l_CommandListState->owns_command_pool) {
            vkDestroyCommandPool(l_State->device,
                                 l_CommandListState->command_pool,
                                 nullptr);
          }

          delete l_CommandListState;
        }

        p_CommandList.queue_role = QueueRole::Graphics;
        p_CommandList.state = CommandListState::Initial;
        p_CommandList.backend_state = nullptr;
      }

      void
      begin_command_list(Detail::ContextImpl &p_Context,
                         Detail::BackendCommandList &p_CommandList)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot begin Vulkan command list "
                            "without context state");

        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        GFX_ASSERT(l_CommandListState,
                   "Cannot begin Vulkan command list "
                   "without command list state");

        VkCommandBufferBeginInfo l_Info = {};
        l_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        l_Info.pNext = nullptr;

        l_Info.pInheritanceInfo = nullptr;
        l_Info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult l_Result = vkBeginCommandBuffer(
            l_CommandListState->command_buffer, &l_Info);
        GFX_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to begin Vulkan command buffer");
      }

      void end_command_list(Detail::ContextImpl &p_Context,
                            Detail::BackendCommandList &p_CommandList)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot end Vulkan command list "
                            "without context state");

        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        GFX_ASSERT(l_CommandListState,
                   "Cannot end Vulkan command list "
                   "without command list state");

        VkResult l_Result =
            vkEndCommandBuffer(l_CommandListState->command_buffer);
        GFX_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to end Vulkan command buffer");
      }

      void
      submit_command_list(Detail::ContextImpl &p_Context,
                          const FrameContext &p_Frame,
                          Detail::BackendCommandList &p_CommandList)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot submit Vulkan command list "
                            "without context state");

        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);
        GFX_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot submit Vulkan command list with invalid "
                   "frame index");
        GFX_ASSERT(p_CommandList.queue_role == QueueRole::Graphics,
                   "Vulkan frame submission currently supports only "
                   "graphics command lists");

        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        GFX_ASSERT(l_CommandListState,
                   "Cannot submit Vulkan command list without command "
                   "list state");
        GFX_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot submit Vulkan command list without command "
                   "buffer");

        FrameState &l_Frame = l_State->frames[l_FrameIndex];
        l_Frame.pending_graphics_submits.push_back(
            l_CommandListState->command_buffer);
      }

      void barrier_image_command_list(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          const ImageBarrier &p_Barrier)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(
            l_State,
            "Cannot record image barrier to Vulkan command list "
            "without context state");
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        GFX_ASSERT(
            l_CommandListState,
            "Cannot record image barrier to Vulkan command list "
            "without command list state");
        GFX_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot record image barrier without Vulkan "
                   "command buffer");

        Detail::BackendImage *l_Image =
            p_Context.images.get(p_Barrier.image);
        GFX_ASSERT(l_Image,
                   "Cannot record Vulkan image barrier for invalid "
                   "image");

        VulkanImageState *l_ImageState =
            static_cast<VulkanImageState *>(l_Image->backend_state);
        GFX_ASSERT(l_ImageState && l_ImageState->image != VK_NULL_HANDLE,
                   "Cannot record Vulkan image barrier without image");

        const VulkanImageStateAccess l_Source =
            to_vulkan_image_state_access(p_Barrier.old_state);
        const VulkanImageStateAccess l_Destination =
            to_vulkan_image_state_access(p_Barrier.new_state);

        VkImageMemoryBarrier2 l_ImageBarrier{};
        l_ImageBarrier.sType =
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        l_ImageBarrier.srcStageMask = l_Source.stage;
        l_ImageBarrier.srcAccessMask = l_Source.access;
        l_ImageBarrier.dstStageMask = l_Destination.stage;
        l_ImageBarrier.dstAccessMask = l_Destination.access;
        l_ImageBarrier.oldLayout = l_Source.layout;
        l_ImageBarrier.newLayout = l_Destination.layout;
        l_ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        l_ImageBarrier.image = l_ImageState->image;

        VkImageSubresourceRange l_SubImage{};
        l_SubImage.aspectMask = to_vulkan_aspect(p_Barrier.aspect);
        l_SubImage.baseMipLevel = p_Barrier.base_mip;
        l_SubImage.levelCount = p_Barrier.mip_count;
        l_SubImage.baseArrayLayer = p_Barrier.base_layer;
        l_SubImage.layerCount = p_Barrier.layer_count;

        l_ImageBarrier.subresourceRange = l_SubImage;

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.imageMemoryBarrierCount = 1;
        l_DependencyInfo.pImageMemoryBarriers = &l_ImageBarrier;

        vkCmdPipelineBarrier2(l_CommandListState->command_buffer,
                              &l_DependencyInfo);

        p_Context.swapchains.for_each(
            [&p_Barrier, &l_Destination](
                Detail::BackendSwapchain &p_Swapchain) {
              VulkanSwapchainState *i_Swapchain =
                  static_cast<VulkanSwapchainState *>(
                      p_Swapchain.backend_state);
              if (!i_Swapchain) {
                return;
              }

              for (VulkanSwapchainImageState &i_Image :
                   i_Swapchain->images) {
                if (i_Image.image_token == p_Barrier.image) {
                  i_Image.layout = l_Destination.layout;
                  return;
                }
              }
            });
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
