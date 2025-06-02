#include "LowRendererDrawCommand.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererRenderScene.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::DrawCommand>
        Low::Renderer::DrawCommand::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t DrawCommand::TYPE_ID = 62;
    uint32_t DrawCommand::ms_Capacity = 0u;
    uint8_t *DrawCommand::ms_Buffer = 0;
    std::shared_mutex DrawCommand::ms_BufferMutex;
    Low::Util::Instances::Slot *DrawCommand::ms_Slots = 0;
    Low::Util::List<DrawCommand> DrawCommand::ms_LivingInstances =
        Low::Util::List<DrawCommand>();

    DrawCommand::DrawCommand() : Low::Util::Handle(0ull)
    {
    }
    DrawCommand::DrawCommand(uint64_t p_Id) : Low::Util::Handle(p_Id)
    {
    }
    DrawCommand::DrawCommand(DrawCommand &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle DrawCommand::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    DrawCommand DrawCommand::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      DrawCommand l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = DrawCommand::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, world_transform,
                              Low::Math::Matrix4x4))
          Low::Math::Matrix4x4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, mesh_info,
                              Low::Renderer::MeshInfo))
          Low::Renderer::MeshInfo();
      new (&ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, render_object,
                              Low::Renderer::RenderObject))
          Low::Renderer::RenderObject();
      new (&ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, material,
                              Low::Renderer::Material))
          Low::Renderer::Material();
      ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, uploaded, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, DrawCommand, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_uploaded(false);
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void DrawCommand::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const DrawCommand *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void DrawCommand::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(DrawCommand));

      initialize_buffer(&ms_Buffer, DrawCommandData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_DrawCommand);
      LOW_PROFILE_ALLOC(type_slots_DrawCommand);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(DrawCommand);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &DrawCommand::is_alive;
      l_TypeInfo.destroy = &DrawCommand::destroy;
      l_TypeInfo.serialize = &DrawCommand::serialize;
      l_TypeInfo.deserialize = &DrawCommand::deserialize;
      l_TypeInfo.find_by_index = &DrawCommand::_find_by_index;
      l_TypeInfo.find_by_name = &DrawCommand::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &DrawCommand::_make;
      l_TypeInfo.duplicate_default = &DrawCommand::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &DrawCommand::living_instances);
      l_TypeInfo.get_living_count = &DrawCommand::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: world_transform
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(world_transform);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, world_transform);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_world_transform();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            world_transform,
                                            Low::Math::Matrix4x4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_world_transform(
              *(Low::Math::Matrix4x4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((Low::Math::Matrix4x4 *)p_Data) =
              l_Handle.get_world_transform();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: world_transform
      }
      {
        // Property: mesh_info
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh_info);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, mesh_info);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::MeshInfo::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_mesh_info();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            mesh_info,
                                            Low::Renderer::MeshInfo);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((Low::Renderer::MeshInfo *)p_Data) =
              l_Handle.get_mesh_info();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh_info
      }
      {
        // Property: slot
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(slot);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(DrawCommandData, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: render_object
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_object);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, render_object);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::RenderObject::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_render_object();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, DrawCommand, render_object,
              Low::Renderer::RenderObject);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((Low::Renderer::RenderObject *)p_Data) =
              l_Handle.get_render_object();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_object
      }
      {
        // Property: material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            material,
                                            Low::Renderer::Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Low::Renderer::Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((Low::Renderer::Material *)p_Data) =
              l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: render_scene_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_scene_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(DrawCommandData, render_scene_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_render_scene_handle();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, DrawCommand, render_scene_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_render_scene_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_render_scene_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_scene_handle
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(DrawCommandData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, DrawCommand,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          DrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          DrawCommand l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = DrawCommand::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderObject);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderObject::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderScene);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::RenderScene::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_MeshInfo);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::MeshInfo::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: get_sort_index
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_sort_index);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_sort_index
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void DrawCommand::cleanup()
    {
      Low::Util::List<DrawCommand> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_DrawCommand);
      LOW_PROFILE_FREE(type_slots_DrawCommand);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle DrawCommand::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    DrawCommand DrawCommand::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      DrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = DrawCommand::TYPE_ID;

      return l_Handle;
    }

    bool DrawCommand::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == DrawCommand::TYPE_ID &&
             check_alive(ms_Slots, DrawCommand::get_capacity());
    }

    uint32_t DrawCommand::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    DrawCommand::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    DrawCommand DrawCommand::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    DrawCommand DrawCommand::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      DrawCommand l_Handle = make(p_Name);
      l_Handle.set_world_transform(get_world_transform());
      if (get_mesh_info().is_alive()) {
        l_Handle.set_mesh_info(get_mesh_info());
      }
      l_Handle.set_slot(get_slot());
      if (get_render_object().is_alive()) {
        l_Handle.set_render_object(get_render_object());
      }
      if (get_material().is_alive()) {
        l_Handle.set_material(get_material());
      }
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_render_scene_handle(get_render_scene_handle());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    DrawCommand DrawCommand::duplicate(DrawCommand p_Handle,
                                       Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    DrawCommand::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Name p_Name)
    {
      DrawCommand l_DrawCommand = p_Handle.get_id();
      return l_DrawCommand.duplicate(p_Name);
    }

    void DrawCommand::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void DrawCommand::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
    {
      DrawCommand l_DrawCommand = p_Handle.get_id();
      l_DrawCommand.serialize(p_Node);
    }

    Low::Util::Handle
    DrawCommand::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    Low::Math::Matrix4x4 &DrawCommand::get_world_transform() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:GETTER_world_transform

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, world_transform,
                      Low::Math::Matrix4x4);
    }
    void
    DrawCommand::set_world_transform(Low::Math::Matrix4x4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_transform
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_transform

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, world_transform, Low::Math::Matrix4x4) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_transform
      if (!get_render_object().is_dirty()) {
        ms_Dirty.insert(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_world_transform
    }

    Low::Renderer::MeshInfo DrawCommand::get_mesh_info() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh_info
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh_info

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, mesh_info,
                      Low::Renderer::MeshInfo);
    }
    void DrawCommand::set_mesh_info(Low::Renderer::MeshInfo p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh_info
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh_info

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, mesh_info, Low::Renderer::MeshInfo) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh_info
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh_info
    }

    uint32_t DrawCommand::get_slot() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, slot, uint32_t);
    }
    void DrawCommand::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, slot, uint32_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot
    }

    Low::Renderer::RenderObject DrawCommand::get_render_object() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_object

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, render_object,
                      Low::Renderer::RenderObject);
    }
    void DrawCommand::set_render_object(
        Low::Renderer::RenderObject p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_object

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, render_object,
               Low::Renderer::RenderObject) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_object
    }

    Low::Renderer::Material DrawCommand::get_material() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, material, Low::Renderer::Material);
    }
    void DrawCommand::set_material(Low::Renderer::Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, material, Low::Renderer::Material) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
      if (!get_render_object().is_dirty()) {
        ms_Dirty.insert(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_material
    }

    bool DrawCommand::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, uploaded, bool);
    }
    void DrawCommand::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, uploaded, bool) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded
    }

    uint64_t DrawCommand::get_render_scene_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_scene_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, render_scene_handle, uint64_t);
    }
    void DrawCommand::set_render_scene_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_scene_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, render_scene_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_scene_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_scene_handle
    }

    Low::Util::Name DrawCommand::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(DrawCommand, name, Low::Util::Name);
    }
    void DrawCommand::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(DrawCommand, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    DrawCommand
    DrawCommand::make(Low::Renderer::RenderObject p_RenderObject,
                      Low::Renderer::RenderScene p_RenderScene,
                      Low::Renderer::MeshInfo p_MeshInfo)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      DrawCommand l_DrawCommand =
          DrawCommand::make(p_RenderObject.get_name());

      _LOW_ASSERT(p_RenderScene.is_alive());

      l_DrawCommand.set_render_object(p_RenderObject);
      l_DrawCommand.set_mesh_info(p_MeshInfo);
      l_DrawCommand.set_render_scene_handle(p_RenderScene.get_id());

      if (p_RenderObject.is_alive()) {
        if (!p_RenderObject.is_dirty()) {
          ms_Dirty.insert(l_DrawCommand);
        }
      }

      return l_DrawCommand;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint64_t DrawCommand::get_sort_index() const
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_sort_index
      _LOW_ASSERT(is_alive());

      union
      {
        struct
        {
          u32 materialIndex;
          u32 meshInfoIndex;
        };
        u64 sortIndex;
      } l_SortIndexAssembler;

      l_SortIndexAssembler.materialIndex = get_material().get_index();
      l_SortIndexAssembler.meshInfoIndex =
          get_mesh_info().get_index();

      return l_SortIndexAssembler.sortIndex;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_sort_index
    }

    uint32_t DrawCommand::create_instance()
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

    void DrawCommand::increase_budget()
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
                            sizeof(DrawCommandData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(DrawCommandData, world_transform) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(DrawCommandData, world_transform) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Math::Matrix4x4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, mesh_info) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, mesh_info) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::MeshInfo));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, slot) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, slot) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, render_object) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, render_object) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::RenderObject));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, material) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, material) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Material));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, uploaded) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, uploaded) *
                          (l_Capacity)],
               l_Capacity * sizeof(bool));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData,
                                     render_scene_handle) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData,
                                   render_scene_handle) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(DrawCommandData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(DrawCommandData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for DrawCommand from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
