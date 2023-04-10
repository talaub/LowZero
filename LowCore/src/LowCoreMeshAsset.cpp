#include "LowCoreMeshAsset.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Core {
    const uint16_t MeshAsset::TYPE_ID = 20;
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
    MeshAsset::MeshAsset(MeshAsset &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    MeshAsset MeshAsset::make(Low::Util::Name p_Name)
    {
      uint32_t l_Index = create_instance();

      MeshAsset l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = MeshAsset::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, MeshAsset, lod0, MeshResource))
          MeshResource();
      ACCESSOR_TYPE_SOA(l_Handle, MeshAsset, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      return l_Handle;
    }

    void MeshAsset::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void MeshAsset::initialize()
    {
      ms_Capacity = Low::Util::Config::get_capacity(N(LowCore), N(MeshAsset));

      initialize_buffer(&ms_Buffer, MeshAssetData::get_size(), get_capacity(),
                        &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_MeshAsset);
      LOW_PROFILE_ALLOC(type_slots_MeshAsset);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(MeshAsset);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &MeshAsset::is_alive;
      l_TypeInfo.destroy = &MeshAsset::destroy;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(lod0);
        l_PropertyInfo.dataOffset = offsetof(MeshAssetData, lod0);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, lod0,
                                            MeshResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, lod0, MeshResource) =
              *(MeshResource *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.dataOffset = offsetof(MeshAssetData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          ACCESSOR_TYPE_SOA(p_Handle, MeshAsset, name, Low::Util::Name) =
              *(Low::Util::Name *)p_Data;
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
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

    bool MeshAsset::is_alive() const
    {
      return m_Data.m_Type == MeshAsset::TYPE_ID &&
             check_alive(ms_Slots, MeshAsset::get_capacity());
    }

    uint32_t MeshAsset::get_capacity()
    {
      return ms_Capacity;
    }

    MeshResource MeshAsset::get_lod0() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshAsset, lod0, MeshResource);
    }
    void MeshAsset::set_lod0(MeshResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MeshAsset, lod0, MeshResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_lod0
      // LOW_CODEGEN::END::CUSTOM:SETTER_lod0
    }

    Low::Util::Name MeshAsset::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(MeshAsset, name, Low::Util::Name);
    }
    void MeshAsset::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // Set new value
      TYPE_SOA(MeshAsset, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
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
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(MeshAssetData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(MeshAssetData, lod0) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshAssetData, lod0) * (l_Capacity)],
               l_Capacity * sizeof(MeshResource));
      }
      {
        memcpy(&l_NewBuffer[offsetof(MeshAssetData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(MeshAssetData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for MeshAsset from " << l_Capacity
                    << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Core
} // namespace Low
