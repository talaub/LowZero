#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"

#include "LowRendererWindow.h"

#include "LowRendererInterface.h"

#include "LowRendererGraphicsPipeline.h"
#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_dds.hpp>

namespace Low {
  namespace Renderer {
    Interface::Context g_Context;
    Interface::CommandPool g_CommandPool;
    Interface::Swapchain g_Swapchain;

    Interface::GraphicsPipeline g_Pipeline;
    Interface::UniformScopeInterface g_UniformScopeInterface;
    Interface::Uniform g_Uniform;
    Interface::Uniform g_TexUniform;
    Interface::UniformScope g_UniformScope;
    Util::List<Interface::UniformScope> g_Scopes;

    Interface::Buffer g_VertexBuffer;
    Interface::Buffer g_IndexBuffer;

    Interface::Image2D g_Texture;

    // TEMP
    static void load_texture()
    {
      LOW_PROFILE_START(Texture load);

      gli::texture2d l_Texture(gli::load_dds(
          (Util::String(LOW_DATA_PATH) + "/assets/img2d/out_wb.dds").c_str()));

      Interface::Image2DCreateParams l_Params;
      l_Params.commandPool = g_CommandPool;
      Backend::imageformat_get_texture(g_Context.get_context(),
                                       l_Params.format);
      l_Params.context = g_Context;
      l_Params.depth = false;
      l_Params.dimensions.x = l_Texture.extent(0).x;
      l_Params.dimensions.y = l_Texture.extent(0).y;
      l_Params.writeable = false;
      l_Params.imageDataSize =
          l_Params.dimensions.x * l_Params.dimensions.y * 4;
      l_Params.imageData = l_Texture.data();
      g_Texture = Interface::Image2D::make(N(TextTextureImage), l_Params);

      LOW_PROFILE_END();
    }

    static void initialize_backend_types()
    {
      Interface::Context::initialize();
      Interface::CommandPool::initialize();
      Interface::CommandBuffer::initialize();
      Interface::Framebuffer::initialize();
      Interface::Renderpass::initialize();
      Interface::Swapchain::initialize();
      Interface::Image2D::initialize();
      Interface::UniformPool::initialize();
      Interface::Uniform::initialize();
      Interface::UniformScopeInterface::initialize();
      Interface::UniformScope::initialize();
      Interface::PipelineInterface::initialize();
      Interface::GraphicsPipeline::initialize();
      Interface::Buffer::initialize();
    }

    static void cleanup_backend_types()
    {
      Interface::Buffer::cleanup();
      Interface::GraphicsPipeline::cleanup();
      Interface::PipelineInterface::cleanup();
      Interface::UniformScope::cleanup();
      Interface::UniformScopeInterface::cleanup();
      Interface::Uniform::cleanup();
      Interface::UniformPool::cleanup();
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
      LOW_PROFILE_START(Render init);

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
        load_texture();

        Interface::UniformPoolCreateParams l_PoolParams;
        l_PoolParams.uniformBufferCount = 10;
        l_PoolParams.storageBufferCount = 0;
        l_PoolParams.samplerCount = 10;
        l_PoolParams.rendertargetCount = 0;
        l_PoolParams.scopeCount = 20;
        l_PoolParams.context = g_Context;

        Interface::UniformScopeInterfaceCreateParams l_UParams;
        l_UParams.context = g_Context;
        {
          l_UParams.uniformInterfaces.resize(2);
          l_UParams.uniformInterfaces[0].uniformCount = 1;
          l_UParams.uniformInterfaces[0].pipelineStep =
              Backend::UniformPipelineStep::GRAPHICS;
          l_UParams.uniformInterfaces[0].type =
              Backend::UniformType::UNIFORM_BUFFER;
          l_UParams.uniformInterfaces[1].uniformCount = 1;
          l_UParams.uniformInterfaces[1].pipelineStep =
              Backend::UniformPipelineStep::GRAPHICS;
          l_UParams.uniformInterfaces[1].type = Backend::UniformType::SAMPLER;
        }
        g_UniformScopeInterface = Interface::UniformScopeInterface::make(
            N(UniformScopeInterfaceTest), l_UParams);

        {
          Interface::UniformBufferCreateParams l_Params;
          l_Params.arrayIndex = 0;
          l_Params.bufferType = Backend::UniformBufferType::UNIFORM_BUFFER;
          l_Params.bufferSize = sizeof(float);
          l_Params.binding = 0;
          l_Params.context = g_Context;
          l_Params.swapchain = g_Swapchain;
          g_Uniform =
              Interface::Uniform::make_buffer(N(TestFloatUniform), l_Params);

          float v = 0.9f;
          g_Uniform.set_buffer_initial(&v);
        }
        {
          Interface::UniformImageCreateParams l_Params;
          l_Params.arrayIndex = 0;
          l_Params.imageType = Backend::UniformImageType::SAMPLER;
          l_Params.image = g_Texture;
          l_Params.binding = 1;
          l_Params.context = g_Context;
          l_Params.swapchain = g_Swapchain;
          g_TexUniform =
              Interface::Uniform::make_image(N(TestSamplerUniform), l_Params);
        }
        {
          Interface::UniformScopeCreateParams l_Params;
          l_Params.uniforms.push_back(g_Uniform);
          l_Params.uniforms.push_back(g_TexUniform);
          l_Params.context = g_Context;
          l_Params.swapchain = g_Swapchain;
          l_Params.pool =
              Interface::UniformPoolUtils::get_uniform_pool(l_PoolParams);
          l_Params.interface = g_UniformScopeInterface;

          g_UniformScope =
              Interface::UniformScope::make(N(TestUniformScope), l_Params);
          g_Scopes.push_back(g_UniformScope);
        }

        Interface::PipelineInterfaceCreateParams l_InterParams;
        l_InterParams.context = g_Context;
        l_InterParams.uniformScopeInterfaces.push_back(g_UniformScopeInterface);
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
        l_Params.vertexInput = true;

        g_Pipeline = Interface::GraphicsPipeline::make(N(TestGraphicsPipeline),
                                                       l_Params);
      }

