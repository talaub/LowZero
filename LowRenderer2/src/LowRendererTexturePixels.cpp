#include "LowRendererTexturePixels.h"

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

    const uint16_t TexturePixels::TYPE_ID = 73;
    uint32_t TexturePixels::ms_Capacity = 0u;
    uint8_t *TexturePixels::ms_Buffer = 0;
    std::shared_mutex TexturePixels::ms_BufferMutex;
    Low::Util::Instances::Slot *TexturePixels::ms_Slots = 0;
    Low::Util::List<TexturePixels> TexturePixels::ms_LivingInstances =
        Low::Util::List<TexturePixels>();

    TexturePixels::TexturePixels() : Low::Util::Handle(0ull)
    {
    }
    TexturePixels::TexturePixels(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    TexturePixels::TexturePixels(TexturePixels &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle TexturePixels::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    TexturePixels TexturePixels::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      TexturePixels l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = TexturePixels::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, TexturePixels, dimensions,
                              Low::Math::UVector2))
          Low::Math::UVector2();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TexturePixels, format,
                              Low::Util::Resource::Image2DFormat))
          Low::Util::Resource::Image2DFormat();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TexturePixels, pixel_data,
                              Low::Util::List<uint8_t>))
          Low::Util::List<uint8_t>();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TexturePixels, state,
                              Low::Renderer::TextureState))
          Low::Renderer::TextureState();
      ACCESSOR_TYPE_SOA(l_Handle, TexturePixels, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void TexturePixels::destroy()
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

    void TexturePixels::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(TexturePixels));

      initialize_buffer(&ms_Buffer, TexturePixelsData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_TexturePixels);
      LOW_PROFILE_ALLOC(type_slots_TexturePixels);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(TexturePixels);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &TexturePixels::is_alive;
      l_TypeInfo.destroy = &TexturePixels::destroy;
      l_TypeInfo.serialize = &TexturePixels::serialize;
      l_TypeInfo.deserialize = &TexturePixels::deserialize;
      l_TypeInfo.find_by_index = &TexturePixels::_find_by_index;
      l_TypeInfo.notify = &TexturePixels::_notify;
      l_TypeInfo.find_by_name = &TexturePixels::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &TexturePixels::_make;
      l_TypeInfo.duplicate_default = &TexturePixels::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &TexturePixels::living_instances);
      l_TypeInfo.get_living_count = &TexturePixels::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: dimensions
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dimensions);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TexturePixelsData, dimensions);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_dimensions();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexturePixels,
                                            dimensions,
                                            Low::Math::UVector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_dimensions(*(Low::Math::UVector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
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
            offsetof(TexturePixelsData, channels);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_channels();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexturePixels,
                                            channels, uint8_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_channels(*(uint8_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
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
            offsetof(TexturePixelsData, format);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_format();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TexturePixels, format,
              Low::Util::Resource::Image2DFormat);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_format(
              *(Low::Util::Resource::Image2DFormat *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
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
            offsetof(TexturePixelsData, pixel_data);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_pixel_data();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexturePixels,
                                            pixel_data,
                                            Low::Util::List<uint8_t>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_pixel_data(
              *(Low::Util::List<uint8_t> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
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
            offsetof(TexturePixelsData, data_size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_data_size();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexturePixels,
                                            data_size, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_data_size(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_data_size();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: data_size
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TexturePixelsData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            Low::Renderer::TextureStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TexturePixels, state,
              Low::Renderer::TextureState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(Low::Renderer::TextureState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
          *((Low::Renderer::TextureState *)p_Data) =
              l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(TexturePixelsData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TexturePixels,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TexturePixels l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TexturePixels l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void TexturePixels::cleanup()
    {
      Low::Util::List<TexturePixels> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_TexturePixels);
      LOW_PROFILE_FREE(type_slots_TexturePixels);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle TexturePixels::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    TexturePixels TexturePixels::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      TexturePixels l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = TexturePixels::TYPE_ID;

      return l_Handle;
    }

    TexturePixels TexturePixels::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      TexturePixels l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = TexturePixels::TYPE_ID;

      return l_Handle;
    }

    bool TexturePixels::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == TexturePixels::TYPE_ID &&
             check_alive(ms_Slots, TexturePixels::get_capacity());
    }

    uint32_t TexturePixels::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    TexturePixels::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    TexturePixels TexturePixels::find_by_name(Low::Util::Name p_Name)
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

    TexturePixels
    TexturePixels::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      TexturePixels l_Handle = make(p_Name);
      l_Handle.set_dimensions(get_dimensions());
      l_Handle.set_channels(get_channels());
      l_Handle.set_format(get_format());
      l_Handle.set_pixel_data(get_pixel_data());
      l_Handle.set_data_size(get_data_size());
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    TexturePixels TexturePixels::duplicate(TexturePixels p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    TexturePixels::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      TexturePixels l_TexturePixels = p_Handle.get_id();
      return l_TexturePixels.duplicate(p_Name);
    }

    void TexturePixels::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["channels"] = get_channels();
      p_Node["data_size"] = get_data_size();
      Low::Util::Serialization::serialize_enum(
          p_Node["state"],
          Low::Renderer::TextureStateEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_state()));
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void TexturePixels::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Yaml::Node &p_Node)
    {
      TexturePixels l_TexturePixels = p_Handle.get_id();
      l_TexturePixels.serialize(p_Node);
    }

    Low::Util::Handle
    TexturePixels::deserialize(Low::Util::Yaml::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {
      TexturePixels l_Handle = TexturePixels::make(N(TexturePixels));

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
      if (p_Node["state"]) {
        l_Handle.set_state(static_cast<Low::Renderer::TextureState>(
            Low::Util::Serialization::deserialize_enum(
                p_Node["state"])));
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void TexturePixels::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 TexturePixels::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 TexturePixels::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void TexturePixels::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void TexturePixels::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      TexturePixels l_TexturePixels = p_Observer.get_id();
      l_TexturePixels.notify(p_Observed, p_Observable);
    }

    Low::Math::UVector2 &TexturePixels::get_dimensions() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:GETTER_dimensions

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, dimensions, Low::Math::UVector2);
    }
    void TexturePixels::set_dimensions(u32 p_X, u32 p_Y)
    {
      Low::Math::UVector2 l_Val(p_X, p_Y);
      set_dimensions(l_Val);
    }

    void TexturePixels::set_dimensions_x(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.x = p_Value;
      set_dimensions(l_Value);
    }

    void TexturePixels::set_dimensions_y(u32 p_Value)
    {
      Low::Math::UVector2 l_Value = get_dimensions();
      l_Value.y = p_Value;
      set_dimensions(l_Value);
    }

    void TexturePixels::set_dimensions(Low::Math::UVector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dimensions

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, dimensions, Low::Math::UVector2) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dimensions
      // LOW_CODEGEN::END::CUSTOM:SETTER_dimensions

      broadcast_observable(N(dimensions));
    }

    uint8_t TexturePixels::get_channels() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_channels
      // LOW_CODEGEN::END::CUSTOM:GETTER_channels

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, channels, uint8_t);
    }
    void TexturePixels::set_channels(uint8_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_channels
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_channels

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, channels, uint8_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_channels
      // LOW_CODEGEN::END::CUSTOM:SETTER_channels

      broadcast_observable(N(channels));
    }

    Low::Util::Resource::Image2DFormat &
    TexturePixels::get_format() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_format
      // LOW_CODEGEN::END::CUSTOM:GETTER_format

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, format,
                      Low::Util::Resource::Image2DFormat);
    }
    void TexturePixels::set_format(
        Low::Util::Resource::Image2DFormat &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_format
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_format

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, format,
               Low::Util::Resource::Image2DFormat) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_format
      // LOW_CODEGEN::END::CUSTOM:SETTER_format

      broadcast_observable(N(format));
    }

    Low::Util::List<uint8_t> &TexturePixels::get_pixel_data() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_data

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, pixel_data,
                      Low::Util::List<uint8_t>);
    }
    void
    TexturePixels::set_pixel_data(Low::Util::List<uint8_t> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_data

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, pixel_data, Low::Util::List<uint8_t>) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_data
      // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_data

      broadcast_observable(N(pixel_data));
    }

    uint64_t TexturePixels::get_data_size() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:GETTER_data_size

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, data_size, uint64_t);
    }
    void TexturePixels::set_data_size(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_data_size

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, data_size, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_data_size
      // LOW_CODEGEN::END::CUSTOM:SETTER_data_size

      broadcast_observable(N(data_size));
    }

    Low::Renderer::TextureState TexturePixels::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, state,
                      Low::Renderer::TextureState);
    }
    void TexturePixels::set_state(Low::Renderer::TextureState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, state, Low::Renderer::TextureState) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Name TexturePixels::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TexturePixels, name, Low::Util::Name);
    }
    void TexturePixels::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TexturePixels, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t TexturePixels::create_instance()
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

    void TexturePixels::increase_budget()
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
                            sizeof(TexturePixelsData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, dimensions) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, dimensions) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::UVector2));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, channels) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, channels) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint8_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, format) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, format) *
                          (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Util::Resource::Image2DFormat));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          TexturePixels i_TexturePixels = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(TexturePixelsData, pixel_data) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<uint8_t>))])
              Low::Util::List<uint8_t>();
          *i_ValPtr =
              ACCESSOR_TYPE_SOA(i_TexturePixels, TexturePixels,
                                pixel_data, Low::Util::List<uint8_t>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, data_size) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, data_size) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, state) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, state) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::TextureState));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TexturePixelsData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TexturePixelsData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for TexturePixels from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
