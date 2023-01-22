#include "LowRenderer.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowRendererBackend.h"
#include "LowRendererWindow.h"

namespace Low {
  namespace Renderer {
    Backend::Context g_Context;

    void initialize()
    {
      Window l_Window;
      WindowInit l_WindowInit;
      l_WindowInit.dimensions.x = 1280;
      l_WindowInit.dimensions.y = 860;
      l_WindowInit.title = "LowEngine";

      window_initialize(l_Window, l_WindowInit);

      Backend::ContextInit l_ContextInit;
      l_ContextInit.validation_enabled = true;
      l_ContextInit.window = &l_Window;

      Backend::context_create(g_Context, l_ContextInit);

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
      Backend::context_cleanup(g_Context);
    }
  } // namespace Renderer
} // namespace Low
