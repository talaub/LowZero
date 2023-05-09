#include "LowCoreRegion.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreEntity.h"

namespace Low {
  namespace Core {
    const uint16_t Region::TYPE_ID = 19;
    uint32_t Region::ms_Capacity = 0u;
    uint8_t *Region::ms_Buffer = 0;
    Low::Util::Instances::Slot *Region::ms_Slots = 0;
    Low::Util::List<Region> Region::ms_LivingInstances =
        Low::Util::List<Region>();

    Region::Region() : Low::Util::Handle(0ull)
    {
    }
    Region::Region(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Region::Region(Region &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Region Region::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      Region l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Region::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, Region, loaded, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, Region, streaming_enabled, bool) = false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, Region, entities,
                              Util::Set<Util::UniqueId>))
          Util::Set<Util::UniqueId>();
      ACCESSOR_TYPE_SOA(l_Handle, Region, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(Low::Util::generate_unique_id(l_Handle.get_id()));
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Region::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Region *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void Region::initialize()
    {
      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(Region));

      initialize_buffer(&ms_Buffer, RegionData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_Region);
      LOW_PROFILE_ALLOC(type_slots_Region);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Region);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Region::is_alive;
      l_TypeInfo.destroy = &Region::destroy;
      l_TypeInfo.serialize = &Region::serialize;
      l_TypeInfo.deserialize = &Region::deserialize;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Region::living_instances);
      l_TypeInfo.get_living_count = &Region::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(loaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RegionData, loaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, loaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_loaded(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(streaming_enabled);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(RegionData, streaming_enabled);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, streaming_enabled,
                                            bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_streaming_enabled(*(bool *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(streaming_radius);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(RegionData, streaming_radius);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, streaming_radius,
                                            float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_streaming_radius(*(float *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(entities);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RegionData, entities);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
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
        l_PropertyInfo.dataOffset = offsetof(RegionData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RegionData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Region, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Region l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Region::cleanup()
    {
      Low::Util::List<Region> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Region);
      LOW_PROFILE_FREE(type_slots_Region);
    }

    Region Region::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Region l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Region::TYPE_ID;

      return l_Handle;
    }

    bool Region::is_alive() const
    {
      return m_Data.m_Type == Region::TYPE_ID &&
             check_alive(ms_Slots, Region::get_capacity());
    }

    uint32_t Region::get_capacity()
    {
      return ms_Capacity;
    }

    Region Region::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void Region::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["streaming_enabled"] = is_streaming_enabled();
      p_Node["streaming_radius"] = get_streaming_radius();
      p_Node["unique_id"] = get_unique_id();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Region::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
    {
      Region l_Region = p_Handle.get_id();
      l_Region.serialize(p_Node);
    }

    Low::Util::Handle Region::deserialize(Low::Util::Yaml::Node &p_Node,
                                          Low::Util::Handle p_Creator)
    {
      Region l_Handle = Region::make(N(Region));

      Low::Util::remove_unique_id(l_Handle.get_unique_id());
      l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      l_Handle.set_streaming_enabled(p_Node["streaming_enabled"].as<bool>());
      l_Handle.set_streaming_radius(p_Node["streaming_radius"].as<float>());
      l_Handle.set_unique_id(p_Node["unique_id"].as<Low::Util::UniqueId>());
      l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    bool Region::is_loaded() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, loaded, bool);
    }
    void Region::set_loaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

      // Set new value
      TYPE_SOA(Region, loaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_loaded
    }

    bool Region::is_streaming_enabled() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, streaming_enabled, bool);
    }
    void Region::set_streaming_enabled(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_streaming_enabled
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_streaming_enabled

      // Set new value
      TYPE_SOA(Region, streaming_enabled, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_streaming_enabled
      // LOW_CODEGEN::END::CUSTOM:SETTER_streaming_enabled
    }

    float Region::get_streaming_radius() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, streaming_radius, float);
    }
    void Region::set_streaming_radius(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_streaming_radius
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_streaming_radius

      // Set new value
      TYPE_SOA(Region, streaming_radius, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_streaming_radius
      // LOW_CODEGEN::END::CUSTOM:SETTER_streaming_radius
    }

    Util::Set<Util::UniqueId> &Region::get_entities() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, entities, Util::Set<Util::UniqueId>);
    }

    Low::Util::UniqueId Region::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, unique_id, Low::Util::UniqueId);
    }
    void Region::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Region, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name Region::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(Region, name, Low::Util::Name);
    }
    void Region::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Region, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    void Region::serialize_entities(Util::Yaml::Node &p_Node)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_entities
      for (auto it = get_entities().begin(); it != get_entities().end(); ++it) {
        Core::Entity i_Entity = Util::find_handle_by_unique_id(*it).get_id();
        if (i_Entity.is_alive()) {
          Util::Yaml::Node i_Node;
          i_Entity.serialize(i_Node);
          p_Node["entities"].push_back(i_Node);
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_entities
    }

    void Region::add_entity(Entity p_Entity)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_entity
      if (p_Entity.get_region().is_alive()) {
        p_Entity.get_region().remove_entity(p_Entity);
      }

      p_Entity.set_region(*this);
      get_entities().insert(p_Entity.get_unique_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_entity
    }

    void Region::remove_entity(Entity p_Entity)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_entity
      p_Entity.set_region(0);
      get_entities().erase(p_Entity.get_unique_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_entity
    }

    void Region::load_entities()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load_entities
      LOW_ASSERT(is_alive(), "Cannot load dead region");
      LOW_ASSERT(!is_loaded(), "Region is already loaded");

      set_loaded(true);

      Util::String l_Path = Util::String(LOW_DATA_PATH) + "\\assets\\regions\\";
      l_Path += std::to_string(get_unique_id()).c_str();
      l_Path += ".entities.yaml";

      Util::Yaml::Node l_RootNode = Util::Yaml::load_file(l_Path.c_str());
      Util::Yaml::Node &l_EntitiesNode = l_RootNode["entities"];

      for (auto it = l_EntitiesNode.begin(); it != l_EntitiesNode.end(); ++it) {
        Util::Yaml::Node &i_EntityNode = *it;
        Entity::deserialize(i_EntityNode, *this);
      }

      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load_entities
    }

    void Region::unload_entities()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload_entities
      LOW_ASSERT(is_alive(), "Cannot unload dead region");
      LOW_ASSERT(is_loaded(), "Cannot unload region that is not loaded");

      set_loaded(false);

      while (!get_entities().empty()) {
        Entity i_Entity =
            Util::find_handle_by_unique_id(*get_entities().begin()).get_id();
        i_Entity.destroy();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload_entities
    }

    uint32_t Region::create_instance()
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

    void Region::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(RegionData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(RegionData, loaded) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RegionData, loaded) * (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RegionData, streaming_enabled) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RegionData, streaming_enabled) * (l_Capacity)],
            l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(RegionData, streaming_radius) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RegionData, streaming_radius) * (l_Capacity)],
            l_Capacity * sizeof(float));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          auto *i_ValPtr =
              new (&l_NewBuffer[offsetof(RegionData, entities) *
                                    (l_Capacity + l_CapacityIncrease) +
                                (it->get_index() *
                                 sizeof(Util::Set<Util::UniqueId>))])
                  Util::Set<Util::UniqueId>();
          *i_ValPtr = it->get_entities();
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RegionData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RegionData, unique_id) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RegionData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RegionData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for Region from " << l_Capacity
                    << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low
