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

      uint32_t imageformat_get_pipeline_write_mask(uint8_t p_Format)
      {
        switch (p_Format) {
        case ImageFormat::BGRA8_SRGB:
        case ImageFormat::BGRA8_UNORM:
        case ImageFormat::RGBA32_SFLOAT:
        case ImageFormat::RGBA8_UNORM:
          return LOW_RENDERER_COLOR_WRITE_BIT_RED |
                 LOW_RENDERER_COLOR_WRITE_BIT_GREEN |
                 LOW_RENDERER_COLOR_WRITE_BIT_BLUE |
                 LOW_RENDERER_COLOR_WRITE_BIT_ALPHA;
        case ImageFormat::R8_UNORM:
        case ImageFormat::R32_UINT:
          return LOW_RENDERER_COLOR_WRITE_BIT_RED;
        default:
          LOW_ASSERT(false, "Unknown image format");
          return 0u;
        }
      }
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
