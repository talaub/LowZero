#include "LowRendererUiCanvas.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t UiCanvas::TYPE_ID = 71;
    uint32_t UiCanvas::ms_Capacity = 0u;
    uint8_t *UiCanvas::ms_Buffer = 0;
    std::shared_mutex UiCanvas::ms_BufferMutex;
    Low::Util::Instances::Slot *UiCanvas::ms_Slots = 0;
    Low::Util::List<UiCanvas> UiCanvas::ms_LivingInstances =
        Low::Util::List<UiCanvas>();

    UiCanvas::UiCanvas() : Low::Util::Handle(0ull)
    {
    }
    UiCanvas::UiCanvas(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    UiCanvas::UiCanvas(UiCanvas &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle UiCanvas::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    UiCanvas UiCanvas::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      UiCanvas l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiCanvas::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, UiCanvas, draw_commands,
                              Low::Util::List<UiDrawCommand>))
          Low::Util::List<UiDrawCommand>();
      ACCESSOR_TYPE_SOA(l_Handle, UiCanvas, z_dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, UiCanvas, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void UiCanvas::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
    }

    void UiCanvas::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(UiCanvas));

      initialize_buffer(&ms_Buffer, UiCanvasData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_UiCanvas);
      LOW_PROFILE_ALLOC(type_slots_UiCanvas);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(UiCanvas);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &UiCanvas::is_alive;
      l_TypeInfo.destroy = &UiCanvas::destroy;
      l_TypeInfo.serialize = &UiCanvas::serialize;
      l_TypeInfo.deserialize = &UiCanvas::deserialize;
      l_TypeInfo.find_by_index = &UiCanvas::_find_by_index;
      l_TypeInfo.notify = &UiCanvas::_notify;
      l_TypeInfo.find_by_name = &UiCanvas::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &UiCanvas::_make;
      l_TypeInfo.duplicate_default = &UiCanvas::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &UiCanvas::living_instances);
      l_TypeInfo.get_living_count = &UiCanvas::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: z_sorting
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_sorting);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(UiCanvasData, z_sorting);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_z_sorting();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiCanvas,
                                            z_sorting, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_z_sorting(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiCanvas l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_z_sorting();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_sorting
      }
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiCanvasData, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiCanvas, draw_commands,
              Low::Util::List<UiDrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiCanvas l_Handle = p_Handle.get_id();
          *((Low::Util::List<UiDrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: z_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(UiCanvasData, z_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.is_z_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiCanvas,
                                            z_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_z_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiCanvas l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_z_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(UiCanvasData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiCanvas, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiCanvas l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiCanvas l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void UiCanvas::cleanup()
    {
      Low::Util::List<UiCanvas> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_UiCanvas);
      LOW_PROFILE_FREE(type_slots_UiCanvas);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle UiCanvas::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    UiCanvas UiCanvas::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      UiCanvas l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiCanvas::TYPE_ID;

      return l_Handle;
    }

    UiCanvas UiCanvas::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      UiCanvas l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = UiCanvas::TYPE_ID;

      return l_Handle;
    }

    bool UiCanvas::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == UiCanvas::TYPE_ID &&
             check_alive(ms_Slots, UiCanvas::get_capacity());
    }

    uint32_t UiCanvas::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle UiCanvas::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    UiCanvas UiCanvas::find_by_name(Low::Util::Name p_Name)
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

    UiCanvas UiCanvas::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      UiCanvas l_Handle = make(p_Name);
      l_Handle.set_z_sorting(get_z_sorting());
      l_Handle.set_z_dirty(is_z_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    UiCanvas UiCanvas::duplicate(UiCanvas p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle UiCanvas::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
    {
      UiCanvas l_UiCanvas = p_Handle.get_id();
      return l_UiCanvas.duplicate(p_Name);
    }

    void UiCanvas::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["z_sorting"] = get_z_sorting();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void UiCanvas::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
    {
      UiCanvas l_UiCanvas = p_Handle.get_id();
      l_UiCanvas.serialize(p_Node);
    }

    Low::Util::Handle
    UiCanvas::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
    {
      UiCanvas l_Handle = UiCanvas::make(N(UiCanvas));

      if (p_Node["z_sorting"]) {
        l_Handle.set_z_sorting(p_Node["z_sorting"].as<uint32_t>());
      }
      if (p_Node["draw_commands"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    UiCanvas::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 UiCanvas::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 UiCanvas::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void UiCanvas::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void UiCanvas::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      UiCanvas l_UiCanvas = p_Observer.get_id();
      l_UiCanvas.notify(p_Observed, p_Observable);
    }

    uint32_t UiCanvas::get_z_sorting() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_sorting
      // LOW_CODEGEN::END::CUSTOM:GETTER_z_sorting

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiCanvas, z_sorting, uint32_t);
    }
    void UiCanvas::set_z_sorting(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_sorting
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_sorting

      if (get_z_sorting() != p_Value) {
        // Set dirty flags
        mark_z_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiCanvas, z_sorting, uint32_t) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_sorting
        // LOW_CODEGEN::END::CUSTOM:SETTER_z_sorting

        broadcast_observable(N(z_sorting));
      }
    }

    Low::Util::List<UiDrawCommand> &
    UiCanvas::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiCanvas, draw_commands,
                      Low::Util::List<UiDrawCommand>);
    }

    bool UiCanvas::is_z_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_z_dirty

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiCanvas, z_dirty, bool);
    }
    void UiCanvas::toggle_z_dirty()
    {
      set_z_dirty(!is_z_dirty());
    }

    void UiCanvas::set_z_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_dirty

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiCanvas, z_dirty, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_dirty
      // LOW_CODEGEN::END::CUSTOM:SETTER_z_dirty

      broadcast_observable(N(z_dirty));
    }

    void UiCanvas::mark_z_dirty()
    {
      if (!is_z_dirty()) {
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiCanvas, z_dirty, bool) = true;
        LOCK_UNLOCK(l_WriteLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
      }
    }

    Low::Util::Name UiCanvas::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiCanvas, name, Low::Util::Name);
    }
    void UiCanvas::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiCanvas, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t UiCanvas::create_instance()
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

    void UiCanvas::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(UiCanvasData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(UiCanvasData, z_sorting) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiCanvasData, z_sorting) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          UiCanvas i_UiCanvas = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(UiCanvasData, draw_commands) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<UiDrawCommand>))])
              Low::Util::List<UiDrawCommand>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_UiCanvas, UiCanvas, draw_commands,
                                Low::Util::List<UiDrawCommand>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiCanvasData, z_dirty) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiCanvasData, z_dirty) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(UiCanvasData, name) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(UiCanvasData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for UiCanvas from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
