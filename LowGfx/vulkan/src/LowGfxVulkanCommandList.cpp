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

      struct VulkanBufferStateAccess
      {
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

      static VulkanBufferStateAccess
      to_vulkan_buffer_state_access(BufferState p_State)
      {
        switch (p_State) {
        case BufferState::Undefined:
          return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
        case BufferState::TransferSrc:
          return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                  VK_ACCESS_2_TRANSFER_READ_BIT};
        case BufferState::TransferDst:
          return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                  VK_ACCESS_2_TRANSFER_WRITE_BIT};
        case BufferState::VertexBuffer:
          return {VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                  VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT};
        case BufferState::IndexBuffer:
          return {VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                  VK_ACCESS_2_INDEX_READ_BIT};
        case BufferState::UniformRead:
          return {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_UNIFORM_READ_BIT};
        case BufferState::StorageRead:
          return {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_SHADER_STORAGE_READ_BIT};
        case BufferState::StorageReadWrite:
          return {VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                  VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
                      VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT};
        case BufferState::IndirectArgument:
          return {VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                  VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT};
        }

        GFX_ASSERT(false, "Unsupported LowGfx buffer state");
        return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
      }

      static VkPipelineStageFlags
      to_vulkan_pipeline_stage_flags(PipelineStage p_Stages)
      {
        VkPipelineStageFlags l_Flags = 0;

        if ((p_Stages & PipelineStage::TopOfPipe) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        if ((p_Stages & PipelineStage::DrawIndirect) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        }
        if ((p_Stages & PipelineStage::VertexInput) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }
        if ((p_Stages & PipelineStage::VertexShader) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }
        if ((p_Stages & PipelineStage::FragmentShader) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        if ((p_Stages & PipelineStage::ColorAttachment) !=
            PipelineStage::None) {
          l_Flags |=
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        if ((p_Stages & PipelineStage::ComputeShader) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        if ((p_Stages & PipelineStage::Transfer) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        if ((p_Stages & PipelineStage::BottomOfPipe) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        if ((p_Stages & PipelineStage::AllGraphics) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        }
        if ((p_Stages & PipelineStage::AllCommands) !=
            PipelineStage::None) {
          l_Flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        GFX_ASSERT(l_Flags != 0,
                   "Cannot convert empty pipeline stage mask");
        return l_Flags;
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

      static VkFilter to_vulkan_filter(FilterMode p_Filter)
      {
        switch (p_Filter) {
        case FilterMode::Nearest:
          return VK_FILTER_NEAREST;
        case FilterMode::Linear:
          return VK_FILTER_LINEAR;
        }

        GFX_ASSERT(false, "Unsupported LowGfx filter mode");
        return VK_FILTER_NEAREST;
      }

      static VkOffset3D
      to_vulkan_offset(const Math::UVector3 &p_Offset)
      {
        return {static_cast<i32>(p_Offset.x),
                static_cast<i32>(p_Offset.y),
                static_cast<i32>(p_Offset.z)};
      }

      static VkExtent3D
      to_vulkan_extent(const Math::UVector3 &p_Extent)
      {
        return {p_Extent.x, p_Extent.y, p_Extent.z};
      }

      static VkImageSubresourceLayers to_vulkan_layers(
          ImageAspect p_Aspect, u32 p_Mip, u32 p_BaseLayer,
          u32 p_LayerCount)
      {
        VkImageSubresourceLayers l_Layers{};
        l_Layers.aspectMask = to_vulkan_aspect(p_Aspect);
        l_Layers.mipLevel = p_Mip;
        l_Layers.baseArrayLayer = p_BaseLayer;
        l_Layers.layerCount = p_LayerCount;
        return l_Layers;
      }

      static VulkanCommandListState *
      get_vulkan_command_list(Detail::BackendCommandList &p_CommandList,
                              const char *p_Message)
      {
        VulkanCommandListState *l_CommandListState =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        GFX_ASSERT(l_CommandListState, p_Message);
        GFX_ASSERT(l_CommandListState->command_buffer !=
                       VK_NULL_HANDLE,
                   "Cannot record copy without Vulkan command "
                   "buffer");
        return l_CommandListState;
      }

      static VulkanBufferState *
      get_vulkan_buffer(Detail::BackendBuffer &p_Buffer,
                        const char *p_Message)
      {
        VulkanBufferState *l_Buffer =
            static_cast<VulkanBufferState *>(p_Buffer.backend_state);
        GFX_ASSERT(l_Buffer && l_Buffer->buffer != VK_NULL_HANDLE,
                   p_Message);
        return l_Buffer;
      }

      static VulkanImageState *
      get_vulkan_image(Detail::BackendImage &p_Image,
                       const char *p_Message)
      {
        VulkanImageState *l_Image =
            static_cast<VulkanImageState *>(p_Image.backend_state);
        GFX_ASSERT(l_Image && l_Image->image != VK_NULL_HANDLE,
                   p_Message);
        return l_Image;
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

      Detail::BackendFence submit(
          Detail::ContextImpl &p_Context, QueueRole p_QueueRole,
          Util::Span<Detail::BackendCommandList *> p_CommandLists,
          Util::Span<const Detail::BackendSubmitWait> p_Waits,
          Util::Span<Detail::BackendSemaphore *> p_Signals)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot submit Vulkan command lists "
                            "without context state");
        GFX_ASSERT(
            l_State->device != VK_NULL_HANDLE,
            "Cannot submit Vulkan command lists without device");
        GFX_ASSERT(!p_CommandLists.empty(),
                   "Cannot submit empty Vulkan command list batch");

        const VulkanQueue &l_Queue = get_queue(*l_State, p_QueueRole);
        GFX_ASSERT(l_Queue.queue != VK_NULL_HANDLE,
                   "Cannot submit Vulkan command lists without queue");

        Util::List<VkCommandBuffer> l_CommandBuffers;
        l_CommandBuffers.reserve(p_CommandLists.size());
        for (Detail::BackendCommandList *i_CommandList :
             p_CommandLists) {
          GFX_ASSERT(i_CommandList,
                     "Cannot submit null Vulkan command list");
          GFX_ASSERT(i_CommandList->queue_role == p_QueueRole,
                     "Vulkan command list queue role does not match "
                     "submit queue");

          VulkanCommandListState *i_CommandListState =
              get_vulkan_command_list(
                  *i_CommandList,
                  "Cannot submit Vulkan command list without command "
                  "list state");
          l_CommandBuffers.push_back(
              i_CommandListState->command_buffer);
        }

        Util::List<VkSemaphore> l_WaitSemaphores;
        Util::List<VkPipelineStageFlags> l_WaitStages;
        l_WaitSemaphores.reserve(p_Waits.size());
        l_WaitStages.reserve(p_Waits.size());
        for (const Detail::BackendSubmitWait &i_Wait : p_Waits) {
          GFX_ASSERT(i_Wait.semaphore,
                     "Cannot submit Vulkan wait with null "
                     "semaphore");
          VulkanSemaphoreState *i_SemaphoreState =
              static_cast<VulkanSemaphoreState *>(
                  i_Wait.semaphore->backend_state);
          GFX_ASSERT(i_SemaphoreState &&
                         i_SemaphoreState->semaphore !=
                             VK_NULL_HANDLE,
                     "Cannot submit Vulkan wait with invalid "
                     "semaphore");
          l_WaitSemaphores.push_back(i_SemaphoreState->semaphore);
          l_WaitStages.push_back(
              to_vulkan_pipeline_stage_flags(i_Wait.stage));
        }

        Util::List<VkSemaphore> l_SignalSemaphores;
        l_SignalSemaphores.reserve(p_Signals.size());
        for (Detail::BackendSemaphore *i_Semaphore : p_Signals) {
          GFX_ASSERT(i_Semaphore,
                     "Cannot submit Vulkan signal with null "
                     "semaphore");
          VulkanSemaphoreState *i_SemaphoreState =
              static_cast<VulkanSemaphoreState *>(
                  i_Semaphore->backend_state);
          GFX_ASSERT(i_SemaphoreState &&
                         i_SemaphoreState->semaphore !=
                             VK_NULL_HANDLE,
                     "Cannot submit Vulkan signal with invalid "
                     "semaphore");
          l_SignalSemaphores.push_back(
              i_SemaphoreState->semaphore);
        }

        VulkanFenceState *l_FenceState = new VulkanFenceState();
        VkFenceCreateInfo l_FenceInfo{};
        l_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkResult l_FenceResult = vkCreateFence(
            l_State->device, &l_FenceInfo, nullptr,
            &l_FenceState->fence);
        if (l_FenceResult != VK_SUCCESS) {
          delete l_FenceState;
          GFX_ASSERT(false, "Failed to create Vulkan submit fence");
        }

        VkSubmitInfo l_SubmitInfo{};
        l_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        l_SubmitInfo.waitSemaphoreCount =
            static_cast<u32>(l_WaitSemaphores.size());
        l_SubmitInfo.pWaitSemaphores = l_WaitSemaphores.data();
        l_SubmitInfo.pWaitDstStageMask = l_WaitStages.data();
        l_SubmitInfo.commandBufferCount =
            static_cast<u32>(l_CommandBuffers.size());
        l_SubmitInfo.pCommandBuffers = l_CommandBuffers.data();
        l_SubmitInfo.signalSemaphoreCount =
            static_cast<u32>(l_SignalSemaphores.size());
        l_SubmitInfo.pSignalSemaphores = l_SignalSemaphores.data();

        VkResult l_SubmitResult = vkQueueSubmit(
            l_Queue.queue, 1, &l_SubmitInfo, l_FenceState->fence);
        if (l_SubmitResult != VK_SUCCESS) {
          vkDestroyFence(l_State->device, l_FenceState->fence,
                         nullptr);
          delete l_FenceState;
          GFX_ASSERT(false, "Failed to submit Vulkan command lists");
        }

        Detail::BackendFence l_Fence;
        l_Fence.backend_state = l_FenceState;
        return l_Fence;
      }

      bool is_fence_complete(Detail::ContextImpl &p_Context,
                             Detail::BackendFence &p_Fence)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot query Vulkan fence without "
                            "context state");

        VulkanFenceState *l_FenceState =
            static_cast<VulkanFenceState *>(p_Fence.backend_state);
        GFX_ASSERT(l_FenceState &&
                       l_FenceState->fence != VK_NULL_HANDLE,
                   "Cannot query invalid Vulkan fence");

        VkResult l_Result =
            vkGetFenceStatus(l_State->device, l_FenceState->fence);
        if (l_Result == VK_SUCCESS) {
          return true;
        }
        if (l_Result == VK_NOT_READY) {
          return false;
        }

        GFX_ASSERT(false, "Failed to query Vulkan fence status");
        return false;
      }

      void wait_fence(Detail::ContextImpl &p_Context,
                      Detail::BackendFence &p_Fence)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State,
                   "Cannot wait Vulkan fence without context state");

        VulkanFenceState *l_FenceState =
            static_cast<VulkanFenceState *>(p_Fence.backend_state);
        GFX_ASSERT(l_FenceState &&
                       l_FenceState->fence != VK_NULL_HANDLE,
                   "Cannot wait invalid Vulkan fence");

        VkResult l_Result =
            vkWaitForFences(l_State->device, 1,
                            &l_FenceState->fence, VK_TRUE,
                            UINT64_MAX);
        GFX_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to wait for Vulkan fence");
      }

      void destroy_fence(Detail::ContextImpl &p_Context,
                         Detail::BackendFence &p_Fence)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot destroy Vulkan fence without "
                            "context state");

        VulkanFenceState *l_FenceState =
            static_cast<VulkanFenceState *>(p_Fence.backend_state);
        if (l_FenceState) {
          if (l_State->device != VK_NULL_HANDLE &&
              l_FenceState->fence != VK_NULL_HANDLE) {
            vkDestroyFence(l_State->device, l_FenceState->fence,
                           nullptr);
          }
          delete l_FenceState;
        }

        p_Fence.backend_state = nullptr;
      }

      Detail::BackendSemaphore
      create_semaphore(Detail::ContextImpl &p_Context)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot create Vulkan semaphore without "
                            "context state");
        GFX_ASSERT(l_State->device != VK_NULL_HANDLE,
                   "Cannot create Vulkan semaphore without device");

        VulkanSemaphoreState *l_SemaphoreState =
            new VulkanSemaphoreState();
        VkSemaphoreCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult l_Result = vkCreateSemaphore(
            l_State->device, &l_Info, nullptr,
            &l_SemaphoreState->semaphore);
        if (l_Result != VK_SUCCESS) {
          delete l_SemaphoreState;
          GFX_ASSERT(false, "Failed to create Vulkan semaphore");
        }

        Detail::BackendSemaphore l_Semaphore;
        l_Semaphore.backend_state = l_SemaphoreState;
        return l_Semaphore;
      }

      void destroy_semaphore(Detail::ContextImpl &p_Context,
                             Detail::BackendSemaphore &p_Semaphore)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        GFX_ASSERT(l_State, "Cannot destroy Vulkan semaphore without "
                            "context state");

        VulkanSemaphoreState *l_SemaphoreState =
            static_cast<VulkanSemaphoreState *>(
                p_Semaphore.backend_state);
        if (l_SemaphoreState) {
          if (l_State->device != VK_NULL_HANDLE &&
              l_SemaphoreState->semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(l_State->device,
                               l_SemaphoreState->semaphore,
                               nullptr);
          }
          delete l_SemaphoreState;
        }

        p_Semaphore.backend_state = nullptr;
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

      void barrier_buffer_command_list(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Buffer,
          const BufferBarrier &p_Barrier)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandListState =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot record Vulkan buffer barrier without command "
                "list state");
        VulkanBufferState *l_Buffer =
            get_vulkan_buffer(p_Buffer,
                              "Cannot record Vulkan buffer barrier "
                              "for invalid buffer");

        const VulkanBufferStateAccess l_Source =
            to_vulkan_buffer_state_access(p_Barrier.old_state);
        const VulkanBufferStateAccess l_Destination =
            to_vulkan_buffer_state_access(p_Barrier.new_state);

        const VkDeviceSize l_Size =
            p_Barrier.size == LOW_UINT64_MAX
                ? VK_WHOLE_SIZE
                : static_cast<VkDeviceSize>(p_Barrier.size);

        VkBufferMemoryBarrier2 l_BufferBarrier{};
        l_BufferBarrier.sType =
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        l_BufferBarrier.srcStageMask = l_Source.stage;
        l_BufferBarrier.srcAccessMask = l_Source.access;
        l_BufferBarrier.dstStageMask = l_Destination.stage;
        l_BufferBarrier.dstAccessMask = l_Destination.access;
        l_BufferBarrier.srcQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
        l_BufferBarrier.dstQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
        l_BufferBarrier.buffer = l_Buffer->buffer;
        l_BufferBarrier.offset =
            static_cast<VkDeviceSize>(p_Barrier.offset);
        l_BufferBarrier.size = l_Size;

        VkDependencyInfo l_DependencyInfo{};
        l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        l_DependencyInfo.bufferMemoryBarrierCount = 1;
        l_DependencyInfo.pBufferMemoryBarriers = &l_BufferBarrier;

        vkCmdPipelineBarrier2(l_CommandListState->command_buffer,
                              &l_DependencyInfo);
      }

      void copy_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Source,
          Detail::BackendBuffer &p_Destination,
          Util::Span<const BufferCopyRegion> p_Regions)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandList =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot copy Vulkan buffer without command list "
                "state");
        VulkanBufferState *l_Source =
            get_vulkan_buffer(p_Source,
                              "Cannot copy from invalid Vulkan "
                              "source buffer");
        VulkanBufferState *l_Destination =
            get_vulkan_buffer(p_Destination,
                              "Cannot copy to invalid Vulkan "
                              "destination buffer");

        Util::List<VkBufferCopy> l_Regions;
        for (const BufferCopyRegion &i_Region : p_Regions) {
          VkBufferCopy i_VulkanRegion{};
          i_VulkanRegion.srcOffset = i_Region.src_offset;
          i_VulkanRegion.dstOffset = i_Region.dst_offset;
          i_VulkanRegion.size = i_Region.size;
          l_Regions.push_back(i_VulkanRegion);
        }

        vkCmdCopyBuffer(l_CommandList->command_buffer,
                        l_Source->buffer, l_Destination->buffer,
                        static_cast<u32>(l_Regions.size()),
                        l_Regions.data());
      }

      void copy_buffer_to_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandList =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot copy Vulkan buffer to image without command "
                "list state");
        VulkanBufferState *l_Source =
            get_vulkan_buffer(p_Source,
                              "Cannot copy from invalid Vulkan "
                              "source buffer");
        VulkanImageState *l_Destination =
            get_vulkan_image(p_Destination,
                             "Cannot copy to invalid Vulkan "
                             "destination image");

        Util::List<VkBufferImageCopy> l_Regions;
        for (const BufferImageCopyRegion &i_Region : p_Regions) {
          VkBufferImageCopy i_VulkanRegion{};
          i_VulkanRegion.bufferOffset = i_Region.buffer_offset;
          i_VulkanRegion.bufferRowLength =
              i_Region.buffer_row_length;
          i_VulkanRegion.bufferImageHeight =
              i_Region.buffer_image_height;
          i_VulkanRegion.imageSubresource = to_vulkan_layers(
              i_Region.image_aspect, i_Region.image_mip,
              i_Region.image_base_layer,
              i_Region.image_layer_count);
          i_VulkanRegion.imageOffset =
              to_vulkan_offset(i_Region.image_offset);
          i_VulkanRegion.imageExtent =
              to_vulkan_extent(i_Region.image_extent);
          l_Regions.push_back(i_VulkanRegion);
        }

        vkCmdCopyBufferToImage(
            l_CommandList->command_buffer, l_Source->buffer,
            l_Destination->image,
            to_vulkan_image_state_access(ImageState::TransferDst)
                .layout,
            static_cast<u32>(l_Regions.size()), l_Regions.data());
      }

      void copy_image_to_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendBuffer &p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandList =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot copy Vulkan image to buffer without command "
                "list state");
        VulkanImageState *l_Source =
            get_vulkan_image(p_Source,
                             "Cannot copy from invalid Vulkan "
                             "source image");
        VulkanBufferState *l_Destination =
            get_vulkan_buffer(p_Destination,
                              "Cannot copy to invalid Vulkan "
                              "destination buffer");

        Util::List<VkBufferImageCopy> l_Regions;
        for (const BufferImageCopyRegion &i_Region : p_Regions) {
          VkBufferImageCopy i_VulkanRegion{};
          i_VulkanRegion.bufferOffset = i_Region.buffer_offset;
          i_VulkanRegion.bufferRowLength =
              i_Region.buffer_row_length;
          i_VulkanRegion.bufferImageHeight =
              i_Region.buffer_image_height;
          i_VulkanRegion.imageSubresource = to_vulkan_layers(
              i_Region.image_aspect, i_Region.image_mip,
              i_Region.image_base_layer,
              i_Region.image_layer_count);
          i_VulkanRegion.imageOffset =
              to_vulkan_offset(i_Region.image_offset);
          i_VulkanRegion.imageExtent =
              to_vulkan_extent(i_Region.image_extent);
          l_Regions.push_back(i_VulkanRegion);
        }

        vkCmdCopyImageToBuffer(
            l_CommandList->command_buffer, l_Source->image,
            to_vulkan_image_state_access(ImageState::TransferSrc)
                .layout,
            l_Destination->buffer, static_cast<u32>(l_Regions.size()),
            l_Regions.data());
      }

      void copy_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const ImageCopyRegion> p_Regions)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandList =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot copy Vulkan image without command list "
                "state");
        VulkanImageState *l_Source =
            get_vulkan_image(p_Source,
                             "Cannot copy from invalid Vulkan "
                             "source image");
        VulkanImageState *l_Destination =
            get_vulkan_image(p_Destination,
                             "Cannot copy to invalid Vulkan "
                             "destination image");

        Util::List<VkImageCopy> l_Regions;
        for (const ImageCopyRegion &i_Region : p_Regions) {
          VkImageCopy i_VulkanRegion{};
          i_VulkanRegion.srcSubresource = to_vulkan_layers(
              i_Region.src_aspect, i_Region.src_mip,
              i_Region.src_base_layer, i_Region.layer_count);
          i_VulkanRegion.srcOffset =
              to_vulkan_offset(i_Region.src_offset);
          i_VulkanRegion.dstSubresource = to_vulkan_layers(
              i_Region.dst_aspect, i_Region.dst_mip,
              i_Region.dst_base_layer, i_Region.layer_count);
          i_VulkanRegion.dstOffset =
              to_vulkan_offset(i_Region.dst_offset);
          i_VulkanRegion.extent =
              to_vulkan_extent(i_Region.extent);
          l_Regions.push_back(i_VulkanRegion);
        }

        vkCmdCopyImage(
            l_CommandList->command_buffer, l_Source->image,
            to_vulkan_image_state_access(ImageState::TransferSrc)
                .layout,
            l_Destination->image,
            to_vulkan_image_state_access(ImageState::TransferDst)
                .layout,
            static_cast<u32>(l_Regions.size()), l_Regions.data());
      }

      void blit_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const ImageBlitRegion> p_Regions,
          FilterMode p_Filter)
      {
        (void)p_Context;
        VulkanCommandListState *l_CommandList =
            get_vulkan_command_list(
                p_CommandList,
                "Cannot blit Vulkan image without command list "
                "state");
        VulkanImageState *l_Source =
            get_vulkan_image(p_Source,
                             "Cannot blit from invalid Vulkan source "
                             "image");
        VulkanImageState *l_Destination =
            get_vulkan_image(p_Destination,
                             "Cannot blit to invalid Vulkan "
                             "destination image");

        Util::List<VkImageBlit> l_Regions;
        for (const ImageBlitRegion &i_Region : p_Regions) {
          VkImageBlit i_VulkanRegion{};
          i_VulkanRegion.srcSubresource = to_vulkan_layers(
              i_Region.src_aspect, i_Region.src_mip,
              i_Region.src_base_layer, i_Region.layer_count);
          i_VulkanRegion.srcOffsets[0] =
              to_vulkan_offset(i_Region.src_min);
          i_VulkanRegion.srcOffsets[1] =
              to_vulkan_offset(i_Region.src_max);
          i_VulkanRegion.dstSubresource = to_vulkan_layers(
              i_Region.dst_aspect, i_Region.dst_mip,
              i_Region.dst_base_layer, i_Region.layer_count);
          i_VulkanRegion.dstOffsets[0] =
              to_vulkan_offset(i_Region.dst_min);
          i_VulkanRegion.dstOffsets[1] =
              to_vulkan_offset(i_Region.dst_max);
          l_Regions.push_back(i_VulkanRegion);
        }

        vkCmdBlitImage(
            l_CommandList->command_buffer, l_Source->image,
            to_vulkan_image_state_access(ImageState::TransferSrc)
                .layout,
            l_Destination->image,
            to_vulkan_image_state_access(ImageState::TransferDst)
                .layout,
            static_cast<u32>(l_Regions.size()), l_Regions.data(),
            to_vulkan_filter(p_Filter));
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
