#include "LowRendererTextureExport.h"

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

    const uint16_t TextureExport::TYPE_ID = 80;
    uint32_t TextureExport::ms_Capacity = 0u;
    uint32_t TextureExport::ms_PageSize = 0u;
    Low::Util::SharedMutex TextureExport::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        TextureExport::ms_PagesLock(TextureExport::ms_PagesMutex,
                                    std::defer_lock);
    Low::Util::List<TextureExport> TextureExport::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        TextureExport::ms_Pages;

    TextureExport::TextureExport() : Low::Util::Handle(0ull)
    {
    }
    TextureExport::TextureExport(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    TextureExport::TextureExport(TextureExport &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle TextureExport::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    TextureExport TextureExport::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      TextureExport l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = TextureExport::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureExport, path,
                                 Low::Util::String))
          Low::Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureExport, texture,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureExport, state,
                                 Low::Renderer::TextureExportState))
          Low::Renderer::TextureExportState();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>))
          Low::Util::Function<bool(Low::Renderer::TextureExport)>();
      ACCESSOR_TYPE_SOA(l_Handle, TextureExport, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(TextureExportState::SCHEDULED);
      l_Handle.set_finish_callback(
          [](TextureExport p_Export) -> bool { return true; });
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void TextureExport::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<TextureExport> l_Lock(get_id());
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

    void TextureExport::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(TextureExport));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, TextureExport::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(TextureExport);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &TextureExport::is_alive;
      l_TypeInfo.destroy = &TextureExport::destroy;
      l_TypeInfo.serialize = &TextureExport::serialize;
      l_TypeInfo.deserialize = &TextureExport::deserialize;
      l_TypeInfo.find_by_index = &TextureExport::_find_by_index;
      l_TypeInfo.notify = &TextureExport::_notify;
      l_TypeInfo.find_by_name = &TextureExport::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &TextureExport::_make;
      l_TypeInfo.duplicate_default = &TextureExport::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &TextureExport::living_instances);
      l_TypeInfo.get_living_count = &TextureExport::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureExport,
                                            path, Low::Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_path(*(Low::Util::String *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((Low::Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: texture
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(texture);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_texture();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureExport,
                                            texture,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_texture(*(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_texture();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: texture
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureExport, state,
              Low::Renderer::TextureExportState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_state(
              *(Low::Renderer::TextureExportState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((Low::Renderer::TextureExportState *)p_Data) =
              l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: finish_callback
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(finish_callback);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, finish_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_finish_callback();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureExport, finish_callback,
              Low::Util::Function<bool(
                  Low::Renderer::TextureExport)>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_finish_callback(*(
              Low::Util::Function<bool(Low::Renderer::TextureExport)>
                  *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((Low::Util::Function<bool(Low::Renderer::TextureExport)>
                 *)p_Data) = l_Handle.get_finish_callback();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: finish_callback
      }
      {
        // Property: data_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(data_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_data_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureExport,
                                            data_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_data_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_data_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_handle
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureExport::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureExport,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureExport l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureExport l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureExport> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: finish
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(finish);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: finish
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void TextureExport::cleanup()
    {
      Low::Util::List<TextureExport> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle TextureExport::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    TextureExport TextureExport::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      TextureExport l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = TextureExport::TYPE_ID;

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

    TextureExport TextureExport::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      TextureExport l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = TextureExport::TYPE_ID;

      return l_Handle;
    }

    bool TextureExport::is_alive() const
    {
      if (m_Data.m_Type != TextureExport::TYPE_ID) {
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
      return m_Data.m_Type == TextureExport::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t TextureExport::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    TextureExport::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    TextureExport TextureExport::find_by_name(Low::Util::Name p_Name)
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

    TextureExport
    TextureExport::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      TextureExport l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      if (get_texture().is_alive()) {
        l_Handle.set_texture(get_texture());
      }
      l_Handle.set_state(get_state());
      l_Handle.set_finish_callback(get_finish_callback());
      l_Handle.set_data_handle(get_data_handle());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    TextureExport TextureExport::duplicate(TextureExport p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    TextureExport::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      TextureExport l_TextureExport = p_Handle.get_id();
      return l_TextureExport.duplicate(p_Name);
    }

    void TextureExport::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["path"] = get_path().c_str();
      if (get_texture().is_alive()) {
        get_texture().serialize(p_Node["texture"]);
      }
      p_Node["data_handle"] = get_data_handle();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void TextureExport::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Yaml::Node &p_Node)
    {
      TextureExport l_TextureExport = p_Handle.get_id();
      l_TextureExport.serialize(p_Node);
    }

    Low::Util::Handle
    TextureExport::deserialize(Low::Util::Yaml::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {
      TextureExport l_Handle = TextureExport::make(N(TextureExport));

      if (p_Node["path"]) {
        l_Handle.set_path(LOW_YAML_AS_STRING(p_Node["path"]));
      }
      if (p_Node["texture"]) {
        l_Handle.set_texture(Low::Renderer::Texture::deserialize(
                                 p_Node["texture"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["state"]) {
      }
      if (p_Node["finish_callback"]) {
      }
      if (p_Node["data_handle"]) {
        l_Handle.set_data_handle(
            p_Node["data_handle"].as<uint64_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void TextureExport::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 TextureExport::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 TextureExport::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void TextureExport::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void TextureExport::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      TextureExport l_TextureExport = p_Observer.get_id();
      l_TextureExport.notify(p_Observed, p_Observable);
    }

    Low::Util::String &TextureExport::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(TextureExport, path, Low::Util::String);
    }
    void TextureExport::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void TextureExport::set_path(Low::Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(TextureExport, path, Low::Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Low::Renderer::Texture TextureExport::get_texture() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      return TYPE_SOA(TextureExport, texture, Low::Renderer::Texture);
    }
    void TextureExport::set_texture(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      // Set new value
      TYPE_SOA(TextureExport, texture, Low::Renderer::Texture) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture

      broadcast_observable(N(texture));
    }

    Low::Renderer::TextureExportState TextureExport::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(TextureExport, state,
                      Low::Renderer::TextureExportState);
    }
    void TextureExport::set_state(
        Low::Renderer::TextureExportState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(TextureExport, state,
               Low::Renderer::TextureExportState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Function<bool(Low::Renderer::TextureExport)>
    TextureExport::get_finish_callback() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_finish_callback

      return TYPE_SOA(
          TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>);
    }
    void TextureExport::set_finish_callback(
        Low::Util::Function<bool(Low::Renderer::TextureExport)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_finish_callback

      // Set new value
      TYPE_SOA(
          TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_finish_callback

      broadcast_observable(N(finish_callback));
    }

    uint64_t TextureExport::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      return TYPE_SOA(TextureExport, data_handle, uint64_t);
    }
    void TextureExport::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      TYPE_SOA(TextureExport, data_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    Low::Util::Name TextureExport::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(TextureExport, name, Low::Util::Name);
    }
    void TextureExport::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(TextureExport, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool TextureExport::finish()
    {
      Low::Util::HandleLock<TextureExport> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_finish
      return get_finish_callback()(get_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_finish
    }

    uint32_t TextureExport::create_instance(
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

    u32 TextureExport::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for TextureExport.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, TextureExport::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool TextureExport::get_page_for_index(const u32 p_Index,
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
