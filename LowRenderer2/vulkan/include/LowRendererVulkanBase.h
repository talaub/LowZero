#pragma once

#include "LowRendererVulkan.h"

#include "LowRendererBackendCompatibility.h"

#include "LowMath.h"

#define LOWR_VK_ASSERT(cond, text)                                   \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << "[VK] " << text << LOW_LOG_END;               \
      return false;                                                  \
    }                                                                \
  }

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      bool initialize(Math::UVector2 p_Dimensions);
      bool cleanup();
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
