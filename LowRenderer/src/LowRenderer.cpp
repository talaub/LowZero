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
    Interface::GraphicsPipeline g_Pipeline;

    static void initialize_backend_types()
    {
      Interface::Context::initialize();
      Interface::CommandPool::initialize();
      Interface::CommandBuffer::initialize();
      Interface::Framebuffer::initialize();
      Interface::Renderpass::initialize();
      Interface::Swapchain::initialize();
      Interface::Image2D::initialize();
      Interface::PipelineInterface::initialize();
      Interface::GraphicsPipeline::initialize();
    }

    static void cleanup_backend_types()
    {
      Interface::GraphicsPipeline::cleanup();
      Interface::PipelineInterface::cleanup();
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

      {
        Interface::PipelineInterfaceCreateParams l_InterParams;
        l_InterParams.context = g_Context;
        Interface::PipelineInterface l_Interface =
            Interface::PipelineInterface::make(N(Interface), l_InterParams);

        Interface::GraphicsPipelineCreateParams l_Params;
        l_Params.vertexPath = "test.vert";
        l_Params.fragmentPath = "test.frag";
        l_Params.context = g_Context;
        l_Params.interface = l_Interface;

        Backend::GraphicsPipelineColorTarget l_Target;
        l_Target.blendEnable = true;
        l_Target.wirteMask = LOW_RENDERER_COLOR_WRITE_BIT_RED |
                             LOW_RENDERER_COLOR_WRITE_BIT_GREEN |
                             LOW_RENDERER_COLOR_WRITE_BIT_BLUE |
                             LOW_RENDERER_COLOR_WRITE_BIT_ALPHA;

        l_Params.colorTargets.push_back(l_Target);
        l_Params.renderpass = g_Swapchain.get_renderpass();
        l_Params.cullMode = Backend::PipelineRasterizerCullMode::BACK;
        l_Params.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
        g_Swapchain.get_framebuffers()[0].get_dimensions(l_Params.dimensions);
        l_Params.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
        l_Params.vertexInput = false;

        g_Pipeline = Interface::GraphicsPipeline::make(N(TestGraphicsPipeline),
                                                       l_Params);
      }
    }

    void tick(float p_Delta)
    {
      Interface::ShaderProgramUtils::tick(p_Delta);

      g_Context.get_window().tick();

      g_Swapchain.prepare();

      g_Swapchain.get_current_commandbuffer().start();

      Interface::RenderpassStartParams l_RpParams;
      l_RpParams.framebuffer = g_Swapchain.get_current_framebuffer();
      l_RpParams.commandbuffer = g_Swapchain.get_current_commandbuffer();
      l_RpParams.clearDepthValue.x = 1.0f;
      l_RpParams.clearDepthValue.y = 0.0f;
      Math::Color l_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      l_RpParams.clearColorValues.push_back(l_ClearColor);
      g_Swapchain.get_renderpass().start(l_RpParams);

      g_Pipeline.bind(g_Swapchain.get_current_commandbuffer());

      Backend::DrawParams l_Params;
      l_Params.commandBuffer =
          &(g_Swapchain.get_current_commandbuffer().get_commandbuffer());
      l_Params.firstInstance = 0;
      l_Params.firstVertex = 0;
      l_Params.vertexCount = 3;
      l_Params.instanceCount = 1;

      Backend::draw(l_Params);

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
