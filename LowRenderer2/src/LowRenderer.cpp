#include "LowRenderer.h"

#include "LowRendererVulkanRenderer.h"
#include "LowRendererBase.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Renderer {
    void initialize()
    {
      LOW_ASSERT(Vulkan::initialize(),
                 "Failed to initialize Vulkan renderer");
    }

    void cleanup()
    {
      LOW_ASSERT(Vulkan::cleanup(),
                 "Failed to cleanup Vulkan renderer");
    }

    void tick(float p_Delta)
    {
      LOW_ASSERT(Vulkan::tick(p_Delta),
                 "Failed to tick Vulkan renderer");
    }

    void check_window_resize(float p_Delta)
    {
      LOW_ASSERT(
          Vulkan::check_window_resize(p_Delta),
          "Failed to check for window resize in Vulkan renderer");
    }
  } // namespace Renderer
} // namespace Low
