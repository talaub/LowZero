#include "LowCoreMeshAsset.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreTaskScheduler.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t MeshAsset::TYPE_ID = 23;
    uint32_t MeshAsset::ms_Capacity = 0u;
    uint8_t *MeshAsset::ms_Buffer = 0;
    Low::Util::Instances::Slot *MeshAsset::ms_Slots = 0;
    Low::Util::List<MeshAsset> MeshAsset::ms_LivingInstances =
        Low::Util::List<MeshAsset>();

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
      uint32_t l_Index = create_instance();

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshAsset, lod0,
                              MeshResource)) MeshResource();
      ACCESSOR_TYPE_SOA(l_Handle, MeshAsset, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      l_Handle.set_unique_id(
          Low::Util::generate_unique_id(l_Handle.get_id()));
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

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      _unload();
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      Low::Util::remove_unique_id(get_unique_id());

      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const MeshAsset *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void MeshAsset::initialize()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(MeshAsset));

      initialize_buffer(&ms_Buffer, MeshAssetData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MeshAsset);
      LOW_PROFILE_ALLOC(type_slots_MeshAsset);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshAsset);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshAsset::is_alive;
      l_TypeInfo.destroy = &MeshAsset::destroy;
      l_TypeInfo.serialize = &MeshAsset::serialize;
      l_TypeInfo.deserialize = &MeshAsset::deserialize;
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
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(lod0);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(MeshAssetData, lod0);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = MeshResource::TYPE_ID;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.get_lod0();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, lod0,
                                            MeshResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.set_lod0(*(MeshResource *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(MeshAssetData, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
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
        l_PropertyInfo.dataOffset =
            offsetof(MeshAssetData, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, MeshAsset, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = true;
        l_PropertyInfo.dataOffset = offsetof(MeshAssetData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get =
            [](Low::Util::Handle p_Handle) -> void const * {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          MeshAsset l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      {
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void MeshAsset::cleanup()
    {
      Low::Util::List<MeshAsset> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_MeshAsset);
      LOW_PROFILE_FREE(type_slots_MeshAsset);
    }

    MeshAsset MeshAsset::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

      return l_Handle;
    }

    bool MeshAsset::is_alive() const
    {
      return m_Data.m_Type == MeshAsset::TYPE_ID &&
             check_alive(ms_Slots, MeshAsset::get_capacity());
    }

    uint32_t MeshAsset::get_capacity()
    {
      return ms_Capacity;
    }

    MeshAsset MeshAsset::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
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
      p_Node["unique_id"] = get_unique_id();
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
      MeshAsset l_Handle = MeshAsset::make(N(MeshAsset));

      if (p_Node["unique_id"]) {
        Low::Util::remove_unique_id(l_Handle.get_unique_id());
        l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());
      }

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

    MeshResource MeshAsset::get_lod0() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_lod0
      // LOW_CODEGEN::END::CUSTOM:GETTER_lod0

      return TYPE_SOA(MeshAsset, lod0, MeshResource);
    }
    void MeshAsset::set_lod0(MeshResource p_Value)
    {
      _LOW_ASSERT(is_alive());

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
    }

    uint32_t MeshAsset::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(MeshAsset, reference_count, uint32_t);
    }
    void MeshAsset::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(MeshAsset, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count
    }

    Low::Util::UniqueId MeshAsset::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(MeshAsset, unique_id, Low::Util::UniqueId);
    }
    void MeshAsset::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(MeshAsset, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
    }

    Low::Util::Name MeshAsset::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(MeshAsset, name, Low::Util::Name);
    }
    void MeshAsset::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(MeshAsset, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    bool MeshAsset::is_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded
      return get_reference_count() > 0;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void MeshAsset::load()
    {
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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload
      LOW_ASSERT(is_alive(), "Cannot unload dead meshasset");

      if (get_lod0().is_alive() && get_lod0().is_loaded()) {
        get_lod0().unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    uint32_t MeshAsset::create_instance()
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

    void MeshAsset::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MeshAssetData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshAssetData, lod0) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshAssetData, lod0) * (l_Capacity)],
            l_Capacity * sizeof(MeshResource));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshAssetData, reference_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshAssetData, reference_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshAssetData, unique_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshAssetData, unique_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::UniqueId));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(MeshAssetData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(MeshAssetData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MeshAsset from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Core
} // namespace Low
