#include "LowRendererVkTexExport.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererTextureExport.h"
#include "LowRendererVulkanBuffer.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t TexExport::TYPE_ID = 81;
      uint32_t TexExport::ms_Capacity = 0u;
      uint8_t *TexExport::ms_Buffer = 0;
      std::shared_mutex TexExport::ms_BufferMutex;
      Low::Util::Instances::Slot *TexExport::ms_Slots = 0;
      Low::Util::List<TexExport> TexExport::ms_LivingInstances =
          Low::Util::List<TexExport>();

      TexExport::TexExport() : Low::Util::Handle(0ull)
      {
      }
      TexExport::TexExport(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      TexExport::TexExport(TexExport &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle TexExport::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      TexExport TexExport::make(Low::Util::Name p_Name)
      {
        WRITE_LOCK(l_Lock);
        uint32_t l_Index = create_instance();

        TexExport l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = TexExport::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(l_Handle, TexExport, staging_buffer,
                                AllocatedBuffer)) AllocatedBuffer();
        ACCESSOR_TYPE_SOA(l_Handle, TexExport, name,
                          Low::Util::Name) = Low::Util::Name(0u);
        LOCK_UNLOCK(l_Lock);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void TexExport::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        BufferUtil::destroy_buffer(get_staging_buffer());
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        broadcast_observable(OBSERVABLE_DESTROY);

        WRITE_LOCK(l_Lock);
        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const TexExport *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void TexExport::initialize()
      {
        WRITE_LOCK(l_Lock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(TexExport));

        initialize_buffer(&ms_Buffer, TexExportData::get_size(),
                          get_capacity(), &ms_Slots);
        LOCK_UNLOCK(l_Lock);

        LOW_PROFILE_ALLOC(type_buffer_TexExport);
        LOW_PROFILE_ALLOC(type_slots_TexExport);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(TexExport);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &TexExport::is_alive;
        l_TypeInfo.destroy = &TexExport::destroy;
        l_TypeInfo.serialize = &TexExport::serialize;
        l_TypeInfo.deserialize = &TexExport::deserialize;
        l_TypeInfo.find_by_index = &TexExport::_find_by_index;
        l_TypeInfo.notify = &TexExport::_notify;
        l_TypeInfo.find_by_name = &TexExport::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &TexExport::_make;
        l_TypeInfo.duplicate_default = &TexExport::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &TexExport::living_instances);
        l_TypeInfo.get_living_count = &TexExport::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: staging_buffer
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(staging_buffer);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TexExportData, staging_buffer);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.get_staging_buffer();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, TexExport, staging_buffer, AllocatedBuffer);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_staging_buffer(*(AllocatedBuffer *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            *((AllocatedBuffer *)p_Data) =
                l_Handle.get_staging_buffer();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: staging_buffer
        }
        {
          // Property: frame_index
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(frame_index);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(TexExportData, frame_index);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.get_frame_index();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexExport,
                                              frame_index, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_frame_index(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            *((uint32_t *)p_Data) = l_Handle.get_frame_index();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: frame_index
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(TexExportData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexExport,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            TexExport l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            TexExport l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void TexExport::cleanup()
      {
        Low::Util::List<TexExport> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        WRITE_LOCK(l_Lock);
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_TexExport);
        LOW_PROFILE_FREE(type_slots_TexExport);
        LOCK_UNLOCK(l_Lock);
      }

      Low::Util::Handle TexExport::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      TexExport TexExport::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        TexExport l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = TexExport::TYPE_ID;

        return l_Handle;
      }

      bool TexExport::is_alive() const
      {
        READ_LOCK(l_Lock);
        return m_Data.m_Type == TexExport::TYPE_ID &&
               check_alive(ms_Slots, TexExport::get_capacity());
      }

      uint32_t TexExport::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      TexExport::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      TexExport TexExport::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
        return 0ull;
      }

      TexExport TexExport::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        TexExport l_Handle = make(p_Name);
        l_Handle.set_staging_buffer(get_staging_buffer());
        l_Handle.set_frame_index(get_frame_index());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      TexExport TexExport::duplicate(TexExport p_Handle,
                                     Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      TexExport::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
      {
        TexExport l_TexExport = p_Handle.get_id();
        return l_TexExport.duplicate(p_Name);
      }

      void TexExport::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["frame_index"] = get_frame_index();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void TexExport::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
      {
        TexExport l_TexExport = p_Handle.get_id();
        l_TexExport.serialize(p_Node);
      }

      Low::Util::Handle
      TexExport::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
      {
        TexExport l_Handle = TexExport::make(N(TexExport));

        if (p_Node["staging_buffer"]) {
        }
        if (p_Node["frame_index"]) {
          l_Handle.set_frame_index(
              p_Node["frame_index"].as<uint32_t>());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void TexExport::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 TexExport::observe(Low::Util::Name p_Observable,
                             Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void TexExport::notify(Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        if (p_Observed.get_type() == TextureExport::TYPE_ID &&
            p_Observable == OBSERVABLE_DESTROY) {
          destroy();
        }
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void TexExport::_notify(Low::Util::Handle p_Observer,
                              Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
      {
        TexExport l_TexExport = p_Observer.get_id();
        l_TexExport.notify(p_Observed, p_Observable);
      }

      AllocatedBuffer &TexExport::get_staging_buffer() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:GETTER_staging_buffer

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(TexExport, staging_buffer, AllocatedBuffer);
      }
      void TexExport::set_staging_buffer(AllocatedBuffer &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging_buffer

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(TexExport, staging_buffer, AllocatedBuffer) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging_buffer
        // LOW_CODEGEN::END::CUSTOM:SETTER_staging_buffer

        broadcast_observable(N(staging_buffer));
      }

      uint32_t TexExport::get_frame_index() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:GETTER_frame_index

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(TexExport, frame_index, uint32_t);
      }
      void TexExport::set_frame_index(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_frame_index

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(TexExport, frame_index, uint32_t) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_frame_index
        // LOW_CODEGEN::END::CUSTOM:SETTER_frame_index

        broadcast_observable(N(frame_index));
      }

      Low::Util::Name TexExport::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        READ_LOCK(l_ReadLock);
        return TYPE_SOA(TexExport, name, Low::Util::Name);
      }
      void TexExport::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(TexExport, name, Low::Util::Name) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t TexExport::create_instance()
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

      void TexExport::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer =
            (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                              sizeof(TexExportData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(TexExportData, staging_buffer) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(TexExportData, staging_buffer) *
                         (l_Capacity)],
              l_Capacity * sizeof(AllocatedBuffer));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TexExportData, frame_index) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TexExportData, frame_index) *
                            (l_Capacity)],
                 l_Capacity * sizeof(uint32_t));
        }
        {
          memcpy(&l_NewBuffer[offsetof(TexExportData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(TexExportData, name) *
                            (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for TexExport from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
