#pragma once

#include "LowRendererVulkanBase.h"
#include "LowRendererVkBufferHolder.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace BufferUtil {
        AllocatedBuffer create_buffer(size_t p_AllocSize,
                                      VkBufferUsageFlags p_Usage,
                                      VmaMemoryUsage p_MemoryUsage);
        BufferHolder
        create_buffer_holder(size_t p_AllocSize,
                             VkBufferUsageFlags p_Usage,
                             VmaMemoryUsage p_MemoryUsage);
        void set_name(const AllocatedBuffer &p_Buffer,
                      const char *p_Name);
        void set_name(const DynamicBuffer &p_Buffer,
                      const char *p_Name);

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, VkBuffer p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);
        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const AllocatedBuffer &p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);
        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const DynamicBuffer &p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);
        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, VkBuffer p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);
        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const AllocatedBuffer &p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);
        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const DynamicBuffer &p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask);

        void destroy_buffer(const AllocatedBuffer &p_Buffer);
      } // namespace BufferUtil
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
