#include "LowRendererEditorImageGpu.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
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
    uint32_t EditorImageGpu::ms_PageSize = 0u;
    Low::Util::SharedMutex EditorImageGpu::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        EditorImageGpu::ms_PagesLock(EditorImageGpu::ms_PagesMutex,
                                     std::defer_lock);
    Low::Util::List<EditorImageGpu>
        EditorImageGpu::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        EditorImageGpu::ms_Pages;

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
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      EditorImageGpu l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = EditorImageGpu::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<EditorImageGpu> l_HandleLock(l_Handle);

      ACCESSOR_TYPE_SOA(l_Handle, EditorImageGpu,
                        imgui_texture_initialized, bool) = false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImageGpu,
                                 imgui_texture_id, ImTextureID))
          ImTextureID();
      ACCESSOR_TYPE_SOA(l_Handle, EditorImageGpu, name,
                        Low::Util::Name) = Low::Util::Name(0u);

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

      {
        Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // TODO: remove vulkan imgui image
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

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

    void EditorImageGpu::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(EditorImageGpu));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, EditorImageGpu::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
            offsetof(EditorImageGpu::Data, imgui_texture_initialized);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
            offsetof(EditorImageGpu::Data, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
            offsetof(EditorImageGpu::Data, editor_image_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
            offsetof(EditorImageGpu::Data, imgui_texture_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
            offsetof(EditorImageGpu::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageGpu l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<EditorImageGpu> l_HandleLock(
              l_Handle);
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

    Low::Util::Handle EditorImageGpu::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    EditorImageGpu EditorImageGpu::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      EditorImageGpu l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = EditorImageGpu::TYPE_ID;

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
      if (m_Data.m_Type != EditorImageGpu::TYPE_ID) {
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
      return m_Data.m_Type == EditorImageGpu::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_texture_initialized

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
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_imgui_texture_initialized

      // Set new value
      TYPE_SOA(EditorImageGpu, imgui_texture_initialized, bool) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_imgui_texture_initialized
      // LOW_CODEGEN::END::CUSTOM:SETTER_imgui_texture_initialized

      broadcast_observable(N(imgui_texture_initialized));
    }

    uint64_t EditorImageGpu::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      return TYPE_SOA(EditorImageGpu, data_handle, uint64_t);
    }
    void EditorImageGpu::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      TYPE_SOA(EditorImageGpu, data_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      ms_Dirty.insert(get_id());
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    uint64_t EditorImageGpu::get_editor_image_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_editor_image_handle

      return TYPE_SOA(EditorImageGpu, editor_image_handle, uint64_t);
    }
    void EditorImageGpu::set_editor_image_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_editor_image_handle

      // Set new value
      TYPE_SOA(EditorImageGpu, editor_image_handle, uint64_t) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_editor_image_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_editor_image_handle

      broadcast_observable(N(editor_image_handle));
    }

    ImTextureID EditorImageGpu::get_imgui_texture_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_imgui_texture_id

      return TYPE_SOA(EditorImageGpu, imgui_texture_id, ImTextureID);
    }
    void EditorImageGpu::set_imgui_texture_id(ImTextureID p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_imgui_texture_id

      // Set new value
      TYPE_SOA(EditorImageGpu, imgui_texture_id, ImTextureID) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_imgui_texture_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_imgui_texture_id

      broadcast_observable(N(imgui_texture_id));
    }

    Low::Util::Name EditorImageGpu::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(EditorImageGpu, name, Low::Util::Name);
    }
    void EditorImageGpu::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageGpu> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(EditorImageGpu, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t EditorImageGpu::create_instance(
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

    u32 EditorImageGpu::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for EditorImageGpu.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, EditorImageGpu::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool EditorImageGpu::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
