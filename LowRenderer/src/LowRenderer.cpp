#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"

#include "LowRendererWindow.h"
#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"
#include "LowRendererComputePipeline.h"
#include "LowRendererPipelineResourceSignature.h"
#include "LowRendererInterface.h"

#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

namespace Low {
  namespace Renderer {

    Interface::Context g_Context;
    Interface::ComputePipeline g_Pipeline;
    Interface::GraphicsPipeline g_GraphicsPipeline;
    Interface::PipelineResourceSignature g_ComputeSignature;
    Interface::PipelineResourceSignature g_GraphicsSignature;

    static void initialize_resource_types()
    {
      Resource::Image::initialize();
      Resource::Buffer::initialize();
    }

    static void initialize_interface_types()
    {
      Interface::Context::initialize();
      Interface::Renderpass::initialize();
      Interface::PipelineResourceSignature::initialize();
      Interface::ComputePipeline::initialize();
      Interface::GraphicsPipeline::initialize();
    }

    static void initialize_types()
    {
      initialize_resource_types();
      initialize_interface_types();
    }

    void initialize()
    {
      Backend::initialize();

      initialize_types();

      Util::Resource::Image2D l_Resource;
      Util::Resource::load_image2d(
          (Util::String(LOW_DATA_PATH) + "/assets/img2d/out_wb.ktx").c_str(),
          l_Resource);

      Window l_Window;
      WindowInit l_WindowInit;
      l_WindowInit.dimensions.x = 1280;
      l_WindowInit.dimensions.y = 860;
      l_WindowInit.title = "LowEngine";

      window_initialize(l_Window, l_WindowInit);

      g_Context =
          Interface::Context::make(N(DefaultContext), &l_Window, 2, true);

      {
        Util::List<Backend::PipelineResourceDescription> l_Resources;

        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(out_Color);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.arraySize = 1;
          l_Resource.type = Backend::ResourceType::IMAGE;
          l_Resources.push_back(l_Resource);
        }
        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_Texture);
          l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
          l_Resource.arraySize = 1;
          l_Resource.type = Backend::ResourceType::SAMPLER;
          l_Resources.push_back(l_Resource);
        }

