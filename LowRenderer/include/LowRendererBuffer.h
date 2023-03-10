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
    namespace Resource {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_EXPORT BufferData
      {
        Backend::Buffer buffer;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(BufferData);
        }
      };

      struct LOW_EXPORT Buffer : public Low::Util::Handle
      {
      public:
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Buffer> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Buffer();
        Buffer(uint64_t p_Id);
        Buffer(Buffer &p_Copy);

      private:
        static Buffer make(Low::Util::Name p_Name);

      public:
        explicit Buffer(const Buffer &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static Buffer *living_instances()
        {
          return ms_LivingInstances.data();
        }

        bool is_alive() const;

        static uint32_t get_capacity();

        Backend::Buffer &get_buffer() const;
        void set_buffer(Backend::Buffer &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Buffer make(Util::Name p_Name,
                           Backend::BufferCreateParams &p_Params);
      };
    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
