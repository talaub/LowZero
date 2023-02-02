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
      struct RenderpassCreateParams;
      struct RenderpassStartParams;
      struct RenderpassStopParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT RenderpassData
      {
        Low::Renderer::Backend::Renderpass renderpass;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(RenderpassData);
        }
      };

      struct LOW_EXPORT Renderpass : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Renderpass> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Renderpass();
        Renderpass(uint64_t p_Id);
        Renderpass(Renderpass &p_Copy);

      private:
        static Renderpass make(Low::Util::Name p_Name);

      public:
        void destroy();

        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Renderpass *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Renderpass &get_renderpass() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Renderpass make(Util::Name p_Name,
                               RenderpassCreateParams &p_Params);
        void start(RenderpassStartParams &p_Params);
        void stop(RenderpassStopParams &p_Params);
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
