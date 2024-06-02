#pragma once

#include "LowRendererVulkan.h"

// CURRENTLY UNUSED

namespace Low {
  namespace Renderer {
    namespace Backend {
      enum class ContextStatus
      {
        UNINITIALIZED,
        INITIALIZED
      };

      struct Context
      {
        union
        {
          Vulkan::Context vk;
        };

        ContextStatus status = ContextStatus::UNINITIALIZED;
      };
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
