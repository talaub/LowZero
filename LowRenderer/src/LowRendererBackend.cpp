#include "LowRendererBackend.h"

#include <stdlib.h>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

namespace Low {
  namespace Renderer {
    namespace Backend {
      void context_create(Context &p_Context, ContextCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_create(p_Context, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void context_cleanup(Context &p_Context)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_cleanup(p_Context);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
        p_Context.m_Window.cleanup();
      }

      void framebuffer_create(Framebuffer &p_Framebuffer,
                              FramebufferCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_create(p_Framebuffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void framebuffer_get_dimensions(Framebuffer &p_Framebuffer,
                                      Math::UVector2 &p_Dimensions)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_get_dimensions(p_Framebuffer, p_Dimensions);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void framebuffer_cleanup(Framebuffer &p_Framebuffer)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_cleanup(p_Framebuffer);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandpool_create(CommandPool &p_CommandPool,
                              CommandPoolCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandpool_create(p_CommandPool, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandpool_cleanup(CommandPool &p_CommandPool)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandpool_cleanup(p_CommandPool);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
