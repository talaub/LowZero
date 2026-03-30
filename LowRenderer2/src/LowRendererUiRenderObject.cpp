#include "LowRendererUiRenderObject.h"

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

#include "LowRendererUiCanvas.h"
#include "LowRendererUiDrawCommand.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    Low::Util::Set<Low::Renderer::UiRenderObject>
        Low::Renderer::UiRenderObject::ms_NeedInitialization;

    Low::Util::Set<Low::Renderer::UiRenderObject>
        Low::Renderer::UiRenderObject::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 UiRenderObject::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        UiRenderObject::IDENTIFIER(LOW_NAME(509652687),
                                   LOW_NAME(958953127));
    uint32_t UiRenderObject::ms_Capacity = 0u;
    uint32_t UiRenderObject::ms_PageSize = 0u;
    Low::Util::SharedMutex UiRenderObject::ms_LivingMutex;
    Low::Util::SharedMutex UiRenderObject::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        UiRenderObject::ms_PagesLock(UiRenderObject::ms_PagesMutex,
                                     std::defer_lock);
    Low::Util::List<UiRenderObject>
        UiRenderObject::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        UiRenderObject::ms_Pages;

    Low::Util::Handle UiRenderObject::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    UiRenderObject UiRenderObject::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      UiRenderObject l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = UiRenderObject::ms_TypeId;

      l_PageLock.unlock();

      Low::Util::HandleLock<UiRenderObject> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, UiRenderObject, texture,
                                 Texture)) Texture();
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, rotation2D, float) =
          0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, UiRenderObject, material,
                                 Material)) Material();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, UiRenderObject, mesh,
                                 Low::Renderer::Mesh))
          Low::Renderer::Mesh();
      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, UiRenderObject,
                                 draw_commands,
                                 Low::Util::List<UiDrawCommand>))
          Low::Util::List<UiDrawCommand>();
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, uploaded, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, z_dirty, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, UiRenderObject, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      {
        Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
            ms_LivingMutex);
        ms_LivingInstances.push_back(l_Handle);
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      l_Handle.mark_dirty();

      l_Handle.set_uv_rect(Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void UiRenderObject::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        for (auto i_DrawCommand : get_draw_commands()) {
          i_DrawCommand.destroy();
        }
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

    void UiRenderObject::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(UiRenderObject));

      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(UiRenderObject));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, UiRenderObject::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(UiRenderObject);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &UiRenderObject::is_alive;
      l_TypeInfo.destroy = &UiRenderObject::destroy;
      l_TypeInfo.serialize = &UiRenderObject::serialize;
      l_TypeInfo.deserialize = &UiRenderObject::deserialize;
      l_TypeInfo.find_by_index = &UiRenderObject::_find_by_index;
      l_TypeInfo.notify = &UiRenderObject::_notify;
      l_TypeInfo.post_load = nullptr;
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
            offsetof(UiRenderObject::Data, canvas_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_canvas_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            canvas_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, texture);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Texture::type_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR3;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, size);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, rotation2D);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::COLOR;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, uv_rect);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, material);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Material::type_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, z_sorting);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, mesh);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_PropertyInfo.handleType = Low::Renderer::Mesh::type_id();
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_mesh();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, UiRenderObject, mesh, Low::Renderer::Mesh);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
            offsetof(UiRenderObject::Data, draw_commands);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          *((Low::Util::List<UiDrawCommand> *)p_Data) =
              l_Handle.get_draw_commands();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: draw_commands
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObject::Data, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObject::Data, slot);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.get_slot();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            slot, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_slot(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_slot();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: slot
      }
      {
        // Property: dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObject::Data, dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.is_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          *((bool *)p_Data) = l_Handle.is_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: dirty
      }
      {
        // Property: z_dirty
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_dirty);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObject::Data, z_dirty);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          l_Handle.is_z_dirty();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, UiRenderObject,
                                            z_dirty, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          UiRenderObject l_Handle = p_Handle.get_id();
          l_Handle.set_z_dirty(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
          *((bool *)p_Data) = l_Handle.is_z_dirty();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_dirty
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(UiRenderObject::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          UiRenderObject l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
              l_Handle);
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
          Low::Util::HandleLock<UiRenderObject> l_HandleLock(
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
        l_FunctionInfo.handleType = UiRenderObject::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Canvas);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = UiCanvas::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Mesh);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = Low::Renderer::Mesh::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void UiRenderObject::cleanup()
    {
      Low::Util::List<UiRenderObject> l_Instances =
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

    Low::Util::Handle UiRenderObject::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    UiRenderObject UiRenderObject::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      UiRenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = UiRenderObject::ms_TypeId;

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

    UiRenderObject UiRenderObject::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      UiRenderObject l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = UiRenderObject::ms_TypeId;

      return l_Handle;
    }

    bool UiRenderObject::is_alive() const
    {
      if (m_Data.m_Type != UiRenderObject::ms_TypeId) {
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
      return m_Data.m_Type == UiRenderObject::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
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
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_slot(get_slot());
      l_Handle.set_dirty(is_dirty());
      l_Handle.set_z_dirty(is_z_dirty());

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
    UiRenderObject::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void UiRenderObject::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Serial::Node &p_Node)
    {
      UiRenderObject l_UiRenderObject = p_Handle.get_id();
      l_UiRenderObject.serialize(p_Node);
    }

    Low::Util::Handle
    UiRenderObject::deserialize(Low::Util::Serial::Node &p_Node,
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

    u64 UiRenderObject::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
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
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:GETTER_canvas_handle

      return TYPE_SOA(UiRenderObject, canvas_handle, uint64_t);
    }
    void UiRenderObject::set_canvas_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas_handle

      // Set new value
      TYPE_SOA(UiRenderObject, canvas_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:SETTER_canvas_handle

      broadcast_observable(N(canvas_handle));
    }

    Texture UiRenderObject::get_texture() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_texture

      // LOW_CODEGEN::END::CUSTOM:GETTER_texture

      return TYPE_SOA(UiRenderObject, texture, Texture);
    }
    void UiRenderObject::set_texture(Texture p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_texture

      if (get_texture().is_alive()) {
        get_texture().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_texture

      if (get_texture() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, texture, Texture) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_texture

        if (p_Value.is_alive()) {
          p_Value.reference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_texture

        broadcast_observable(N(texture));
      }
    }

    Low::Math::Vector3 UiRenderObject::get_position() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_position

      // LOW_CODEGEN::END::CUSTOM:GETTER_position

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

    void UiRenderObject::set_position(Low::Math::Vector3 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position

      if (get_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, position, Low::Math::Vector3) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position

        // LOW_CODEGEN::END::CUSTOM:SETTER_position

        broadcast_observable(N(position));
      }
    }

    Low::Math::Vector2 UiRenderObject::get_size() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size

      // LOW_CODEGEN::END::CUSTOM:GETTER_size

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

    void UiRenderObject::set_size(Low::Math::Vector2 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_size

      if (get_size() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, size, Low::Math::Vector2) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size

        // LOW_CODEGEN::END::CUSTOM:SETTER_size

        broadcast_observable(N(size));
      }
    }

    float UiRenderObject::get_rotation2D() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation2D

      // LOW_CODEGEN::END::CUSTOM:GETTER_rotation2D

      return TYPE_SOA(UiRenderObject, rotation2D, float);
    }
    void UiRenderObject::set_rotation2D(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation2D

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation2D

      if (get_rotation2D() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, rotation2D, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation2D

        // LOW_CODEGEN::END::CUSTOM:SETTER_rotation2D

        broadcast_observable(N(rotation2D));
      }
    }

    Low::Math::Color UiRenderObject::get_color() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color

      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      return TYPE_SOA(UiRenderObject, color, Low::Math::Color);
    }
    void UiRenderObject::set_color(Low::Math::Color p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, color, Low::Math::Color) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color

        // LOW_CODEGEN::END::CUSTOM:SETTER_color

        broadcast_observable(N(color));
      }
    }

    Low::Math::Vector4 UiRenderObject::get_uv_rect() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uv_rect

      // LOW_CODEGEN::END::CUSTOM:GETTER_uv_rect

      return TYPE_SOA(UiRenderObject, uv_rect, Low::Math::Vector4);
    }
    void UiRenderObject::set_uv_rect(Low::Math::Vector4 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uv_rect

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uv_rect

      if (get_uv_rect() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, uv_rect, Low::Math::Vector4) =
            p_Value;

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
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_material

      // LOW_CODEGEN::END::CUSTOM:GETTER_material

      return TYPE_SOA(UiRenderObject, material, Material);
    }
    void UiRenderObject::set_material(Material p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_material

      Material l_OldMaterial = get_material();
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_material

      if (get_material() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, material, Material) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_material

        if (l_OldMaterial.is_alive()) {
          l_OldMaterial.dereference(get_id());
        }
        if (p_Value.is_alive()) {
          p_Value.reference(get_id());
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_material

        broadcast_observable(N(material));
      }
    }

    uint32_t UiRenderObject::get_z_sorting() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_sorting

      // LOW_CODEGEN::END::CUSTOM:GETTER_z_sorting

      return TYPE_SOA(UiRenderObject, z_sorting, uint32_t);
    }
    void UiRenderObject::set_z_sorting(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_sorting

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_sorting

      if (get_z_sorting() != p_Value) {
        // Set dirty flags
        mark_dirty();
        mark_z_dirty();

        // Set new value
        TYPE_SOA(UiRenderObject, z_sorting, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_sorting

        // LOW_CODEGEN::END::CUSTOM:SETTER_z_sorting

        broadcast_observable(N(z_sorting));
      }
    }

    Low::Renderer::Mesh UiRenderObject::get_mesh() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_mesh

      // LOW_CODEGEN::END::CUSTOM:GETTER_mesh

      return TYPE_SOA(UiRenderObject, mesh, Low::Renderer::Mesh);
    }
    void UiRenderObject::set_mesh(Low::Renderer::Mesh p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_mesh

      if (get_mesh().is_alive()) {
        get_mesh().dereference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_mesh

      // Set new value
      TYPE_SOA(UiRenderObject, mesh, Low::Renderer::Mesh) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_mesh

      if (p_Value.is_alive()) {
        p_Value.reference(get_id());
      }
      // LOW_CODEGEN::END::CUSTOM:SETTER_mesh

      broadcast_observable(N(mesh));
    }

    Low::Util::List<UiDrawCommand> &
    UiRenderObject::get_draw_commands() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_draw_commands

      // LOW_CODEGEN::END::CUSTOM:GETTER_draw_commands

      return TYPE_SOA(UiRenderObject, draw_commands,
                      Low::Util::List<UiDrawCommand>);
    }

    bool UiRenderObject::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      return TYPE_SOA(UiRenderObject, uploaded, bool);
    }
    void UiRenderObject::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void UiRenderObject::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      TYPE_SOA(UiRenderObject, uploaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded
      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    uint32_t UiRenderObject::get_slot() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_slot
      // LOW_CODEGEN::END::CUSTOM:GETTER_slot

      return TYPE_SOA(UiRenderObject, slot, uint32_t);
    }
    void UiRenderObject::set_slot(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_slot
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_slot

      // Set new value
      TYPE_SOA(UiRenderObject, slot, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_slot
      // LOW_CODEGEN::END::CUSTOM:SETTER_slot

      broadcast_observable(N(slot));
    }

    bool UiRenderObject::is_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

      return TYPE_SOA(UiRenderObject, dirty, bool);
    }
    void UiRenderObject::toggle_dirty()
    {
      set_dirty(!is_dirty());
    }

    void UiRenderObject::set_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

      // Set new value
      TYPE_SOA(UiRenderObject, dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty

      // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

      broadcast_observable(N(dirty));
    }

    void UiRenderObject::mark_dirty()
    {
      if (!is_dirty()) {
        TYPE_SOA(UiRenderObject, dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty

        ms_Dirty.insert(get_id());
        // LOW_CODEGEN::END::CUSTOM:MARK_dirty
      }
    }

    bool UiRenderObject::is_z_dirty() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:GETTER_z_dirty

      return TYPE_SOA(UiRenderObject, z_dirty, bool);
    }
    void UiRenderObject::toggle_z_dirty()
    {
      set_z_dirty(!is_z_dirty());
    }

    void UiRenderObject::set_z_dirty(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_dirty

      // Set new value
      TYPE_SOA(UiRenderObject, z_dirty, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_dirty

      // LOW_CODEGEN::END::CUSTOM:SETTER_z_dirty

      broadcast_observable(N(z_dirty));
    }

    void UiRenderObject::mark_z_dirty()
    {
      if (!is_z_dirty()) {
        TYPE_SOA(UiRenderObject, z_dirty, bool) = true;
        // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty

        mark_dirty();

        UiCanvas l_Canvas = get_canvas_handle();
        l_Canvas.mark_z_dirty();
        // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
      }
    }

    Low::Util::Name UiRenderObject::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(UiRenderObject, name, Low::Util::Name);
    }
    void UiRenderObject::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<UiRenderObject> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(UiRenderObject, name, Low::Util::Name) = p_Value;

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

    uint32_t UiRenderObject::create_instance(
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

    u32 UiRenderObject::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for UiRenderObject.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, UiRenderObject::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool UiRenderObject::get_page_for_index(const u32 p_Index,
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
