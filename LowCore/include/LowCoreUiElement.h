#pragma once

#include "LowCoreApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreUiView.h"

#include "shared_mutex"
// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE

// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      namespace Component {
        struct Display;
      }
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      struct LOW_CORE_API ElementData
      {
        Util::Map<uint16_t, Util::Handle> components;
        Low::Core::UI::View view;
        bool click_passthrough;
        Low::Util::UniqueId unique_id;
        Low::Util::Name name;

        static size_t get_size()
        {
          return sizeof(ElementData);
        }
      };

      struct LOW_CORE_API Element : public Low::Util::Handle
      {
      public:
        static std::shared_mutex ms_BufferMutex;
        static uint8_t *ms_Buffer;
        static Low::Util::Instances::Slot *ms_Slots;

        static Low::Util::List<Element> ms_LivingInstances;

        const static uint16_t TYPE_ID;

        Element();
        Element(uint64_t p_Id);
        Element(Element &p_Copy);

        static Element make(Low::Util::Name p_Name);
        static Low::Util::Handle _make(Low::Util::Name p_Name);
        static Element make(Low::Util::Name p_Name,
                            Low::Util::UniqueId p_UniqueId);
        explicit Element(const Element &p_Copy)
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
        static Element *living_instances()
        {
          return ms_LivingInstances.data();
        }

        static Element find_by_index(uint32_t p_Index);
        static Low::Util::Handle _find_by_index(uint32_t p_Index);

        bool is_alive() const;

        u64 observe(Low::Util::Name p_Observable,
                    Low::Util::Handle p_Observer) const;
        void notify(Low::Util::Handle p_Observed,
                    Low::Util::Name p_Observable);
        void broadcast_observable(Low::Util::Name p_Observable) const;

        static void _notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable);

        static uint32_t get_capacity();

        void serialize(Low::Util::Yaml::Node &p_Node) const;

        Element duplicate(Low::Util::Name p_Name) const;
        static Element duplicate(Element p_Handle,
                                 Low::Util::Name p_Name);
        static Low::Util::Handle
        _duplicate(Low::Util::Handle p_Handle,
                   Low::Util::Name p_Name);

        static Element find_by_name(Low::Util::Name p_Name);
        static Low::Util::Handle
        _find_by_name(Low::Util::Name p_Name);

        static void serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node);
        static Low::Util::Handle
        deserialize(Low::Util::Yaml::Node &p_Node,
                    Low::Util::Handle p_Creator);
        static bool is_alive(Low::Util::Handle p_Handle)
        {
          READ_LOCK(l_Lock);
          return p_Handle.get_type() == Element::TYPE_ID &&
                 p_Handle.check_alive(ms_Slots, get_capacity());
        }

        static void destroy(Low::Util::Handle p_Handle)
        {
          _LOW_ASSERT(is_alive(p_Handle));
          Element l_Element = p_Handle.get_id();
          l_Element.destroy();
        }

        Util::Map<uint16_t, Util::Handle> &get_components() const;

        Low::Core::UI::View get_view() const;
        void set_view(Low::Core::UI::View p_Value);

        bool is_click_passthrough() const;
        void set_click_passthrough(bool p_Value);
        void toggle_click_passthrough();

        Low::Util::UniqueId get_unique_id() const;

        Low::Util::Name get_name() const;
        void set_name(Low::Util::Name p_Value);

        static Element make(Low::Util::Name p_Name,
                            Low::Core::UI::View p_View);
        uint64_t get_component(uint16_t p_TypeId) const;
        void add_component(Low::Util::Handle &p_Component);
        void remove_component(uint16_t p_ComponentType);
        bool has_component(uint16_t p_ComponentType);
        Low::Core::UI::Component::Display get_display() const;
        void serialize(Util::Yaml::Node &p_Node,
                       bool p_AddHandles) const;
        void serialize_hierarchy(Util::Yaml::Node &p_Node,
                                 bool p_AddHandles) const;
        static UI::Element
        deserialize_hierarchy(Util::Yaml::Node &p_Node,
                              Util::Handle p_Creator);

      private:
        static uint32_t ms_Capacity;
        static uint32_t create_instance();
        static void increase_budget();
        void set_unique_id(Low::Util::UniqueId p_Value);

        // LOW_CODEGEN:BEGIN:CUSTOM:STRUCT_END_CODE
        // LOW_CODEGEN::END::CUSTOM:STRUCT_END_CODE
      };

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

    } // namespace UI
  }   // namespace Core
} // namespace Low
