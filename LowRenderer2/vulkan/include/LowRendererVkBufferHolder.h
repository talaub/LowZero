#pragma once

#include "LowRenderer2Api.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

#include "LowRendererVulkan.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_RENDERER2_API BufferHolder : public Low::Util::Handle
      {
      public:
        struct Data
        {
        public:
          AllocatedBuffer bf;
          Low::Util::Name name;

          static size_t get_size()
          {
            return sizeof(Data);
          }
        };

      private:
        static u16 ms_TypeId;

      public:
        static Low::Util::List<Low::Util::Instances::Page *> ms_Pages;

        static Low::Util::List<BufferHolder> ms_LivingInstances;

        const static Low::Util::TypeIdentifier IDENTIFIER;

        [[nodiscard]] static u16 type_id()
        {
          return ms_TypeId;
        }

        static BufferHolder make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        explicit BufferHolder(const BufferHolder &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        void destroy();

        static void initialize();
        static void cleanup();

        BufferHolder(u64 p_Id) : Low::Util::Handle(p_Id)
        {
        }
        BufferHolder() : Low::Util::Handle()
        {
        }
        BufferHolder(Low::Util::Handle p_Handle)
            : Low::Util::Handle(p_Handle.get_id())
        {
        }

        using Handle::operator=;

        BufferHolder &operator=(const BufferHolder &) = default;
        BufferHolder &operator=(BufferHolder &&) noexcept = default;

        static uint32_t living_count()
        {
          return static_cast<uint32_t>(ms_LivingInstances.size());
        }
        static BufferHolder *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static BufferHolder create_handle_by_index(u32 p_Index);

        static BufferHolder find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Handle p_Observer) const;
        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Function<void(Low::Util::Handle,
                                             Low::Util::Name)>
                        p_Observer) const;
        void notify(Low::Util::Handle p_Observed,
                    Low::Util::Name p_Observable);
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Serial::Node &p_Node) const;

        BufferHolder duplicate(Low::Util::Name p_Name) const;
        static BufferHolder duplicate(BufferHolder p_Handle,
                                      Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static BufferHolder find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Serial::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Serial::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          BufferHolder l_Handle = p_Handle.get_id();
          return l_Handle.is_alive();
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          BufferHolder l_BufferHolder = p_Handle.get_id();
          l_BufferHolder.destroy();
        }

        AllocatedBuffer get() const;
        void set_bf(AllocatedBuffer p_Value);

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static bool get_page_for_index(const u32 p_Index,
                                       u32 &p_PageIndex,
                                       u32 &p_SlotIndex);

      private:
        static u32 ms_Capacity;
        static u32 ms_PageSize;
        static u32 create_instance(u32 &p_PageIndex,
                                   u32 &p_SlotIndex);
        static u32 create_page();

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE

        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low

// LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_HEADER_CODE
// LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_HEADER_CODE
