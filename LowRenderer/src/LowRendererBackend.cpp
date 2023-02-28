#include "LowRendererBackend.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include <stdlib.h>
#include <string>

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
      ApiBackendCallback g_Callbacks;

      void initialize()
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::initialize_callback(g_Callbacks);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      ApiBackendCallback &callbacks()
      {
        return g_Callbacks;
      }

    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
