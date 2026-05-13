#include "LowRendererVulkanBuffer.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace BufferUtil {
        AllocatedBuffer create_buffer(size_t p_AllocSize,
                                      VkBufferUsageFlags p_Usage,
                                      VmaMemoryUsage p_MemoryUsage)
        {
          VkBufferCreateInfo l_BufferInfo{};
          l_BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
          l_BufferInfo.pNext = nullptr;
          l_BufferInfo.size = p_AllocSize;

          l_BufferInfo.usage = p_Usage;

          VmaAllocationCreateInfo l_VmaAllocInfo = {};
          l_VmaAllocInfo.usage = p_MemoryUsage;
          l_VmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
          AllocatedBuffer l_NewBuffer;

          LOWR_VK_CHECK(vmaCreateBuffer(
              Global::get_allocator(), &l_BufferInfo, &l_VmaAllocInfo,
              &l_NewBuffer.buffer, &l_NewBuffer.allocation,
              &l_NewBuffer.info));

          return l_NewBuffer;
        }

        BufferHolder
        create_buffer_holder(size_t p_AllocSize,
                             VkBufferUsageFlags p_Usage,
                             VmaMemoryUsage p_MemoryUsage)
        {

          BufferHolder l_Holder = BufferHolder::make(N(Buffer));
          l_Holder.set_bf(
              create_buffer(p_AllocSize, p_Usage, p_MemoryUsage));
          return l_Holder;
        }

        void set_name(const AllocatedBuffer &p_Buffer,
                      const char *p_Name)
        {
          if (p_Buffer.buffer == VK_NULL_HANDLE || !p_Name) {
            return;
          }

          vmaSetAllocationName(Global::get_allocator(),
                               p_Buffer.allocation, p_Name);

          PFN_vkSetDebugUtilsObjectNameEXT l_SetObjectName =
              (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
                  Global::get_device(),
                  "vkSetDebugUtilsObjectNameEXT");
          if (!l_SetObjectName) {
            return;
          }

          VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
          l_NameInfo.sType =
              VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
          l_NameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
          l_NameInfo.objectHandle = (uint64_t)p_Buffer.buffer;
          l_NameInfo.pObjectName = p_Name;

          l_SetObjectName(Global::get_device(), &l_NameInfo);
        }

        void set_name(const DynamicBuffer &p_Buffer,
                      const char *p_Name)
        {
          set_name(p_Buffer.m_Buffer, p_Name);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, VkBuffer p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          VkBufferMemoryBarrier2 l_Barrier{};
          l_Barrier.sType =
              VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
          l_Barrier.srcStageMask = p_SrcStageMask;
          l_Barrier.srcAccessMask = p_SrcAccessMask;
          l_Barrier.dstStageMask = p_DstStageMask;
          l_Barrier.dstAccessMask = p_DstAccessMask;
          l_Barrier.buffer = p_Buffer;
          l_Barrier.offset = p_Offset;
          l_Barrier.size = p_Size;

          VkDependencyInfo l_DependencyInfo{};
          l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
          l_DependencyInfo.bufferMemoryBarrierCount = 1;
          l_DependencyInfo.pBufferMemoryBarriers = &l_Barrier;

          vkCmdPipelineBarrier2(p_Cmd, &l_DependencyInfo);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const AllocatedBuffer &p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          cmd_buffer_barrier(p_Cmd, p_Buffer.buffer, p_Offset,
                             p_Size, p_SrcStageMask,
                             p_SrcAccessMask, p_DstStageMask,
                             p_DstAccessMask);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const DynamicBuffer &p_Buffer,
            VkDeviceSize p_Offset, VkDeviceSize p_Size,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          cmd_buffer_barrier(p_Cmd, p_Buffer.m_Buffer, p_Offset,
                             p_Size, p_SrcStageMask,
                             p_SrcAccessMask, p_DstStageMask,
                             p_DstAccessMask);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, VkBuffer p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          cmd_buffer_barrier(p_Cmd, p_Buffer,
                             p_CopyRegion.dstOffset,
                             p_CopyRegion.size, p_SrcStageMask,
                             p_SrcAccessMask, p_DstStageMask,
                             p_DstAccessMask);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const AllocatedBuffer &p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          cmd_buffer_barrier(p_Cmd, p_Buffer.buffer, p_CopyRegion,
                             p_SrcStageMask, p_SrcAccessMask,
                             p_DstStageMask, p_DstAccessMask);
        }

        void cmd_buffer_barrier(
            VkCommandBuffer p_Cmd, const DynamicBuffer &p_Buffer,
            const VkBufferCopy &p_CopyRegion,
            VkPipelineStageFlags2 p_SrcStageMask,
            VkAccessFlags2 p_SrcAccessMask,
            VkPipelineStageFlags2 p_DstStageMask,
            VkAccessFlags2 p_DstAccessMask)
        {
          cmd_buffer_barrier(p_Cmd, p_Buffer.m_Buffer,
                             p_CopyRegion, p_SrcStageMask,
                             p_SrcAccessMask, p_DstStageMask,
                             p_DstAccessMask);
        }

        void destroy_buffer(const AllocatedBuffer &p_Buffer)
        {
          vmaDestroyBuffer(Global::get_allocator(), p_Buffer.buffer,
                           p_Buffer.allocation);
        }
      } // namespace BufferUtil
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
