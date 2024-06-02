#pragma once

#include "LowRendererVulkanBase.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace BufferUtil {
        AllocatedBuffer create_buffer(Context &p_Context,
                                      size_t p_AllocSize,
                                      VkBufferUsageFlags p_Usage,
                                      VmaMemoryUsage p_MemoryUsage);

        void destroy_buffer(const Context &p_Context,
                            const AllocatedBuffer &p_Buffer);
      } // namespace BufferUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
