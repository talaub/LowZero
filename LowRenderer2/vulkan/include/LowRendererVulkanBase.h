#pragma once

#include "LowRendererVulkan.h"

#include "LowRendererBackendCompatibility.h"

#include "LowMath.h"

#include <vulkan/vk_enum_string_helper.h>

#define LOWR_VK_ASSERT(cond, text)                                   \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << "[VK] " << text << LOW_LOG_END;               \
      return false;                                                  \
    }                                                                \
  }

#define LOWR_VK_CHECK_RETURN(x)                                      \
  do {                                                               \
    VkResult err = x;                                                \
    if (err) {                                                       \
      LOW_LOG_ERROR << "[VK] " << string_VkResult(err)               \
                    << LOW_LOG_END;                                  \
      abort();                                                       \
      return false;                                                  \
    }                                                                \
  } while (0)

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      bool initialize(Math::UVector2 p_Dimensions);
      bool cleanup();

      bool draw();
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
