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
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT RenderpassData
      {
        Backend::Renderpass renderpass;
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

        static Renderpass make(Low::Util::Name p_Name);
        explicit Renderpass(const Renderpass &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
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

        Backend::Renderpass &get_renderpass() const;
        void set_renderpass(Backend::Renderpass &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Renderpass make(Util::Name p_Name,
                               RenderpassCreateParams &p_Params);
        Math::UVector2 &get_dimensions();
        void begin();
        void end();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
