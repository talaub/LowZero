#include "LowRendererVulkanRenderer.h"

#include "LowRendererVulkanBase.h"
#include "LowRendererCompatibility.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      Context g_Context;

      bool setup_renderflow()
      {
        return true;
      }

      bool initialize()
      {
        int l_Width, l_Height;
        SDL_GetWindowSize(Util::Window::get_main_window().sdlwindow,
                          &l_Width, &l_Height);

        LOWR_VK_ASSERT_RETURN(
            Base::initialize(g_Context,
                             Math::UVector2(l_Width, l_Height)),
            "Failed to initialize vulkan renderer");

        LOWR_VK_ASSERT_RETURN(setup_renderflow(),
                              "Failed to setup renderflow");

        return true;
      }

      bool cleanup()
      {
        LOWR_VK_ASSERT_RETURN(Base::cleanup(g_Context),
                              "Failed to cleanup vulkan renderer");

        return true;
      }

      bool tick(float p_Delta)
      {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        LOWR_VK_ASSERT_RETURN(Base::draw(g_Context),
                              "Failed to draw vulkan renderer");
        return true;
      }

      bool check_window_resize(float p_Delta)
      {
        LOWR_VK_ASSERT_RETURN(
            Base::swapchain_resize(g_Context),
            "Failed to swapchain_resize vulkan renderer");
        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
