#include "LowRendererTextureStaging.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererGlobals.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t TextureStaging::TYPE_ID = 75;
    uint32_t TextureStaging::ms_Capacity = 0u;
    uint8_t *TextureStaging::ms_Buffer = 0;
    std::shared_mutex TextureStaging::ms_BufferMutex;
    Low::Util::Instances::Slot *TextureStaging::ms_Slots = 0;
    Low::Util::List<TextureStaging>
        TextureStaging::ms_LivingInstances =
            Low::Util::List<TextureStaging>();

    TextureStaging::TextureStaging() : Low::Util::Handle(0ull)
    {
    }
    TextureStaging::TextureStaging(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    TextureStaging::TextureStaging(TextureStaging &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle TextureStaging::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    TextureStaging TextureStaging::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      TextureStaging l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = TextureStaging::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureStaging, mip0,
                              Low::Renderer::TexturePixels))
          Low::Renderer::TexturePixels();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureStaging, mip1,
                              Low::Renderer::TexturePixels))
          Low::Renderer::TexturePixels();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureStaging, mip2,
                              Low::Renderer::TexturePixels))
          Low::Renderer::TexturePixels();
      new (&ACCESSOR_TYPE_SOA(l_Handle, TextureStaging, mip3,
                              Low::Renderer::TexturePixels))
          Low::Renderer::TexturePixels();
      ACCESSOR_TYPE_SOA(l_Handle, TextureStaging, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void TextureStaging::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // Destroy all pixels still stored in this staging
      for (u8 i = 0u; i < IMAGE_MIPMAP_COUNT; ++i) {
        TexturePixels i_Pixels = get_pixels_for_miplevel(i);
        if (i_Pixels.is_alive()) {
          i_Pixels.destroy();
        }
      }
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

    void TextureStaging::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(TextureStaging));

      initialize_buffer(&ms_Buffer, TextureStagingData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_TextureStaging);
      LOW_PROFILE_ALLOC(type_slots_TextureStaging);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(TextureStaging);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &TextureStaging::is_alive;
      l_TypeInfo.destroy = &TextureStaging::destroy;
      l_TypeInfo.serialize = &TextureStaging::serialize;
      l_TypeInfo.deserialize = &TextureStaging::deserialize;
      l_TypeInfo.find_by_index = &TextureStaging::_find_by_index;
      l_TypeInfo.notify = &TextureStaging::_notify;
      l_TypeInfo.find_by_name = &TextureStaging::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &TextureStaging::_make;
      l_TypeInfo.duplicate_default = &TextureStaging::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &TextureStaging::living_instances);
      l_TypeInfo.get_living_count = &TextureStaging::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: mip0
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mip0);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureStagingData, mip0);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::TexturePixels::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.get_mip0();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureStaging, mip0,
              Low::Renderer::TexturePixels);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.set_mip0(*(Low::Renderer::TexturePixels *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureStaging l_Handle = p_Handle.get_id();
          *((Low::Renderer::TexturePixels *)p_Data) =
              l_Handle.get_mip0();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mip0
      }
      {
        // Property: mip1
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mip1);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureStagingData, mip1);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::TexturePixels::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.get_mip1();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureStaging, mip1,
              Low::Renderer::TexturePixels);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.set_mip1(*(Low::Renderer::TexturePixels *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureStaging l_Handle = p_Handle.get_id();
          *((Low::Renderer::TexturePixels *)p_Data) =
              l_Handle.get_mip1();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mip1
      }
      {
        // Property: mip2
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mip2);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureStagingData, mip2);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::TexturePixels::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.get_mip2();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureStaging, mip2,
              Low::Renderer::TexturePixels);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.set_mip2(*(Low::Renderer::TexturePixels *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureStaging l_Handle = p_Handle.get_id();
          *((Low::Renderer::TexturePixels *)p_Data) =
              l_Handle.get_mip2();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mip2
      }
      {
        // Property: mip3
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mip3);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureStagingData, mip3);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::TexturePixels::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.get_mip3();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureStaging, mip3,
              Low::Renderer::TexturePixels);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.set_mip3(*(Low::Renderer::TexturePixels *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureStaging l_Handle = p_Handle.get_id();
          *((Low::Renderer::TexturePixels *)p_Data) =
              l_Handle.get_mip3();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mip3
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureStagingData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureStaging,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureStaging l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureStaging l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: get_pixels_for_miplevel
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_pixels_for_miplevel);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType =
            Low::Renderer::TexturePixels::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_MipLevel);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_pixels_for_miplevel
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void TextureStaging::cleanup()
    {
      Low::Util::List<TextureStaging> l_Instances =
          ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_TextureStaging);
      LOW_PROFILE_FREE(type_slots_TextureStaging);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle TextureStaging::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    TextureStaging TextureStaging::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      TextureStaging l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = TextureStaging::TYPE_ID;

      return l_Handle;
    }

    TextureStaging TextureStaging::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      TextureStaging l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = TextureStaging::TYPE_ID;

      return l_Handle;
    }

    bool TextureStaging::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == TextureStaging::TYPE_ID &&
             check_alive(ms_Slots, TextureStaging::get_capacity());
    }

    uint32_t TextureStaging::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    TextureStaging::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    TextureStaging
    TextureStaging::find_by_name(Low::Util::Name p_Name)
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

    TextureStaging
    TextureStaging::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      TextureStaging l_Handle = make(p_Name);
      if (get_mip0().is_alive()) {
        l_Handle.set_mip0(get_mip0());
      }
      if (get_mip1().is_alive()) {
        l_Handle.set_mip1(get_mip1());
      }
      if (get_mip2().is_alive()) {
        l_Handle.set_mip2(get_mip2());
      }
      if (get_mip3().is_alive()) {
        l_Handle.set_mip3(get_mip3());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    TextureStaging TextureStaging::duplicate(TextureStaging p_Handle,
                                             Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    TextureStaging::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Name p_Name)
    {
      TextureStaging l_TextureStaging = p_Handle.get_id();
      return l_TextureStaging.duplicate(p_Name);
    }

    void
    TextureStaging::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_mip0().is_alive()) {
        get_mip0().serialize(p_Node["mip0"]);
      }
      if (get_mip1().is_alive()) {
        get_mip1().serialize(p_Node["mip1"]);
      }
      if (get_mip2().is_alive()) {
        get_mip2().serialize(p_Node["mip2"]);
      }
      if (get_mip3().is_alive()) {
        get_mip3().serialize(p_Node["mip3"]);
      }
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void TextureStaging::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
    {
      TextureStaging l_TextureStaging = p_Handle.get_id();
      l_TextureStaging.serialize(p_Node);
    }

    Low::Util::Handle
    TextureStaging::deserialize(Low::Util::Yaml::Node &p_Node,
                                Low::Util::Handle p_Creator)
    {
      TextureStaging l_Handle =
          TextureStaging::make(N(TextureStaging));

      if (p_Node["mip0"]) {
        l_Handle.set_mip0(Low::Renderer::TexturePixels::deserialize(
                              p_Node["mip0"], l_Handle.get_id())
                              .get_id());
      }
      if (p_Node["mip1"]) {
        l_Handle.set_mip1(Low::Renderer::TexturePixels::deserialize(
                              p_Node["mip1"], l_Handle.get_id())
                              .get_id());
      }
      if (p_Node["mip2"]) {
        l_Handle.set_mip2(Low::Renderer::TexturePixels::deserialize(
                              p_Node["mip2"], l_Handle.get_id())
                              .get_id());
      }
      if (p_Node["mip3"]) {
        l_Handle.set_mip3(Low::Renderer::TexturePixels::deserialize(
                              p_Node["mip3"], l_Handle.get_id())
                              .get_id());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void TextureStaging::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 TextureStaging::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 TextureStaging::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void TextureStaging::notify(Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void TextureStaging::_notify(Low::Util::Handle p_Observer,
                                 Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      TextureStaging l_TextureStaging = p_Observer.get_id();
      l_TextureStaging.notify(p_Observed, p_Observable);
    }

    Low::Renderer::TexturePixels TextureStaging::get_mip0() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mip0
      // LOW_CODEGEN::END::CUSTOM:GETTER_mip0

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureStaging, mip0,
                      Low::Renderer::TexturePixels);
    }
    void
    TextureStaging::set_mip0(Low::Renderer::TexturePixels p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mip0
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mip0

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureStaging, mip0, Low::Renderer::TexturePixels) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mip0
      // LOW_CODEGEN::END::CUSTOM:SETTER_mip0

      broadcast_observable(N(mip0));
    }

    Low::Renderer::TexturePixels TextureStaging::get_mip1() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mip1
      // LOW_CODEGEN::END::CUSTOM:GETTER_mip1

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureStaging, mip1,
                      Low::Renderer::TexturePixels);
    }
    void
    TextureStaging::set_mip1(Low::Renderer::TexturePixels p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mip1
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mip1

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureStaging, mip1, Low::Renderer::TexturePixels) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mip1
      // LOW_CODEGEN::END::CUSTOM:SETTER_mip1

      broadcast_observable(N(mip1));
    }

    Low::Renderer::TexturePixels TextureStaging::get_mip2() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mip2
      // LOW_CODEGEN::END::CUSTOM:GETTER_mip2

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureStaging, mip2,
                      Low::Renderer::TexturePixels);
    }
    void
    TextureStaging::set_mip2(Low::Renderer::TexturePixels p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mip2
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mip2

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureStaging, mip2, Low::Renderer::TexturePixels) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mip2
      // LOW_CODEGEN::END::CUSTOM:SETTER_mip2

      broadcast_observable(N(mip2));
    }

    Low::Renderer::TexturePixels TextureStaging::get_mip3() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mip3
      // LOW_CODEGEN::END::CUSTOM:GETTER_mip3

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureStaging, mip3,
                      Low::Renderer::TexturePixels);
    }
    void
    TextureStaging::set_mip3(Low::Renderer::TexturePixels p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mip3
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mip3

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureStaging, mip3, Low::Renderer::TexturePixels) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mip3
      // LOW_CODEGEN::END::CUSTOM:SETTER_mip3

      broadcast_observable(N(mip3));
    }

    Low::Util::Name TextureStaging::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(TextureStaging, name, Low::Util::Name);
    }
    void TextureStaging::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(TextureStaging, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Low::Renderer::TexturePixels
    TextureStaging::get_pixels_for_miplevel(uint8_t p_MipLevel)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_pixels_for_miplevel
      switch (p_MipLevel) {
      case 0:
        return get_mip0();
      case 1:
        return get_mip1();
      case 2:
        return get_mip2();
      case 3:
        return get_mip3();
      default: {
        LOW_LOG_ERROR << "Requested miplevel " << p_MipLevel
                      << " that is not supported by textures."
                      << LOW_LOG_END;
        break;
      }
      }

      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_pixels_for_miplevel
    }

    uint32_t TextureStaging::create_instance()
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

    void TextureStaging::increase_budget()
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
                            sizeof(TextureStagingData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(TextureStagingData, mip0) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureStagingData, mip0) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::TexturePixels));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureStagingData, mip1) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureStagingData, mip1) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::TexturePixels));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureStagingData, mip2) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureStagingData, mip2) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::TexturePixels));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureStagingData, mip3) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureStagingData, mip3) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::TexturePixels));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureStagingData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureStagingData, name) *
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
          << "Auto-increased budget for TextureStaging from "
          << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
          << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
