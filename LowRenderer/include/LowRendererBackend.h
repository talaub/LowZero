#pragma once

#include "LowRendererWindow.h"

#include "LowRendererVulkan.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct Context
      {
        union
        {
          Vulkan::VulkanContext vk;
        };
        Window m_Window;
      };

      struct ContextInit
      {
        Window *window;
        bool validation_enabled;
      };

      void context_create(Context &p_Context, ContextInit &p_Init);
      void context_cleanup(Context &p_Context);
      void context_wait_idle(Context &p_Context);
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
