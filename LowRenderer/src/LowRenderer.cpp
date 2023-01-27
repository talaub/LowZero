#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowRendererBackend.h"
#include "LowRendererWindow.h"

namespace Low {
  namespace Renderer {
    Backend::Context g_Context;
    Backend::CommandPool g_CommandPool;
    Backend::Swapchain g_Swapchain;

    void initialize()
    {
      Window l_Window;
      WindowInit l_WindowInit;
      l_WindowInit.dimensions.x = 1280;
      l_WindowInit.dimensions.y = 860;
      l_WindowInit.title = "LowEngine";

      window_initialize(l_Window, l_WindowInit);

      Backend::ContextCreateParams l_ContextInit;
      l_ContextInit.validation_enabled = true;
      l_ContextInit.window = &l_Window;

      Backend::context_create(g_Context, l_ContextInit);

      Backend::CommandPoolCreateParams l_CommandPoolParams;
      l_CommandPoolParams.context = &g_Context;
      Backend::commandpool_create(g_CommandPool, l_CommandPoolParams);

      Backend::SwapchainCreateParams l_SwapchainParams;
      l_SwapchainParams.context = &g_Context;
      l_SwapchainParams.commandPool = &g_CommandPool;
      Backend::swapchain_create(g_Swapchain, l_SwapchainParams);

      LOW_LOG_INFO("Renderer initialized");
    }

    void tick(float p_Delta)
    {
      g_Context.m_Window.tick();
    }

    bool window_is_open()
    {
      return g_Context.m_Window.is_open();
    }

    void cleanup()
    {
      Backend::swapchain_cleanup(g_Swapchain);
      Backend::commandpool_cleanup(g_CommandPool);
      Backend::context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
