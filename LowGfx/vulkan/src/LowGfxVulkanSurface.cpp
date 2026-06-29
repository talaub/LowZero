
#include "LowGfxBackend.h"
#include "LowGfxContext.h"
#include "LowGfxLogInternal.h"

#include "LowGfxAssert.h"

#include "LowGfxVulkanState.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      static void create_window_surface(
          Detail::InstanceImpl &p_Instance,
          const VulkanInstanceState &p_State,
          VulkanSurfaceState &p_Surface, const SurfaceDesc &p_Desc)
      {
        switch (p_Desc.window.backend) {
        case WindowBackend::None:
          return;
        case WindowBackend::SDL: {
          SDL_Window *l_Window =
              static_cast<SDL_Window *>(p_Desc.window.handle);
          GFX_ASSERT(l_Window,
                     "SDL surface creation requires a window handle");
          if (!SDL_Vulkan_CreateSurface(l_Window, p_State.instance,
                                        &p_Surface.surface)) {
            Detail::logf(p_Instance, LogLevel::Error,
                         "Failed to create SDL Vulkan surface: {}",
                         SDL_GetError());
            GFX_ASSERT(false, "Failed to create SDL Vulkan surface");
          }
          return;
        }
        }

        GFX_ASSERT(false, "Unsupported LowGfx window backend");
      }

      Detail::BackendSurface
      create_surface(Detail::InstanceImpl &p_Instance,
                     const SurfaceDesc &p_Desc)
      {
        VulkanInstanceState *l_State =
            static_cast<VulkanInstanceState *>(
                p_Instance.backend_state);

        GFX_ASSERT(
            l_State,
            "Cannot create Vulkan surface without instance state");

        VulkanSurfaceState *l_Surface = new VulkanSurfaceState;
        create_window_surface(p_Instance, *l_State, *l_Surface,
                              p_Desc);

        Detail::BackendSurface l_Result;
        l_Result.backend_state = l_Surface;
        return l_Result;
      }

      void destroy_surface(Detail::InstanceImpl &p_Instance,
                           Detail::BackendSurface &p_Surface)
      {
        VulkanInstanceState *l_State =
            static_cast<VulkanInstanceState *>(
                p_Instance.backend_state);

        GFX_ASSERT(
            l_State,
            "Cannot destroy Vulkan surface without instance state");

        VulkanSurfaceState *l_Surface =
            static_cast<VulkanSurfaceState *>(
                p_Surface.backend_state);

        if (l_Surface) {
          if (l_Surface->surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(l_State->instance,
                                l_Surface->surface, nullptr);
          }

          delete l_Surface;
        }

        p_Surface.backend_state = nullptr;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
