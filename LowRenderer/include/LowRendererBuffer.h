#pragma once

#include "LowRendererApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowRendererBackend.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Resource {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER_API BufferData
      {
        Backend::Buffer buffer;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(BufferData);
        }
      };

      struct LOW_RENDERER_API Buffer : public Low::Util::Handle
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

        static Buffer find_by_index(uint32_t p_Index);

        bool is_alive() const;

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        static Buffer find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle deserialize(Low::Util::Yaml::Node &p_Node,
                                             Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          return p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Buffer l_Buffer = p_Handle.get_id();
          l_Buffer.destroy();
        }

        Backend::Buffer &get_buffer() const;
        void set_buffer(Backend::Buffer &p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Buffer make(Util::Name p_Name,
                           Backend::BufferCreateParams &p_Params);
        void set(void *p_Data);
        void write(void *p_Data, uint32_t p_DataSize, uint32_t p_Start);
        void read(void *p_Data, uint32_t p_DataSize, uint32_t p_Start);
        void bind_vertex();
        void bind_index(uint8_t p_BindType);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
      };
    } // namespace Resource
  }   // namespace Renderer
} // namespace Low
