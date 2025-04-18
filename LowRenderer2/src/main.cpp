#include "LowUtilLogger.h"
#include "LowUtil.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include "LowRendererCompatibility.h"

#include "LowRenderer.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderView.h"

#include "imgui_impl_sdl2.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

Low::Util::Window g_MainWindow;

Low::Renderer::RenderView g_RenderView;

namespace Low {
  namespace Util {
    Window &Window::get_main_window()
    {
      return g_MainWindow;
    }
  } // namespace Util
} // namespace Low
  //
Low::Math::UVector2 g_Dimensions{1700, 900};

void draw()
{
  Low::Renderer::tick(0.1f);
}

bool stop_rendering = false;

void run()
{
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit) {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0) {
      // close the window when user alt-f4s or clicks the X button
      if (e.type == SDL_QUIT)
        bQuit = true;

      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
          stop_rendering = true;
        }
        if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
          stop_rendering = false;
        }
      }

      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    // do not draw if we are minimized
    if (stop_rendering) {
      // throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    Low::Renderer::check_window_resize(0.0f);

    draw();
  }
}

void init()
{
  Low::Renderer::initialize();
  {
    Low::Util::String l_BasePath = Low::Util::get_project().dataPath;
    l_BasePath += "/_internal/assets/meshes/cube.glb";

    Low::Renderer::MeshResource l_MeshResource =
        Low::Renderer::load_mesh(l_BasePath);

    Low::Math::Vector3 l_Position(0.0f);
    Low::Math::Quaternion l_Rotation(1.0f, 0.0f, 0.0f, 0.0f);
    Low::Math::Vector3 l_Scale(1.0f);

    Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

    l_LocalMatrix = glm::translate(l_LocalMatrix, l_Position);
    l_LocalMatrix *= glm::toMat4(l_Rotation);
    l_LocalMatrix = glm::scale(l_LocalMatrix, l_Scale);

    g_RenderView = Low::Renderer::RenderView::make("Default");
    g_RenderView.set_dimensions(g_Dimensions);

    Low::Math::Vector3 p(0.0f, 0.0f, -14.0f);
    Low::Math::Vector3 d(0.0f, 0.0f, 1.0f);

    g_RenderView.set_camera_position(p);
    g_RenderView.set_camera_direction(d);

    Low::Renderer::RenderObject l_RenderObject =
        Low::Renderer::RenderObject::make(g_RenderView);
    l_RenderObject.set_mesh_resource(l_MeshResource);
    l_RenderObject.set_world_transform(l_LocalMatrix);
  }
}

void cleanup()
{
  Low::Renderer::cleanup();
}

int main(int argc, char *argv[])
{
  Low::Util::initialize();

  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  g_MainWindow.sdlwindow =
      SDL_CreateWindow("LowRenderer2", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, g_Dimensions.x,
                       g_Dimensions.y, window_flags);

  init();
  LOW_LOG_DEBUG << "Test" << LOW_LOG_END;

  run();

  cleanup();

  LOW_LOG_DEBUG << "Testrenderer2" << LOW_LOG_END;

  SDL_DestroyWindow(g_MainWindow.sdlwindow);

  Low::Util::cleanup();
  return 0;
}
