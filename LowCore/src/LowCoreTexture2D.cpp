#include "LowCoreTexture2D.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowUtilResource.h"
#include "LowUtilJobManager.h"
#include "LowRenderer.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

#define TEXTURE_COUNT 50
    Util::List<bool> g_TextureSlots;
    Util::List<Util::Resource::Image2D> g_Image2Ds;

    struct TextureLoadSchedule
    {
      uint32_t textureIndex;
      Util::Future<void> future;
      Texture2D textureResource;

      TextureLoadSchedule(uint64_t p_Id, Util::Future<void> p_Future,
                          Texture2D p_TextureResource)
          : textureIndex(p_Id), future(std::move(p_Future)),
            textureResource(p_TextureResource)
      {
      }
    };

    Util::List<TextureLoadSchedule> g_TextureLoadSchedules;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t Texture2D::TYPE_ID = 22;
    uint32_t Texture2D::ms_Capacity = 0u;
    uint32_t Texture2D::ms_PageSize = 0u;
    Low::Util::SharedMutex Texture2D::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Texture2D::ms_PagesLock(Texture2D::ms_PagesMutex,
                                std::defer_lock);
    Low::Util::List<Texture2D> Texture2D::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Texture2D::ms_Pages;

    Texture2D::Texture2D() : Low::Util::Handle(0ull)
    {
    }
    Texture2D::Texture2D(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Texture2D::Texture2D(Texture2D &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Texture2D::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Texture2D Texture2D::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture2D, path,
                                 Util::String)) Util::String();
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, Texture2D, renderer_texture, Renderer::Texture2D))
          Renderer::Texture2D();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Texture2D, state,
                                 ResourceState)) ResourceState();
      ACCESSOR_TYPE_SOA(l_Handle, Texture2D, name, Low::Util::Name) =
          Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void Texture2D::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Texture2D> l_Lock(get_id());
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

    void Texture2D::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      g_Image2Ds.resize(TEXTURE_COUNT);
      g_TextureSlots.resize(TEXTURE_COUNT);
      for (uint32_t i = 0; i < TEXTURE_COUNT; ++i) {
        g_TextureSlots[i] = false;
      }
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowCore), N(Texture2D));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Texture2D::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Texture2D);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Texture2D::is_alive;
      l_TypeInfo.destroy = &Texture2D::destroy;
      l_TypeInfo.serialize = &Texture2D::serialize;
      l_TypeInfo.deserialize = &Texture2D::deserialize;
      l_TypeInfo.find_by_index = &Texture2D::_find_by_index;
      l_TypeInfo.notify = &Texture2D::_notify;
      l_TypeInfo.find_by_name = &Texture2D::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Texture2D::_make;
      l_TypeInfo.duplicate_default = &Texture2D::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Texture2D::living_instances);
      l_TypeInfo.get_living_count = &Texture2D::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: path
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(path);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2D::Data, path);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          l_Handle.get_path();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, path,
                                            Util::String);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          *((Util::String *)p_Data) = l_Handle.get_path();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: path
      }
      {
        // Property: renderer_texture
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(renderer_texture);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Texture2D::Data, renderer_texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Renderer::Texture2D::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          l_Handle.get_renderer_texture();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D,
                                            renderer_texture,
                                            Renderer::Texture2D);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          *((Renderer::Texture2D *)p_Data) =
              l_Handle.get_renderer_texture();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: renderer_texture
      }
      {
        // Property: reference_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(reference_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Texture2D::Data, reference_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
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
        // End property: reference_count
      }
      {
        // Property: state
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(state);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2D::Data, state);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          l_Handle.get_state();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D,
                                            state, ResourceState);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.set_state(*(ResourceState *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          *((ResourceState *)p_Data) = l_Handle.get_state();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: state
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Texture2D::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Texture2D, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Texture2D l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Texture2D l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Texture2D> l_HandleLock(l_Handle);
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
        l_FunctionInfo.handleType = Texture2D::TYPE_ID;
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
        // Function: is_loaded
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(is_loaded);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: is_loaded
      }
      {
        // Function: load
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(load);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: load
      }
      {
        // Function: unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: unload
      }
      {
        // Function: _unload
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(_unload);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: _unload
      }
      {
        // Function: update
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(update);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: update
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Texture2D::cleanup()
    {
      Low::Util::List<Texture2D> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Texture2D::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Texture2D Texture2D::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

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

    Texture2D Texture2D::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Texture2D l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Texture2D::TYPE_ID;

      return l_Handle;
    }

    bool Texture2D::is_alive() const
    {
      if (m_Data.m_Type != Texture2D::TYPE_ID) {
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
      return m_Data.m_Type == Texture2D::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Texture2D::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Texture2D::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Texture2D Texture2D::find_by_name(Low::Util::Name p_Name)
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

    Texture2D Texture2D::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Texture2D l_Handle = make(p_Name);
      l_Handle.set_path(get_path());
      if (get_renderer_texture().is_alive()) {
        l_Handle.set_renderer_texture(get_renderer_texture());
      }
      l_Handle.set_reference_count(get_reference_count());
      l_Handle.set_state(get_state());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Texture2D Texture2D::duplicate(Texture2D p_Handle,
                                   Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    Texture2D::_duplicate(Low::Util::Handle p_Handle,
                          Low::Util::Name p_Name)
    {
      Texture2D l_Texture2D = p_Handle.get_id();
      return l_Texture2D.duplicate(p_Name);
    }

    void Texture2D::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      p_Node = get_path().c_str();
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Texture2D::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node)
    {
      Texture2D l_Texture2D = p_Handle.get_id();
      l_Texture2D.serialize(p_Node);
    }

    Low::Util::Handle
    Texture2D::deserialize(Low::Util::Yaml::Node &p_Node,
                           Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      Texture2D l_Texture =
          Texture2D::make(LOW_YAML_AS_STRING(p_Node));
      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void Texture2D::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Texture2D::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Texture2D::observe(Low::Util::Name p_Observable,
                           Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Texture2D::notify(Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Texture2D::_notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
    {
      Texture2D l_Texture2D = p_Observer.get_id();
      l_Texture2D.notify(p_Observed, p_Observable);
    }

    Util::String &Texture2D::get_path() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path

      // LOW_CODEGEN::END::CUSTOM:GETTER_path

      return TYPE_SOA(Texture2D, path, Util::String);
    }
    void Texture2D::set_path(const char *p_Value)
    {
      Low::Util::String l_Val(p_Value);
      set_path(l_Val);
    }

    void Texture2D::set_path(Util::String &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

      // Set new value
      TYPE_SOA(Texture2D, path, Util::String) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path

      // LOW_CODEGEN::END::CUSTOM:SETTER_path

      broadcast_observable(N(path));
    }

    Renderer::Texture2D Texture2D::get_renderer_texture() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_renderer_texture

      // LOW_CODEGEN::END::CUSTOM:GETTER_renderer_texture

      return TYPE_SOA(Texture2D, renderer_texture,
                      Renderer::Texture2D);
    }
    void Texture2D::set_renderer_texture(Renderer::Texture2D p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_renderer_texture

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_renderer_texture

      // Set new value
      TYPE_SOA(Texture2D, renderer_texture, Renderer::Texture2D) =
          p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_renderer_texture

      // LOW_CODEGEN::END::CUSTOM:SETTER_renderer_texture

      broadcast_observable(N(renderer_texture));
    }

    uint32_t Texture2D::get_reference_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_reference_count

      return TYPE_SOA(Texture2D, reference_count, uint32_t);
    }
    void Texture2D::set_reference_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_reference_count

      // Set new value
      TYPE_SOA(Texture2D, reference_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_reference_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_reference_count

      broadcast_observable(N(reference_count));
    }

    ResourceState Texture2D::get_state() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state

      // LOW_CODEGEN::END::CUSTOM:GETTER_state

      return TYPE_SOA(Texture2D, state, ResourceState);
    }
    void Texture2D::set_state(ResourceState p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

      // Set new value
      TYPE_SOA(Texture2D, state, ResourceState) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state

      // LOW_CODEGEN::END::CUSTOM:SETTER_state

      broadcast_observable(N(state));
    }

    Low::Util::Name Texture2D::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Texture2D, name, Low::Util::Name);
    }
    void Texture2D::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Texture2D, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    Texture2D Texture2D::make(Util::String &p_Path)
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
      Texture2D l_Texture =
          Texture2D::make(LOW_NAME(l_FileName.c_str()));
      l_Texture.set_path(p_Path);

      l_Texture.set_reference_count(0);

      return l_Texture;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    bool Texture2D::is_loaded()
    {
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_is_loaded

      return get_state() == ResourceState::LOADED;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_is_loaded
    }

    void Texture2D::load()
    {
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load

      LOW_ASSERT(is_alive(), "Texture2D was not alive on load");

      set_reference_count(get_reference_count() + 1);

      LOW_ASSERT(get_reference_count() > 0,
                 "Increased Texture2D reference count, but its "
                 "not over 0. "
                 "Something went wrong.");

      if (get_state() != ResourceState::UNLOADED) {
        return;
      }

      set_state(ResourceState::STREAMING);

      Util::String l_P = get_path();
      uint32_t l_TextureIndex = 0;
      bool l_FoundIndex = false;

      do {
        for (uint32_t i = 0u; i < TEXTURE_COUNT; ++i) {
          if (!g_TextureSlots[i]) {
            l_TextureIndex = i;
            l_FoundIndex = true;
            g_TextureSlots[i] = true;
            break;
          }
        }
      } while (!l_FoundIndex);

      Util::String l_FullPath = Util::get_project().dataPath +
                                "\\resources\\img2d\\" + get_path();

      set_renderer_texture(Renderer::reserve_texture(get_name()));

      u64 l_HandleId = get_id();

      TextureLoadSchedule &l_LoadSchedule =
          g_TextureLoadSchedules.emplace_back(
              l_TextureIndex,
              Util::JobManager::default_pool().enqueue(
                  [l_FullPath, l_TextureIndex, l_HandleId]() {
                    for (auto it = g_TextureLoadSchedules.begin();
                         it != g_TextureLoadSchedules.end(); ++it) {
                      if (it->textureResource.get_id() ==
                          l_HandleId) {
                        Util::Resource::load_image2d(
                            Util::String(l_FullPath.c_str()),
                            g_Image2Ds[it->textureIndex]);
                        break;
                      }
                    }
                  }),
              *this);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_load
    }

    void Texture2D::unload()
    {
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload

      set_reference_count(get_reference_count() - 1);

      LOW_ASSERT(get_reference_count() >= 0,
                 "Texture2D reference count < 0. Something "
                 "went wrong.");

      if (get_reference_count() <= 0) {
        _unload();
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload
    }

    void Texture2D::_unload()
    {
      Low::Util::HandleLock<Texture2D> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION__unload

      if (!is_loaded()) {
        return;
      }

      get_renderer_texture().destroy();

      set_state(ResourceState::UNLOADED);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION__unload
    }

    void Texture2D::update()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_update

      LOW_PROFILE_CPU("Core", "Update Texture2D");
      for (auto it = g_TextureLoadSchedules.begin();
           it != g_TextureLoadSchedules.end();) {
        std::future_status i_Status =
            it->future.wait_for(std::chrono::seconds(0));

        Texture2D i_Texture = it->textureResource;
        if (i_Status == std::future_status::ready) {
          Renderer::upload_texture(i_Texture.get_renderer_texture(),
                                   g_Image2Ds[it->textureIndex]);

          g_TextureSlots[it->textureIndex] = false;

          i_Texture.set_state(ResourceState::LOADED);

          it = g_TextureLoadSchedules.erase(it);
        } else {
          ++it;
        }
      }
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_update
    }

    uint32_t Texture2D::create_instance(
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

    u32 Texture2D::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Texture2D.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Texture2D::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Texture2D::get_page_for_index(const u32 p_Index,
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

  } // namespace Core
} // namespace Low
