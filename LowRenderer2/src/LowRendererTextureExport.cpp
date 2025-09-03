#include "LowRendererTextureExport.h"

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

    const uint16_t TextureExport::TYPE_ID = 80;
    uint32_t TextureExport::ms_Capacity = 0u;
    uint8_t *TextureExport::ms_Buffer = 0;
    std::shared_mutex TextureExport::ms_BufferMutex;
    Low::Util::Instances::Slot *TextureExport::ms_Slots = 0;
    Low::Util::List<TextureExport> TextureExport::ms_LivingInstances =
        Low::Util::List<TextureExport>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      TextureExport l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = TextureExport::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureExport, path,
                              Low::Util::String)) Low::Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureExport, texture,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureExport, state,
                              Low::Renderer::TextureExportState))
          Low::Renderer::TextureExportState();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>))
          Low::Util::Function<bool(Low::Renderer::TextureExport)>();
      ACCESSOR_TYPE_SOA(l_Handle, TextureExport, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

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

    void TextureExport::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(TextureExport));

      initialize_buffer(&ms_Buffer, TextureExportData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_TextureExport);
      LOW_PROFILE_ALLOC(type_slots_TextureExport);

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
        l_PropertyInfo.dataOffset = offsetof(TextureExportData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
            offsetof(TextureExportData, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
            offsetof(TextureExportData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
            offsetof(TextureExportData, finish_callback);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
            offsetof(TextureExportData, data_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(TextureExportData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureExport l_Handle = p_Handle.get_id();
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
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_TextureExport);
      LOW_PROFILE_FREE(type_slots_TextureExport);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = TextureExport::TYPE_ID;

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
      READ_LOCK(l_Lock);
      return m_Data.m_Type == TextureExport::TYPE_ID &&
             check_alive(ms_Slots, TextureExport::get_capacity());
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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      READ_LOCK(l_ReadLock);
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

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureExport, path, Low::Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Low::Renderer::Texture TextureExport::get_texture() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureExport, texture, Low::Renderer::Texture);
    }
    void TextureExport::set_texture(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureExport, texture, Low::Renderer::Texture) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture

      broadcast_observable(N(texture));
    }

    Low::Renderer::TextureExportState TextureExport::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureExport, state,
                      Low::Renderer::TextureExportState);
    }
    void TextureExport::set_state(
        Low::Renderer::TextureExportState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureExport, state,
               Low::Renderer::TextureExportState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Function<bool(Low::Renderer::TextureExport)>
    TextureExport::get_finish_callback() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:GETTER_finish_callback

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(
          TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>);
    }
    void TextureExport::set_finish_callback(
        Low::Util::Function<bool(Low::Renderer::TextureExport)>
            p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_finish_callback

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(
          TextureExport, finish_callback,
          Low::Util::Function<bool(Low::Renderer::TextureExport)>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_finish_callback
      // LOW_CODEGEN::END::CUSTOM:SETTER_finish_callback

      broadcast_observable(N(finish_callback));
    }

    uint64_t TextureExport::get_data_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureExport, data_handle, uint64_t);
    }
    void TextureExport::set_data_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureExport, data_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_handle

      broadcast_observable(N(data_handle));
    }

    Low::Util::Name TextureExport::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureExport, name, Low::Util::Name);
    }
    void TextureExport::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureExport, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool TextureExport::finish()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_finish
      return get_finish_callback()(get_id());
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_finish
    }

    uint32_t TextureExport::create_instance()
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

    void TextureExport::increase_budget()
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
                            sizeof(TextureExportData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(TextureExportData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureExportData, path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureExportData, texture) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureExportData, texture) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureExportData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureExportData, state) *
                          (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Renderer::TextureExportState));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(TextureExportData,
                                  finish_callback) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(TextureExportData, finish_callback) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Util::Function<bool(
                                    Low::Renderer::TextureExport)>));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureExportData, data_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureExportData, data_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureExportData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureExportData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for TextureExport from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
