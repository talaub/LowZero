#include "LowRendererMesh.h"

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

    const uint16_t Mesh::TYPE_ID = 15;
    uint32_t Mesh::ms_Capacity = 0u;
    uint32_t Mesh::ms_PageSize = 0u;
    Low::Util::SharedMutex Mesh::ms_LivingMutex;
    Low::Util::SharedMutex Mesh::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        Mesh::ms_PagesLock(Mesh::ms_PagesMutex, std::defer_lock);
    Low::Util::List<Mesh> Mesh::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *> Mesh::ms_Pages;

    Mesh::Mesh() : Low::Util::Handle(0ull)
    {
    }
    Mesh::Mesh(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    Mesh::Mesh(Mesh &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle Mesh::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    Mesh Mesh::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);

      ACCESSOR_TYPE_SOA(l_Handle, Mesh, name, Low::Util::Name) =
          Low::Util::Name(0u);

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

    void Mesh::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<Mesh> l_Lock(get_id());
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

    void Mesh::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(Mesh));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, Mesh::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(Mesh);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &Mesh::is_alive;
      l_TypeInfo.destroy = &Mesh::destroy;
      l_TypeInfo.serialize = &Mesh::serialize;
      l_TypeInfo.deserialize = &Mesh::deserialize;
      l_TypeInfo.find_by_index = &Mesh::_find_by_index;
      l_TypeInfo.notify = &Mesh::_notify;
      l_TypeInfo.find_by_name = &Mesh::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &Mesh::_make;
      l_TypeInfo.duplicate_default = &Mesh::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &Mesh::living_instances);
      l_TypeInfo.get_living_count = &Mesh::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: vertex_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, vertex_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_vertex_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertex_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_vertex_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_buffer_start
      }
      {
        // Property: vertex_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertex_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, vertex_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_vertex_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            vertex_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertex_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_vertex_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertex_count
      }
      {
        // Property: index_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, index_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_index_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, index_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_index_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_buffer_start
      }
      {
        // Property: index_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(index_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, index_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_index_count();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh,
                                            index_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_index_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_index_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: index_count
      }
      {
        // Property: vertexweight_buffer_start
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertexweight_buffer_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, vertexweight_buffer_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_vertexweight_buffer_start();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertexweight_buffer_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertexweight_buffer_start(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) =
              l_Handle.get_vertexweight_buffer_start();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertexweight_buffer_start
      }
      {
        // Property: vertexweight_count
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(vertexweight_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(Mesh::Data, vertexweight_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_vertexweight_count();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, Mesh, vertexweight_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_vertexweight_count(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_vertexweight_count();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: vertexweight_count
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(Mesh::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Mesh, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          Mesh l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          Mesh l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<Mesh> l_HandleLock(l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void Mesh::cleanup()
    {
      Low::Util::List<Mesh> l_Instances = ms_LivingInstances;
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

    Low::Util::Handle Mesh::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    Mesh Mesh::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

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

    Mesh Mesh::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      Mesh l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = Mesh::TYPE_ID;

      return l_Handle;
    }

    bool Mesh::is_alive() const
    {
      if (m_Data.m_Type != Mesh::TYPE_ID) {
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
      return m_Data.m_Type == Mesh::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t Mesh::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle Mesh::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    Mesh Mesh::find_by_name(Low::Util::Name p_Name)
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

    Mesh Mesh::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      Mesh l_Handle = make(p_Name);
      l_Handle.set_vertex_buffer_start(get_vertex_buffer_start());
      l_Handle.set_vertex_count(get_vertex_count());
      l_Handle.set_index_buffer_start(get_index_buffer_start());
      l_Handle.set_index_count(get_index_count());
      l_Handle.set_vertexweight_buffer_start(
          get_vertexweight_buffer_start());
      l_Handle.set_vertexweight_count(get_vertexweight_count());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    Mesh Mesh::duplicate(Mesh p_Handle, Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle Mesh::_duplicate(Low::Util::Handle p_Handle,
                                       Low::Util::Name p_Name)
    {
      Mesh l_Mesh = p_Handle.get_id();
      return l_Mesh.duplicate(p_Name);
    }

    void Mesh::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["vertex_buffer_start"] = get_vertex_buffer_start();
      p_Node["vertex_count"] = get_vertex_count();
      p_Node["index_buffer_start"] = get_index_buffer_start();
      p_Node["index_count"] = get_index_count();
      p_Node["vertexweight_buffer_start"] =
          get_vertexweight_buffer_start();
      p_Node["vertexweight_count"] = get_vertexweight_count();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void Mesh::serialize(Low::Util::Handle p_Handle,
                         Low::Util::Yaml::Node &p_Node)
    {
      Mesh l_Mesh = p_Handle.get_id();
      l_Mesh.serialize(p_Node);
    }

    Low::Util::Handle Mesh::deserialize(Low::Util::Yaml::Node &p_Node,
                                        Low::Util::Handle p_Creator)
    {
      Mesh l_Handle = Mesh::make(N(Mesh));

      if (p_Node["vertex_buffer_start"]) {
        l_Handle.set_vertex_buffer_start(
            p_Node["vertex_buffer_start"].as<uint32_t>());
      }
      if (p_Node["vertex_count"]) {
        l_Handle.set_vertex_count(
            p_Node["vertex_count"].as<uint32_t>());
      }
      if (p_Node["index_buffer_start"]) {
        l_Handle.set_index_buffer_start(
            p_Node["index_buffer_start"].as<uint32_t>());
      }
      if (p_Node["index_count"]) {
        l_Handle.set_index_count(
            p_Node["index_count"].as<uint32_t>());
      }
      if (p_Node["vertexweight_buffer_start"]) {
        l_Handle.set_vertexweight_buffer_start(
            p_Node["vertexweight_buffer_start"].as<uint32_t>());
      }
      if (p_Node["vertexweight_count"]) {
        l_Handle.set_vertexweight_count(
            p_Node["vertexweight_count"].as<uint32_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void
    Mesh::broadcast_observable(Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 Mesh::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 Mesh::observe(Low::Util::Name p_Observable,
                      Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void Mesh::notify(Low::Util::Handle p_Observed,
                      Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void Mesh::_notify(Low::Util::Handle p_Observer,
                       Low::Util::Handle p_Observed,
                       Low::Util::Name p_Observable)
    {
      Mesh l_Mesh = p_Observer.get_id();
      l_Mesh.notify(p_Observed, p_Observable);
    }

    uint32_t Mesh::get_vertex_buffer_start() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_buffer_start

      return TYPE_SOA(Mesh, vertex_buffer_start, uint32_t);
    }
    void Mesh::set_vertex_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_buffer_start

      // Set new value
      TYPE_SOA(Mesh, vertex_buffer_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_buffer_start

      broadcast_observable(N(vertex_buffer_start));
    }

    uint32_t Mesh::get_vertex_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertex_count

      return TYPE_SOA(Mesh, vertex_count, uint32_t);
    }
    void Mesh::set_vertex_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertex_count

      // Set new value
      TYPE_SOA(Mesh, vertex_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertex_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertex_count

      broadcast_observable(N(vertex_count));
    }

    uint32_t Mesh::get_index_buffer_start() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_index_buffer_start

      return TYPE_SOA(Mesh, index_buffer_start, uint32_t);
    }
    void Mesh::set_index_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_buffer_start

      // Set new value
      TYPE_SOA(Mesh, index_buffer_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_index_buffer_start

      broadcast_observable(N(index_buffer_start));
    }

    uint32_t Mesh::get_index_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_index_count

      return TYPE_SOA(Mesh, index_count, uint32_t);
    }
    void Mesh::set_index_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_index_count

      // Set new value
      TYPE_SOA(Mesh, index_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_index_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_index_count

      broadcast_observable(N(index_count));
    }

    uint32_t Mesh::get_vertexweight_buffer_start() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertexweight_buffer_start

      return TYPE_SOA(Mesh, vertexweight_buffer_start, uint32_t);
    }
    void Mesh::set_vertexweight_buffer_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertexweight_buffer_start

      // Set new value
      TYPE_SOA(Mesh, vertexweight_buffer_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertexweight_buffer_start

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertexweight_buffer_start

      broadcast_observable(N(vertexweight_buffer_start));
    }

    uint32_t Mesh::get_vertexweight_count() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:GETTER_vertexweight_count

      return TYPE_SOA(Mesh, vertexweight_count, uint32_t);
    }
    void Mesh::set_vertexweight_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_vertexweight_count

      // Set new value
      TYPE_SOA(Mesh, vertexweight_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_vertexweight_count

      // LOW_CODEGEN::END::CUSTOM:SETTER_vertexweight_count

      broadcast_observable(N(vertexweight_count));
    }

    Low::Util::Name Mesh::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(Mesh, name, Low::Util::Name);
    }
    void Mesh::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<Mesh> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(Mesh, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t Mesh::create_instance(
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

    u32 Mesh::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for Mesh.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, Mesh::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool Mesh::get_page_for_index(const u32 p_Index, u32 &p_PageIndex,
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
