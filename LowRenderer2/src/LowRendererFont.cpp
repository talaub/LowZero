#include "LowRendererFont.h"

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
#include "LowRendererResourceManager.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Font::TYPE_ID = 77;
    uint32_t Font::ms_Capacity = 0u;
    uint32_t Font::ms_PageSize = 0u;
    Low::Util::SharedMutex Font::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Font::ms_PagesLock(Font::ms_PagesMutex, std::defer_lock);
    Low::Util::List<Font> Font::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Font::ms_Pages;

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
      return make(p_Name, 0ull);
    }

    Font Font::make(Low::Util::Name p_Name,
                    Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Font l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Font> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Font, texture,
                                 Low::Renderer::Texture))
          Low::Renderer::Texture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Font, resource,
                                 Low::Renderer::FontResource))
          Low::Renderer::FontResource();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Font, glyphs,
          SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>)))
          Low::Util::UnorderedMap<char, Glyph>();
      ACCESSOR_TYPE_SOA(l_Handle, Font, sidecar_loaded, bool) = false;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Font, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Font, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      if (p_UniqueId > 0ull) {
        l_Handle.set_unique_id(p_UniqueId);
      } else {
        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
      }
      Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                    l_Handle.get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_sidecar_loaded(false);

      ResourceManager::register_asset(l_Handle.get_unique_id(),
                                      l_Handle);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Font::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Font> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        if (get_texture().is_alive()) {
          get_texture().dereference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      Low::Util::remove_unique_id(get_unique_id());

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

    void Font::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer2), N(Font));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Font::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

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
        l_PropertyInfo.dataOffset = offsetof(Font::Data, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(Font::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::FontResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(Font::Data, glyphs);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((Low::Util::UnorderedMap<char, Glyph> *)p_Data) =
              l_Handle.get_glyphs();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: glyphs
      }
      {
        // Property: sidecar_loaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(sidecar_loaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Font::Data, sidecar_loaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.is_sidecar_loaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font,
                                            sidecar_loaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Font l_Handle = p_Handle.get_id();
          l_Handle.set_sidecar_loaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((bool *)p_Data) = l_Handle.is_sidecar_loaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: sidecar_loaded
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, references);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          return nullptr;
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {};
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: references
      }
      {
        // Property: unique_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(unique_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Font, unique_id,
                                            Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
          *((Low::Util::UniqueId *)p_Data) = l_Handle.get_unique_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: unique_id
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Font::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Font l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
          Low::Util::HandleLock<Font> l_HandleLock(l_Handle);
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
      {
        // Function: get_editor_image
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_editor_image);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = EditorImage::TYPE_ID;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_editor_image
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Font::cleanup()
    {
      Low::Util::List<Font> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Font::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Font Font::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

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

    Font Font::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Font l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Font::TYPE_ID;

      return l_Handle;
    }

    bool Font::is_alive() const
    {
      if (m_Data.m_Type != Font::TYPE_ID) {
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
      return m_Data.m_Type == Font::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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
      l_Handle.set_sidecar_loaded(is_sidecar_loaded());

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
      p_Node["sidecar_loaded"] = is_sidecar_loaded();
      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
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
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      Font l_Handle = Font::make(N(Font), l_HandleUniqueId);

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
      if (p_Node["sidecar_loaded"]) {
        l_Handle.set_sidecar_loaded(
            p_Node["sidecar_loaded"].as<bool>());
      }
      if (p_Node["references"]) {
      }
      if (p_Node["unique_id"]) {
        l_Handle.set_unique_id(
            p_Node["unique_id"].as<Low::Util::UniqueId>());
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

    u64 Font::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
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

    void Font::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Font> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Font, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Font, references, Low::Util::Set<u64>)).insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Font, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        if (l_References > 0) {
          ResourceManager::load_font(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void Font::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Font> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Font, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Font, references, Low::Util::Set<u64>)).erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Font, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 Font::references() const
    {
      return get_references().size();
    }

    Low::Renderer::Texture Font::get_texture() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      return TYPE_SOA(Font, texture, Low::Renderer::Texture);
    }
    void Font::set_texture(Low::Renderer::Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      if (get_texture().is_alive()) {
        get_texture().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      // Set new value
      TYPE_SOA(Font, texture, Low::Renderer::Texture) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
      if (p_Value.is_alive()) {
        p_Value.reference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture

      broadcast_observable(N(texture));
    }

    Low::Renderer::FontResource Font::get_resource() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(Font, resource, Low::Renderer::FontResource);
    }
    void Font::set_resource(Low::Renderer::FontResource p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(Font, resource, Low::Renderer::FontResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    Low::Util::UnorderedMap<char, Glyph> &Font::get_glyphs() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:GETTER_glyphs

      return TYPE_SOA(
          Font, glyphs,
          SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>));
    }
    void
    Font::set_glyphs(Low::Util::UnorderedMap<char, Glyph> &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_glyphs

      // Set new value
      TYPE_SOA(Font, glyphs,
               SINGLE_ARG(Low::Util::UnorderedMap<char, Glyph>)) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_glyphs
      // LOW_CODEGEN::END::CUSTOM:SETTER_glyphs

      broadcast_observable(N(glyphs));
    }

    bool Font::is_sidecar_loaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_sidecar_loaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_sidecar_loaded

      return TYPE_SOA(Font, sidecar_loaded, bool);
    }
    void Font::toggle_sidecar_loaded()
    {
      set_sidecar_loaded(!is_sidecar_loaded());
    }

    void Font::set_sidecar_loaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_sidecar_loaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_sidecar_loaded

      // Set new value
      TYPE_SOA(Font, sidecar_loaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_sidecar_loaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_sidecar_loaded

      broadcast_observable(N(sidecar_loaded));
    }

    Low::Util::Set<u64> &Font::get_references() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(Font, references, Low::Util::Set<u64>);
    }

    Low::Util::UniqueId Font::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Font, unique_id, Low::Util::UniqueId);
    }
    void Font::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Font, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Font::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Font, name, Low::Util::Name);
    }
    void Font::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Font> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Font, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    bool Font::is_fully_loaded()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_fully_loaded
      return get_texture().is_alive() &&
             get_texture().get_state() == TextureState::LOADED &&
             is_sidecar_loaded();
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_fully_loaded
    }

    Font Font::make_from_resource_config(FontResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      FontResource l_Resource =
          FontResource::make_from_config(p_Config);

      Font l_Font = Font::make(p_Config.name);
      l_Font.set_resource(l_Resource);

      l_Font.set_texture(Texture::make(p_Config.name));
      l_Font.get_texture().set_state(TextureState::UNLOADED);
      l_Font.get_texture().set_resource(
          TextureResource::make(p_Config.fontPath));

      ResourceManager::register_asset(l_Font.get_unique_id(), l_Font);
      ResourceManager::register_asset(
          l_Font.get_texture().get_unique_id(), l_Font.get_texture());

      return l_Font;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    EditorImage Font::get_editor_image()
    {
      Low::Util::HandleLock<Font> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_editor_image
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_editor_image
    }

    uint32_t Font::create_instance(
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

    u32 Font::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Font.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Font::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Font::get_page_for_index(const u32 p_Index, u32 &p_PageIndex,
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
