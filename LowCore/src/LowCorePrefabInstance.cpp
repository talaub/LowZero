#include "LowCorePrefabInstance.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCorePrefabInstance.h"
#include "LowCoreTransform.h"
#include "LowUtilString.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace Component {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t PrefabInstance::TYPE_ID = 34;
      uint32_t PrefabInstance::ms_Capacity = 0u;
      uint32_t PrefabInstance::ms_PageSize = 0u;
      Low::Util::SharedMutex PrefabInstance::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          PrefabInstance::ms_PagesLock(PrefabInstance::ms_PagesMutex,
                                       std::defer_lock);
      Low::Util::List<PrefabInstance>
          PrefabInstance::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          PrefabInstance::ms_Pages;

      PrefabInstance::PrefabInstance() : Low::Util::Handle(0ull)
      {
      }
      PrefabInstance::PrefabInstance(uint64_t p_Id)
          : Low::Util::Handle(p_Id)
      {
      }
      PrefabInstance::PrefabInstance(PrefabInstance &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle
      PrefabInstance::_make(Low::Util::Handle p_Entity)
      {
        Low::Core::Entity l_Entity = p_Entity.get_id();
        LOW_ASSERT(l_Entity.is_alive(),
                   "Cannot create component for dead entity");
        return make(l_Entity).get_id();
      }

      PrefabInstance PrefabInstance::make(Low::Core::Entity p_Entity)
      {
        return make(p_Entity, 0ull);
      }

      PrefabInstance
      PrefabInstance::make(Low::Core::Entity p_Entity,
                           Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        PrefabInstance l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = PrefabInstance::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<PrefabInstance> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, PrefabInstance, prefab,
                                   Prefab)) Prefab();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, PrefabInstance, overrides,
            SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>)))
            Util::Map<uint16_t, Util::List<Util::Name>>();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, PrefabInstance, entity,
                                   Low::Core::Entity))
            Low::Core::Entity();

        l_Handle.set_entity(p_Entity);
        p_Entity.add_component(l_Handle);

        ms_LivingInstances.push_back(l_Handle);

        if (p_UniqueId > 0ull) {
          l_Handle.set_unique_id(p_UniqueId);
        } else {
          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
        }
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void PrefabInstance::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        ms_PagesLock.lock();
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
        ms_PagesLock.unlock();
      }

      void PrefabInstance::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(PrefabInstance));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, PrefabInstance::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(PrefabInstance);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &PrefabInstance::is_alive;
        l_TypeInfo.destroy = &PrefabInstance::destroy;
        l_TypeInfo.serialize = &PrefabInstance::serialize;
        l_TypeInfo.deserialize = &PrefabInstance::deserialize;
        l_TypeInfo.find_by_index = &PrefabInstance::_find_by_index;
        l_TypeInfo.notify = &PrefabInstance::_notify;
        l_TypeInfo.make_default = nullptr;
        l_TypeInfo.make_component = &PrefabInstance::_make;
        l_TypeInfo.duplicate_default = nullptr;
        l_TypeInfo.duplicate_component = &PrefabInstance::_duplicate;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &PrefabInstance::living_instances);
        l_TypeInfo.get_living_count = &PrefabInstance::living_count;
        l_TypeInfo.component = true;
        l_TypeInfo.uiComponent = false;
        {
          // Property: prefab
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(prefab);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PrefabInstance::Data, prefab);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Prefab::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_prefab();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PrefabInstance, prefab, Prefab);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_prefab(*(Prefab *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            *((Prefab *)p_Data) = l_Handle.get_prefab();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: prefab
        }
        {
          // Property: overrides
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(overrides);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PrefabInstance::Data, overrides);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_overrides();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PrefabInstance, overrides,
                SINGLE_ARG(
                    Util::Map<uint16_t, Util::List<Util::Name>>));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_overrides(*(
                Util::Map<uint16_t, Util::List<Util::Name>> *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            *((Util::Map<uint16_t, Util::List<Util::Name>> *)p_Data) =
                l_Handle.get_overrides();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: overrides
        }
        {
          // Property: entity
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(entity);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PrefabInstance::Data, entity);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::Entity::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_entity();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PrefabInstance, entity, Low::Core::Entity);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PrefabInstance l_Handle = p_Handle.get_id();
            l_Handle.set_entity(*(Low::Core::Entity *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            *((Low::Core::Entity *)p_Data) = l_Handle.get_entity();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: entity
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PrefabInstance::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PrefabInstance, unique_id,
                Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PrefabInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<PrefabInstance> l_HandleLock(
                l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Function: update_component_from_prefab
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(update_component_from_prefab);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ComponentType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT16;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: update_component_from_prefab
        }
        {
          // Function: update_from_prefab
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(update_from_prefab);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: update_from_prefab
        }
        {
          // Function: override
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(override);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ComponentType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT16;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_PropertyName);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_IsOverride);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: override
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void PrefabInstance::cleanup()
      {
        Low::Util::List<PrefabInstance> l_Instances =
            ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        ms_PagesLock.lock();
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          free(i_Page->lockWords);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;

        ms_PagesLock.unlock();
      }

      Low::Util::Handle
      PrefabInstance::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      PrefabInstance PrefabInstance::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        PrefabInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = PrefabInstance::TYPE_ID;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      PrefabInstance
      PrefabInstance::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        PrefabInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = PrefabInstance::TYPE_ID;

        return l_Handle;
      }

      bool PrefabInstance::is_alive() const
      {
        if (m_Data.m_Type != PrefabInstance::TYPE_ID) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
            l_Page->mutex);
        return m_Data.m_Type == PrefabInstance::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t PrefabInstance::get_capacity()
      {
        return ms_Capacity;
      }

      PrefabInstance
      PrefabInstance::duplicate(Low::Core::Entity p_Entity) const
      {
        _LOW_ASSERT(is_alive());

        PrefabInstance l_Handle = make(p_Entity);
        if (get_prefab().is_alive()) {
          l_Handle.set_prefab(get_prefab());
        }
        l_Handle.set_overrides(get_overrides());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      PrefabInstance
      PrefabInstance::duplicate(PrefabInstance p_Handle,
                                Low::Core::Entity p_Entity)
      {
        return p_Handle.duplicate(p_Entity);
      }

      Low::Util::Handle
      PrefabInstance::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Handle p_Entity)
      {
        PrefabInstance l_PrefabInstance = p_Handle.get_id();
        Low::Core::Entity l_Entity = p_Entity.get_id();
        return l_PrefabInstance.duplicate(l_Entity);
      }

      void
      PrefabInstance::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["_unique_id"] =
            Low::Util::hash_to_string(get_unique_id()).c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        p_Node["prefab"] = 0;
        if (get_prefab().is_alive()) {
          p_Node["prefab"] = get_prefab().get_unique_id();
        }

        for (auto cit = get_overrides().begin();
             cit != get_overrides().end(); ++cit) {
          for (auto pit = cit->second.begin();
               pit != cit->second.end(); ++pit) {
            p_Node["overrides"][LOW_TO_STRING(cit->first).c_str()]
                .push_back(pit->c_str());
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
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              LOW_YAML_AS_STRING(p_Node["_unique_id"]));
        }

        PrefabInstance l_Handle = PrefabInstance::make(
            p_Creator.get_id(), l_HandleUniqueId);

        if (p_Node["overrides"]) {
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        if (p_Node["prefab"]) {
          l_Handle.set_prefab(Util::find_handle_by_unique_id(
                                  p_Node["prefab"].as<uint64_t>())
                                  .get_id());
        }

        if (p_Node["overrides"]) {
          for (auto cit = p_Node["overrides"].begin();
               cit != p_Node["overrides"].end(); ++cit) {
            for (uint32_t i = 0; i < cit->second.size(); ++i) {
              l_Handle.get_overrides()[cit->first.as<uint16_t>()]
                  .push_back(LOW_YAML_AS_NAME(cit->second[i]));
            }
          }
        }
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void PrefabInstance::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 PrefabInstance::observe(
          Low::Util::Name p_Observable,
          Low::Util::Function<void(Low::Util::Handle,
                                   Low::Util::Name)>
              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 PrefabInstance::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void PrefabInstance::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void PrefabInstance::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
      {
        PrefabInstance l_PrefabInstance = p_Observer.get_id();
        l_PrefabInstance.notify(p_Observed, p_Observable);
      }

      Prefab PrefabInstance::get_prefab() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_prefab

        // LOW_CODEGEN::END::CUSTOM:GETTER_prefab

        return TYPE_SOA(PrefabInstance, prefab, Prefab);
      }
      void PrefabInstance::set_prefab(Prefab p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_prefab

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_prefab

        // Set new value
        TYPE_SOA(PrefabInstance, prefab, Prefab) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_prefab

        // LOW_CODEGEN::END::CUSTOM:SETTER_prefab

        broadcast_observable(N(prefab));
      }

      Util::Map<uint16_t, Util::List<Util::Name>> &
      PrefabInstance::get_overrides() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

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
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_overrides

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_overrides

        // Set new value
        TYPE_SOA(
            PrefabInstance, overrides,
            SINGLE_ARG(Util::Map<uint16_t, Util::List<Util::Name>>)) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_overrides

        // LOW_CODEGEN::END::CUSTOM:SETTER_overrides

        broadcast_observable(N(overrides));
      }

      Low::Core::Entity PrefabInstance::get_entity() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_entity

        // LOW_CODEGEN::END::CUSTOM:GETTER_entity

        return TYPE_SOA(PrefabInstance, entity, Low::Core::Entity);
      }
      void PrefabInstance::set_entity(Low::Core::Entity p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_entity

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_entity

        // Set new value
        TYPE_SOA(PrefabInstance, entity, Low::Core::Entity) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_entity

        // LOW_CODEGEN::END::CUSTOM:SETTER_entity

        broadcast_observable(N(entity));
      }

      Low::Util::UniqueId PrefabInstance::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(PrefabInstance, unique_id,
                        Low::Util::UniqueId);
      }
      void PrefabInstance::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(PrefabInstance, unique_id, Low::Util::UniqueId) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      void PrefabInstance::update_component_from_prefab(
          uint16_t p_ComponentType)
      {
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update_component_from_prefab

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_ComponentType);

        Util::Handle l_Handle =
            get_entity().get_component(p_ComponentType);

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

          it->second.set(
              l_Handle,
              &get_prefab()
                   .get_components()[p_ComponentType][it->first]
                   .m_Bool);
        }

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_update_component_from_prefab
      }

      void PrefabInstance::update_from_prefab()
      {
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());
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
          if (it->first == Transform::TYPE_ID &&
              !l_Parent.is_alive()) {
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
        Low::Util::HandleLock<PrefabInstance> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_override

        if (p_IsOverride) {
          if (std::find(get_overrides()[p_ComponentType].begin(),
                        get_overrides()[p_ComponentType].end(),
                        p_PropertyName) ==
              get_overrides()[p_ComponentType].end()) {
            get_overrides()[p_ComponentType].push_back(
                p_PropertyName);
          }
        } else {
          if (std::find(get_overrides()[p_ComponentType].begin(),
                        get_overrides()[p_ComponentType].end(),
                        p_PropertyName) !=
              get_overrides()[p_ComponentType].end()) {
            get_overrides()[p_ComponentType].erase_first(
                p_PropertyName);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_override
      }

      uint32_t PrefabInstance::create_instance(
          u32 &p_PageIndex, u32 &p_SlotIndex,
          Low::Util::UniqueLock<Low::Util::Mutex> &p_PageLock)
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          Low::Util::UniqueLock<Low::Util::Mutex> i_PageLock(
              ms_Pages[l_PageIndex]->mutex);
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
              l_PageLock = std::move(i_PageLock);
              break;
            }
            l_Index++;
          }
          if (l_FoundIndex) {
            break;
          }
        }
        if (!l_FoundIndex) {
          l_SlotIndex = 0;
          l_PageIndex = create_page();
          Low::Util::UniqueLock<Low::Util::Mutex> l_NewLock(
              ms_Pages[l_PageIndex]->mutex);
          l_PageLock = std::move(l_NewLock);
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        p_PageLock = std::move(l_PageLock);
        LOCK_UNLOCK(l_PagesLock);
        return l_Index;
      }

      u32 PrefabInstance::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for PrefabInstance.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, PrefabInstance::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool PrefabInstance::get_page_for_index(const u32 p_Index,
                                              u32 &p_PageIndex,
                                              u32 &p_SlotIndex)
      {
        if (p_Index >= get_capacity()) {
          p_PageIndex = LOW_UINT32_MAX;
          p_SlotIndex = LOW_UINT32_MAX;
          return false;
        }
        p_PageIndex = p_Index / ms_PageSize;
        if (p_PageIndex > (ms_Pages.size() - 1)) {
          return false;
        }
        p_SlotIndex = p_Index - (ms_PageSize * p_PageIndex);
        return true;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Component
  } // namespace Core
} // namespace Low
