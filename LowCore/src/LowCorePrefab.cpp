#include "LowCorePrefab.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreRegion.h"
#include "LowCoreTransform.h"
#include "LowCorePrefabInstance.h"
#include "LowUtilString.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    static void
    read_component_property(Prefab p_Prefab, Util::Handle p_Handle,
                            Util::RTTI::PropertyInfo &p_PropertyInfo)
    {
      Util::Handle::fill_variants(
          p_Handle, p_PropertyInfo,
          p_Prefab.get_components()[p_Handle.get_type()]);
    }

    static void populate_component(Prefab p_Prefab,
                                   Util::Handle p_Handle)
    {
      Util::RTTI::TypeInfo &p_TypeInfo =
          Util::Handle::get_type_info(p_Handle.get_type());

      if (!p_TypeInfo.is_alive(p_Handle)) {
        return;
      }

      for (auto it = p_TypeInfo.properties.begin();
           it != p_TypeInfo.properties.end(); ++it) {
        if (it->second.editorProperty) {
          read_component_property(p_Prefab, p_Handle, it->second);
        }
      }
    }

    static void populate_prefab(Prefab p_Prefab, Entity p_Entity)
    {
      for (auto it = p_Entity.get_components().begin();
           it != p_Entity.get_components().end(); ++it) {
        populate_component(p_Prefab, it->second);
      }
    }
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Prefab::TYPE_ID = 33;
    uint32_t Prefab::ms_Capacity = 0u;
    uint8_t *Prefab::ms_Buffer = 0;
    Low::Util::Instances::Slot *Prefab::ms_Slots = 0;
    Low::Util::List<Prefab> Prefab::ms_LivingInstances =
        Low::Util::List<Prefab>();

    Prefab::Prefab() : Low::Util::Handle(0ull)
    {
    }
    Prefab::Prefab(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Prefab::Prefab(Prefab &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Prefab::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Prefab Prefab::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Prefab l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Prefab::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Prefab, parent, Util::Handle))
          Util::Handle();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Prefab, children,
                              Util::List<Util::Handle>))
          Util::List<Util::Handle>();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, Prefab, components,
          SINGLE_ARG(
              Util::Map<uint16_t,
                        Util::Map<Util::Name, Util::Variant>>)))
          Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>();
      ACCESSOR_TYPE_SOA(l_Handle, Prefab, name, Low::Util::Name) =
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

    void Prefab::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Prefab *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Prefab::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Prefab));

      initialize_buffer(&ms_Buffer, PrefabData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Prefab);
      LOW_PROFILE_ALLOC(type_slots_Prefab);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Prefab);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Prefab::is_alive;
      l_TypeInfo.destroy = &Prefab::destroy;
      l_TypeInfo.serialize = &Prefab::serialize;
      l_TypeInfo.deserialize = &Prefab::deserialize;
      l_TypeInfo.find_by_index = &Prefab::_find_by_index;
      l_TypeInfo.find_by_name = &Prefab::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Prefab::_make;
      l_TypeInfo.duplicate_default = &Prefab::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Prefab::living_instances);
      l_TypeInfo.get_living_count = &Prefab::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: parent
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(parent);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PrefabData, parent);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.get_parent();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Prefab, parent,
                                            Util::Handle);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.set_parent(*(Util::Handle *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Prefab l_Handle = p_Handle.get_id();
          *((Util::Handle *)p_Data) = l_Handle.get_parent();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: parent
      }
      {
        // Property: children
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(children);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PrefabData, children);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.get_children();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Prefab, children, Util::List<Util::Handle>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.set_children(*(Util::List<Util::Handle> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Prefab l_Handle = p_Handle.get_id();
          *((Util::List<Util::Handle> *)p_Data) =
              l_Handle.get_children();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: children
      }
      {
        // Property: components
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(components);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PrefabData, components);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.get_components();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Prefab, components,
              SINGLE_ARG(
                  Util::Map<uint16_t,
                            Util::Map<Util::Name, Util::Variant>>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.set_components(
              *(Util::Map<uint16_t,
                          Util::Map<Util::Name, Util::Variant>> *)
                  p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Prefab l_Handle = p_Handle.get_id();
          *((Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>
                 *)p_Data) = l_Handle.get_components();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: components
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(PrefabData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Prefab, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Prefab l_Handle = p_Handle.get_id();
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(PrefabData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Prefab, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Prefab l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Prefab l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Prefab::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Entity);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: spawn
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(spawn);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Entity::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Region);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Region::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: spawn
      }
      {
        // Function: compare_property
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(compare_property);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Component);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_PropertyName);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: compare_property
      }
      {
        // Function: apply
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(apply);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Component);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_PropertyName);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: apply
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Prefab::cleanup()
    {
      Low::Util::List<Prefab> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Prefab);
      LOW_PROFILE_FREE(type_slots_Prefab);
    }

    Low::Util::Handle Prefab::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Prefab Prefab::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Prefab l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Prefab::TYPE_ID;

      return l_Handle;
    }

    bool Prefab::is_alive() const
    {
      return m_Data.m_Type == Prefab::TYPE_ID &&
             check_alive(ms_Slots, Prefab::get_capacity());
    }

    uint32_t Prefab::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Prefab::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Prefab Prefab::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Prefab Prefab::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Prefab l_Handle = make(p_Name);
      l_Handle.set_parent(get_parent());
      l_Handle.set_children(get_children());
      l_Handle.set_components(get_components());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Prefab Prefab::duplicate(Prefab p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Prefab::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
    {
      Prefab l_Prefab = p_Handle.get_id();
      return l_Prefab.duplicate(p_Name);
    }

    void Prefab::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["unique_id"] = get_unique_id();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      Prefab l_Parent(get_parent().get_id());
      p_Node["parent"] = 0;
      if (l_Parent.is_alive()) {
        p_Node["parent"] = l_Parent.get_unique_id();
      }

      for (auto cit = get_components().begin();
           cit != get_components().end(); ++cit) {
        for (auto pit = cit->second.begin(); pit != cit->second.end();
             ++pit) {
          const char *i_TypeIdStr = LOW_TO_STRING(cit->first).c_str();
          const char *i_PropertyNameStr = pit->first.c_str();
          Util::Serialization::serialize_variant(
              p_Node["components"][i_TypeIdStr][i_PropertyNameStr],
              pit->second);
        }
      }

      for (auto cit = get_children().begin();
           cit != get_children().end(); ++cit) {
        Prefab i_Prefab(cit->get_id());
        LOW_ASSERT(i_Prefab.is_alive(),
                   "Cannot serialize dead prefab (child)");
        Util::Yaml::Node i_Node;
        i_Prefab.serialize(i_Node);
        p_Node["children"].push_back(i_Node);
      }
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Prefab::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
    {
      Prefab l_Prefab = p_Handle.get_id();
      l_Prefab.serialize(p_Node);
    }

    Low::Util::Handle
    Prefab::deserialize(Low::Util::Yaml::Node &p_Node,
                        Low::Util::Handle p_Creator)
    {
      Prefab l_Handle = Prefab::make(N(Prefab));

      if (p_Node["unique_id"]) {
        Low::Util::remove_unique_id(l_Handle.get_unique_id());
        l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());
      }

      if (p_Node["parent"]) {
      }
      if (p_Node["children"]) {
      }
      if (p_Node["components"]) {
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      Prefab l_Parent(p_Creator.get_id());
      if (l_Parent.is_alive()) {
        l_Handle.set_parent(p_Creator.get_id());
        l_Parent.get_children().push_back(l_Handle);
      }

      if (p_Node["components"]) {
        for (auto cit = p_Node["components"].begin();
             cit != p_Node["components"].end(); ++cit) {
          uint16_t i_TypeId = cit->first.as<uint16_t>();
          for (auto pit = cit->second.begin();
               pit != cit->second.end(); ++pit) {
            l_Handle.get_components()[i_TypeId]
                                     [LOW_YAML_AS_NAME(pit->first)] =
                Util::Serialization::deserialize_variant(pit->second);
          }
        }
      }
      if (p_Node["children"]) {
        for (auto cit = p_Node["children"].begin();
             cit != p_Node["children"].end(); ++cit) {
          Prefab::deserialize(*cit, l_Handle);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    Util::Handle Prefab::get_parent() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent

      // LOW_CODEGEN::END::CUSTOM:GETTER_parent

      return TYPE_SOA(Prefab, parent, Util::Handle);
    }
    void Prefab::set_parent(Util::Handle p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent

      // Set new value
      TYPE_SOA(Prefab, parent, Util::Handle) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent

      // LOW_CODEGEN::END::CUSTOM:SETTER_parent
    }

    Util::List<Util::Handle> &Prefab::get_children() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_children

      // LOW_CODEGEN::END::CUSTOM:GETTER_children

      return TYPE_SOA(Prefab, children, Util::List<Util::Handle>);
    }
    void Prefab::set_children(Util::List<Util::Handle> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_children

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_children

      // Set new value
      TYPE_SOA(Prefab, children, Util::List<Util::Handle>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_children

      // LOW_CODEGEN::END::CUSTOM:SETTER_children
    }

    Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>> &
    Prefab::get_components() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_components

      // LOW_CODEGEN::END::CUSTOM:GETTER_components

      return TYPE_SOA(
          Prefab, components,
          SINGLE_ARG(
              Util::Map<uint16_t,
                        Util::Map<Util::Name, Util::Variant>>));
    }
    void Prefab::set_components(
        Util::Map<uint16_t, Util::Map<Util::Name, Util::Variant>>
            &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_components

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_components

      // Set new value
      TYPE_SOA(Prefab, components,
               SINGLE_ARG(
                   Util::Map<uint16_t,
                             Util::Map<Util::Name, Util::Variant>>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_components

      // LOW_CODEGEN::END::CUSTOM:SETTER_components
    }

    Low::Util::UniqueId Prefab::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Prefab, unique_id, Low::Util::UniqueId);
    }
    void Prefab::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Prefab, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name Prefab::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Prefab, name, Low::Util::Name);
    }
    void Prefab::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Prefab, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    Prefab Prefab::make(Entity &p_Entity)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      Prefab l_Prefab = Prefab::make(p_Entity.get_name());
      populate_prefab(l_Prefab, p_Entity);

      for (auto it = p_Entity.get_transform().get_children().begin();
           it != p_Entity.get_transform().get_children().end();
           ++it) {
        Component::Transform i_Transform(*it);
        LOW_ASSERT(i_Transform.is_alive(),
                   "Cannot create prefab from dead entity child");
        Entity i_Entity = i_Transform.get_entity();
        Prefab i_Prefab = Prefab::make(i_Entity);
        i_Prefab.set_parent(l_Prefab);
        l_Prefab.get_children().push_back(i_Prefab);
      }

      if (!p_Entity.has_component(
              Component::PrefabInstance::TYPE_ID)) {
        Component::PrefabInstance::make(p_Entity);
      }

      Component::PrefabInstance l_PrefabInstance =
          p_Entity.get_component(Component::PrefabInstance::TYPE_ID);
      l_PrefabInstance.set_prefab(l_Prefab);

      return l_Prefab;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    Entity Prefab::spawn(Region p_Region)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn

      Util::String l_Name = get_name().c_str();
      l_Name += " (Instance)";
      Entity l_Entity =
          Entity::make(LOW_NAME(l_Name.c_str()), p_Region);

      for (auto cit = get_components().begin();
           cit != get_components().end(); ++cit) {
        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(cit->first);
        Util::Handle i_Component =
            i_TypeInfo.make_component(l_Entity);

        for (auto pit = i_TypeInfo.properties.begin();
             pit != i_TypeInfo.properties.end(); ++pit) {
          if (!pit->second.editorProperty) {
            continue;
          }
          auto i_PropPos = cit->second.find(pit->first);
          if (pit->second.type == Util::RTTI::PropertyType::SHAPE) {
            Util::String i_BaseName = pit->first.c_str();
            i_BaseName += "__";
            if (cit->second.find(
                    LOW_NAME((i_BaseName + "type").c_str())) ==
                cit->second.end()) {
              continue;
            }

            Util::Name i_ShapeTypeName =
                cit->second[LOW_NAME((i_BaseName + "type").c_str())];
            Math::Shape i_Shape;

            if (i_ShapeTypeName == N(BOX)) {
              i_BaseName += "box_";
              i_Shape.type = Math::ShapeType::BOX;
              i_Shape.box.position = cit->second[LOW_NAME(
                  (i_BaseName + "position").c_str())];
              i_Shape.box.rotation = cit->second[LOW_NAME(
                  (i_BaseName + "rotation").c_str())];
              i_Shape.box.halfExtents = cit->second[LOW_NAME(
                  (i_BaseName + "halfextents").c_str())];

            } else {
              LOW_ASSERT(false, "Unknown shape type while creating "
                                "entity from prefab");
            }

            i_TypeInfo.properties[pit->first].set(i_Component,
                                                  &i_Shape);
          } else if (i_PropPos != cit->second.end()) {
            i_TypeInfo.properties[pit->first].set(
                i_Component, &i_PropPos->second.m_Bool);
          }
        }
      }

      Component::PrefabInstance l_PrefabInstance =
          Component::PrefabInstance::make(l_Entity);
      l_PrefabInstance.set_prefab(*this);

      for (auto cit = get_children().begin();
           cit != get_children().end(); ++cit) {
        Prefab i_Prefab = cit->get_id();
        Entity i_Entity = i_Prefab.spawn(p_Region);
        i_Entity.get_transform().set_parent(
            l_Entity.get_transform().get_id());
      }

      return l_Entity;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn
    }

    bool Prefab::compare_property(Util::Handle p_Component,
                                  Util::Name p_PropertyName)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_compare_property

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_Component.get_type());

      Util::RTTI::PropertyInfo &l_PropertyInfo =
          l_TypeInfo.properties[p_PropertyName];

      Util::Map<Util::Name, Util::Variant> l_Values;
      fill_variants(p_Component, l_PropertyInfo, l_Values);

      for (auto it = l_Values.begin(); it != l_Values.end(); ++it) {
        if (it->second !=
            get_components()[p_Component.get_type()][it->first]) {
          return false;
        }
      }
      return true;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_compare_property
    }

    void Prefab::apply(Util::Handle p_Component,
                       Util::Name p_PropertyName)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_apply

      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_Component.get_type());
      Util::RTTI::PropertyInfo &l_PropertyInfo =
          l_TypeInfo.properties[p_PropertyName];

      Util::Map<Util::Name, Util::Variant> l_Values;
      fill_variants(p_Component, l_PropertyInfo, l_Values);

      for (auto it = l_Values.begin(); it != l_Values.end(); ++it) {
        get_components()[p_Component.get_type()][it->first] =
            it->second;
      }

      for (auto it =
               Component::PrefabInstance::ms_LivingInstances.begin();
           it != Component::PrefabInstance::ms_LivingInstances.end();
           ++it) {
        if (it->get_prefab() == *this) {
          it->update_from_prefab();
        }
      }

      Entity l_Entity;
      l_TypeInfo.properties[N(entity)].get(p_Component, &l_Entity);
      Component::PrefabInstance l_PrefabInstance =
          l_Entity.get_component(Component::PrefabInstance::TYPE_ID);

      l_PrefabInstance.override(p_Component.get_type(),
                                p_PropertyName, false);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_apply
    }

    uint32_t Prefab::create_instance()
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

    void Prefab::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(PrefabData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(PrefabData, parent) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(PrefabData, parent) * (l_Capacity)],
            l_Capacity * sizeof(Util::Handle));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(PrefabData, children) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Util::List<Util::Handle>))])
              Util::List<Util::Handle>();
          *i_ValPtr = it->get_children();
        }
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr = new (
              &l_NewBuffer
                  [offsetof(PrefabData, components) *
                       (l_Capacity + l_CapacityIncrease) +
                   (it->get_index() *
                    sizeof(Util::Map<
                           uint16_t,
                           Util::Map<Util::Name, Util::Variant>>))])
              Util::Map<uint16_t,
                        Util::Map<Util::Name, Util::Variant>>();
          *i_ValPtr = it->get_components();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(PrefabData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PrefabData, unique_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(&l_NewBuffer[offsetof(PrefabData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(PrefabData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Prefab from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
