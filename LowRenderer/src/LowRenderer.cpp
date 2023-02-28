#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilProfiler.h"

#include "LowRendererWindow.h"
#include "LowRendererBackend.h"

#include <stdint.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

namespace Low {
  namespace Renderer {

    Backend::Context g_Context;

    void initialize()
    {
      LOW_PROFILE_START(Render init);

      Backend::initialize();

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

      LOW_PROFILE_END();
    }

    void tick(float p_Delta)
    {
      g_Context.window.tick();

      Backend::callbacks().frame_prepare(g_Context);
      Backend::callbacks().renderpass_begin(
          g_Context.renderpasses[g_Context.currentImageIndex]);
      Backend::callbacks().renderpass_end(
          g_Context.renderpasses[g_Context.currentImageIndex]);
      Backend::callbacks().frame_render(g_Context);
    }

    bool window_is_open()
    {
      return g_Context.window.is_open();
    }

    void cleanup()
    {
      Backend::callbacks().context_wait_idle(g_Context);
      Backend::callbacks().context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
