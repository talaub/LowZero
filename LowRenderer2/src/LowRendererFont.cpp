#include "LowRendererFont.h"

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

    const uint16_t Font::TYPE_ID = 77;
    uint32_t Font::ms_Capacity = 0u;
    uint8_t *Font::ms_Buffer = 0;
    std::shared_mutex Font::ms_BufferMutex;
    Low::Util::Instances::Slot *Font::ms_Slots = 0;
    Low::Util::List<Font> Font::ms_LivingInstances =
        Low::Util::List<Font>();

    Font::Font() : Low::Util::Handle(0ull)
    {
    }
    Font::Font(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Font::Font(Font &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Font::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Font Font::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Font l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Font, texture,
                              Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Font, resource,
                              Low::Renderer::FontResource))
          Low::Renderer::FontResource();
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, Font, glyphs,
          SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>)))
          Low::Util::UnorderedMap<char, Glyph>();
      ACCESSOR_TYPE_SOA(l_Handle, Font, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Font::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const Font *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void Font::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer2), N(Font));

      initialize_buffer(&ms_Buffer, FontData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Font);
      LOW_PROFILE_ALLOC(type_slots_Font);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Font);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Font::is_alive;
      l_TypeInfo.destroy = &Font::destroy;
      l_TypeInfo.serialize = &Font::serialize;
      l_TypeInfo.deserialize = &Font::deserialize;
      l_TypeInfo.find_by_index = &Font::_find_by_index;
      l_TypeInfo.notify = &Font::_notify;
      l_TypeInfo.find_by_name = &Font::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Font::_make;
      l_TypeInfo.duplicate_default = &Font::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Font::living_instances);
      l_TypeInfo.get_living_count = &Font::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: texture
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(texture);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_texture();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, texture,
                                            Low::Renderer::Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_texture(*(Low::Renderer::Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          *((Low::Renderer::Texture *)p_Data) =
              l_Handle.get_texture();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: texture
      }
      {
        // Property: resource
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::FontResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Font, resource, Low::Renderer::FontResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_resource(
              *(Low::Renderer::FontResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          *((Low::Renderer::FontResource *)p_Data) =
              l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: glyphs
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(glyphs);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, glyphs);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_glyphs();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Font, glyphs,
              SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_glyphs(
              *(Low::Util::UnorderedMap<char, Glyph> *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          *((Low::Util::UnorderedMap<char, Glyph> *)p_Data) =
              l_Handle.get_glyphs();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: glyphs
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(FontData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: is_fully_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_fully_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_fully_loaded
      }
      {
        // Function: make_from_resource_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_resource_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Font::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Config);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_from_resource_config
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Font::cleanup()
    {
      Low::Util::List<Font> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Font);
      LOW_PROFILE_FREE(type_slots_Font);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle Font::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Font Font::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      return l_Handle;
    }

    bool Font::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Font::TYPE_ID &&
             check_alive(ms_Slots, Font::get_capacity());
    }

    uint32_t Font::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Font::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Font Font::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    Font Font::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Font l_Handle = make(p_Name);
      if (get_texture().is_alive()) {
        l_Handle.set_texture(get_texture());
      }
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      l_Handle.set_glyphs(get_glyphs());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Font Font::duplicate(Font p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Font::_duplicate(Low::Util::Handle p_Handle,
                                       Low::Util::Name p_Name)
    {
      Font l_Font = p_Handle.get_id();
      return l_Font.duplicate(p_Name);
    }

    void Font::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_texture().is_alive()) {
        get_texture().serialize(p_Node["texture"]);
      }
      if (get_resource().is_alive()) {
        get_resource().serialize(p_Node["resource"]);
      }
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Font::serialize(Low::Util::Handle p_Handle,
                         Low::Util::Yaml::Node &p_Node)
    {
      Font l_Font = p_Handle.get_id();
      l_Font.serialize(p_Node);
    }

    Low::Util::Handle Font::deserialize(Low::Util::Yaml::Node &p_Node,
                                        Low::Util::Handle p_Creator)
    {
      Font l_Handle = Font::make(N(Font));

      if (p_Node["texture"]) {
        l_Handle.set_texture(Low::Renderer::Texture::deserialize(
                                 p_Node["texture"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["resource"]) {
        l_Handle.set_resource(
            Low::Renderer::FontResource::deserialize(
                p_Node["resource"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["glyphs"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    Font::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Font::observe(Low::Util::Name p_Observable,
                      Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Font::notify(Low::Util::Handle p_Observed,
                      Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Font::_notify(Low::Util::Handle p_Observer,
                       Low::Util::Handle p_Observed,
                       Low::Util::Name p_Observable)
    {
      Font l_Font = p_Observer.get_id();
      l_Font.notify(p_Observed, p_Observable);
    }

    Low::Renderer::Texture Font::get_texture() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Font, texture, Low::Renderer::Texture);
    }
    void Font::set_texture(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Font, texture, Low::Renderer::Texture) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture

      broadcast_observable(N(texture));
    }

    Low::Renderer::FontResource Font::get_resource() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Font, resource, Low::Renderer::FontResource);
    }
    void Font::set_resource(Low::Renderer::FontResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Font, resource, Low::Renderer::FontResource) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    Low::Util::UnorderedMap<char, Glyph> &Font::get_glyphs() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:GETTER_glyphs

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(
          Font, glyphs,
          SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>));
    }
    void
    Font::set_glyphs(Low::Util::UnorderedMap<char, Glyph> &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_glyphs

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Font, glyphs,
               SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>)) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:SETTER_glyphs

      broadcast_observable(N(glyphs));
    }

    Low::Util::Name Font::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Font, name, Low::Util::Name);
    }
    void Font::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Font, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool Font::is_fully_loaded()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_fully_loaded
      return get_texture().is_alive() &&
             get_texture().get_state() == TextureState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_fully_loaded
    }

    Font Font::make_from_resource_config(FontResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      Font l_Font = Font::make(p_Config.name);
      l_Font.set_resource(FontResource::make_from_config(p_Config));

      l_Font.set_texture(Texture::make(p_Config.name));
      l_Font.get_texture().set_state(TextureState::UNLOADED);
      l_Font.get_texture().set_resource(
          TextureResource::make(p_Config.fontPath));

      return l_Font;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    uint32_t Font::create_instance()
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

    void Font::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(FontData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(FontData, texture) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, texture) * (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Texture));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(FontData, resource) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(FontData, resource) * (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::FontResource));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontData, glyphs) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, glyphs) * (l_Capacity)],
               l_Capacity *
                   sizeof(Low::Util::UnorderedMap<char, Glyph>));
      }
      {
        memcpy(&l_NewBuffer[offsetof(FontData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(FontData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Font from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
