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
      struct ContextCreateParams;
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT ContextData
      {
        Low::Renderer::Backend::Context context;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ContextData);
        }
      };

      struct LOW_EXPORT Context : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Context> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Context();
        Context(uint64_t p_Id);
        Context(Context &p_Copy);

      private:
        static Context make(Low::Util::Name p_Name);

      public:
        explicit Context(const Context &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Context *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Low::Renderer::Backend::Context &get_context() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Context make(Util::Name p_Name, ContextCreateParams &p_Params);
        void wait_idle();
        Window &get_window();
      };
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
