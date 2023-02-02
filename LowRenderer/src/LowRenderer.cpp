#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowRendererBackend.h"
#include "LowRendererWindow.h"

#include "LowRendererGraphicsPipeline.h"

namespace Low {
  namespace Renderer {
    Backend::Context g_Context;
    Backend::CommandPool g_CommandPool;
    Backend::Swapchain g_Swapchain;

    static void initialize_backend_types()
    {
      initialize_buffer(
          &Low::Renderer::Interface::GraphicsPipeline::ms_Buffer,
          Low::Renderer::Interface::GraphicsPipelineData::get_size(),
          Low::Renderer::Interface::GraphicsPipeline::get_capacity(),
          &Low::Renderer::Interface::GraphicsPipeline::ms_Slots);
    }

    static void cleanup_backend_types()
    {
      Low::Renderer::Interface::GraphicsPipeline::cleanup();
    }

    void initialize()
    {
      initialize_backend_types();

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

      Backend::swapchain_prepare(g_Swapchain);

      Backend::commandbuffer_start(
          Backend::swapchain_get_current_commandbuffer(g_Swapchain));

      Backend::RenderpassStartParams l_RpParams;
      l_RpParams.framebuffer =
          &(Backend::swapchain_get_current_framebuffer(g_Swapchain));
      l_RpParams.commandBuffer =
          &(Backend::swapchain_get_current_commandbuffer(g_Swapchain));
      l_RpParams.clearDepthValue.x = 1.0f;
      l_RpParams.clearDepthValue.y = 0.0f;
      Math::Color l_ClearColor(0.0f, 1.0f, 0.0f, 1.0f);
      l_RpParams.clearColorValues = &l_ClearColor;
      Backend::renderpass_start(g_Swapchain.renderpass, l_RpParams);

      Backend::RenderpassStopParams l_RpStopParams;
      l_RpStopParams.commandBuffer =
          &(Backend::swapchain_get_current_commandbuffer(g_Swapchain));

      Backend::renderpass_stop(g_Swapchain.renderpass, l_RpStopParams);

      Backend::commandbuffer_stop(
          Backend::swapchain_get_current_commandbuffer(g_Swapchain));

      Backend::swapchain_swap(g_Swapchain);
    }

    bool window_is_open()
    {
      return g_Context.m_Window.is_open();
    }

    void cleanup()
    {
      cleanup_backend_types();

      Backend::context_wait_idle(g_Context);

      Backend::swapchain_cleanup(g_Swapchain);
      Backend::commandpool_cleanup(g_CommandPool);
      Backend::context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
