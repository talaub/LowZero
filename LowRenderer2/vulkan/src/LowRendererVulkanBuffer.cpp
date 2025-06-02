#include "LowRendererVulkanBuffer.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace BufferUtil {
        AllocatedBuffer create_buffer(
                                      size_t p_AllocSize,
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

        void destroy_buffer(
                            const AllocatedBuffer &p_Buffer)
        {
          vmaDestroyBuffer(Global::get_allocator(), p_Buffer.buffer,
                           p_Buffer.allocation);
        }
      } // namespace BufferUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
