#include "LowCorePrefabInstance.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"
#include "LowCoreTransform.h"
#include "LowUtilString.h"

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t PrefabInstance::TYPE_ID = 34;
      uint32_t PrefabInstance::ms_Capacity = 0u;
      uint8_t *PrefabInstance::ms_Buffer = 0;
      Low::Util::Instances::Slot *PrefabInstance::ms_Slots = 0;
      Low::Util::List<PrefabInstance> PrefabInstance::ms_LivingInstances =
          Low::Util::List<PrefabInstance>();

      PrefabInstance::PrefabInstance() : Low::Util::Handle(0ull)
      {
      }
      PrefabInstance::PrefabInstance(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      PrefabInstance::PrefabInstance(PrefabInstance &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle PrefabInstance::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      PrefabInstance PrefabInstance::make(Low::Core::Entity p_Entity)
      {
        uint32_t l_Index = create_instance();

        PrefabInstance l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = PrefabInstance::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, PrefabInstance, prefab, Prefab))
            Prefab();
        new (&ACCESSOR_TYPE_SOA(
            l_Handle, PrefabInstance, overrides,
            SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>)))
            Util::Map<uint16_t, Util::List<Util::Name>>();
        new (&ACCESSOR_TYPE_SOA(l_Handle, PrefabInstance, entity,
                                Low::Core::Entity)) Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void PrefabInstance::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const PrefabInstance *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void PrefabInstance::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(PrefabInstance));

        initialize_buffer(&ms_Buffer, PrefabInstanceData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_PrefabInstance);
        LOW_PROFILE_ALLOC(type_slots_PrefabInstance);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(PrefabInstance);
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &PrefabInstance::is_alive;
        l_TypeInfo.destroy = &PrefabInstance::destroy;
        l_TypeInfo.serialize = &PrefabInstance::serialize;
        l_TypeInfo.deserialize = &PrefabInstance::deserialize;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &PrefabInstance::_make;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &PrefabInstance::living_instances);
        l_TypeInfo.get_living_count = &PrefabInstance::living_count;
        l_TypeInfo.component = true;
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(prefab);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PrefabInstanceData, prefab);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Prefab::TYPE_ID;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.get_prefab();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PrefabInstance, prefab,
                                              Prefab);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_prefab(*(Prefab *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(overrides);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PrefabInstanceData, overrides);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.get_overrides();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PrefabInstance, overrides,
                SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_overrides(
                *(Util::Map<uint16_t, Util::List<Util::Name>> *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PrefabInstanceData, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PrefabInstance, entity,
                                              Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(PrefabInstanceData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, PrefabInstance,
                                              unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void PrefabInstance::cleanup()
      {
        Low::Util::List<PrefabInstance> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_PrefabInstance);
        LOW_PROFILE_FREE(type_slots_PrefabInstance);
      }

      PrefabInstance PrefabInstance::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        PrefabInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = PrefabInstance::TYPE_ID;

        return l_Handle;
      }

      bool PrefabInstance::is_alive() const
      {
        return m_Data.m_Type == PrefabInstance::TYPE_ID &&
               check_alive(ms_Slots, PrefabInstance::get_capacity());
      }

      uint32_t PrefabInstance::get_capacity()
      {
        return ms_Capacity;
      }

      void PrefabInstance::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["unique_id"] = get_unique_id();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        p_Node["prefab"] = 0;
        if (get_prefab().is_alive()) {
          p_Node["prefab"] = get_prefab().get_unique_id();
        }

        for (auto cit = get_overrides().begin(); cit != get_overrides().end();
             ++cit) {
          for (auto pit = cit->second.begin(); pit != cit->second.end();
               ++pit) {
            p_Node["overrides"][LOW_TO_STRING(cit->first).c_str()].push_back(
                pit->c_str());
          }
        }
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void PrefabInstance::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Yaml::Node &p_Node)
      {
        PrefabInstance l_PrefabInstance = p_Handle.get_id();
        l_PrefabInstance.serialize(p_Node);
      }

      Low::Util::Handle
      PrefabInstance::deserialize(Low::Util::Yaml::Node &p_Node,
                                  Low::Util::Handle p_Creator)
      {
        PrefabInstance l_Handle = PrefabInstance::make(p_Creator.get_id());

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

        if (p_Node["overrides"]) {
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        if (p_Node["prefab"]) {
          l_Handle.set_prefab(
              Util::find_handle_by_unique_id(p_Node["prefab"].as<uint64_t>())
                  .get_id());
        }

        if (p_Node["overrides"]) {
          for (auto cit = p_Node["overrides"].begin();
               cit != p_Node["overrides"].end(); ++cit) {
            for (uint32_t i = 0; i < cit->second.size(); ++i) {
              l_Handle.get_overrides()[cit->first.as<uint16_t>()].push_back(
                  LOW_YAML_AS_NAME(cit->second[i]));
            }
          }
        }
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      Prefab PrefabInstance::get_prefab() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_prefab
        // LOW_CODEGEN::END::CUSTOM:GETTER_prefab

        return TYPE_SOA(PrefabInstance, prefab, Prefab);
      }
      void PrefabInstance::set_prefab(Prefab p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_prefab
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_prefab

        // Set new value
        TYPE_SOA(PrefabInstance, prefab, Prefab) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_prefab
        // LOW_CODEGEN::END::CUSTOM:SETTER_prefab
      }

      Util::Map<uint16_t, Util::List<Util::Name>> &
      PrefabInstance::get_overrides() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_overrides
        // LOW_CODEGEN::END::CUSTOM:GETTER_overrides

        return TYPE_SOA(
            PrefabInstance, overrides,
            SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>));
      }
      void PrefabInstance::set_overrides(
          Util::Map<uint16_t, Util::List<Util::Name>> &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_overrides
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_overrides

        // Set new value
        TYPE_SOA(PrefabInstance, overrides,
                 SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>)) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_overrides
        // LOW_CODEGEN::END::CUSTOM:SETTER_overrides
      }

      Low::Core::Entity PrefabInstance::get_entity() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity
        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(PrefabInstance, entity, Low::Core::Entity);
      }
      void PrefabInstance::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(PrefabInstance, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity
        // LOW_CODEGEN::END::CUSTOM:SETTER_entity
      }

      Low::Util::UniqueId PrefabInstance::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(PrefabInstance, unique_id, Low::Util::UniqueId);
      }
      void PrefabInstance::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(PrefabInstance, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      void
      PrefabInstance::update_component_from_prefab(uint16_t p_ComponentType)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_component_from_prefab
        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_ComponentType);

        Util::Handle l_Handle = get_entity().get_component(p_ComponentType);

        for (auto it = l_TypeInfo.properties.begin();
             it != l_TypeInfo.properties.end(); ++it) {
          if (!it->second.editorProperty) {
            continue;
          }
          bool i_IsOverride = false;
          for (auto pit = get_overrides()[p_ComponentType].begin();
               pit != get_overrides()[p_ComponentType].end(); ++pit) {
            if (*pit == it->first) {
              i_IsOverride = true;
              break;
            }
          }

          if (i_IsOverride) {
            continue;
          }

          it->second.set(l_Handle,
                         &get_prefab()
                              .get_components()[p_ComponentType][it->first]
                              .m_Bool);
        }

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_component_from_prefab
      }

      void PrefabInstance::update_from_prefab()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_from_prefab
        Entity l_Entity = get_entity();
        Prefab l_Prefab = get_prefab();

        if (!l_Prefab.is_alive()) {
          l_Entity.remove_component(TYPE_ID);
          return;
        }

        Prefab l_Parent = l_Prefab.get_parent().get_id();

        for (auto it = l_Entity.get_components().begin();
             it != l_Entity.get_components().end(); ++it) {
          if (it->first == TYPE_ID) {
            continue;
          }
          if (it->first == Transform::TYPE_ID && !l_Parent.is_alive()) {
            continue;
          }
          if (l_Prefab.get_components().find(it->first) ==
              l_Prefab.get_components().end()) {
            continue;
          }
          update_component_from_prefab(it->first);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_from_prefab
      }

      void PrefabInstance::override(uint16_t p_ComponentType,
                                    Util::Name p_PropertyName,
                                    bool p_IsOverride)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_override
        if (p_IsOverride) {
          if (std::find(get_overrides()[p_ComponentType].begin(),
                        get_overrides()[p_ComponentType].end(),
                        p_PropertyName) ==
              get_overrides()[p_ComponentType].end()) {
            get_overrides()[p_ComponentType].push_back(p_PropertyName);
          }
        } else {
          if (std::find(get_overrides()[p_ComponentType].begin(),
                        get_overrides()[p_ComponentType].end(),
                        p_PropertyName) !=
              get_overrides()[p_ComponentType].end()) {
            get_overrides()[p_ComponentType].erase_first(p_PropertyName);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_override
      }

      uint32_t PrefabInstance::create_instance()
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

      void PrefabInstance::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(PrefabInstanceData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(PrefabInstanceData, prefab) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(PrefabInstanceData, prefab) * (l_Capacity)],
              l_Capacity * sizeof(Prefab));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            auto *i_ValPtr =
                new (&l_NewBuffer[offsetof(PrefabInstanceData, overrides) *
                                      (l_Capacity + l_CapacityIncrease) +
                                  (it->get_index() *
                                   sizeof(Util::Map<uint16_t,
                                                    Util::List<Util::Name>>))])
                    Util::Map<uint16_t, Util::List<Util::Name>>();
            *i_ValPtr = it->get_overrides();
          }
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(PrefabInstanceData, entity) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(PrefabInstanceData, entity) * (l_Capacity)],
              l_Capacity * sizeof(Low::Core::Entity));
        }
        {
          memcpy(&l_NewBuffer[offsetof(PrefabInstanceData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(PrefabInstanceData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
             ++i) {
          l_NewSlots[i].m_Occupied = false;
          l_NewSlots[i].m_Generation = 0;
        }
        free(ms_Buffer);
        free(ms_Slots);
        ms_Buffer = l_NewBuffer;
        ms_Slots = l_NewSlots;
        ms_Capacity = l_Capacity + l_CapacityIncrease;

        LOW_LOG_DEBUG << "Auto-increased budget for PrefabInstance from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
      }
    } // namespace Component
  }   // namespace Core
} // namespace Low