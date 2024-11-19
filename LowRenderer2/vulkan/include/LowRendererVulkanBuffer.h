#pragma once

#include "LowRendererVulkanBase.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace BufferUtil {
        AllocatedBuffer create_buffer(size_t p_AllocSize,
                                      VkBufferUsageFlags p_Usage,
                                      VmaMemoryUsage p_MemoryUsage);

        void destroy_buffer(const AllocatedBuffer &p_Buffer);
      } // namespace BufferUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
