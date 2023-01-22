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
      void context_create(Context &p_Context, ContextInit &p_Init)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_create(p_Context, p_Init);
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
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
