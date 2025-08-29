#include "LowRendererUiRenderObject.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowRendererUiCanvas.h"
#include "LowRendererUiDrawCommand.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    Low::Util::Set<Low::Renderer::UiRenderObject>
        Low::Renderer::UiRenderObject::ms_NeedInitialization;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t UiRenderObject::TYPE_ID = 76;
    uint32_t UiRenderObject::ms_Capacity = 0u;
    uint8_t *UiRenderObject::ms_Buffer = 0;
    std::shared_mutex UiRenderObject::ms_BufferMutex;
    Low::Util::Instances::Slot *UiRenderObject::ms_Slots = 0;
    Low::Util::List<UiRenderObject>
        UiRenderObject::ms_LivingInstances =
            Low::Util::List<UiRenderObject>();

    UiRenderObject::UiRenderObject() : Low::Util::Handle(0ull)
    {
    }
    UiRenderObject::UiRenderObject(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    UiRenderObject::UiRenderObject(UiRenderObject &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle UiRenderObject::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    UiRenderObject UiRenderObject::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      UiRenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiRenderObject::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, texture,
                              Texture)) Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, position,
                              Low::Math::Vector3))
          Low::Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, size,
                              Low::Math::Vector2))
          Low::Math::Vector2();
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, rotation2D, float) =
          0.0f;
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, color,
                              Low::Math::Color)) Low::Math::Color();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, uv_rect,
                              Low::Math::Vector4))
          Low::Math::Vector4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, material,
                              Material)) Material();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, mesh,
                              Low::Renderer::Mesh))
          Low::Renderer::Mesh();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, draw_commands,
                              Low::Util::List<UiDrawCommand>))
          Low::Util::List<UiDrawCommand>();
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.mark_dirty();
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void UiRenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // TODO: remove ui draw commands from canvas
      // LOW_CODEGEN::END::CUSTOM:DESTROY

      broadcast_observable(OBSERVABLE_DESTROY);

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const UiRenderObject *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void UiRenderObject::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(UiRenderObject));

      initialize_buffer(&ms_Buffer, UiRenderObjectData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_UiRenderObject);
      LOW_PROFILE_ALLOC(type_slots_UiRenderObject);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(UiRenderObject);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &UiRenderObject::is_alive;
      l_TypeInfo.destroy = &UiRenderObject::destroy;
      l_TypeInfo.serialize = &UiRenderObject::serialize;
      l_TypeInfo.deserialize = &UiRenderObject::deserialize;
      l_TypeInfo.find_by_index = &UiRenderObject::_find_by_index;
      l_TypeInfo.notify = &UiRenderObject::_notify;
      l_TypeInfo.find_by_name = &UiRenderObject::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &UiRenderObject::_make;
      l_TypeInfo.duplicate_default = &UiRenderObject::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &UiRenderObject::living_instances);
      l_TypeInfo.get_living_count = &UiRenderObject::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: canvas_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(canvas_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, canvas_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_canvas_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            canvas_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((uint64_t *)p_Data) = l_Handle.get_canvas_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: canvas_handle
      }
      {
        // Property: texture
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(texture);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_texture();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            texture, Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_texture(*(Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Texture *)p_Data) = l_Handle.get_texture();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: texture
      }
      {
        // Property: position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_position();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiRenderObject, position, Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_position(*(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Math::Vector3 *)p_Data) = l_Handle.get_position();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: position
      }
      {
        // Property: size
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(size);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_size();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            size, Low::Math::Vector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_size(*(Low::Math::Vector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Math::Vector2 *)p_Data) = l_Handle.get_size();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: size
      }
      {
        // Property: rotation2D
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rotation2D);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, rotation2D);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_rotation2D();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            rotation2D, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_rotation2D(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((float *)p_Data) = l_Handle.get_rotation2D();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: rotation2D
      }
      {
        // Property: color
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(color);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_color();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            color, Low::Math::Color);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_color(*(Low::Math::Color *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Math::Color *)p_Data) = l_Handle.get_color();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: color
      }
      {
        // Property: uv_rect
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uv_rect);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, uv_rect);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_uv_rect();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiRenderObject, uv_rect, Low::Math::Vector4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_uv_rect(*(Low::Math::Vector4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Math::Vector4 *)p_Data) = l_Handle.get_uv_rect();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uv_rect
      }
      {
        // Property: material
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(material);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            material, Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Material *)p_Data) = l_Handle.get_material();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: material
      }
      {
        // Property: z_sorting
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_sorting);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, z_sorting);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_z_sorting();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            z_sorting, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_z_sorting(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_z_sorting();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_sorting
      }
      {
        // Property: mesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(mesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Mesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_mesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiRenderObject, mesh, Low::Renderer::Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Renderer::Mesh *)p_Data) = l_Handle.get_mesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: mesh
      }
      {
        // Property: draw_commands
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(draw_commands);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_draw_commands();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiRenderObject, draw_commands,
              Low::Util::List<UiDrawCommand>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          *((Low::Util::List<UiDrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObjectData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = UiRenderObject::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Canvas);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = UiCanvas::TYPE_ID;
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

    void UiRenderObject::cleanup()
    {
      Low::Util::List<UiRenderObject> l_Instances =
          ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_UiRenderObject);
      LOW_PROFILE_FREE(type_slots_UiRenderObject);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle UiRenderObject::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    UiRenderObject UiRenderObject::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      UiRenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiRenderObject::TYPE_ID;

      return l_Handle;
    }

    bool UiRenderObject::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == UiRenderObject::TYPE_ID &&
             check_alive(ms_Slots, UiRenderObject::get_capacity());
    }

    uint32_t UiRenderObject::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    UiRenderObject::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    UiRenderObject
    UiRenderObject::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
    }

    UiRenderObject
    UiRenderObject::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      UiRenderObject l_Handle = make(p_Name);
      l_Handle.set_canvas_handle(get_canvas_handle());
      if (get_texture().is_alive()) {
        l_Handle.set_texture(get_texture());
      }
      l_Handle.set_position(get_position());
      l_Handle.set_size(get_size());
      l_Handle.set_rotation2D(get_rotation2D());
      l_Handle.set_color(get_color());
      l_Handle.set_uv_rect(get_uv_rect());
      if (get_material().is_alive()) {
        l_Handle.set_material(get_material());
      }
      l_Handle.set_z_sorting(get_z_sorting());
      if (get_mesh().is_alive()) {
        l_Handle.set_mesh(get_mesh());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    UiRenderObject UiRenderObject::duplicate(UiRenderObject p_Handle,
                                             Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    UiRenderObject::_duplicate(Low::Util::Handle p_Handle,
                               Low::Util::Name p_Name)
    {
      UiRenderObject l_UiRenderObject = p_Handle.get_id();
      return l_UiRenderObject.duplicate(p_Name);
    }

    void
    UiRenderObject::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void UiRenderObject::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Yaml::Node &p_Node)
    {
      UiRenderObject l_UiRenderObject = p_Handle.get_id();
      l_UiRenderObject.serialize(p_Node);
    }

    Low::Util::Handle
    UiRenderObject::deserialize(Low::Util::Yaml::Node &p_Node,
                                Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      LOW_NOT_IMPLEMENTED;
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void UiRenderObject::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 UiRenderObject::observe(Low::Util::Name p_Observable,
                                Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void UiRenderObject::notify(Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void UiRenderObject::_notify(Low::Util::Handle p_Observer,
                                 Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      UiRenderObject l_UiRenderObject = p_Observer.get_id();
      l_UiRenderObject.notify(p_Observed, p_Observable);
    }

    uint64_t UiRenderObject::get_canvas_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_canvas_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, canvas_handle, uint64_t);
    }
    void UiRenderObject::set_canvas_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiRenderObject, canvas_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_canvas_handle

      broadcast_observable(N(canvas_handle));
    }

    Texture UiRenderObject::get_texture() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, texture, Texture);
    }
    void UiRenderObject::set_texture(Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      if (get_texture() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, texture, Texture) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
        // LOW_CODEGEN::END::CUSTOM:SETTER_texture

        broadcast_observable(N(texture));
      }
    }

    Low::Math::Vector3 &UiRenderObject::get_position() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_position

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, position, Low::Math::Vector3);
    }
    void UiRenderObject::set_position(float p_X, float p_Y, float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_position(p_Val);
    }

    void UiRenderObject::set_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.x = p_Value;
      set_position(l_Value);
    }

    void UiRenderObject::set_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.y = p_Value;
      set_position(l_Value);
    }

    void UiRenderObject::set_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.z = p_Value;
      set_position(l_Value);
    }

    void UiRenderObject::set_position(Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position

      if (get_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, position, Low::Math::Vector3) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_position

        broadcast_observable(N(position));
      }
    }

    Low::Math::Vector2 &UiRenderObject::get_size() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size
      // LOW_CODEGEN::END::CUSTOM:GETTER_size

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, size, Low::Math::Vector2);
    }
    void UiRenderObject::set_size(float p_X, float p_Y)
    {
      Low::Math::Vector2 l_Val(p_X, p_Y);
      set_size(l_Val);
    }

    void UiRenderObject::set_size_x(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_size();
      l_Value.x = p_Value;
      set_size(l_Value);
    }

    void UiRenderObject::set_size_y(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_size();
      l_Value.y = p_Value;
      set_size(l_Value);
    }

    void UiRenderObject::set_size(Low::Math::Vector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_size

      if (get_size() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, size, Low::Math::Vector2) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size
        // LOW_CODEGEN::END::CUSTOM:SETTER_size

        broadcast_observable(N(size));
      }
    }

    float UiRenderObject::get_rotation2D() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation2D
      // LOW_CODEGEN::END::CUSTOM:GETTER_rotation2D

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, rotation2D, float);
    }
    void UiRenderObject::set_rotation2D(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation2D
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation2D

      if (get_rotation2D() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, rotation2D, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation2D
        // LOW_CODEGEN::END::CUSTOM:SETTER_rotation2D

        broadcast_observable(N(rotation2D));
      }
    }

    Low::Math::Color &UiRenderObject::get_color() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, color, Low::Math::Color);
    }
    void UiRenderObject::set_color(Low::Math::Color &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, color, Low::Math::Color) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
        // LOW_CODEGEN::END::CUSTOM:SETTER_color

        broadcast_observable(N(color));
      }
    }

    Low::Math::Vector4 &UiRenderObject::get_uv_rect() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uv_rect
      // LOW_CODEGEN::END::CUSTOM:GETTER_uv_rect

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, uv_rect, Low::Math::Vector4);
    }
    void UiRenderObject::set_uv_rect(Low::Math::Vector4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uv_rect
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uv_rect

      if (get_uv_rect() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, uv_rect, Low::Math::Vector4) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uv_rect
        if (get_draw_commands().size() == 1) {
          get_draw_commands()[0].set_uv_rect(p_Value);
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_uv_rect

        broadcast_observable(N(uv_rect));
      }
    }

    Material UiRenderObject::get_material() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, material, Material);
    }
    void UiRenderObject::set_material(Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, material, Material) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    uint32_t UiRenderObject::get_z_sorting() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_sorting
      // LOW_CODEGEN::END::CUSTOM:GETTER_z_sorting

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, z_sorting, uint32_t);
    }
    void UiRenderObject::set_z_sorting(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_sorting
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_sorting

      if (get_z_sorting() != p_Value) {
        // Set dirty flags
        mark_dirty();
        mark_z_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiRenderObject, z_sorting, uint32_t) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_sorting
        // LOW_CODEGEN::END::CUSTOM:SETTER_z_sorting

        broadcast_observable(N(z_sorting));
      }
    }

    Low::Renderer::Mesh UiRenderObject::get_mesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, mesh, Low::Renderer::Mesh);
    }
    void UiRenderObject::set_mesh(Low::Renderer::Mesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiRenderObject, mesh, Low::Renderer::Mesh) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh

      broadcast_observable(N(mesh));
    }

    Low::Util::List<UiDrawCommand> &
    UiRenderObject::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands
      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, draw_commands,
                      Low::Util::List<UiDrawCommand>);
    }

    void UiRenderObject::mark_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
      // LOW_CODEGEN::END::CUSTOM:MARK_dirty
    }

    void UiRenderObject::mark_z_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty
      mark_dirty();

      UiCanvas l_Canvas = get_canvas_handle();
      l_Canvas.mark_z_dirty();
      // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
    }

    Low::Util::Name UiRenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiRenderObject, name, Low::Util::Name);
    }
    void UiRenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiRenderObject, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    UiRenderObject UiRenderObject::make(UiCanvas p_Canvas,
                                        Low::Renderer::Mesh p_Mesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      _LOW_ASSERT(p_Canvas.is_alive());
      _LOW_ASSERT(p_Mesh.is_alive());

      UiRenderObject l_Handle = make(N(UiRenderObject));
      l_Handle.set_canvas_handle(p_Canvas.get_id());
      l_Handle.set_mesh(p_Mesh);

      ms_NeedInitialization.insert(l_Handle);

      return l_Handle;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t UiRenderObject::create_instance()
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

    void UiRenderObject::increase_budget()
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
                            sizeof(UiRenderObjectData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(UiRenderObjectData, canvas_handle) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(UiRenderObjectData, canvas_handle) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, texture) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, texture) *
                          (l_Capacity)],
               l_Capacity * sizeof(Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, position) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, position) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, size) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, size) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector2));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, rotation2D) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, rotation2D) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, color) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, color) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Color));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, uv_rect) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, uv_rect) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, material) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, material) *
                          (l_Capacity)],
               l_Capacity * sizeof(Material));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, z_sorting) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, z_sorting) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, mesh) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, mesh) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::Mesh));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          UiRenderObject i_UiRenderObject = *it;

          auto *i_ValPtr = new (
              &l_NewBuffer[offsetof(UiRenderObjectData,
                                    draw_commands) *
                               (l_Capacity + l_CapacityIncrease) +
                           (it->get_index() *
                            sizeof(Low::Util::List<UiDrawCommand>))])
              Low::Util::List<UiDrawCommand>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_UiRenderObject, UiRenderObject, draw_commands,
              Low::Util::List<UiDrawCommand>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiRenderObjectData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiRenderObjectData, name) *
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
          << "Auto-increased budget for UiRenderObject from "
          << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
          << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
