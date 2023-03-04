#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"

#include "LowRendererWindow.h"
#include "LowRendererBackend.h"
#include "LowRendererImage.h"

#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

namespace Low {
  namespace Renderer {

    Backend::Context g_Context;
    Backend::Pipeline g_Pipeline;

    static void initialize_resource_types()
    {
      Resource::Image::initialize();
    }

    static void initialize_types()
    {
      initialize_resource_types();
    }

    void initialize()
    {
      Backend::initialize();

      initialize_types();

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

      Backend::PipelineResourceSignature l_Signature;
      {
        Backend::PipelineResourceDescription l_Resource;
        l_Resource.name = N(out_Color);
        l_Resource.step = Backend::ResourcePipelineStep::COMPUTE;
        l_Resource.arraySize = 1;
        l_Resource.type = Backend::ResourceType::IMAGE;

        Backend::PipelineResourceSignatureCreateParams l_Params;
        l_Params.resourceDescriptionCount = 1;
        l_Params.resourceDescriptions = &l_Resource;
        l_Params.context = &g_Context;
        l_Params.binding = 0;

        Backend::callbacks().pipeline_resource_signature_create(l_Signature,
                                                                l_Params);

        Backend::callbacks().pipeline_resource_signature_commit(l_Signature);
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
          l_Signature, N(out_Color), 0, l_ImageResource);

      {

        Util::String s =
            Util::String(LOW_DATA_PATH) + "/shader/dst/spv/test.comp.spv";

        Backend::PipelineComputeCreateParams l_Params;
        l_Params.context = &g_Context;
        l_Params.shaderPath = s.c_str();
        l_Params.signatureCount = 1;
        l_Params.signatures = &l_Signature;

        Backend::callbacks().pipeline_compute_create(g_Pipeline, l_Params);
      }
    }

    void tick(float p_Delta)
    {
      g_Context.window.tick();

      Backend::callbacks().frame_prepare(g_Context);
      Backend::callbacks().renderpass_begin(
          g_Context.renderpasses[g_Context.currentImageIndex]);
      Backend::callbacks().renderpass_end(
          g_Context.renderpasses[g_Context.currentImageIndex]);

      Backend::callbacks().pipeline_bind(g_Pipeline);
      Backend::callbacks().compute_dispatch(g_Context, {10, 10, 1});

      Backend::callbacks().frame_render(g_Context);
    }

    bool window_is_open()
    {
      return g_Context.window.is_open();
    }

    static void cleanup_resource_types()
    {
      Resource::Image::cleanup();
    }

    static void cleanup_types()
    {
      cleanup_resource_types();
    }

    void cleanup()
    {
      Backend::callbacks().context_wait_idle(g_Context);

      Backend::callbacks().pipeline_cleanup(g_Pipeline);

      cleanup_types();

      Backend::callbacks().context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
