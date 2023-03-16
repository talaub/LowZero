#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

#include "LowRendererWindow.h"
#include "LowRendererBackend.h"
#include "LowRendererImage.h"
#include "LowRendererBuffer.h"
#include "LowRendererComputePipeline.h"
#include "LowRendererPipelineResourceSignature.h"
#include "LowRendererInterface.h"
#include "LowRendererComputeStepConfig.h"
#include "LowRendererComputeStep.h"
#include "LowRendererRenderFlow.h"

#include "LowRendererResourceRegistry.h"
#include "LowRendererFrontendConfig.h"

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
    Interface::PipelineGraphicsCreateParams g_GraphicsPipelineCreateParams;

    Interface::PipelineGraphicsCreateParams g_GP2Pipelines;
    Interface::GraphicsPipeline g_GP2;
    Interface::Renderpass g_Rp;
    Resource::Image g_Image;

    ResourceRegistry g_ResourceRegistry;
    Util::String g_ConfigPath;

    RenderFlow g_MainRenderFlow;
    Util::Name g_MainRenderFlowName;

    static void initialize_frontend_types()
    {
      ComputeStepConfig::initialize();
      ComputeStep::initialize();
      RenderFlow::initialize();
    }

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
      initialize_frontend_types();
    }

    static void initialize_global_resources()
    {
      Util::Yaml::Node l_RootNode = Util::Yaml::load_file(
          (g_ConfigPath + "/renderer_global_resources.yaml").c_str());

      /*
      g_ResourceRegistry.initialize(g_Context, l_RootNode);

      g_ResourceRegistry.get_buffer_resource(N(context_dimensions))
          .set(&g_Context.get_dimensions());
      */
    }

    static void load_renderflows()
    {
      Util::String l_RenderFlowDirectory = g_ConfigPath + "/renderflows";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_RenderFlowDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".renderflow.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Util::String i_Filename = i_Path.substr(
              l_RenderFlowDirectory.length() + 1, i_Path.length());
          Util::String i_Name =
              i_Filename.substr(0, i_Filename.length() - l_Ending.length());

          RenderFlow::make(LOW_NAME(i_Name.c_str()), g_Context, i_Node);

          LOW_LOG_DEBUG << "RenderFlow " << i_Name << " loaded" << LOW_LOG_END;
        }
      }
    }

    static void load_renderstep_configs()
    {
      Util::String l_RenderpassDirectory = g_ConfigPath + "/rendersteps";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_RenderpassDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".renderstep.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());
          Util::String i_Filename = i_Path.substr(
              l_RenderpassDirectory.length() + 1, i_Path.length());
          Util::String i_Name =
              i_Filename.substr(0, i_Filename.length() - l_Ending.length());

          if (LOW_YAML_AS_STRING(i_Node["type"]) == "compute") {
            ComputeStepConfig::make(LOW_NAME(i_Name.c_str()), i_Node);
          } else {
            LOW_ASSERT(false, "Unknown renderstep type");
          }
          LOW_LOG_DEBUG << "Renderstep " << i_Name << " loaded" << LOW_LOG_END;
        }
      }
    }

    void initialize()
    {
      g_ConfigPath = Util::String(LOW_DATA_PATH) + "/_internal/renderer_config";
      g_MainRenderFlowName = N(test);

      Backend::initialize();

      initialize_types();

      load_renderstep_configs();

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

      initialize_global_resources();

      load_renderflows();

      {
        RenderFlow *l_RenderFlows = RenderFlow::living_instances();
        for (uint32_t i = 0; i < RenderFlow::living_count(); ++i) {
          if (l_RenderFlows[i].get_name() == g_MainRenderFlowName) {
            g_MainRenderFlow = l_RenderFlows[i];
            break;
          }
        }

        LOW_ASSERT(g_MainRenderFlow.is_alive(),
                   "Could not find main renderflow");
      }

      {{Backend::ImageResourceCreateParams l_Params;
      l_Params.context = &g_Context.get_context();
      l_Params.createImage = true;
      l_Params.imageData = nullptr;
      l_Params.dimensions = g_Context.get_dimensions();
      l_Params.imageDataSize = 0;
      l_Params.depth = false;
      l_Params.writable = true;
      l_Params.format = Backend::ImageFormat::RGBA32_SFLOAT;

      g_Image = Resource::Image::make(N(RenderImage), l_Params);
    }

    {
      Interface::RenderpassCreateParams l_Params;
      l_Params.useDepth = false;
      l_Params.context = g_Context;
      l_Params.dimensions = g_Context.get_dimensions();
      l_Params.clearColors.push_back({1.0f, 0.0f, 0.0f, 1.0f});
      l_Params.renderTargets.push_back(g_Image);

      g_Rp = Interface::Renderpass::make(N(RP), l_Params);
    }

    {
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
      Interface::PipelineGraphicsCreateParams l_Params;
      l_Params.context = g_Context;
      l_Params.vertexShaderPath = "fs2.vert";
      l_Params.fragmentShaderPath = "fs2.frag";
      l_Params.signatures = {};
      l_Params.cullMode = Backend::PipelineRasterizerCullMode::BACK;
      l_Params.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
      l_Params.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
      l_Params.dimensions = g_Context.get_dimensions();
      l_Params.renderpass = g_Rp;
      l_Params.colorTargets = l_ColorTargets;
      l_Params.vertexDataAttributeTypes = {};

      g_GP2Pipelines = l_Params;

      g_GP2 = Interface::GraphicsPipeline::make(N(FP), l_Params);
    }
  } // namespace Renderer

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
  g_ComputeSignature.set_sampler_resource(N(u_Texture), 0, g_Image);

  g_GraphicsSignature.set_sampler_resource(N(u_Texture), 0, l_ImageResource);

  {
    Interface::PipelineComputeCreateParams l_Params;
    l_Params.context = g_Context;
    l_Params.shaderPath = "test.comp";
    l_Params.signatures = {g_ComputeSignature};

    g_Pipeline = Interface::ComputePipeline::make(N(CompPipeline), l_Params);
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

    g_GraphicsPipelineCreateParams = l_Params;

    g_GraphicsPipeline =
        Interface::GraphicsPipeline::make(N(GraphicsPipeline), l_Params);
  }
} // namespace Low

