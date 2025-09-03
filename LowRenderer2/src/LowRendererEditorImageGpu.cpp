#include "LowRendererEditorImageGpu.h"

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
    Low::Util::Set<Low::Renderer::EditorImageGpu>
        Low::Renderer::EditorImageGpu::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t EditorImageGpu::TYPE_ID = 84;
    uint32_t EditorImageGpu::ms_Capacity = 0u;
    uint8_t *EditorImageGpu::ms_Buffer = 0;
    std::shared_mutex EditorImageGpu::ms_BufferMutex;
    Low::Util::Instances::Slot *EditorImageGpu::ms_Slots = 0;
    Low::Util::List<EditorImageGpu>
        EditorImageGpu::ms_LivingInstances =
            Low::Util::List<EditorImageGpu>();

    EditorImageGpu::EditorImageGpu() : Low::Util::Handle(0ull)
    {
    }
    EditorImageGpu::EditorImageGpu(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    EditorImageGpu::EditorImageGpu(EditorImageGpu &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle EditorImageGpu::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    EditorImageGpu EditorImageGpu::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      EditorImageGpu l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = EditorImageGpu::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, EditorImageGpu,
                        imgui_texture_initialized, bool) = false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, EditorImageGpu,
                              imgui_texture_id, ImTextureID))
          ImTextureID();
      ACCESSOR_TYPE_SOA(l_Handle, EditorImageGpu, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      ms_Dirty.insert(l_Handle);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void EditorImageGpu::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // TODO: remove vulkan imgui image
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

    void EditorImageGpu::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(EditorImageGpu));

      initialize_buffer(&ms_Buffer, EditorImageGpuData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_EditorImageGpu);
      LOW_PROFILE_ALLOC(type_slots_EditorImageGpu);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(EditorImageGpu);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &EditorImageGpu::is_alive;
      l_TypeInfo.destroy = &EditorImageGpu::destroy;
      l_TypeInfo.serialize = &EditorImageGpu::serialize;
      l_TypeInfo.deserialize = &EditorImageGpu::deserialize;
      l_TypeInfo.find_by_index = &EditorImageGpu::_find_by_index;
      l_TypeInfo.notify = &EditorImageGpu::_notify;
      l_TypeInfo.find_by_name = &EditorImageGpu::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &EditorImageGpu::_make;
      l_TypeInfo.duplicate_default = &EditorImageGpu::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &EditorImageGpu::living_instances);
      l_TypeInfo.get_living_count = &EditorImageGpu::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: imgui_texture_initialized
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(imgui_texture_initialized);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageGpuData, imgui_texture_initialized);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.is_imgui_texture_initialized();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImageGpu,
                                            imgui_texture_initialized,
                                            bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.set_imgui_texture_initialized(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageGpu l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_imgui_texture_initialized();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: imgui_texture_initialized
      }
      {
        // Property: data_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageGpuData, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.get_data_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImageGpu,
                                            data_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.set_data_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageGpu l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_data_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_handle
      }
      {
        // Property: editor_image_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(editor_image_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageGpuData, editor_image_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.get_editor_image_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImageGpu,
                                            editor_image_handle,
                                            uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.set_editor_image_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageGpu l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_editor_image_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: editor_image_handle
      }
      {
        // Property: imgui_texture_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(imgui_texture_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageGpuData, imgui_texture_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.get_imgui_texture_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImageGpu,
                                            imgui_texture_id,
                                            ImTextureID);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.set_imgui_texture_id(*(ImTextureID *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageGpu l_Handle = p_Handle.get_id();
          *((ImTextureID *)p_Data) = l_Handle.get_imgui_texture_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: imgui_texture_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageGpuData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, EditorImageGpu,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageGpu l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageGpu l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void EditorImageGpu::cleanup()
    {
      Low::Util::List<EditorImageGpu> l_Instances =
          ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_EditorImageGpu);
      LOW_PROFILE_FREE(type_slots_EditorImageGpu);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle EditorImageGpu::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    EditorImageGpu EditorImageGpu::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      EditorImageGpu l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = EditorImageGpu::TYPE_ID;

      return l_Handle;
    }

    EditorImageGpu EditorImageGpu::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      EditorImageGpu l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = EditorImageGpu::TYPE_ID;

      return l_Handle;
    }

    bool EditorImageGpu::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == EditorImageGpu::TYPE_ID &&
             check_alive(ms_Slots, EditorImageGpu::get_capacity());
    }

    uint32_t EditorImageGpu::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    EditorImageGpu::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    EditorImageGpu
    EditorImageGpu::find_by_name(Low::Util::Name p_Name)
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

    EditorImageGpu
    EditorImageGpu::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      EditorImageGpu l_Handle = make(p_Name);
      l_Handle.set_imgui_texture_initialized(
          is_imgui_texture_initialized());
      l_Handle.set_data_handle(get_data_handle());
      l_Handle.set_editor_image_handle(get_editor_image_handle());
      l_Handle.set_imgui_texture_id(get_imgui_texture_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    EditorImageGpu EditorImageGpu::duplicate(EditorImageGpu p_Handle,
                                             Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    EditorImageGpu::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Name p_Name)
    {
      EditorImageGpu l_EditorImageGpu = p_Handle.get_id();
      return l_EditorImageGpu.duplicate(p_Name);
    }

    void
    EditorImageGpu::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["imgui_texture_initialized"] =
          is_imgui_texture_initialized();
      p_Node["data_handle"] = get_data_handle();
      p_Node["editor_image_handle"] = get_editor_image_handle();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void EditorImageGpu::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
    {
      EditorImageGpu l_EditorImageGpu = p_Handle.get_id();
      l_EditorImageGpu.serialize(p_Node);
    }

    Low::Util::Handle
    EditorImageGpu::deserialize(Low::Util::Yaml::Node &p_Node,
                                Low::Util::Handle p_Creator)
    {
      EditorImageGpu l_Handle =
          EditorImageGpu::make(N(EditorImageGpu));

      if (p_Node["imgui_texture_initialized"]) {
        l_Handle.set_imgui_texture_initialized(
            p_Node["imgui_texture_initialized"].as<bool>());
      }
      if (p_Node["data_handle"]) {
        l_Handle.set_data_handle(
            p_Node["data_handle"].as<uint64_t>());
      }
      if (p_Node["editor_image_handle"]) {
        l_Handle.set_editor_image_handle(
            p_Node["editor_image_handle"].as<uint64_t>());
      }
      if (p_Node["imgui_texture_id"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void EditorImageGpu::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 EditorImageGpu::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 EditorImageGpu::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void EditorImageGpu::notify(Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void EditorImageGpu::_notify(Low::Util::Handle p_Observer,
                                 Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      EditorImageGpu l_EditorImageGpu = p_Observer.get_id();
      l_EditorImageGpu.notify(p_Observed, p_Observable);
    }

    bool EditorImageGpu::is_imgui_texture_initialized() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_texture_initialized

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(EditorImageGpu, imgui_texture_initialized,
                      bool);
    }
    void EditorImageGpu::toggle_imgui_texture_initialized()
    {
      set_imgui_texture_initialized(!is_imgui_texture_initialized());
    }

    void EditorImageGpu::set_imgui_texture_initialized(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_imgui_texture_initialized

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(EditorImageGpu, imgui_texture_initialized, bool) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:SETTER_imgui_texture_initialized

      broadcast_observable(N(imgui_texture_initialized));
    }

    uint64_t EditorImageGpu::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(EditorImageGpu, data_handle, uint64_t);
    }
    void EditorImageGpu::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(EditorImageGpu, data_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      ms_Dirty.insert(get_id());
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    uint64_t EditorImageGpu::get_editor_image_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_editor_image_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(EditorImageGpu, editor_image_handle, uint64_t);
    }
    void EditorImageGpu::set_editor_image_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_editor_image_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(EditorImageGpu, editor_image_handle, uint64_t) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_editor_image_handle

      broadcast_observable(N(editor_image_handle));
    }

    ImTextureID EditorImageGpu::get_imgui_texture_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_texture_id

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(EditorImageGpu, imgui_texture_id, ImTextureID);
    }
    void EditorImageGpu::set_imgui_texture_id(ImTextureID p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_imgui_texture_id

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(EditorImageGpu, imgui_texture_id, ImTextureID) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_imgui_texture_id

      broadcast_observable(N(imgui_texture_id));
    }

    Low::Util::Name EditorImageGpu::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(EditorImageGpu, name, Low::Util::Name);
    }
    void EditorImageGpu::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(EditorImageGpu, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t EditorImageGpu::create_instance()
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

    void EditorImageGpu::increase_budget()
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
                            sizeof(EditorImageGpuData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(EditorImageGpuData,
                                     imgui_texture_initialized) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EditorImageGpuData,
                                   imgui_texture_initialized) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(EditorImageGpuData, data_handle) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(EditorImageGpuData, data_handle) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EditorImageGpuData,
                                     editor_image_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EditorImageGpuData,
                                   editor_image_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EditorImageGpuData,
                                     imgui_texture_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EditorImageGpuData,
                                   imgui_texture_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(ImTextureID));
      }
      {
        memcpy(&l_NewBuffer[offsetof(EditorImageGpuData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(EditorImageGpuData, name) *
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

      LOW_LOG_DEBUG
          << "Auto-increased budget for EditorImageGpu from "
          << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
          << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
