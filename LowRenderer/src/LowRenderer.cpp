#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"

#include "LowRendererWindow.h"
#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"

#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

namespace Low {
  namespace Renderer {

    Backend::Context g_Context;
    Backend::Pipeline g_Pipeline;
    Backend::Pipeline g_GraphicsPipeline;
    Backend::PipelineResourceSignature g_ComputeSignature;
    Backend::PipelineResourceSignature g_GraphicsSignature;

    static void initialize_resource_types()
    {
      Resource::Image::initialize();
      Resource::Buffer::initialize();
    }

    static void initialize_types()
    {
      initialize_resource_types();
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

      {
        Backend::ContextCreateParams l_Params;
        l_Params.window = &l_Window;
        l_Params.validation_enabled = true;
        l_Params.framesInFlight = 2;
        Backend::callbacks().context_create(g_Context, l_Params);
      }

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

        Backend::PipelineResourceSignatureCreateParams l_Params;
        l_Params.resourceDescriptionCount = l_Resources.size();
        l_Params.resourceDescriptions = l_Resources.data();
        l_Params.context = &g_Context;
        l_Params.binding = 0;

        Backend::callbacks().pipeline_resource_signature_create(
            g_ComputeSignature, l_Params);
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

        Backend::PipelineResourceSignatureCreateParams l_Params;
        l_Params.resourceDescriptionCount = l_Resources.size();
        l_Params.resourceDescriptions = l_Resources.data();
        l_Params.context = &g_Context;
        l_Params.binding = 0;

        Backend::callbacks().pipeline_resource_signature_create(
            g_GraphicsSignature, l_Params);
      }

      Resource::Image l_TextureResource;
      {
        Backend::ImageResourceCreateParams l_Params;
        l_Params.createImage = true;
        l_Params.imageData = l_Resource.data[0].data();
        l_Params.imageDataSize = l_Resource.data[0].size();
        l_Params.dimensions = l_Resource.dimensions[0];
        l_Params.context = &g_Context;
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
        l_Params.context = &g_Context;
        l_Params.depth = false;
        l_Params.writable = true;
        l_Params.format = Backend::ImageFormat::RGBA32_SFLOAT;

        l_ImageResource = Resource::Image::make(N(TestImage), l_Params);
      }

      Backend::callbacks().pipeline_resource_signature_set_image(
          g_ComputeSignature, N(out_Color), 0, l_ImageResource);

      Backend::callbacks().pipeline_resource_signature_set_sampler(
          g_ComputeSignature, N(u_Texture), 0, l_TextureResource);

      Backend::callbacks().pipeline_resource_signature_set_sampler(
          g_GraphicsSignature, N(u_Texture), 0, l_ImageResource);

      {

        Util::String s =
            Util::String(LOW_DATA_PATH) + "/shader/dst/spv/test.comp.spv";

        Backend::PipelineComputeCreateParams l_Params;
        l_Params.context = &g_Context;
        l_Params.shaderPath = s.c_str();
        l_Params.signatureCount = 1;
        l_Params.signatures = &g_ComputeSignature;

        Backend::callbacks().pipeline_compute_create(g_Pipeline, l_Params);
      }

      {
        Util::String vertex =
            Util::String(LOW_DATA_PATH) + "/shader/dst/spv/fs.vert.spv";
        Util::String fragment =
            Util::String(LOW_DATA_PATH) + "/shader/dst/spv/fs.frag.spv";

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

        Backend::PipelineGraphicsCreateParams l_Params;
        l_Params.context = &g_Context;
        l_Params.vertexShaderPath = vertex.c_str();
        l_Params.fragmentShaderPath = fragment.c_str();
        l_Params.signatureCount = 1;
        l_Params.signatures = &g_GraphicsSignature;
        l_Params.cullMode = Backend::PipelineRasterizerCullMode::BACK;
        l_Params.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
        l_Params.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
        l_Params.dimensions = {1280, 860};
        l_Params.renderpass = g_Context.renderpasses;
        l_Params.colorTargetCount = l_ColorTargets.size();
        l_Params.colorTargets = l_ColorTargets.data();
        l_Params.vertexDataAttributeCount = l_VertexAttributes.size();
        l_Params.vertexDataAttributesType = l_VertexAttributes.data();

        Backend::callbacks().pipeline_graphics_create(g_GraphicsPipeline,
                                                      l_Params);
      }
    }

    void tick(float p_Delta)
    {
      static int t = 0;
      g_Context.window.tick();

      Backend::callbacks().frame_prepare(g_Context);

      Backend::callbacks().pipeline_resource_signature_commit(
          g_ComputeSignature);

      Backend::callbacks().pipeline_bind(g_Pipeline);
      Backend::callbacks().compute_dispatch(g_Context, {38, 38, 1});

      Backend::callbacks().pipeline_resource_signature_commit(
          g_GraphicsSignature);

      Backend::callbacks().renderpass_begin(
          g_Context.renderpasses[g_Context.currentImageIndex]);

      Backend::callbacks().pipeline_bind(g_GraphicsPipeline);

      {
        Backend::DrawParams l_Params;
        l_Params.context = &g_Context;
        l_Params.firstVertex = 0;
        l_Params.vertexCount = 3;
        Backend::callbacks().draw(l_Params);
      }
      Backend::callbacks().renderpass_end(
          g_Context.renderpasses[g_Context.currentImageIndex]);

      Backend::callbacks().frame_render(g_Context);
    }

    bool window_is_open()
    {
      return g_Context.window.is_open();
    }

    static void cleanup_resource_types()
    {
      Resource::Image::cleanup();
      Resource::Buffer::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_resource_types();
    }

    void cleanup()
    {
      Backend::callbacks().context_wait_idle(g_Context);

      Backend::callbacks().pipeline_cleanup(g_Pipeline);
      Backend::callbacks().pipeline_cleanup(g_GraphicsPipeline);

      cleanup_types();

      Backend::callbacks().context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
