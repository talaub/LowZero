#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct Context;
      struct ContextInit;
    } // namespace Backend

    namespace Vulkan {
      struct VulkanContext
      {
        VkSurfaceKHR m_Surface;
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        bool m_ValidationEnabled;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
      };

      void vk_context_create(Backend::Context &p_Context,
                             Backend::ContextInit &p_Init);
      void vk_context_cleanup(Backend::Context &p_Context);
      void vk_context_wait_idle(Backend::Context &p_Context);
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
