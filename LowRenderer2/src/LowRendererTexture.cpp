#include "LowRendererTexture.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
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
    uint8_t *Texture::ms_Buffer = 0;
    std::shared_mutex Texture::ms_BufferMutex;
    Low::Util::Instances::Slot *Texture::ms_Slots = 0;
    Low::Util::List<Texture> Texture::ms_LivingInstances =
        Low::Util::List<Texture>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      Texture l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture, gpu, GpuTexture))
          GpuTexture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture, resource,
                              TextureResource)) TextureResource();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture, staging,
                              TextureStaging)) TextureStaging();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture, state,
                              Low::Renderer::TextureState))
          Low::Renderer::TextureState();
      new (&ACCESSOR_TYPE_SOA(l_Handle, Texture, references,
                              Low::Util::Set<u64>))
          Low::Util::Set<u64>();
      ACCESSOR_TYPE_SOA(l_Handle, Texture, name, Low::Util::Name) =
          Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_state(TextureState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Texture::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

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

    void Texture::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(Texture));

      initialize_buffer(&ms_Buffer, TextureData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_Texture);
      LOW_PROFILE_ALLOC(type_slots_Texture);

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
        l_PropertyInfo.dataOffset = offsetof(TextureData, gpu);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = GpuTexture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(TextureData, resource);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = TextureResource::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(TextureData, staging);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = TextureStaging::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(TextureData, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
        l_PropertyInfo.handleType =
            Low::Renderer::TextureStateEnumHelper::get_enum_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(TextureData, references);
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
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(TextureData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture l_Handle = p_Handle.get_id();
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
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Texture::cleanup()
    {
      Low::Util::List<Texture> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_Texture);
      LOW_PROFILE_FREE(type_slots_Texture);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = Texture::TYPE_ID;

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
      READ_LOCK(l_Lock);
      return m_Data.m_Type == Texture::TYPE_ID &&
             check_alive(ms_Slots, Texture::get_capacity());
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
      Texture l_Handle = Texture::make(N(Texture));

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

      WRITE_LOCK(l_WriteLock);
      const u32 l_OldReferences =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Texture, references, Low::Util::Set<u64>))
          .insert(p_Id);

      const u32 l_References =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();
      LOCK_UNLOCK(l_WriteLock);

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

      WRITE_LOCK(l_WriteLock);
      const u32 l_OldReferences =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();

      (TYPE_SOA(Texture, references, Low::Util::Set<u64>))
          .erase(p_Id);

      const u32 l_References =
          (TYPE_SOA(Texture, references, Low::Util::Set<u64>)).size();
      LOCK_UNLOCK(l_WriteLock);

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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:GETTER_gpu

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, gpu, GpuTexture);
    }
    void Texture::set_gpu(GpuTexture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_gpu
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_gpu

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Texture, gpu, GpuTexture) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource
      // LOW_CODEGEN::END::CUSTOM:GETTER_resource

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, resource, TextureResource);
    }
    void Texture::set_resource(TextureResource p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Texture, resource, TextureResource) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource
      // LOW_CODEGEN::END::CUSTOM:SETTER_resource

      broadcast_observable(N(resource));
    }

    TextureStaging Texture::get_staging() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_staging
      // LOW_CODEGEN::END::CUSTOM:GETTER_staging

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, staging, TextureStaging);
    }
    void Texture::set_staging(TextureStaging p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_staging
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_staging

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Texture, staging, TextureStaging) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_staging
      // LOW_CODEGEN::END::CUSTOM:SETTER_staging

      broadcast_observable(N(staging));
    }

    Low::Renderer::TextureState Texture::get_state() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, state, Low::Renderer::TextureState);
    }
    void Texture::set_state(Low::Renderer::TextureState p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Texture, state, Low::Renderer::TextureState) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

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

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_references
      // LOW_CODEGEN::END::CUSTOM:GETTER_references

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, references, Low::Util::Set<u64>);
    }

    Low::Util::Name Texture::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(Texture, name, Low::Util::Name);
    }
    void Texture::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(Texture, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

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
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_editor_image
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_editor_image
    }

    uint32_t Texture::create_instance()
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

    void Texture::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(TextureData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(TextureData, gpu) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureData, gpu) * (l_Capacity)],
               l_Capacity * sizeof(GpuTexture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureData, resource) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureData, resource) *
                          (l_Capacity)],
               l_Capacity * sizeof(TextureResource));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(TextureData, staging) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(TextureData, staging) * (l_Capacity)],
            l_Capacity * sizeof(TextureStaging));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(TextureData, state) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(TextureData, state) * (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::TextureState));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          Texture i_Texture = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(TextureData, references) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::Set<u64>))])
              Low::Util::Set<u64>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_Texture, Texture, references, Low::Util::Set<u64>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(TextureData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(TextureData, name) * (l_Capacity)],
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

      LOW_LOG_DEBUG << "Auto-increased budget for Texture from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