void tick(float p_Delta)
{
  g_Context.get_window().tick();

  Interface::PipelineManager::tick(p_Delta);

  uint8_t l_ContextState = g_Context.prepare_frame();

  LOW_ASSERT(l_ContextState != Backend::ContextState::FAILED,
             "Frame prepare was not successful");

  if (l_ContextState == Backend::ContextState::OUT_OF_DATE) {
    g_Context.update_dimensions();

    g_GraphicsPipelineCreateParams.dimensions = g_Context.get_dimensions();
    Interface::PipelineManager::register_graphics_pipeline(
        g_GraphicsPipeline, g_GraphicsPipelineCreateParams);
    return;
  }

  g_MainRenderFlow.execute();

  g_Context.clear_committed_resource_signatures();
  g_Rp.begin();
  g_GP2.bind();
  {
    Backend::DrawParams l_Params;
    l_Params.context = &g_Context.get_context();
    l_Params.firstVertex = 0;
    l_Params.vertexCount = 3;
    Backend::callbacks().draw(l_Params);
  }
  g_Rp.end();

  g_ComputeSignature.commit();

  g_Pipeline.bind();
  Backend::callbacks().compute_dispatch(g_Context.get_context(), {38, 38, 1});

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

static void cleanup_frontend_types()
{
  RenderFlow::cleanup();
  ComputeStep::cleanup();
  ComputeStepConfig::cleanup();
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
  cleanup_frontend_types();
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
