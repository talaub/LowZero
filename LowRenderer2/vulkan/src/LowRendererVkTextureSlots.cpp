#include "LowRendererVkTextureSlots.h"

#include "LowRendererVulkan.h"

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

        struct RetiredSlot
        {
          TextureFormatCategory category;
          u32 slot;
          u64 availableFrame;
        };

        static Util::List<RetiredSlot> g_RetiredSlots;

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

        static void promote_retired_slots()
        {
          const u64 l_FrameNumber = Global::get_frame_number();
          for (auto it = g_RetiredSlots.begin();
               it != g_RetiredSlots.end();) {
            if (it->availableFrame > l_FrameNumber) {
              ++it;
              continue;
            }

            get_free_list(it->category).push_back(it->slot);
            it = g_RetiredSlots.erase(it);
          }
        }

        void initialize()
        {
          // TODO: Adjust Uint/Int capacities here as needed.
          //       Float gets the bulk (full GpuTexture capacity).
          //       Global.cpp and globals.glsl read/mirror these via
          //       get_capacity().
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
          g_RetiredSlots.clear();
        }

        u32 allocate(TextureFormatCategory p_Category)
        {
          promote_retired_slots();

          Util::List<u32> &l_FreeList = get_free_list(p_Category);
          LOW_ASSERT(!l_FreeList.empty(),
                     "No free bindless texture slots available");
          u32 l_Slot = l_FreeList.back();
          l_FreeList.pop_back();
          return l_Slot;
        }

        void release(TextureFormatCategory p_Category, u32 p_Slot)
        {
          RetiredSlot l_Slot;
          l_Slot.category = p_Category;
          l_Slot.slot = p_Slot;
          l_Slot.availableFrame = Global::get_frame_number() +
                                  Global::get_frame_overlap() + 1u;
          g_RetiredSlots.push_back(l_Slot);
        }
      } // namespace TextureSlots
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
