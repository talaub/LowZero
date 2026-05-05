#pragma once

#include "LowRendererGpuTexture.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace TextureSlots {
        void initialize();
        void cleanup();

        u32 allocate(TextureFormatCategory p_Category);
        void release(TextureFormatCategory p_Category, u32 p_Slot);
      } // namespace TextureSlots
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
