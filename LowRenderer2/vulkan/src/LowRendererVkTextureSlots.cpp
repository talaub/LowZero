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

        static u32 g_FloatCapacity;
        static u32 g_UintCapacity;
        static u32 g_IntCapacity;

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
          // TODO: Adjust Uint/Int capacities here as needed.
          //       Float gets the bulk (full GpuTexture capacity).
          //       Global.cpp and globals.glsl read/mirror these via get_capacity().
          g_FloatCapacity = GpuTexture::get_capacity();
          g_UintCapacity = 64;
          g_IntCapacity = 32;

          g_FreeFloatSlots.reserve(g_FloatCapacity);
          g_FreeUintSlots.reserve(g_UintCapacity);
          g_FreeIntSlots.reserve(g_IntCapacity);

          for (u32 i = 0; i < g_FloatCapacity; ++i) {
            g_FreeFloatSlots.push_back(i);
          }
          for (u32 i = 0; i < g_UintCapacity; ++i) {
            g_FreeUintSlots.push_back(i);
          }
          for (u32 i = 0; i < g_IntCapacity; ++i) {
            g_FreeIntSlots.push_back(i);
          }
        }

        u32 get_capacity(TextureFormatCategory p_Category)
        {
          if (p_Category == TextureFormatCategory::Float) {
            return g_FloatCapacity;
          } else if (p_Category == TextureFormatCategory::Int) {
            return g_IntCapacity;
          } else {
            return g_UintCapacity;
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
