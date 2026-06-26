#include "LowGfxContext.h"
#include "LowUtil.h"
#include "LowUtilLogger.h"

#include <chrono>
#include <thread>

namespace {
  uint8_t map_log_level(Low::Gfx::LogLevel p_Level)
  {
    switch (p_Level) {
    case Low::Gfx::LogLevel::Trace:
      return Low::Util::Log::LogLevel::TRACE;
    case Low::Gfx::LogLevel::Debug:
      return Low::Util::Log::LogLevel::DEBUG;
    case Low::Gfx::LogLevel::Info:
      return Low::Util::Log::LogLevel::INFO;
    case Low::Gfx::LogLevel::Warning:
      return Low::Util::Log::LogLevel::WARN;
    case Low::Gfx::LogLevel::Error:
      return Low::Util::Log::LogLevel::ERROR;
    case Low::Gfx::LogLevel::Fatal:
      return Low::Util::Log::LogLevel::FATAL;
    }

    return Low::Util::Log::LogLevel::INFO;
  }

  void lowgfx_log_callback(Low::Gfx::LogLevel p_Level,
                           const char *p_Message, void *p_UserData)
  {
    (void)p_UserData;

    Low::Util::Log::begin_log(map_log_level(p_Level), LOW_MODULE_NAME)
        << p_Message << LOW_LOG_END;
  }
} // namespace

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  Low::Util::set_main_window_initially_hidden(false);
  Low::Util::initialize();

  {
    Low::Util::Window &l_Window =
        Low::Util::Window::get_main_window();

    Low::Gfx::WindowDesc l_WindowDesc;
    l_WindowDesc.backend = Low::Gfx::WindowBackend::SDL;
    l_WindowDesc.handle = l_Window.sdlwindow;

    Low::Gfx::InstanceDesc l_InstanceDesc;
    l_InstanceDesc.log_callback = &lowgfx_log_callback;
    l_InstanceDesc.surface_window = l_WindowDesc;

    Low::Gfx::Instance l_Instance(l_InstanceDesc);

    Low::Gfx::SurfaceDesc l_SurfaceDesc;
    l_SurfaceDesc.window = l_WindowDesc;
    Low::Gfx::Surface l_Surface =
        l_Instance.create_surface(l_SurfaceDesc);

    Low::Gfx::AdapterSelectionDesc l_AdapterDesc;
    l_AdapterDesc.compatible_surface = l_Surface;
    Low::Gfx::Adapter l_Adapter =
        l_Instance.select_adapter(l_AdapterDesc);

    Low::Gfx::ContextDesc l_ContextDesc;
    Low::Gfx::Context l_Context(l_Instance, l_Adapter,
                                l_ContextDesc);

    Low::Gfx::SwapchainDesc l_SwapchainDesc;
    l_SwapchainDesc.surface = l_Surface;
    int l_WindowWidth = 0;
    int l_WindowHeight = 0;
    l_Window.get_size(&l_WindowWidth, &l_WindowHeight);
    l_SwapchainDesc.width = static_cast<u32>(l_WindowWidth);
    l_SwapchainDesc.height = static_cast<u32>(l_WindowHeight);
    Low::Gfx::Swapchain l_Swapchain =
        l_Context.create_swapchain(l_SwapchainDesc);

    while (!l_Window.shouldClose) {
      Low::Util::tick(1.0f / 60.0f);
      Low::Gfx::FrameContext l_Frame =
          l_Context.begin_frame();
      Low::Gfx::SwapchainFrame l_SwapchainFrame =
          l_Context.acquire_swapchain(l_Frame, l_Swapchain);
      l_Context.present(l_SwapchainFrame);
      l_Context.end_frame(l_Frame);
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    l_Context.destroy(l_Swapchain);
    l_Instance.destroy(l_Surface);
  }

  Low::Util::cleanup();
}
