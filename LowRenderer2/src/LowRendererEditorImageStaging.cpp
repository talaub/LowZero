#include "LowRendererEditorImageStaging.h"

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
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t EditorImageStaging::TYPE_ID = 83;
    uint32_t EditorImageStaging::ms_Capacity = 0u;
    uint32_t EditorImageStaging::ms_PageSize = 0u;
    Low::Util::SharedMutex EditorImageStaging::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        EditorImageStaging::ms_PagesLock(
            EditorImageStaging::ms_PagesMutex, std::defer_lock);
    Low::Util::List<EditorImageStaging>
        EditorImageStaging::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        EditorImageStaging::ms_Pages;

    EditorImageStaging::EditorImageStaging() : Low::Util::Handle(0ull)
    {
    }
    EditorImageStaging::EditorImageStaging(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    EditorImageStaging::EditorImageStaging(EditorImageStaging &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle
    EditorImageStaging::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    EditorImageStaging
    EditorImageStaging::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      EditorImageStaging l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = EditorImageStaging::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
          l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImageStaging,
                                 dimensions, Low::Math::UVector2))
          Low::Math::UVector2();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, EditorImageStaging, format,
                                 Low::Util::Resource::Image2DFormat))
          Low::Util::Resource::Image2DFormat();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, EditorImageStaging, pixel_data,
          Low::Util::List<uint8_t>)) Low::Util::List<uint8_t>();
      ACCESSOR_TYPE_SOA(l_Handle, EditorImageStaging, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void EditorImageStaging::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
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

    void EditorImageStaging::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(EditorImageStaging));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, EditorImageStaging::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(EditorImageStaging);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &EditorImageStaging::is_alive;
      l_TypeInfo.destroy = &EditorImageStaging::destroy;
      l_TypeInfo.serialize = &EditorImageStaging::serialize;
      l_TypeInfo.deserialize = &EditorImageStaging::deserialize;
      l_TypeInfo.find_by_index = &EditorImageStaging::_find_by_index;
      l_TypeInfo.notify = &EditorImageStaging::_notify;
      l_TypeInfo.find_by_name = &EditorImageStaging::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &EditorImageStaging::_make;
      l_TypeInfo.duplicate_default = &EditorImageStaging::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &EditorImageStaging::living_instances);
      l_TypeInfo.get_living_count = &EditorImageStaging::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, dimensions,
              Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions(*(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((Low::Math::UVector2 *)p_Data) =
              l_Handle.get_dimensions();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dimensions
      }
      {
        // Property: channels
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(channels);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, channels);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_channels();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, channels, uint8_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_channels(*(uint8_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((uint8_t *)p_Data) = l_Handle.get_channels();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: channels
      }
      {
        // Property: format
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(format);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, format);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_format();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, format,
              Low::Util::Resource::Image2DFormat);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_format(
              *(Low::Util::Resource::Image2DFormat *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((Low::Util::Resource::Image2DFormat *)p_Data) =
              l_Handle.get_format();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: format
      }
      {
        // Property: pixel_data
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(pixel_data);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, pixel_data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_pixel_data();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, pixel_data,
              Low::Util::List<uint8_t>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_pixel_data(
              *(Low::Util::List<uint8_t> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((Low::Util::List<uint8_t> *)p_Data) =
              l_Handle.get_pixel_data();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: pixel_data
      }
      {
        // Property: data_size
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_size);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, data_size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_data_size();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, data_size, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_data_size(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_data_size();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_size
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(EditorImageStaging::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, EditorImageStaging, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          EditorImageStaging l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          EditorImageStaging l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<EditorImageStaging> l_HandleLock(
              l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void EditorImageStaging::cleanup()
    {
      Low::Util::List<EditorImageStaging> l_Instances =
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
    EditorImageStaging::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    EditorImageStaging
    EditorImageStaging::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      EditorImageStaging l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = EditorImageStaging::TYPE_ID;

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

    EditorImageStaging
    EditorImageStaging::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      EditorImageStaging l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = EditorImageStaging::TYPE_ID;

      return l_Handle;
    }

    bool EditorImageStaging::is_alive() const
    {
      if (m_Data.m_Type != EditorImageStaging::TYPE_ID) {
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
      return m_Data.m_Type == EditorImageStaging::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t EditorImageStaging::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    EditorImageStaging::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    EditorImageStaging
    EditorImageStaging::find_by_name(Low::Util::Name p_Name)
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

    EditorImageStaging
    EditorImageStaging::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      EditorImageStaging l_Handle = make(p_Name);
      l_Handle.set_dimensions(get_dimensions());
      l_Handle.set_channels(get_channels());
      l_Handle.set_format(get_format());
      l_Handle.set_pixel_data(get_pixel_data());
      l_Handle.set_data_size(get_data_size());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    EditorImageStaging
    EditorImageStaging::duplicate(EditorImageStaging p_Handle,
                                  Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    EditorImageStaging::_duplicate(Low::Util::Handle p_Handle,
                                   Low::Util::Name p_Name)
    {
      EditorImageStaging l_EditorImageStaging = p_Handle.get_id();
      return l_EditorImageStaging.duplicate(p_Name);
    }

    void
    EditorImageStaging::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["channels"] = get_channels();
      p_Node["data_size"] = get_data_size();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void EditorImageStaging::serialize(Low::Util::Handle p_Handle,
                                       Low::Util::Yaml::Node &p_Node)
    {
      EditorImageStaging l_EditorImageStaging = p_Handle.get_id();
      l_EditorImageStaging.serialize(p_Node);
    }

    Low::Util::Handle
    EditorImageStaging::deserialize(Low::Util::Yaml::Node &p_Node,
                                    Low::Util::Handle p_Creator)
    {
      EditorImageStaging l_Handle =
          EditorImageStaging::make(N(EditorImageStaging));

      if (p_Node["dimensions"]) {
      }
      if (p_Node["channels"]) {
        l_Handle.set_channels(p_Node["channels"].as<uint8_t>());
      }
      if (p_Node["format"]) {
      }
      if (p_Node["pixel_data"]) {
      }
      if (p_Node["data_size"]) {
        l_Handle.set_data_size(p_Node["data_size"].as<uint64_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void EditorImageStaging::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 EditorImageStaging::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64
    EditorImageStaging::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void EditorImageStaging::notify(Low::Util::Handle p_Observed,
                                    Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void EditorImageStaging::_notify(Low::Util::Handle p_Observer,
                                     Low::Util::Handle p_Observed,
                                     Low::Util::Name p_Observable)
    {
      EditorImageStaging l_EditorImageStaging = p_Observer.get_id();
      l_EditorImageStaging.notify(p_Observed, p_Observable);
    }

    Low::Math::UVector2 &EditorImageStaging::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      return TYPE_SOA(EditorImageStaging, dimensions,
                      Low::Math::UVector2);
    }
    void EditorImageStaging::set_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_dimensions(l_Val);
    }

    void EditorImageStaging::set_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.x = p_Value;
      set_dimensions(l_Value);
    }

    void EditorImageStaging::set_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.y = p_Value;
      set_dimensions(l_Value);
    }

    void
    EditorImageStaging::set_dimensions(Low::Math::UVector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions

      // Set new value
      TYPE_SOA(EditorImageStaging, dimensions, Low::Math::UVector2) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions

      broadcast_observable(N(dimensions));
    }

    uint8_t EditorImageStaging::get_channels() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_channels
      // LOW_CODEGEN::END::CUSTOM:GETTER_channels

      return TYPE_SOA(EditorImageStaging, channels, uint8_t);
    }
    void EditorImageStaging::set_channels(uint8_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_channels
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_channels

      // Set new value
      TYPE_SOA(EditorImageStaging, channels, uint8_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_channels
      // LOW_CODEGEN::END::CUSTOM:SETTER_channels

      broadcast_observable(N(channels));
    }

    Low::Util::Resource::Image2DFormat
    EditorImageStaging::get_format() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_format
      // LOW_CODEGEN::END::CUSTOM:GETTER_format

      return TYPE_SOA(EditorImageStaging, format,
                      Low::Util::Resource::Image2DFormat);
    }
    void EditorImageStaging::set_format(
        Low::Util::Resource::Image2DFormat p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_format
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_format

      // Set new value
      TYPE_SOA(EditorImageStaging, format,
               Low::Util::Resource::Image2DFormat) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_format
      // LOW_CODEGEN::END::CUSTOM:SETTER_format

      broadcast_observable(N(format));
    }

    Low::Util::List<uint8_t> &
    EditorImageStaging::get_pixel_data() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_data

      return TYPE_SOA(EditorImageStaging, pixel_data,
                      Low::Util::List<uint8_t>);
    }
    void EditorImageStaging::set_pixel_data(
        Low::Util::List<uint8_t> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_data

      // Set new value
      TYPE_SOA(EditorImageStaging, pixel_data,
               Low::Util::List<uint8_t>) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_data

      broadcast_observable(N(pixel_data));
    }

    uint64_t EditorImageStaging::get_data_size() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_size

      return TYPE_SOA(EditorImageStaging, data_size, uint64_t);
    }
    void EditorImageStaging::set_data_size(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_size

      // Set new value
      TYPE_SOA(EditorImageStaging, data_size, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_size

      broadcast_observable(N(data_size));
    }

    Low::Util::Name EditorImageStaging::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(EditorImageStaging, name, Low::Util::Name);
    }
    void EditorImageStaging::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<EditorImageStaging> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(EditorImageStaging, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t EditorImageStaging::create_instance(
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

    u32 EditorImageStaging::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for EditorImageStaging.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, EditorImageStaging::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool EditorImageStaging::get_page_for_index(const u32 p_Index,
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
