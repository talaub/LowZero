#include "LowRendererRenderObject.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererRenderScene.h"
#include "LowRendererDrawCommand.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::RenderObject>
        Low::Renderer::RenderObject::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t RenderObject::TYPE_ID = 52;
    uint32_t RenderObject::ms_Capacity = 0u;
    uint8_t *RenderObject::ms_Buffer = 0;
    std::shared_mutex RenderObject::ms_BufferMutex;
    Low::Util::Instances::Slot *RenderObject::ms_Slots = 0;
    Low::Util::List<RenderObject> RenderObject::ms_LivingInstances =
        Low::Util::List<RenderObject>();

    RenderObject::RenderObject() : Low::Util::Handle(0ull)
    {
    }
    RenderObject::RenderObject(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    RenderObject::RenderObject(RenderObject &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle RenderObject::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    RenderObject RenderObject::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, world_transform,
                              Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, mesh,
                              Low::Renderer::Mesh))
          Low::Renderer::Mesh();
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, uploaded, bool) =
          false;
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, material,
                              Low::Renderer::Material))
          Low::Renderer::Material();
      new (&ACCESSOR_TYPE_SOA(l_Handle, RenderObject, draw_commands,
                              Low::Util::List<DrawCommand>))
          Low::Util::List<DrawCommand>();
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, dirty, bool) = false;
      ACCESSOR_TYPE_SOA(l_Handle, RenderObject, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_dirty(true);
      l_Handle.set_uploaded(false);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void RenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      if (get_mesh().is_alive()) {
        get_mesh().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const RenderObject *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void RenderObject::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(RenderObject));

      initialize_buffer(&ms_Buffer, RenderObjectData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_RenderObject);
      LOW_PROFILE_ALLOC(type_slots_RenderObject);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(RenderObject);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &RenderObject::is_alive;
      l_TypeInfo.destroy = &RenderObject::destroy;
      l_TypeInfo.serialize = &RenderObject::serialize;
      l_TypeInfo.deserialize = &RenderObject::deserialize;
      l_TypeInfo.find_by_index = &RenderObject::_find_by_index;
      l_TypeInfo.notify = &RenderObject::_notify;
      l_TypeInfo.find_by_name = &RenderObject::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &RenderObject::_make;
      l_TypeInfo.duplicate_default = &RenderObject::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &RenderObject::living_instances);
      l_TypeInfo.get_living_count = &RenderObject::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObjectData, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            world_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: mesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Mesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_mesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, mesh, Low::Renderer::Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((Low::Renderer::Mesh *)p_Data) = l_Handle.get_mesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObjectData, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: slot
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(slot);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObjectData, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_render_scene_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene_handle
      }
      {
        // Property: material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObjectData, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            material,
                                            Low::Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Low::Renderer::Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((Low::Renderer::Material *)p_Data) =
              l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(RenderObjectData, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, RenderObject, draw_commands,
              Low::Util::List<DrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((Low::Util::List<DrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(RenderObjectData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, RenderObject,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          RenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          RenderObject l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = RenderObject::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderScene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Mesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Low::Renderer::Mesh::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void RenderObject::cleanup()
    {
      Low::Util::List<RenderObject> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_RenderObject);
      LOW_PROFILE_FREE(type_slots_RenderObject);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle RenderObject::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    RenderObject RenderObject::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      RenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = RenderObject::TYPE_ID;

      return l_Handle;
    }

    bool RenderObject::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == RenderObject::TYPE_ID &&
             check_alive(ms_Slots, RenderObject::get_capacity());
    }

    uint32_t RenderObject::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    RenderObject::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    RenderObject RenderObject::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    RenderObject RenderObject::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      RenderObject l_Handle = make(p_Name);
      l_Handle.set_world_transform(get_world_transform());
      if (get_mesh().is_alive()) {
        l_Handle.set_mesh(get_mesh());
      }
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_slot(get_slot());
      l_Handle.set_render_scene_handle(get_render_scene_handle());
      if (get_material().is_alive()) {
        l_Handle.set_material(get_material());
      }
      l_Handle.set_dirty(is_dirty());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    RenderObject RenderObject::duplicate(RenderObject p_Handle,
                                         Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    RenderObject::_duplicate(Low::Util::Handle p_Handle,
                             Low::Util::Name p_Name)
    {
      RenderObject l_RenderObject = p_Handle.get_id();
      return l_RenderObject.duplicate(p_Name);
    }

    void RenderObject::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void RenderObject::serialize(Low::Util::Handle p_Handle,
                                 Low::Util::Yaml::Node &p_Node)
    {
      RenderObject l_RenderObject = p_Handle.get_id();
      l_RenderObject.serialize(p_Node);
    }

    Low::Util::Handle
    RenderObject::deserialize(Low::Util::Yaml::Node &p_Node,
                              Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void RenderObject::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 RenderObject::observe(Low::Util::Name p_Observable,
                              Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void RenderObject::notify(Low::Util::Handle p_Observed,
                              Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void RenderObject::_notify(Low::Util::Handle p_Observer,
                               Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      RenderObject l_RenderObject = p_Observer.get_id();
      l_RenderObject.notify(p_Observed, p_Observable);
    }

    Low::Math::Matrix4x4 &RenderObject::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, world_transform,
                      Low::Math::Matrix4x4);
    }
    void
    RenderObject::set_world_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      if (get_world_transform() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderObject, world_transform,
                 Low::Math::Matrix4x4) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform

        broadcast_observable(N(world_transform));
      }
    }

    Low::Renderer::Mesh RenderObject::get_mesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, mesh, Low::Renderer::Mesh);
    }
    void RenderObject::set_mesh(Low::Renderer::Mesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh
      if (get_mesh().is_alive()) {
        get_mesh().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, mesh, Low::Renderer::Mesh) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
      if (p_Value.is_alive()) {
        p_Value.reference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh

      broadcast_observable(N(mesh));
    }

    bool RenderObject::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, uploaded, bool);
    }
    void RenderObject::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void RenderObject::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, uploaded, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    uint32_t RenderObject::get_slot() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, slot, uint32_t);
    }
    void RenderObject::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, slot, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot

      broadcast_observable(N(slot));
    }

    uint64_t RenderObject::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, render_scene_handle, uint64_t);
    }
    void RenderObject::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, render_scene_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle

      broadcast_observable(N(render_scene_handle));
    }

    Low::Renderer::Material RenderObject::get_material() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, material,
                      Low::Renderer::Material);
    }
    void RenderObject::set_material(Low::Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderObject, material, Low::Renderer::Material) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    Low::Util::List<DrawCommand> &
    RenderObject::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, draw_commands,
                      Low::Util::List<DrawCommand>);
    }

    bool RenderObject::is_dirty() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, dirty, bool);
    }
    void RenderObject::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void RenderObject::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, dirty, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
      if (p_Value) {
        ms_Dirty.insert(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void RenderObject::mark_dirty()
    {
      if (!is_dirty()) {
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(RenderObject, dirty, bool) = true;
        LOCK_UNLOCK(l_WriteLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    Low::Util::Name RenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(RenderObject, name, Low::Util::Name);
    }
    void RenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(RenderObject, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    RenderObject RenderObject::make(RenderScene p_RenderScene,
                                    Low::Renderer::Mesh p_Mesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      _LOW_ASSERT(p_RenderScene.is_alive());

      RenderObject l_Handle = make(N(RenderObject));
      l_Handle.set_render_scene_handle(p_RenderScene.get_id());
      l_Handle.set_mesh(p_Mesh);

      LOW_ASSERT(
          p_Mesh.is_alive(),
          "Cannot initialize render object without valid mesh");

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t RenderObject::create_instance()
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

    void RenderObject::increase_budget()
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
                            sizeof(RenderObjectData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(RenderObjectData, world_transform) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(RenderObjectData, world_transform) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Matrix4x4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, mesh) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, mesh) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Mesh));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, uploaded) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, uploaded) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, slot) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, slot) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData,
                                     render_scene_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData,
                                   render_scene_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, material) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, material) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Material));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          RenderObject i_RenderObject = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(RenderObjectData, draw_commands) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<DrawCommand>))])
              Low::Util::List<DrawCommand>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(i_RenderObject, RenderObject,
                                        draw_commands,
                                        Low::Util::List<DrawCommand>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, dirty) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, dirty) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(RenderObjectData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(RenderObjectData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for RenderObject from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
