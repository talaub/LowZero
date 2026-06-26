#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      struct VulkanCommandListState
      {
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        QueueRole queue_role = QueueRole::Graphics;
        u32 queue_family_index = LOW_UINT32_MAX;
        bool owns_command_pool = true;
      };

      static const VulkanQueue &get_queue(const VulkanContextState &p_State,
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

      static VkCommandBuffer acquire_frame_command_buffer(
          VulkanContextState &p_State, VulkanFrameCommandPool &p_Pool)
      {
        LOW_ASSERT(p_Pool.pool != VK_NULL_HANDLE,
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
        LOW_ASSERT(l_AllocateResult == VK_SUCCESS,
                   "Failed to allocate Vulkan frame command buffer");

        p_Pool.command_buffers.push_back(l_CommandBuffer);
        ++p_Pool.next_command_buffer;
        return l_CommandBuffer;
      }

      Detail::BackendCommandList
      request_command_list(Detail::ContextImpl &p_Context,
                           const FrameContext &p_Frame,
                           QueueRole p_QueueRole)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State, "Cannot create Vulkan command list "
                            "without context state");
        LOW_ASSERT(
            l_State->device != VK_NULL_HANDLE,
            "Cannot create Vulkan command list without device");

        const VulkanQueue &l_Queue = get_queue(*l_State, p_QueueRole);
        LOW_ASSERT(l_Queue.queue != VK_NULL_HANDLE,
                   "Cannot create Vulkan command list without queue");
        LOW_ASSERT(
            l_Queue.family_index != LOW_UINT32_MAX,
            "Cannot create Vulkan command list without queue family");

        VulkanCommandListState *l_CommandListState =
            new VulkanCommandListState();
        l_CommandListState->queue_role = p_QueueRole;
        l_CommandListState->queue_family_index = l_Queue.family_index;
        l_CommandListState->owns_command_pool = false;

        const u32 l_FrameIndex =
            Detail::FrameContextAccess::frame_index(p_Frame);
        LOW_ASSERT(l_FrameIndex < l_State->frames.size(),
                   "Cannot allocate frame command list with invalid "
                   "frame index");

        VulkanFrameCommandPool &l_FramePool =
            get_frame_command_pool(l_State->frames[l_FrameIndex],
                                   p_QueueRole);
        l_CommandListState->command_pool = l_FramePool.pool;
        l_CommandListState->command_buffer =
            acquire_frame_command_buffer(*l_State, l_FramePool);

        Detail::BackendCommandList l_CommandList;
        l_CommandList.queue_role = p_QueueRole;
        l_CommandList.state = CommandListState::Initial;
        l_CommandList.backend_state = l_CommandListState;
        return l_CommandList;
      }

      Detail::BackendCommandList request_immediate_command_list(
          Detail::ContextImpl &p_Context, QueueRole p_QueueRole)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State, "Cannot create Vulkan command list "
                            "without context state");
        LOW_ASSERT(
            l_State->device != VK_NULL_HANDLE,
            "Cannot create Vulkan command list without device");

        const VulkanQueue &l_Queue = get_queue(*l_State, p_QueueRole);
        LOW_ASSERT(l_Queue.queue != VK_NULL_HANDLE,
                   "Cannot create Vulkan command list without queue");
        LOW_ASSERT(
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
        LOW_ASSERT(l_PoolResult == VK_SUCCESS,
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
          LOW_ASSERT(false,
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
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State, "Cannot destroy Vulkan command list "
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
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
