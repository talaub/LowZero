#include "LowRendererTextureResource.h"

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

    const uint16_t TextureResource::TYPE_ID = 72;
    uint32_t TextureResource::ms_Capacity = 0u;
    uint32_t TextureResource::ms_PageSize = 0u;
    Low::Util::SharedMutex TextureResource::ms_LivingMutex;
    Low::Util::SharedMutex TextureResource::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        TextureResource::ms_PagesLock(TextureResource::ms_PagesMutex,
                                      std::defer_lock);
    Low::Util::List<TextureResource>
        TextureResource::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        TextureResource::ms_Pages;

    TextureResource::TextureResource() : Low::Util::Handle(0ull)
    {
    }
    TextureResource::TextureResource(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    TextureResource::TextureResource(TextureResource &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle TextureResource::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    TextureResource TextureResource::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      TextureResource l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = TextureResource::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<TextureResource> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureResource, path,
                                 Util::String)) Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureResource,
                                 texture_path, Util::String))
          Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureResource,
                                 sidecar_path, Util::String))
          Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, TextureResource,
                                 source_file, Util::String))
          Util::String();
      ACCESSOR_TYPE_SOA(l_Handle, TextureResource, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void TextureResource::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<TextureResource> l_Lock(get_id());
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
      Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
      l_LivingLock.unlock();
    }

    void TextureResource::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(TextureResource));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, TextureResource::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(TextureResource);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &TextureResource::is_alive;
      l_TypeInfo.destroy = &TextureResource::destroy;
      l_TypeInfo.serialize = &TextureResource::serialize;
      l_TypeInfo.deserialize = &TextureResource::deserialize;
      l_TypeInfo.find_by_index = &TextureResource::_find_by_index;
      l_TypeInfo.notify = &TextureResource::_notify;
      l_TypeInfo.find_by_name = &TextureResource::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &TextureResource::_make;
      l_TypeInfo.duplicate_default = &TextureResource::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &TextureResource::living_instances);
      l_TypeInfo.get_living_count = &TextureResource::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureResource,
                                            path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: texture_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(texture_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, texture_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_texture_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureResource, texture_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_texture_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: texture_path
      }
      {
        // Property: sidecar_path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(sidecar_path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, sidecar_path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_sidecar_path();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureResource, sidecar_path, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
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
            offsetof(TextureResource::Data, source_file);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_source_file();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, TextureResource, source_file, Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_source_file();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: source_file
      }
      {
        // Property: texture_id
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(texture_id);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, texture_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_texture_id();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureResource,
                                            texture_id, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_texture_id();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: texture_id
      }
      {
        // Property: asset_hash
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(asset_hash);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, asset_hash);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_asset_hash();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureResource,
                                            asset_hash, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(TextureResource::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, TextureResource,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          TextureResource l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          TextureResource l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<TextureResource> l_HandleLock(
              l_Handle);
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
        l_FunctionInfo.handleType = TextureResource::TYPE_ID;
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
        l_FunctionInfo.handleType = TextureResource::TYPE_ID;
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
      {
        // Function: find_by_path
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(find_by_path);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = TextureResource::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Path);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::STRING;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: find_by_path
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void TextureResource::cleanup()
    {
      Low::Util::List<TextureResource> l_Instances =
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
    TextureResource::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    TextureResource TextureResource::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      TextureResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = TextureResource::TYPE_ID;

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

    TextureResource
    TextureResource::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      TextureResource l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = TextureResource::TYPE_ID;

      return l_Handle;
    }

    bool TextureResource::is_alive() const
    {
      if (m_Data.m_Type != TextureResource::TYPE_ID) {
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
      return m_Data.m_Type == TextureResource::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t TextureResource::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    TextureResource::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    TextureResource
    TextureResource::find_by_name(Low::Util::Name p_Name)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:FIND_BY_NAME
      // LOW_CODEGEN::END::CUSTOM:FIND_BY_NAME

      Low::Util::SharedLock<Low::Util::SharedMutex> l_LivingLock(
          ms_LivingMutex);
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return Low::Util::Handle::DEAD;
    }

    TextureResource
    TextureResource::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      TextureResource l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      l_Handle.set_texture_path(get_texture_path());
      l_Handle.set_sidecar_path(get_sidecar_path());
      l_Handle.set_source_file(get_source_file());
      l_Handle.set_texture_id(get_texture_id());
      l_Handle.set_asset_hash(get_asset_hash());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    TextureResource
    TextureResource::duplicate(TextureResource p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    TextureResource::_duplicate(Low::Util::Handle p_Handle,
                                Low::Util::Name p_Name)
    {
      TextureResource l_TextureResource = p_Handle.get_id();
      return l_TextureResource.duplicate(p_Name);
    }

    void
    TextureResource::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void TextureResource::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Yaml::Node &p_Node)
    {
      TextureResource l_TextureResource = p_Handle.get_id();
      l_TextureResource.serialize(p_Node);
    }

    Low::Util::Handle
    TextureResource::deserialize(Low::Util::Yaml::Node &p_Node,
                                 Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return 0;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void TextureResource::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 TextureResource::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 TextureResource::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void TextureResource::notify(Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void TextureResource::_notify(Low::Util::Handle p_Observer,
                                  Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      TextureResource l_TextureResource = p_Observer.get_id();
      l_TextureResource.notify(p_Observed, p_Observable);
    }

    Util::String &TextureResource::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(TextureResource, path, Util::String);
    }
    void TextureResource::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void TextureResource::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(TextureResource, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Util::String &TextureResource::get_texture_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture_path

      return TYPE_SOA(TextureResource, texture_path, Util::String);
    }
    void TextureResource::set_texture_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_texture_path(l_Val);
    }

    void TextureResource::set_texture_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture_path

      // Set new value
      TYPE_SOA(TextureResource, texture_path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture_path

      broadcast_observable(N(texture_path));
    }

    Util::String &TextureResource::get_sidecar_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:GETTER_sidecar_path

      return TYPE_SOA(TextureResource, sidecar_path, Util::String);
    }
    void TextureResource::set_sidecar_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_sidecar_path(l_Val);
    }

    void TextureResource::set_sidecar_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_sidecar_path

      // Set new value
      TYPE_SOA(TextureResource, sidecar_path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_sidecar_path
      // LOW_CODEGEN::END::CUSTOM:SETTER_sidecar_path

      broadcast_observable(N(sidecar_path));
    }

    Util::String &TextureResource::get_source_file() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:GETTER_source_file

      return TYPE_SOA(TextureResource, source_file, Util::String);
    }
    void TextureResource::set_source_file(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_source_file(l_Val);
    }

    void TextureResource::set_source_file(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_source_file

      // Set new value
      TYPE_SOA(TextureResource, source_file, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_source_file
      // LOW_CODEGEN::END::CUSTOM:SETTER_source_file

      broadcast_observable(N(source_file));
    }

    uint64_t TextureResource::get_texture_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture_id

      return TYPE_SOA(TextureResource, texture_id, uint64_t);
    }
    void TextureResource::set_texture_id(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture_id

      // Set new value
      TYPE_SOA(TextureResource, texture_id, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_texture_id

      broadcast_observable(N(texture_id));
    }

    uint64_t TextureResource::get_asset_hash() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:GETTER_asset_hash

      return TYPE_SOA(TextureResource, asset_hash, uint64_t);
    }
    void TextureResource::set_asset_hash(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_asset_hash

      // Set new value
      TYPE_SOA(TextureResource, asset_hash, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_asset_hash
      // LOW_CODEGEN::END::CUSTOM:SETTER_asset_hash

      broadcast_observable(N(asset_hash));
    }

    Low::Util::Name TextureResource::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(TextureResource, name, Low::Util::Name);
    }
    void TextureResource::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<TextureResource> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(TextureResource, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    TextureResource TextureResource::make(Util::String &p_Path)
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
      TextureResource l_TextureResource =
          TextureResource::make(LOW_NAME(l_FileName.c_str()));
      l_TextureResource.set_path(p_Path);
      l_TextureResource.set_texture_path(p_Path);

      return l_TextureResource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    TextureResource
    TextureResource::make_from_config(TextureResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_config
      TextureResource l_Resource =
          TextureResource::make(p_Config.name);
      l_Resource.set_path(p_Config.path);
      l_Resource.set_texture_id(p_Config.textureId);
      l_Resource.set_asset_hash(p_Config.assetHash);
      l_Resource.set_source_file(p_Config.sourceFile);
      l_Resource.set_sidecar_path(p_Config.sidecarPath);
      l_Resource.set_texture_path(p_Config.texturePath);

      return l_Resource;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_config
    }

    TextureResource
    TextureResource::find_by_path(Util::String &p_Path)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_by_path
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_path() == p_Path) {
          return *it;
        }
      }

      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_by_path
    }

    uint32_t TextureResource::create_instance(
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

    u32 TextureResource::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for TextureResource.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, TextureResource::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool TextureResource::get_page_for_index(const u32 p_Index,
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
