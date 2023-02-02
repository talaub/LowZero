#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"

#include "LowRendererBackend.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Interface {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT SwapchainData
      {
        Low::Renderer::Backend::Swapchain pipeline;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(SwapchainData);
        }
      };

      struct LOW_EXPORT Swapchain : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Swapchain> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Swapchain();
        Swapchain(uint64_t p_Id);
        Swapchain(Swapchain &p_Copy);

        static Swapchain make(Low::Util::Name p_Name);
        void destroy();

        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Swapchain *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Swapchain &get_pipeline() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
