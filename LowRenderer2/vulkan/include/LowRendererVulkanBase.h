#pragma once

#define IMGUI_API

#include "LowRendererVulkan.h"

#include "LowRendererBackendCompatibility.h"

#include "LowMath.h"

#include "LowUtilLogger.h"

#include <vulkan/vk_enum_string_helper.h>

#define LOWR_VK_ASSERT(cond, text)                                   \
  {                                                                  \
    if (!cond) {                                                     \
      LOW_LOG_ERROR << "[VK] " << text << LOW_LOG_END;               \
      return false;                                                  \
    }                                                                \
  }

#define LOWR_VK_ASSERT_RETURN(cond, text)                            \
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
      return false;                                                  \
    }                                                                \
  } while (0)

#define LOWR_VK_CHECK(x)                                             \
  do {                                                               \
    VkResult err = x;                                                \
    if (err) {                                                       \
      LOW_LOG_FATAL << "[VK] " << string_VkResult(err)               \
                    << LOW_LOG_END;                                  \
      abort();                                                       \
    }                                                                \
  } while (0)

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace Base {
        bool initialize(Context &p_Context,
                        Math::UVector2 p_Dimensions);

        bool cleanup(Context &p_Context);

        bool swapchain_resize(Context &p_Context);

        bool context_prepare_draw(Context &p_Context);
        bool context_present(Context &p_Context);
      } // namespace Base
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
