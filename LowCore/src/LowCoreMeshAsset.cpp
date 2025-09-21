#include "LowCoreMeshAsset.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCoreTaskScheduler.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t MeshAsset::TYPE_ID = 23;
    uint32_t MeshAsset::ms_Capacity = 0u;
    uint32_t MeshAsset::ms_PageSize = 0u;
    Low::Util::SharedMutex MeshAsset::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        MeshAsset::ms_PagesLock(MeshAsset::ms_PagesMutex,
                                std::defer_lock);
    Low::Util::List<MeshAsset> MeshAsset::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> MeshAsset::ms_Pages;

    MeshAsset::MeshAsset() : Low::Util::Handle(0ull)
    {
    }
    MeshAsset::MeshAsset(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    MeshAsset::MeshAsset(MeshAsset &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle MeshAsset::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    MeshAsset MeshAsset::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    MeshAsset MeshAsset::make(Low::Util::Name p_Name,
                              Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, MeshAsset, lod0,
                                 MeshResource)) MeshResource();
      ACCESSOR_TYPE_SOA(l_Handle, MeshAsset, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

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

      l_Handle.set_reference_count(0u);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void MeshAsset::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<MeshAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        _unload();
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      Low::Util::remove_unique_id(get_unique_id());

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
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

    void MeshAsset::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(MeshAsset));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, MeshAsset::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshAsset);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshAsset::is_alive;
      l_TypeInfo.destroy = &MeshAsset::destroy;
      l_TypeInfo.serialize = &MeshAsset::serialize;
      l_TypeInfo.deserialize = &MeshAsset::deserialize;
      l_TypeInfo.find_by_index = &MeshAsset::_find_by_index;
      l_TypeInfo.notify = &MeshAsset::_notify;
      l_TypeInfo.find_by_name = &MeshAsset::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &MeshAsset::_make;
      l_TypeInfo.duplicate_default = &MeshAsset::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &MeshAsset::living_instances);
      l_TypeInfo.get_living_count = &MeshAsset::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: lod0
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(lod0);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(MeshAsset::Data, lod0);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = MeshResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
          l_Handle.get_lod0();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, lod0,
                                            MeshResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.set_lod0(*(MeshResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
          *((MeshResource *)p_Data) = l_Handle.get_lod0();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: lod0
      }
      {
        // Property: reference_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshAsset::Data, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: reference_count
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshAsset::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshAsset, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(MeshAsset::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          MeshAsset l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<MeshAsset> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: is_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_loaded
      }
      {
        // Function: load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load
      }
      {
        // Function: unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload
      }
      {
        // Function: _unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _unload
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshAsset::cleanup()
    {
      Low::Util::List<MeshAsset> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle MeshAsset::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    MeshAsset MeshAsset::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

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

    MeshAsset MeshAsset::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

      return l_Handle;
    }

    bool MeshAsset::is_alive() const
    {
      if (m_Data.m_Type != MeshAsset::TYPE_ID) {
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
      return m_Data.m_Type == MeshAsset::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t MeshAsset::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle MeshAsset::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    MeshAsset MeshAsset::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    MeshAsset MeshAsset::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      MeshAsset l_Handle = make(p_Name);
      if (get_lod0().is_alive()) {
        l_Handle.set_lod0(get_lod0());
      }
      l_Handle.set_reference_count(get_reference_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    MeshAsset MeshAsset::duplicate(MeshAsset p_Handle,
                                   Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    MeshAsset::_duplicate(Low::Util::Handle p_Handle,
                          Low::Util::Name p_Name)
    {
      MeshAsset l_MeshAsset = p_Handle.get_id();
      return l_MeshAsset.duplicate(p_Name);
    }

    void MeshAsset::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_lod0().is_alive()) {
        get_lod0().serialize(p_Node["lod0"]);
      }
      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void MeshAsset::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node)
    {
      MeshAsset l_MeshAsset = p_Handle.get_id();
      l_MeshAsset.serialize(p_Node);
    }

    Low::Util::Handle
    MeshAsset::deserialize(Low::Util::Yaml::Node &p_Node,
                           Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      MeshAsset l_Handle =
          MeshAsset::make(N(MeshAsset), l_HandleUniqueId);

      if (p_Node["lod0"]) {
        l_Handle.set_lod0(MeshResource::deserialize(p_Node["lod0"],
                                                    l_Handle.get_id())
                              .get_id());
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

    void MeshAsset::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 MeshAsset::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 MeshAsset::observe(Low::Util::Name p_Observable,
                           Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void MeshAsset::notify(Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void MeshAsset::_notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      MeshAsset l_MeshAsset = p_Observer.get_id();
      l_MeshAsset.notify(p_Observed, p_Observable);
    }

    MeshResource MeshAsset::get_lod0() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lod0

      // LOW_CODEGEN::END::CUSTOM:GETTER_lod0

      return TYPE_SOA(MeshAsset, lod0, MeshResource);
    }
    void MeshAsset::set_lod0(MeshResource p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_lod0

      if (get_lod0().is_alive() && get_lod0().is_loaded()) {
        get_lod0().unload();
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_lod0

      // Set new value
      TYPE_SOA(MeshAsset, lod0, MeshResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lod0

      if (is_loaded()) {
        // If this asset is already loaded we need to increase
        // the reference count of the resource
        p_Value.load();
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_lod0

      broadcast_observable(N(lod0));
    }

    uint32_t MeshAsset::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(MeshAsset, reference_count, uint32_t);
    }
    void MeshAsset::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(MeshAsset, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count

      broadcast_observable(N(reference_count));
    }

    Low::Util::UniqueId MeshAsset::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(MeshAsset, unique_id, Low::Util::UniqueId);
    }
    void MeshAsset::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(MeshAsset, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name MeshAsset::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MeshAsset, name, Low::Util::Name);
    }
    void MeshAsset::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MeshAsset, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool MeshAsset::is_loaded()
    {
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_reference_count() > 0;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void MeshAsset::load()
    {
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load

      LOW_ASSERT(is_alive(), "MeshAsset was not alive on load");

      bool l_HasBeenLoaded = is_loaded();

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased MeshAsset reference count, but its "
                 "not over 0. "
                 "Something went wrong.");

      if (l_HasBeenLoaded) {
        return;
      }

      if (get_lod0().is_alive() && !get_lod0().is_loaded()) {
        TaskScheduler::schedule_mesh_resource_load(get_lod0());
      } else if (get_lod0().is_alive() && get_lod0().is_loaded()) {
        get_lod0().load();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void MeshAsset::unload()
    {
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload

      LOW_ASSERT(is_alive(), "MeshAsset was not alive on unload");

      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "MeshAsset reference count < 0. Something "
                 "went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void MeshAsset::_unload()
    {
      Low::Util::HandleLock<MeshAsset> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload

      LOW_ASSERT(is_alive(), "Cannot unload dead meshasset");

      if (get_lod0().is_alive() && get_lod0().is_loaded()) {
        get_lod0().unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    uint32_t MeshAsset::create_instance(
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 MeshAsset::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for MeshAsset.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, MeshAsset::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool MeshAsset::get_page_for_index(const u32 p_Index,
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

  } // namespace Core
} // namespace Low
