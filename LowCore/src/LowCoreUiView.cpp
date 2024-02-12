#include "LowCoreUiView.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreUiElement.h"
#include "LowUtilFileIO.h"

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t View::TYPE_ID = 38;
      uint32_t View::ms_Capacity = 0u;
      uint8_t *View::ms_Buffer = 0;
      Low::Util::Instances::Slot *View::ms_Slots = 0;
      Low::Util::List<View> View::ms_LivingInstances =
          Low::Util::List<View>();

      View::View() : Low::Util::Handle(0ull)
      {
      }
      View::View(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      View::View(View &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle View::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      View View::make(Low::Util::Name p_Name)
      {
        uint32_t l_Index = create_instance();

        View l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, View, loaded, bool) = false;
        new (&ACCESSOR_TYPE_SOA(l_Handle, View, entities,
                                Util::Set<Util::UniqueId>))
            Util::Set<Util::UniqueId>();
        ACCESSOR_TYPE_SOA(l_Handle, View, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void View::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const View *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void View::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(View));

        initialize_buffer(&ms_Buffer, ViewData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_View);
        LOW_PROFILE_ALLOC(type_slots_View);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(View);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &View::is_alive;
        l_TypeInfo.destroy = &View::destroy;
        l_TypeInfo.serialize = &View::serialize;
        l_TypeInfo.deserialize = &View::deserialize;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &View::_make;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &View::living_instances);
        l_TypeInfo.get_living_count = &View::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(loaded);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, loaded);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.is_loaded();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, loaded,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_loaded(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entities);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, entities);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            return nullptr;
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(ViewData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void View::cleanup()
      {
        Low::Util::List<View> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_View);
        LOW_PROFILE_FREE(type_slots_View);
      }

      View View::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        View l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        return l_Handle;
      }

      bool View::is_alive() const
      {
        return m_Data.m_Type == View::TYPE_ID &&
               check_alive(ms_Slots, View::get_capacity());
      }

      uint32_t View::get_capacity()
      {
        return ms_Capacity;
      }

      View View::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
      }

      void View::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["unique_id"] = get_unique_id();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void View::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
      {
        View l_View = p_Handle.get_id();
        l_View.serialize(p_Node);
      }

      Low::Util::Handle
      View::deserialize(Low::Util::Yaml::Node &p_Node,
                        Low::Util::Handle p_Creator)
      {
        View l_Handle = View::make(N(View));

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      bool View::is_loaded() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:GETTER_loaded

        return TYPE_SOA(View, loaded, bool);
      }
      void View::set_loaded(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

        // Set new value
        TYPE_SOA(View, loaded, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:SETTER_loaded
      }

      Util::Set<Util::UniqueId> &View::get_entities() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entities
        // LOW_CODEGEN::END::CUSTOM:GETTER_entities

        return TYPE_SOA(View, entities, Util::Set<Util::UniqueId>);
      }

      Low::Util::UniqueId View::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(View, unique_id, Low::Util::UniqueId);
      }
      void View::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(View, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      Low::Util::Name View::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(View, name, Low::Util::Name);
      }
      void View::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(View, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      void View::serialize_views(Util::Yaml::Node &p_Node)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_views
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_views
      }

      void View::add_element(Element p_Element)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_element
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_element
      }

      void View::remove_element(Element p_Element)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_element
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_element
      }

      void View::load_elements()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load_elements
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_load_elements
      }

      void View::unload_elements()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload_elements
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload_elements
      }

      uint32_t View::create_instance()
      {
        uint32_t l_Index = 0u;

        for (; l_Index < get_capacity(); ++l_Index) {
          if (!ms_Slots[l_Index].m_Occupied) {
            break;
          }
        }
        if (l_Index >= get_capacity()) {
          increase_budget();
        }
        ms_Slots[l_Index].m_Occupied = true;
        return l_Index;
      }

      void View::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ViewData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewData, loaded) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewData, loaded) * (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(ViewData, entities) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(Util::Set<Util::UniqueId>))])
                Util::Set<Util::UniqueId>();
            *i_ValPtr = it->get_entities();
          }
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, name) * (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::Name));
        }
        for (uint32_t i = l_Capacity;
             i < l_Capacity + l_CapacityIncrease; ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for View from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }
    } // namespace UI
  }   // namespace Core
} // namespace Low
