#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowRendererWindow.h"

#include "LowRendererInterface.h"

#include "LowRendererGraphicsPipeline.h"

namespace Low {
  namespace Renderer {
    Interface::Context g_Context;
    Interface::CommandPool g_CommandPool;
    Interface::Swapchain g_Swapchain;

    static void initialize_backend_types()
    {
      Interface::Context::initialize();
      Interface::CommandPool::initialize();
      Interface::CommandBuffer::initialize();
      Interface::Framebuffer::initialize();
      Interface::Renderpass::initialize();
      Interface::Swapchain::initialize();
      Interface::Image2D::initialize();
      Interface::GraphicsPipeline::initialize();
    }

    static void cleanup_backend_types()
    {
      Interface::GraphicsPipeline::cleanup();
      Interface::Image2D::cleanup();
      Interface::Swapchain::cleanup();
      Interface::Renderpass::cleanup();
      Interface::Framebuffer::cleanup();
      Interface::CommandBuffer::cleanup();
      Interface::CommandPool::cleanup();
      Interface::Context::cleanup();
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

      Interface::ContextCreateParams l_ContextInit;
      l_ContextInit.validation_enabled = true;
      l_ContextInit.window = &l_Window;
      g_Context = Interface::Context::make(N(MainContext), l_ContextInit);

      Interface::CommandPoolCreateParams l_CommandPoolParams;
      l_CommandPoolParams.context = g_Context;
      g_CommandPool =
          Interface::CommandPool::make(N(MainCommandPool), l_CommandPoolParams);

      Interface::SwapchainCreateParams l_SwapchainParams;
      l_SwapchainParams.context = g_Context;
      l_SwapchainParams.commandPool = g_CommandPool;
      g_Swapchain =
          Interface::Swapchain::make(N(MainSwapchain), l_SwapchainParams);

      LOW_LOG_INFO("Renderer initialized");
    }

    void tick(float p_Delta)
    {
      g_Context.get_window().tick();

      g_Swapchain.prepare();

      g_Swapchain.get_current_commandbuffer().start();

      Interface::RenderpassStartParams l_RpParams;
      l_RpParams.framebuffer = g_Swapchain.get_current_framebuffer();
      l_RpParams.commandbuffer = g_Swapchain.get_current_commandbuffer();
      l_RpParams.clearDepthValue.x = 1.0f;
      l_RpParams.clearDepthValue.y = 0.0f;
      Math::Color l_ClearColor(0.0f, 1.0f, 0.0f, 1.0f);
      l_RpParams.clearColorValues.push_back(l_ClearColor);
      g_Swapchain.get_renderpass().start(l_RpParams);

      Interface::RenderpassStopParams l_RpStopParams;
      l_RpStopParams.commandbuffer = g_Swapchain.get_current_commandbuffer();
      g_Swapchain.get_renderpass().stop(l_RpStopParams);

      g_Swapchain.get_current_commandbuffer().stop();

      g_Swapchain.swap();
    }

    bool window_is_open()
    {
      return g_Context.get_window().is_open();
    }

    void cleanup()
    {
      g_Context.wait_idle();

      cleanup_backend_types();
    }
  } // namespace Renderer
} // namespace Low
