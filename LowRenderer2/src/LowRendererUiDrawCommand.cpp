#include "LowRendererUiDrawCommand.h"

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
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t UiDrawCommand::TYPE_ID = 70;
    uint32_t UiDrawCommand::ms_Capacity = 0u;
    uint8_t *UiDrawCommand::ms_Buffer = 0;
    std::shared_mutex UiDrawCommand::ms_BufferMutex;
    Low::Util::Instances::Slot *UiDrawCommand::ms_Slots = 0;
    Low::Util::List<UiDrawCommand> UiDrawCommand::ms_LivingInstances =
        Low::Util::List<UiDrawCommand>();

    UiDrawCommand::UiDrawCommand() : Low::Util::Handle(0ull)
    {
    }
    UiDrawCommand::UiDrawCommand(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    UiDrawCommand::UiDrawCommand(UiDrawCommand &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle UiDrawCommand::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    UiDrawCommand UiDrawCommand::make(Low::Util::Name p_Name)
    {
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      UiDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiDrawCommand::TYPE_ID;

      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, render_object,
                              Low::Renderer::UiRenderObject))
          Low::Renderer::UiRenderObject();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, texture,
                              Texture)) Texture();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, position,
                              Low::Math::Vector3))
          Low::Math::Vector3();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, size,
                              Low::Math::Vector2))
          Low::Math::Vector2();
      ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, rotation2D, float) =
          0.0f;
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, color,
                              Low::Math::Color)) Low::Math::Color();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, uv_rect,
                              Low::Math::Vector4))
          Low::Math::Vector4();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, material,
                              Material)) Material();
      new (&ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, submesh,
                              Low::Renderer::GpuSubmesh))
          Low::Renderer::GpuSubmesh();
      ACCESSOR_TYPE_SOA(l_Handle, UiDrawCommand, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
      l_Handle.set_uv_rect(
          Low::Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void UiDrawCommand::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
      // TODO: remove from canvas
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

    void UiDrawCommand::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                    N(UiDrawCommand));

      initialize_buffer(&ms_Buffer, UiDrawCommandData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_UiDrawCommand);
      LOW_PROFILE_ALLOC(type_slots_UiDrawCommand);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(UiDrawCommand);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &UiDrawCommand::is_alive;
      l_TypeInfo.destroy = &UiDrawCommand::destroy;
      l_TypeInfo.serialize = &UiDrawCommand::serialize;
      l_TypeInfo.deserialize = &UiDrawCommand::deserialize;
      l_TypeInfo.find_by_index = &UiDrawCommand::_find_by_index;
      l_TypeInfo.notify = &UiDrawCommand::_notify;
      l_TypeInfo.find_by_name = &UiDrawCommand::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &UiDrawCommand::_make;
      l_TypeInfo.duplicate_default = &UiDrawCommand::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &UiDrawCommand::living_instances);
      l_TypeInfo.get_living_count = &UiDrawCommand::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: render_object
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(render_object);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiDrawCommandData, render_object);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::UiRenderObject::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_render_object();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiDrawCommand, render_object,
              Low::Renderer::UiRenderObject);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_render_object(
              *(Low::Renderer::UiRenderObject *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
          *((Low::Renderer::UiRenderObject *)p_Data) =
              l_Handle.get_render_object();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: render_object
      }
      {
        // Property: canvas_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(canvas_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiDrawCommandData, canvas_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_canvas_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            canvas_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_canvas_handle(*(uint64_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Texture::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_texture();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            texture, Texture);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_texture(*(Texture *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_position();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiDrawCommand, position, Low::Math::Vector3);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_position(*(Low::Math::Vector3 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
        l_PropertyInfo.dataOffset = offsetof(UiDrawCommandData, size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_size();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            size, Low::Math::Vector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_size(*(Low::Math::Vector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, rotation2D);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_rotation2D();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            rotation2D, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_rotation2D(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_color();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            color, Low::Math::Color);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_color(*(Low::Math::Color *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, uv_rect);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_uv_rect();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiDrawCommand, uv_rect, Low::Math::Vector4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_uv_rect(*(Low::Math::Vector4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Material::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_material();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            material, Material);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_material(*(Material *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
            offsetof(UiDrawCommandData, z_sorting);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_z_sorting();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            z_sorting, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_z_sorting(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
          *((uint32_t *)p_Data) = l_Handle.get_z_sorting();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_sorting
      }
      {
        // Property: submesh
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(submesh);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiDrawCommandData, submesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType =
            Low::Renderer::GpuSubmesh::TYPE_ID;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_submesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiDrawCommand, submesh,
              Low::Renderer::GpuSubmesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
          *((Low::Renderer::GpuSubmesh *)p_Data) =
              l_Handle.get_submesh();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: submesh
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(UiDrawCommandData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiDrawCommand,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiDrawCommand l_Handle = p_Handle.get_id();
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
        l_FunctionInfo.handleType = UiDrawCommand::TYPE_ID;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_RenderObject);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::UiRenderObject::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Canvas);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::UiCanvas::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Submesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType =
              Low::Renderer::GpuSubmesh::TYPE_ID;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void UiDrawCommand::cleanup()
    {
      Low::Util::List<UiDrawCommand> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_UiDrawCommand);
      LOW_PROFILE_FREE(type_slots_UiDrawCommand);
      LOCK_UNLOCK(l_Lock);
    }

    Low::Util::Handle UiDrawCommand::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    UiDrawCommand UiDrawCommand::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      UiDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = UiDrawCommand::TYPE_ID;

      return l_Handle;
    }

    UiDrawCommand UiDrawCommand::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      UiDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = UiDrawCommand::TYPE_ID;

      return l_Handle;
    }

    bool UiDrawCommand::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == UiDrawCommand::TYPE_ID &&
             check_alive(ms_Slots, UiDrawCommand::get_capacity());
    }

    uint32_t UiDrawCommand::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    UiDrawCommand::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    UiDrawCommand UiDrawCommand::find_by_name(Low::Util::Name p_Name)
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

    UiDrawCommand
    UiDrawCommand::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      UiDrawCommand l_Handle = make(p_Name);
      if (get_render_object().is_alive()) {
        l_Handle.set_render_object(get_render_object());
      }
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
      if (get_submesh().is_alive()) {
        l_Handle.set_submesh(get_submesh());
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    UiDrawCommand UiDrawCommand::duplicate(UiDrawCommand p_Handle,
                                           Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    UiDrawCommand::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
    {
      UiDrawCommand l_UiDrawCommand = p_Handle.get_id();
      return l_UiDrawCommand.duplicate(p_Name);
    }

    void UiDrawCommand::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void UiDrawCommand::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Yaml::Node &p_Node)
    {
      UiDrawCommand l_UiDrawCommand = p_Handle.get_id();
      l_UiDrawCommand.serialize(p_Node);
    }

    Low::Util::Handle
    UiDrawCommand::deserialize(Low::Util::Yaml::Node &p_Node,
                               Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      LOW_NOT_IMPLEMENTED;
      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void UiDrawCommand::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 UiDrawCommand::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 UiDrawCommand::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void UiDrawCommand::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void UiDrawCommand::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
    {
      UiDrawCommand l_UiDrawCommand = p_Observer.get_id();
      l_UiDrawCommand.notify(p_Observed, p_Observable);
    }

    Low::Renderer::UiRenderObject
    UiDrawCommand::get_render_object() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:GETTER_render_object

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, render_object,
                      Low::Renderer::UiRenderObject);
    }
    void UiDrawCommand::set_render_object(
        Low::Renderer::UiRenderObject p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_render_object

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiDrawCommand, render_object,
               Low::Renderer::UiRenderObject) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_render_object
      // LOW_CODEGEN::END::CUSTOM:SETTER_render_object

      broadcast_observable(N(render_object));
    }

    uint64_t UiDrawCommand::get_canvas_handle() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:GETTER_canvas_handle

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, canvas_handle, uint64_t);
    }
    void UiDrawCommand::set_canvas_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas_handle

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiDrawCommand, canvas_handle, uint64_t) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas_handle
      // LOW_CODEGEN::END::CUSTOM:SETTER_canvas_handle

      broadcast_observable(N(canvas_handle));
    }

    Texture UiDrawCommand::get_texture() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture
      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, texture, Texture);
    }
    void UiDrawCommand::set_texture(Texture p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      if (get_texture() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, texture, Texture) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture
        // LOW_CODEGEN::END::CUSTOM:SETTER_texture

        broadcast_observable(N(texture));
      }
    }

    Low::Math::Vector3 &UiDrawCommand::get_position() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_position
      // LOW_CODEGEN::END::CUSTOM:GETTER_position

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, position, Low::Math::Vector3);
    }
    void UiDrawCommand::set_position(float p_X, float p_Y, float p_Z)
    {
      Low::Math::Vector3 p_Val(p_X, p_Y, p_Z);
      set_position(p_Val);
    }

    void UiDrawCommand::set_position_x(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.x = p_Value;
      set_position(l_Value);
    }

    void UiDrawCommand::set_position_y(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.y = p_Value;
      set_position(l_Value);
    }

    void UiDrawCommand::set_position_z(float p_Value)
    {
      Low::Math::Vector3 l_Value = get_position();
      l_Value.z = p_Value;
      set_position(l_Value);
    }

    void UiDrawCommand::set_position(Low::Math::Vector3 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position

      if (get_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, position, Low::Math::Vector3) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position
        // LOW_CODEGEN::END::CUSTOM:SETTER_position

        broadcast_observable(N(position));
      }
    }

    Low::Math::Vector2 &UiDrawCommand::get_size() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size
      // LOW_CODEGEN::END::CUSTOM:GETTER_size

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, size, Low::Math::Vector2);
    }
    void UiDrawCommand::set_size(float p_X, float p_Y)
    {
      Low::Math::Vector2 l_Val(p_X, p_Y);
      set_size(l_Val);
    }

    void UiDrawCommand::set_size_x(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_size();
      l_Value.x = p_Value;
      set_size(l_Value);
    }

    void UiDrawCommand::set_size_y(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_size();
      l_Value.y = p_Value;
      set_size(l_Value);
    }

    void UiDrawCommand::set_size(Low::Math::Vector2 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_size

      if (get_size() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, size, Low::Math::Vector2) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size
        // LOW_CODEGEN::END::CUSTOM:SETTER_size

        broadcast_observable(N(size));
      }
    }

    float UiDrawCommand::get_rotation2D() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation2D
      // LOW_CODEGEN::END::CUSTOM:GETTER_rotation2D

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, rotation2D, float);
    }
    void UiDrawCommand::set_rotation2D(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation2D
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation2D

      if (get_rotation2D() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, rotation2D, float) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation2D
        // LOW_CODEGEN::END::CUSTOM:SETTER_rotation2D

        broadcast_observable(N(rotation2D));
      }
    }

    Low::Math::Color &UiDrawCommand::get_color() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, color, Low::Math::Color);
    }
    void UiDrawCommand::set_color(Low::Math::Color &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, color, Low::Math::Color) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
        // LOW_CODEGEN::END::CUSTOM:SETTER_color

        broadcast_observable(N(color));
      }
    }

    Low::Math::Vector4 &UiDrawCommand::get_uv_rect() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uv_rect
      // LOW_CODEGEN::END::CUSTOM:GETTER_uv_rect

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, uv_rect, Low::Math::Vector4);
    }
    void UiDrawCommand::set_uv_rect(Low::Math::Vector4 &p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uv_rect
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uv_rect

      if (get_uv_rect() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, uv_rect, Low::Math::Vector4) =
            p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uv_rect
        // LOW_CODEGEN::END::CUSTOM:SETTER_uv_rect

        broadcast_observable(N(uv_rect));
      }
    }

    Material UiDrawCommand::get_material() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material
      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, material, Material);
    }
    void UiDrawCommand::set_material(Material p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        WRITE_LOCK(l_WriteLock);
        TYPE_SOA(UiDrawCommand, material, Material) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    uint32_t UiDrawCommand::get_z_sorting() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_sorting
      // LOW_CODEGEN::END::CUSTOM:GETTER_z_sorting

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, z_sorting, uint32_t);
    }
    void UiDrawCommand::set_z_sorting(uint32_t p_Value)
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
        TYPE_SOA(UiDrawCommand, z_sorting, uint32_t) = p_Value;
        LOCK_UNLOCK(l_WriteLock);

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_sorting
        // LOW_CODEGEN::END::CUSTOM:SETTER_z_sorting

        broadcast_observable(N(z_sorting));
      }
    }

    Low::Renderer::GpuSubmesh UiDrawCommand::get_submesh() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:GETTER_submesh

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, submesh,
                      Low::Renderer::GpuSubmesh);
    }
    void UiDrawCommand::set_submesh(Low::Renderer::GpuSubmesh p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_submesh

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiDrawCommand, submesh, Low::Renderer::GpuSubmesh) =
          p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_submesh
      // LOW_CODEGEN::END::CUSTOM:SETTER_submesh

      broadcast_observable(N(submesh));
    }

    void UiDrawCommand::mark_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
      // LOW_CODEGEN::END::CUSTOM:MARK_dirty
    }

    void UiDrawCommand::mark_z_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty
      mark_dirty();

      UiCanvas l_Canvas = get_canvas_handle();
      l_Canvas.mark_z_dirty();
      // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
    }

    Low::Util::Name UiDrawCommand::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(UiDrawCommand, name, Low::Util::Name);
    }
    void UiDrawCommand::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(UiDrawCommand, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    UiDrawCommand
    UiDrawCommand::make(Low::Renderer::UiRenderObject p_RenderObject,
                        Low::Renderer::UiCanvas p_Canvas,
                        Low::Renderer::GpuSubmesh p_Submesh)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make
      UiDrawCommand l_DrawCommand =
          UiDrawCommand::make(p_RenderObject.get_name());

      _LOW_ASSERT(p_Canvas.is_alive());

      l_DrawCommand.set_render_object(p_RenderObject);
      l_DrawCommand.set_submesh(p_Submesh);
      l_DrawCommand.set_canvas_handle(p_Canvas.get_id());
      l_DrawCommand.set_position(p_RenderObject.get_position());
      l_DrawCommand.set_z_sorting(p_RenderObject.get_z_sorting());
      l_DrawCommand.set_material(p_RenderObject.get_material());
      l_DrawCommand.set_texture(p_RenderObject.get_texture());
      l_DrawCommand.set_size(p_RenderObject.get_size());
      l_DrawCommand.set_rotation2D(p_RenderObject.get_rotation2D());
      l_DrawCommand.set_uv_rect(p_RenderObject.get_uv_rect());

      l_DrawCommand.mark_z_dirty();

      p_Canvas.get_draw_commands().push_back(l_DrawCommand);

      return l_DrawCommand;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    uint32_t UiDrawCommand::create_instance()
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

    void UiDrawCommand::increase_budget()
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
                            sizeof(UiDrawCommandData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(UiDrawCommandData, render_object) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(UiDrawCommandData, render_object) *
                       (l_Capacity)],
            l_Capacity * sizeof(Low::Renderer::UiRenderObject));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(UiDrawCommandData, canvas_handle) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(UiDrawCommandData, canvas_handle) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint64_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, texture) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, texture) *
                          (l_Capacity)],
               l_Capacity * sizeof(Texture));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, position) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, position) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector3));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, size) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, size) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector2));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, rotation2D) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, rotation2D) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, color) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, color) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Color));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, uv_rect) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, uv_rect) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Math::Vector4));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, material) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, material) *
                          (l_Capacity)],
               l_Capacity * sizeof(Material));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, z_sorting) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, z_sorting) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, submesh) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, submesh) *
                          (l_Capacity)],
               l_Capacity * sizeof(Low::Renderer::GpuSubmesh));
      }
      {
        memcpy(&l_NewBuffer[offsetof(UiDrawCommandData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(UiDrawCommandData, name) *
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

      LOW_LOG_DEBUG << "Auto-increased budget for UiDrawCommand from "
                    << l_Capacity << " to "
                    << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
