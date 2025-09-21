#include "LowRendererTexture.h"

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
#include "LowRendererVkImage.h"
#include "LowRendererResourceManager.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Texture::TYPE_ID = 60;
    uint32_t Texture::ms_Capacity = 0u;
    uint32_t Texture::ms_PageSize = 0u;
    Low::Util::SharedMutex Texture::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Texture::ms_PagesLock(Texture::ms_PagesMutex,
                              std::defer_lock);
    Low::Util::List<Texture> Texture::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Texture::ms_Pages;

    Texture::Texture() : Low::Util::Handle(0ull)
    {
    }
    Texture::Texture(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Texture::Texture(Texture &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Texture::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Texture Texture::make(Low::Util::Name p_Name)
    {
      return make(p_Name, 0ull);
    }

    Texture Texture::make(Low::Util::Name p_Name,
                          Low::Util::UniqueId p_UniqueId)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Texture l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Texture::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture, gpu, GpuTexture))
          GpuTexture();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture, resource,
                                 TextureResource)) TextureResource();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture, staging,
                                 TextureStaging)) TextureStaging();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture, state,
                                 Low::Renderer::TextureState))
          Low::Renderer::TextureState();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture, references,
                                 Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Texture, name, Low::Util::Name) =
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
      l_Handle.set_state(TextureState::UNLOADED);

      ResourceManager::register_asset(l_Handle.get_unique_id(),
                                      l_Handle);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Texture::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Texture> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // TODO: unload of loaded
        if (get_gpu().is_alive()) {
          get_gpu().destroy();
        }
        if (get_resource().is_alive()) {
          get_resource().destroy();
        }
        if (get_staging().is_alive()) {
          get_staging().destroy();
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

    void Texture::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Texture));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Texture::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Texture);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Texture::is_alive;
      l_TypeInfo.destroy = &Texture::destroy;
      l_TypeInfo.serialize = &Texture::serialize;
      l_TypeInfo.deserialize = &Texture::deserialize;
      l_TypeInfo.find_by_index = &Texture::_find_by_index;
      l_TypeInfo.notify = &Texture::_notify;
      l_TypeInfo.find_by_name = &Texture::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Texture::_make;
      l_TypeInfo.duplicate_default = &Texture::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Texture::living_instances);
      l_TypeInfo.get_living_count = &Texture::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: gpu
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(gpu);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture::Data, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = GpuTexture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_gpu();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture, gpu,
                                            GpuTexture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture l_Handle = p_Handle.get_id();
          l_Handle.set_gpu(*(GpuTexture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          *((GpuTexture *)p_Data) = l_Handle.get_gpu();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: gpu
      }
      {
        // Property: resource
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(resource);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture::Data, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = TextureResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_resource();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Texture, resource, TextureResource);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture l_Handle = p_Handle.get_id();
          l_Handle.set_resource(*(TextureResource *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          *((TextureResource *)p_Data) = l_Handle.get_resource();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: resource
      }
      {
        // Property: staging
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(staging);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture::Data, staging);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = TextureStaging::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_staging();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture,
                                            staging, TextureStaging);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture l_Handle = p_Handle.get_id();
          l_Handle.set_staging(*(TextureStaging *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          *((TextureStaging *)p_Data) = l_Handle.get_staging();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: staging
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            Low::Renderer::TextureStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Texture, state, Low::Renderer::TextureState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(Low::Renderer::TextureState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          *((Low::Renderer::TextureState *)p_Data) =
              l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: references
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(references);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Texture::Data, references);
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
        l_PropertyInfo.dataOffset =
            offsetof(Texture::Data, unique_id);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_unique_id();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Texture, unique_id, Low::Util::UniqueId);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
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
        l_PropertyInfo.dataOffset = offsetof(Texture::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Function: make_gpu_ready
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_gpu_ready);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Low::Renderer::Texture::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make_gpu_ready
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
      {
        // Function: make_from_resource_config
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make_from_resource_config);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = Texture::TYPE_ID;
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

    void Texture::cleanup()
    {
      Low::Util::List<Texture> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Texture::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Texture Texture::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Texture l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Texture::TYPE_ID;

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

    Texture Texture::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Texture l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Texture::TYPE_ID;

      return l_Handle;
    }

    bool Texture::is_alive() const
    {
      if (m_Data.m_Type != Texture::TYPE_ID) {
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
      return m_Data.m_Type == Texture::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Texture::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Texture::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Texture Texture::find_by_name(Low::Util::Name p_Name)
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

    Texture Texture::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Texture l_Handle = make(p_Name);
      if (get_gpu().is_alive()) {
        l_Handle.set_gpu(get_gpu());
      }
      if (get_resource().is_alive()) {
        l_Handle.set_resource(get_resource());
      }
      if (get_staging().is_alive()) {
        l_Handle.set_staging(get_staging());
      }
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Texture Texture::duplicate(Texture p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Texture::_duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Name p_Name)
    {
      Texture l_Texture = p_Handle.get_id();
      return l_Texture.duplicate(p_Name);
    }

    void Texture::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      if (get_gpu().is_alive()) {
        get_gpu().serialize(p_Node["gpu"]);
      }
      if (get_resource().is_alive()) {
        get_resource().serialize(p_Node["resource"]);
      }
      if (get_staging().is_alive()) {
        get_staging().serialize(p_Node["staging"]);
      }
      Low::Util::Serialization::serialize_enum(
          p_Node["state"],
          Low::Renderer::TextureStateEnumHelper::get_enum_id(),
          static_cast<uint8_t>(get_state()));
      p_Node["_unique_id"] =
          Low::Util::hash_to_string(get_unique_id()).c_str();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Texture::serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node)
    {
      Texture l_Texture = p_Handle.get_id();
      l_Texture.serialize(p_Node);
    }

    Low::Util::Handle
    Texture::deserialize(Low::Util::Yaml::Node &p_Node,
                         Low::Util::Handle p_Creator)
    {
      Low::Util::UniqueId l_HandleUniqueId = 0ull;
      if (p_Node["unique_id"]) {
        l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
      } else if (p_Node["_unique_id"]) {
        l_HandleUniqueId = Low::Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["_unique_id"]));
      }

      Texture l_Handle = Texture::make(N(Texture), l_HandleUniqueId);

      if (p_Node["gpu"]) {
        l_Handle.set_gpu(
            GpuTexture::deserialize(p_Node["gpu"], l_Handle.get_id())
                .get_id());
      }
      if (p_Node["resource"]) {
        l_Handle.set_resource(
            TextureResource::deserialize(p_Node["resource"],
                                         l_Handle.get_id())
                .get_id());
      }
      if (p_Node["staging"]) {
        l_Handle.set_staging(TextureStaging::deserialize(
                                 p_Node["staging"], l_Handle.get_id())
                                 .get_id());
      }
      if (p_Node["state"]) {
        l_Handle.set_state(static_cast<Low::Renderer::TextureState>(
            Low::Util::Serialization::deserialize_enum(
                p_Node["state"])));
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
    Texture::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Texture::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Texture::observe(Low::Util::Name p_Observable,
                         Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Texture::notify(Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Texture::_notify(Low::Util::Handle p_Observer,
                          Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
    {
      Texture l_Texture = p_Observer.get_id();
      l_Texture.notify(p_Observed, p_Observable);
    }

    void Texture::reference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Texture> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Texture, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:NEW_REFERENCE
        if (l_References > 0 &&
            get_state() == TextureState::UNLOADED) {
          ResourceManager::load_texture(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:NEW_REFERENCE
      }
    }

    void Texture::dereference(const u64 p_Id)
    {
      _LOW_ASSERT(is_alive());

      Low::Util::HandleLock<Texture> l_HandleLock(get_id());
      const u32 l_OldReferences =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Texture, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      if (l_OldReferences != l_References) {
        // LOW_CODEGEN:BEGIN:CUSTOM:REFERENCE_REMOVED
        // LOW_CODEGEN::END::CUSTOM:REFERENCE_REMOVED
      }
    }

    u32 Texture::references() const
    {
      return get_references().size();
    }

    GpuTexture Texture::get_gpu() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      return TYPE_SOA(Texture, gpu, GpuTexture);
    }
    void Texture::set_gpu(GpuTexture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      TYPE_SOA(Texture, gpu, GpuTexture) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_gpu
      if (p_Value.is_alive()) {
        p_Value.set_texture_handle(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_gpu

      broadcast_observable(N(gpu));
    }

    TextureResource Texture::get_resource() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      return TYPE_SOA(Texture, resource, TextureResource);
    }
    void Texture::set_resource(TextureResource p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      TYPE_SOA(Texture, resource, TextureResource) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    TextureStaging Texture::get_staging() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging
      // LOW_CODEGEN::END::CUSTOM:GETTER_staging

      return TYPE_SOA(Texture, staging, TextureStaging);
    }
    void Texture::set_staging(TextureStaging p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging

      // Set new value
      TYPE_SOA(Texture, staging, TextureStaging) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging
      // LOW_CODEGEN::END::CUSTOM:SETTER_staging

      broadcast_observable(N(staging));
    }

    Low::Renderer::TextureState Texture::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Texture, state, Low::Renderer::TextureState);
    }
    void Texture::set_state(Low::Renderer::TextureState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Texture, state, Low::Renderer::TextureState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
      if (p_Value == TextureState::LOADED) {
        broadcast_observable(N(TEXTURE_LOADED));
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Set<u64> &Texture::get_references() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      return TYPE_SOA(Texture, references, Low::Util::Set<u64>);
    }

    Low::Util::UniqueId Texture::get_unique_id() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

      return TYPE_SOA(Texture, unique_id, Low::Util::UniqueId);
    }
    void Texture::set_unique_id(Low::Util::UniqueId p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

      // Set new value
      TYPE_SOA(Texture, unique_id, Low::Util::UniqueId) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
      // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

      broadcast_observable(N(unique_id));
    }

    Low::Util::Name Texture::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Texture, name, Low::Util::Name);
    }
    void Texture::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Texture, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Low::Renderer::Texture
    Texture::make_gpu_ready(Low::Util::Name p_Name)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_gpu_ready
      Texture l_Texture = make(p_Name);

      LOW_ASSERT(GpuTexture::living_count() <
                     GpuTexture::get_capacity(),
                 "GpuTexture capacity blown, we cannot set up a new "
                 "gpu ready texture.");

      l_Texture.set_gpu(GpuTexture::make(p_Name));

      l_Texture.set_state(TextureState::LOADED);

      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_gpu_ready
    }

    EditorImage Texture::get_editor_image()
    {
      Low::Util::HandleLock<Texture> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_editor_image
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_editor_image
    }

    Texture Texture::make_from_resource_config(
        TextureResourceConfig &p_Config)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_resource_config
      TextureResource l_Resource =
          TextureResource::make_from_config(p_Config);
      Texture l_Texture =
          Texture::make(p_Config.name, l_Resource.get_texture_id());
      l_Texture.set_resource(l_Resource);

      l_Texture.set_state(TextureState::UNLOADED);

      ResourceManager::register_asset(l_Texture.get_unique_id(),
                                      l_Texture);

      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_resource_config
    }

    uint32_t Texture::create_instance(
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

    u32 Texture::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Texture.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Texture::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Texture::get_page_for_index(const u32 p_Index,
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