        g_ComputeSignature = Interface::PipelineResourceSignature::make(
            N(ComputeSignature), g_Context, 0, l_Resources);
      }

      {
        Util::List<Backend::PipelineResourceDescription> l_Resources;

        {
          Backend::PipelineResourceDescription l_Resource;
          l_Resource.name = N(u_Texture);
          l_Resource.step = Backend::ResourcePipelineStep::FRAGMENT;
          l_Resource.arraySize = 1;
          l_Resource.type = Backend::ResourceType::SAMPLER;
          l_Resources.push_back(l_Resource);
        }

        g_GraphicsSignature = Interface::PipelineResourceSignature::make(
            N(GraphicsSignature), g_Context, 0, l_Resources);
      }

      Resource::Image l_TextureResource;
      {
        Backend::ImageResourceCreateParams l_Params;
        l_Params.createImage = true;
        l_Params.imageData = l_Resource.data[0].data();
        l_Params.imageDataSize = l_Resource.data[0].size();
        l_Params.dimensions = l_Resource.dimensions[0];
        l_Params.context = &g_Context.get_context();
        l_Params.depth = false;
        l_Params.writable = false;
        l_Params.format = Backend::ImageFormat::RGBA8_UNORM;

        l_TextureResource = Resource::Image::make(N(Texture), l_Params);
      }

      Resource::Image l_ImageResource;
      {
        Backend::ImageResourceCreateParams l_Params;
        l_Params.createImage = true;
        l_Params.imageData = nullptr;
        l_Params.imageDataSize = 0;
        l_Params.dimensions = Math::UVector2(600, 600);
        l_Params.context = &g_Context.get_context();
        l_Params.depth = false;
        l_Params.writable = true;
        l_Params.format = Backend::ImageFormat::RGBA32_SFLOAT;

        l_ImageResource = Resource::Image::make(N(TestImage), l_Params);
      }

      g_ComputeSignature.set_image_resource(N(out_Color), 0, l_ImageResource);
      g_ComputeSignature.set_sampler_resource(N(u_Texture), 0,
                                              l_TextureResource);

      g_GraphicsSignature.set_sampler_resource(N(u_Texture), 0,
                                               l_ImageResource);

      {
        Interface::PipelineComputeCreateParams l_Params;
        l_Params.context = g_Context;
        l_Params.shaderPath = "test.comp";
        l_Params.signatures = {g_ComputeSignature};

        g_Pipeline =
            Interface::ComputePipeline::make(N(CompPipeline), l_Params);
      }

      {
        Util::String vertex = "fs.vert";
        Util::String fragment = "fs.frag";

        Util::List<Backend::GraphicsPipelineColorTarget> l_ColorTargets;
        {
          Backend::GraphicsPipelineColorTarget l_Target;
          l_Target.blendEnable = false;
          l_Target.wirteMask = LOW_RENDERER_COLOR_WRITE_BIT_RED |
                               LOW_RENDERER_COLOR_WRITE_BIT_GREEN |
                               LOW_RENDERER_COLOR_WRITE_BIT_BLUE |
                               LOW_RENDERER_COLOR_WRITE_BIT_ALPHA;
          l_ColorTargets.push_back(l_Target);
        }

        Util::List<uint8_t> l_VertexAttributes;

        Interface::PipelineGraphicsCreateParams l_Params;
        l_Params.context = g_Context;
        l_Params.vertexShaderPath = vertex;
        l_Params.fragmentShaderPath = fragment;
        l_Params.signatures = {g_GraphicsSignature};
        l_Params.cullMode = Backend::PipelineRasterizerCullMode::BACK;
        l_Params.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
        l_Params.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
        l_Params.dimensions = g_Context.get_dimensions();
        l_Params.renderpass = g_Context.get_renderpasses()[0];
        l_Params.colorTargets = l_ColorTargets;
        l_Params.vertexDataAttributeTypes = l_VertexAttributes;

        g_GraphicsPipeline =
            Interface::GraphicsPipeline::make(N(GraphicsPipeline), l_Params);
      }
    }

    void tick(float p_Delta)
    {
      g_Context.get_window().tick();

      Interface::PipelineManager::tick(p_Delta);

      uint8_t l_ContextState = g_Context.prepare_frame();

      LOW_ASSERT(l_ContextState == Backend::ContextState::SUCCESS,
                 "Frame prepare was not successful");

      g_ComputeSignature.commit();

      g_Pipeline.bind();
      Backend::callbacks().compute_dispatch(g_Context.get_context(),
                                            {38, 38, 1});

      g_GraphicsSignature.commit();

      g_Context.get_current_renderpass().begin();

      g_GraphicsPipeline.bind();

      {
        Backend::DrawParams l_Params;
        l_Params.context = &g_Context.get_context();
        l_Params.firstVertex = 0;
        l_Params.vertexCount = 3;
        Backend::callbacks().draw(l_Params);
      }

      g_Context.get_current_renderpass().end();

      g_Context.render_frame();
    }

    bool window_is_open()
    {
      return g_Context.get_window().is_open();
    }

    static void cleanup_resource_types()
    {
      Resource::Image::cleanup();
      Resource::Buffer::cleanup();
    }

    static void cleanup_interface_types()
    {
      Interface::ComputePipeline::cleanup();
      Interface::GraphicsPipeline::cleanup();
      Interface::PipelineResourceSignature::cleanup();
      Interface::Renderpass::cleanup();
      Interface::Context::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_resource_types();
      cleanup_interface_types();
    }

    void cleanup()
    {
      g_Context.wait_idle();

      cleanup_types();
    }
  } // namespace Renderer
} // namespace Low