      {
        Util::List<Interface::Vertex> l_Vertices;

        float z = 0.0f;

        {
          Interface::Vertex l_Vertex;
          l_Vertex.position = Math::Vector3(-0.5f, -0.5f, z);
          l_Vertex.textureCoordinates = Math::Vector2(1.0f, 0.0f);
          l_Vertices.push_back(l_Vertex);
        }
        {
          Interface::Vertex l_Vertex;
          l_Vertex.position = Math::Vector3(0.5f, -0.5f, z);
          l_Vertex.textureCoordinates = Math::Vector2(0.0f, 0.0f);
          l_Vertices.push_back(l_Vertex);
        }
        {
          Interface::Vertex l_Vertex;
          l_Vertex.position = Math::Vector3(0.5f, 0.5f, z);
          l_Vertex.textureCoordinates = Math::Vector2(0.0f, 1.0f);
          l_Vertices.push_back(l_Vertex);
        }
        {
          Interface::Vertex l_Vertex;
          l_Vertex.position = Math::Vector3(-0.5f, 0.5f, z);
          l_Vertex.textureCoordinates = Math::Vector2(1.0f, 1.0f);
          l_Vertices.push_back(l_Vertex);
        }

        Interface::BufferCreateParams l_Params;
        l_Params.commandPool = g_CommandPool;
        l_Params.context = g_Context;
        l_Params.data = l_Vertices.data();
        l_Params.bufferSize = static_cast<uint32_t>(l_Vertices.size() *
                                                    sizeof(Interface::Vertex));
        l_Params.bufferUsageType = Backend::BufferUsageType::VERTEX;
        g_VertexBuffer = Interface::Buffer::make(N(VertexBuffer), l_Params);
      }

      {
        Util::List<uint32_t> l_Indices = {0, 1, 2, 2, 3, 0};
        Interface::BufferCreateParams l_Params;
        l_Params.commandPool = g_CommandPool;
        l_Params.context = g_Context;
        l_Params.data = l_Indices.data();
        l_Params.bufferSize =
            static_cast<uint32_t>(l_Indices.size() * sizeof(uint32_t));
        l_Params.bufferUsageType = Backend::BufferUsageType::INDEX;
        g_IndexBuffer = Interface::Buffer::make(N(IndexBuffer), l_Params);
      }

      LOW_PROFILE_END();
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

      {
        Interface::UniformScopeBindGraphicsParams l_Params;
        l_Params.context = g_Context;
        l_Params.startIndex = 0;
        l_Params.swapchain = g_Swapchain;
        l_Params.scopes = g_Scopes;
        l_Params.pipeline = g_Pipeline;

        Interface::UniformScope::bind(l_Params);
      }

      g_VertexBuffer.bind_vertex(g_Swapchain, 0);
      g_IndexBuffer.bind_index(g_Swapchain, 0,
                               Backend::BufferBindIndexType::UINT32);

      Interface::DrawIndexedParams l_Params;
      l_Params.commandBuffer = g_Swapchain.get_current_commandbuffer();
      l_Params.firstInstance = 0;
      l_Params.firstIndex = 0;
      l_Params.indexCount = 6;
      l_Params.vertexOffset = 0;
      l_Params.instanceCount = 1;

      Interface::draw_indexed(l_Params);

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
