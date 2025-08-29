#include "LowRendererFontResource.h"

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

    const uint16_t FontResource::TYPE_ID = 78;
    uint32_t FontResource::ms_Capacity = 0u;
    uint8_t *FontResource::ms_Buffer = 0;
    std::shared_mutex FontResource::ms_BufferMutex;
    Low::Util::Instances::Slot *FontResource::ms_Slots = 0;
    Low::Util::List<FontResource> FontResource::ms_LivingInstances =
        Low::Util::List<FontResource>();

    FontResource::FontResource() : Low::Util::Handle(0ull)
    {
    }
    FontResource::FontResource(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    FontResource::FontResource(FontResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle FontResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    FontResource FontResource::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      FontResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = FontResource::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, font_path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, sidecar_path,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, source_file,
                              Util::String)) Util::String();
      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, font_id,
                              uint64_t)) uint64_t();
      new (&ACCESSOR_TYPE_SOA(l_Handle, FontResource, asset_hash,
                              uint64_t)) uint64_t();
      ACCESSOR_TYPE_SOA(l_Handle, FontResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void FontResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const FontResource *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void FontResource::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(FontResource));

      initialize_buffer(&ms_Buffer, FontResourceData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_FontResource);
      LOW_PROFILE_ALLOC(type_slots_FontResource);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(FontResource);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &FontResource::is_alive;
      l_TypeInfo.destroy = &FontResource::destroy;
      l_TypeInfo.serialize = &FontResource::serialize;
      l_TypeInfo.deserialize = &FontResource::deserialize;
      l_TypeInfo.find_by_index = &FontResource::_find_by_index;
      l_TypeInfo.notify = &FontResource::_notify;
      l_TypeInfo.find_by_name = &FontResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &FontResource::_make;
      l_TypeInfo.duplicate_default = &FontResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &FontResource::living_instances);
      l_TypeInfo.get_living_count = &FontResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontResourceData, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, FontResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: font_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(font_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontResourceData, font_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_font_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, FontResource,
                                            font_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_font_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: font_path
      }
      {
        // Property: sidecar_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(sidecar_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontResourceData, sidecar_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_sidecar_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, FontResource, sidecar_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_sidecar_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: sidecar_path
      }
      {
        // Property: source_file
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(source_file);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontResourceData, source_file);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_source_file();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, FontResource, source_file, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((Util::String *)p_Data) = l_Handle.get_source_file();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: source_file
      }
      {
        // Property: font_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(font_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontResourceData, font_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_font_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, FontResource,
                                            font_id, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_font_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: font_id
      }
      {
        // Property: asset_hash
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(asset_hash);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(FontResourceData, asset_hash);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_asset_hash();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, FontResource,
                                            asset_hash, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_asset_hash();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: asset_hash
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontResourceData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, FontResource,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          FontResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          FontResource l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = FontResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: make_from_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = FontResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_config
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void FontResource::cleanup()
    {
      Low::Util::List<FontResource> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_FontResource);
      LOW_PROFILE_FREE(type_slots_FontResource);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle FontResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    FontResource FontResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      FontResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = FontResource::TYPE_ID;

      return l_Handle;
    }

    bool FontResource::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == FontResource::TYPE_ID &&
             check_alive(ms_Slots, FontResource::get_capacity());
    }

    uint32_t FontResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    FontResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    FontResource FontResource::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    FontResource FontResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      FontResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_font_path(get_font_path());
      l_Handle.set_sidecar_path(get_sidecar_path());
      l_Handle.set_source_file(get_source_file());
      l_Handle.set_font_id(get_font_id());
      l_Handle.set_asset_hash(get_asset_hash());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    FontResource FontResource::duplicate(FontResource p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    FontResource::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      FontResource l_FontResource = p_Handle.get_id();
      return l_FontResource.duplicate(p_Name);
    }

    void FontResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void FontResource::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      FontResource l_FontResource = p_Handle.get_id();
      l_FontResource.serialize(p_Node);
    }

    Low::Util::Handle
    FontResource::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      LOW_NOT_IMPLEMENTED;
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void FontResource::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 FontResource::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void FontResource::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void FontResource::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      FontResource l_FontResource = p_Observer.get_id();
      l_FontResource.notify(p_Observed, p_Observable);
    }

    Util::String &FontResource::get_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, path, Util::String);
    }
    void FontResource::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void FontResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::String &FontResource::get_font_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_font_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, font_path, Util::String);
    }
    void FontResource::set_font_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_font_path(l_Val);
    }

    void FontResource::set_font_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_font_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, font_path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_font_path

      broadcast_observable(N(font_path));
    }

    Util::String &FontResource::get_sidecar_path() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_sidecar_path

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, sidecar_path, Util::String);
    }
    void FontResource::set_sidecar_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_sidecar_path(l_Val);
    }

    void FontResource::set_sidecar_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_sidecar_path

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, sidecar_path, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_sidecar_path

      broadcast_observable(N(sidecar_path));
    }

    Util::String &FontResource::get_source_file() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:GETTER_source_file

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, source_file, Util::String);
    }
    void FontResource::set_source_file(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_source_file(l_Val);
    }

    void FontResource::set_source_file(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_source_file

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, source_file, Util::String) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:SETTER_source_file

      broadcast_observable(N(source_file));
    }

    uint64_t &FontResource::get_font_id() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_font_id

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, font_id, uint64_t);
    }
    void FontResource::set_font_id(uint64_t &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_font_id

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, font_id, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_font_id

      broadcast_observable(N(font_id));
    }

    uint64_t &FontResource::get_asset_hash() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:GETTER_asset_hash

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, asset_hash, uint64_t);
    }
    void FontResource::set_asset_hash(uint64_t &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_asset_hash

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, asset_hash, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:SETTER_asset_hash

      broadcast_observable(N(asset_hash));
    }

    Low::Util::Name FontResource::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(FontResource, name, Low::Util::Name);
    }
    void FontResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(FontResource, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    FontResource FontResource::make(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      Util::String l_FileName =
          p_Path.substr(p_Path.find_last_of("/\\") + 1);
      FontResource l_FontResource =
          FontResource::make(LOW_NAME(l_FileName.c_str()));
      l_FontResource.set_path(p_Path);

      return l_FontResource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    FontResource
    FontResource::make_from_config(FontResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_config
      FontResource l_FontResource = FontResource::make(p_Config.name);
      l_FontResource.set_path(p_Config.path);
      l_FontResource.set_font_id(p_Config.fontId);
      l_FontResource.set_asset_hash(p_Config.assetHash);
      l_FontResource.set_source_file(p_Config.sourceFile);
      l_FontResource.set_sidecar_path(p_Config.sidecarPath);
      l_FontResource.set_font_path(p_Config.fontPath);

      return l_FontResource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_config
    }

    uint32_t FontResource::create_instance()
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

    void FontResource::increase_budget()
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
                            sizeof(FontResourceData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, font_path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, font_path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, sidecar_path) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, sidecar_path) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, source_file) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, source_file) *
                          (l_Capacity)],
               l_Capacity * sizeof(Util::String));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, font_id) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, font_id) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, asset_hash) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, asset_hash) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontResourceData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontResourceData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for FontResource from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
