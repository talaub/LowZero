#include "LowRendererVkTextureSlots.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace TextureSlots {
        static Util::List<u32> g_FreeFloatSlots;
        static Util::List<u32> g_FreeIntSlots;
        static Util::List<u32> g_FreeUintSlots;

        static Util::List<u32> &
        get_free_list(TextureFormatCategory p_Category)
        {
          if (p_Category == TextureFormatCategory::Float) {
            return g_FreeFloatSlots;
          } else if (p_Category == TextureFormatCategory::Int) {
            return g_FreeIntSlots;
          } else {
            return g_FreeUintSlots;
          }
        }

        void initialize()
        {
          u32 l_Capacity = GpuTexture::get_capacity();
          g_FreeFloatSlots.reserve(l_Capacity);
          g_FreeIntSlots.reserve(l_Capacity);
          g_FreeUintSlots.reserve(l_Capacity);
          for (u32 i = 0; i < l_Capacity; ++i) {
            g_FreeFloatSlots.push_back(i);
            g_FreeIntSlots.push_back(i);
            g_FreeUintSlots.push_back(i);
          }
        }

        void cleanup()
        {
          g_FreeFloatSlots.clear();
          g_FreeIntSlots.clear();
          g_FreeUintSlots.clear();
        }

        u32 allocate(TextureFormatCategory p_Category)
        {
          Util::List<u32> &l_FreeList = get_free_list(p_Category);
          LOW_ASSERT(!l_FreeList.empty(),
                     "No free bindless texture slots available");
          u32 l_Slot = l_FreeList.back();
          l_FreeList.pop_back();
          return l_Slot;
        }

        void release(TextureFormatCategory p_Category, u32 p_Slot)
        {
          get_free_list(p_Category).push_back(p_Slot);
        }
      } // namespace TextureSlots
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
