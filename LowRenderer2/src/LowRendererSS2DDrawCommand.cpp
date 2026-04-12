#include "LowRendererSS2DDrawCommand.h"

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

#include "LowRendererSS2DCanvas.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    Low::Util::Set<Low::Renderer::SS2DDrawCommand>
        Low::Renderer::SS2DDrawCommand::ms_Dirty;
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    u16 SS2DDrawCommand::ms_TypeId = 0;
    const Low::Util::TypeIdentifier
        SS2DDrawCommand::IDENTIFIER(LOW_NAME(509652687),
                                    LOW_NAME(3802565009));
    uint32_t SS2DDrawCommand::ms_Capacity = 0u;
    uint32_t SS2DDrawCommand::ms_PageSize = 0u;
    Low::Util::SharedMutex SS2DDrawCommand::ms_LivingMutex;
    Low::Util::SharedMutex SS2DDrawCommand::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        SS2DDrawCommand::ms_PagesLock(SS2DDrawCommand::ms_PagesMutex,
                                      std::defer_lock);
    Low::Util::List<SS2DDrawCommand>
        SS2DDrawCommand::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SS2DDrawCommand::ms_Pages;

    Low::Util::Handle SS2DDrawCommand::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SS2DDrawCommand SS2DDrawCommand::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      SS2DDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SS2DDrawCommand::ms_TypeId;

      l_PageLock.unlock();

      Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(l_Handle);

      new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SS2DDrawCommand, type,
                                 SS2DType)) SS2DType();
      ACCESSOR_TYPE_SOA(l_Handle, SS2DDrawCommand, rotation, float) =
          0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, SS2DDrawCommand, uploaded, bool) =
          false;
      ACCESSOR_TYPE_SOA(l_Handle, SS2DDrawCommand, name,
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

    void SS2DDrawCommand::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());
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

    void SS2DDrawCommand::initialize()
    {
      const Low::Util::TypeIdentifier l_IdentifierNames(
          N(LowRenderer2), N(SS2DDrawCommand));

      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer2), N(SS2DDrawCommand));

      ms_PageSize = ms_Capacity;
      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SS2DDrawCommand::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SS2DDrawCommand);
      l_TypeInfo.typeId = ms_TypeId;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SS2DDrawCommand::is_alive;
      l_TypeInfo.destroy = &SS2DDrawCommand::destroy;
      l_TypeInfo.serialize = &SS2DDrawCommand::serialize;
      l_TypeInfo.deserialize = &SS2DDrawCommand::deserialize;
      l_TypeInfo.find_by_index = &SS2DDrawCommand::_find_by_index;
      l_TypeInfo.notify = &SS2DDrawCommand::_notify;
      l_TypeInfo.post_load = nullptr;
      l_TypeInfo.find_by_name = &SS2DDrawCommand::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SS2DDrawCommand::_make;
      l_TypeInfo.duplicate_default = &SS2DDrawCommand::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SS2DDrawCommand::living_instances);
      l_TypeInfo.get_living_count = &SS2DDrawCommand::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: type
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(type);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, type);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_type();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            type, SS2DType);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((SS2DType *)p_Data) = l_Handle.get_type();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: type
      }
      {
        // Property: position
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(position);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, position);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_position();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            position,
                                            Low::Math::Vector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_position(*(Low::Math::Vector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Math::Vector2 *)p_Data) = l_Handle.get_position();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: position
      }
      {
        // Property: half_extents
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(half_extents);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, half_extents);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::VECTOR2;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_half_extents();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            half_extents,
                                            Low::Math::Vector2);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_half_extents(*(Low::Math::Vector2 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Math::Vector2 *)p_Data) =
              l_Handle.get_half_extents();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: half_extents
      }
      {
        // Property: rotation
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rotation);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, rotation);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_rotation();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            rotation, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_rotation(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((float *)p_Data) = l_Handle.get_rotation();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: rotation
      }
      {
        // Property: color
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(color);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, color);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::COLOR;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_color();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            color, Low::Math::Color);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_color(*(Low::Math::Color *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Math::Color *)p_Data) = l_Handle.get_color();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: color
      }
      {
        // Property: corner_radius
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(corner_radius);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, corner_radius);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_corner_radius();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            corner_radius,
                                            Low::Math::Vector4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_corner_radius(*(Low::Math::Vector4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Math::Vector4 *)p_Data) =
              l_Handle.get_corner_radius();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: corner_radius
      }
      {
        // Property: uv_rect
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uv_rect);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, uv_rect);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_uv_rect();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SS2DDrawCommand, uv_rect, Low::Math::Vector4);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_uv_rect(*(Low::Math::Vector4 *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Math::Vector4 *)p_Data) = l_Handle.get_uv_rect();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uv_rect
      }
      {
        // Property: z_sorting
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(z_sorting);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, z_sorting);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_z_sorting();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            z_sorting, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_z_sorting(*(uint32_t *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((uint32_t *)p_Data) = l_Handle.get_z_sorting();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: z_sorting
      }
      {
        // Property: uploaded
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(uploaded);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, uploaded);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.is_uploaded();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            uploaded, bool);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_uploaded(*(bool *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((bool *)p_Data) = l_Handle.is_uploaded();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: uploaded
      }
      {
        // Property: canvas_handle
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(canvas_handle);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, canvas_handle);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_canvas_handle();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            canvas_handle, uint64_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((uint64_t *)p_Data) = l_Handle.get_canvas_handle();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: canvas_handle
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SS2DDrawCommand::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SS2DDrawCommand,
                                            name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      {
        // Virtual property: radius
        Low::Util::RTTI::VirtualPropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(radius);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SS2DDrawCommand> l_HandleLock(
              l_Handle);
          float l_Data = l_Handle.get_radius();
          memcpy(p_Data, &l_Data, sizeof(float));
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SS2DDrawCommand l_Handle = p_Handle.get_id();
          l_Handle.set_radius(*(float *)p_Data);
        };
        l_TypeInfo.virtualProperties[l_PropertyInfo.name] =
            l_PropertyInfo;
        // End virtual property: radius
      }
      {
        // Function: make
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(make);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
        l_FunctionInfo.handleType = SS2DDrawCommand::type_id();
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Name);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Canvas);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::HANDLE;
          l_ParameterInfo.handleType = SS2DCanvas::type_id();
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Type);
          l_ParameterInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: make
      }
      {
        // Function: get_radius
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(get_radius);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_FunctionInfo.handleType = 0;
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: get_radius
      }
      {
        // Function: set_radius
        Low::Util::RTTI::FunctionInfo l_FunctionInfo;
        l_FunctionInfo.name = N(set_radius);
        l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
        l_FunctionInfo.handleType = 0;
        {
          Low::Util::RTTI::ParameterInfo l_ParameterInfo;
          l_ParameterInfo.name = N(p_Value);
          l_ParameterInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_ParameterInfo.handleType = 0;
          l_FunctionInfo.parameters.push_back(l_ParameterInfo);
        }
        l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        // End function: set_radius
      }
      ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                        l_TypeInfo);
      // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
    }

    void SS2DDrawCommand::cleanup()
    {
      Low::Util::List<SS2DDrawCommand> l_Instances =
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
    SS2DDrawCommand::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SS2DDrawCommand SS2DDrawCommand::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SS2DDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SS2DDrawCommand::ms_TypeId;

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

    SS2DDrawCommand
    SS2DDrawCommand::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SS2DDrawCommand l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SS2DDrawCommand::ms_TypeId;

      return l_Handle;
    }

    bool SS2DDrawCommand::is_alive() const
    {
      if (m_Data.m_Type != SS2DDrawCommand::ms_TypeId) {
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
      return m_Data.m_Type == SS2DDrawCommand::ms_TypeId &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SS2DDrawCommand::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SS2DDrawCommand::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SS2DDrawCommand
    SS2DDrawCommand::find_by_name(Low::Util::Name p_Name)
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

    SS2DDrawCommand
    SS2DDrawCommand::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SS2DDrawCommand l_Handle = make(p_Name);
      l_Handle.set_type(get_type());
      l_Handle.set_position(get_position());
      l_Handle.set_half_extents(get_half_extents());
      l_Handle.set_rotation(get_rotation());
      l_Handle.set_color(get_color());
      l_Handle.set_corner_radius(get_corner_radius());
      l_Handle.set_uv_rect(get_uv_rect());
      l_Handle.set_z_sorting(get_z_sorting());
      l_Handle.set_uploaded(is_uploaded());
      l_Handle.set_canvas_handle(get_canvas_handle());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SS2DDrawCommand
    SS2DDrawCommand::duplicate(SS2DDrawCommand p_Handle,
                               Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SS2DDrawCommand::_duplicate(Low::Util::Handle p_Handle,
                                Low::Util::Name p_Name)
    {
      SS2DDrawCommand l_SS2DDrawCommand = p_Handle.get_id();
      return l_SS2DDrawCommand.duplicate(p_Name);
    }

    void
    SS2DDrawCommand::serialize(Low::Util::Serial::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SS2DDrawCommand::serialize(Low::Util::Handle p_Handle,
                                    Low::Util::Serial::Node &p_Node)
    {
      SS2DDrawCommand l_SS2DDrawCommand = p_Handle.get_id();
      l_SS2DDrawCommand.serialize(p_Node);
    }

    Low::Util::Handle
    SS2DDrawCommand::deserialize(Low::Util::Serial::Node &p_Node,
                                 Low::Util::Handle p_Creator)
    {

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      return Low::Util::Handle::DEAD;
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
    }

    void SS2DDrawCommand::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SS2DDrawCommand::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SS2DDrawCommand::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SS2DDrawCommand::notify(Low::Util::Handle p_Observed,
                                 Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SS2DDrawCommand::_notify(Low::Util::Handle p_Observer,
                                  Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
    {
      SS2DDrawCommand l_SS2DDrawCommand = p_Observer.get_id();
      l_SS2DDrawCommand.notify(p_Observed, p_Observable);
    }

    SS2DType &SS2DDrawCommand::get_type() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_type

      // LOW_CODEGEN::END::CUSTOM:GETTER_type

      return TYPE_SOA(SS2DDrawCommand, type, SS2DType);
    }
    void SS2DDrawCommand::set_type(SS2DType &p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_type

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_type

      // Set new value
      TYPE_SOA(SS2DDrawCommand, type, SS2DType) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_type

      // LOW_CODEGEN::END::CUSTOM:SETTER_type

      broadcast_observable(N(type));
    }

    Low::Math::Vector2 SS2DDrawCommand::get_position() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_position

      // LOW_CODEGEN::END::CUSTOM:GETTER_position

      return TYPE_SOA(SS2DDrawCommand, position, Low::Math::Vector2);
    }
    void SS2DDrawCommand::set_position(float p_X, float p_Y)
    {
      Low::Math::Vector2 l_Val(p_X, p_Y);
      set_position(l_Val);
    }

    void SS2DDrawCommand::set_position_x(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_position();
      l_Value.x = p_Value;
      set_position(l_Value);
    }

    void SS2DDrawCommand::set_position_y(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_position();
      l_Value.y = p_Value;
      set_position(l_Value);
    }

    void SS2DDrawCommand::set_position(Low::Math::Vector2 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position

      if (get_position() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, position, Low::Math::Vector2) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position

        // LOW_CODEGEN::END::CUSTOM:SETTER_position

        broadcast_observable(N(position));
      }
    }

    Low::Math::Vector2 SS2DDrawCommand::get_half_extents() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_half_extents

      // LOW_CODEGEN::END::CUSTOM:GETTER_half_extents

      return TYPE_SOA(SS2DDrawCommand, half_extents,
                      Low::Math::Vector2);
    }
    void SS2DDrawCommand::set_half_extents(float p_X, float p_Y)
    {
      Low::Math::Vector2 l_Val(p_X, p_Y);
      set_half_extents(l_Val);
    }

    void SS2DDrawCommand::set_half_extents_x(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_half_extents();
      l_Value.x = p_Value;
      set_half_extents(l_Value);
    }

    void SS2DDrawCommand::set_half_extents_y(float p_Value)
    {
      Low::Math::Vector2 l_Value = get_half_extents();
      l_Value.y = p_Value;
      set_half_extents(l_Value);
    }

    void SS2DDrawCommand::set_half_extents(Low::Math::Vector2 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_half_extents

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_half_extents

      if (get_half_extents() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, half_extents, Low::Math::Vector2) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_half_extents

        // LOW_CODEGEN::END::CUSTOM:SETTER_half_extents

        broadcast_observable(N(half_extents));
      }
    }

    float SS2DDrawCommand::get_rotation() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation

      // LOW_CODEGEN::END::CUSTOM:GETTER_rotation

      return TYPE_SOA(SS2DDrawCommand, rotation, float);
    }
    void SS2DDrawCommand::set_rotation(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation

      if (get_rotation() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, rotation, float) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation

        // LOW_CODEGEN::END::CUSTOM:SETTER_rotation

        broadcast_observable(N(rotation));
      }
    }

    Low::Math::Color SS2DDrawCommand::get_color() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color

      // LOW_CODEGEN::END::CUSTOM:GETTER_color

      return TYPE_SOA(SS2DDrawCommand, color, Low::Math::Color);
    }
    void SS2DDrawCommand::set_color(Low::Math::Color p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

      if (get_color() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, color, Low::Math::Color) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color

        // LOW_CODEGEN::END::CUSTOM:SETTER_color

        broadcast_observable(N(color));
      }
    }

    Low::Math::Vector4 SS2DDrawCommand::get_corner_radius() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_corner_radius

      // LOW_CODEGEN::END::CUSTOM:GETTER_corner_radius

      return TYPE_SOA(SS2DDrawCommand, corner_radius,
                      Low::Math::Vector4);
    }
    void
    SS2DDrawCommand::set_corner_radius(Low::Math::Vector4 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_corner_radius

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_corner_radius

      if (get_corner_radius() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, corner_radius, Low::Math::Vector4) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_corner_radius

        // LOW_CODEGEN::END::CUSTOM:SETTER_corner_radius

        broadcast_observable(N(corner_radius));
      }
    }

    Low::Math::Vector4 SS2DDrawCommand::get_uv_rect() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uv_rect

      // LOW_CODEGEN::END::CUSTOM:GETTER_uv_rect

      return TYPE_SOA(SS2DDrawCommand, uv_rect, Low::Math::Vector4);
    }
    void SS2DDrawCommand::set_uv_rect(Low::Math::Vector4 p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uv_rect

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uv_rect

      if (get_uv_rect() != p_Value) {
        // Set dirty flags
        mark_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, uv_rect, Low::Math::Vector4) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uv_rect

        // LOW_CODEGEN::END::CUSTOM:SETTER_uv_rect

        broadcast_observable(N(uv_rect));
      }
    }

    uint32_t SS2DDrawCommand::get_z_sorting() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_z_sorting

      // LOW_CODEGEN::END::CUSTOM:GETTER_z_sorting

      return TYPE_SOA(SS2DDrawCommand, z_sorting, uint32_t);
    }
    void SS2DDrawCommand::set_z_sorting(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_z_sorting

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_z_sorting

      if (get_z_sorting() != p_Value) {
        // Set dirty flags
        mark_dirty();
        mark_z_dirty();

        // Set new value
        TYPE_SOA(SS2DDrawCommand, z_sorting, uint32_t) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_z_sorting

        // LOW_CODEGEN::END::CUSTOM:SETTER_z_sorting

        broadcast_observable(N(z_sorting));
      }
    }

    bool SS2DDrawCommand::is_uploaded() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_uploaded

      // LOW_CODEGEN::END::CUSTOM:GETTER_uploaded

      return TYPE_SOA(SS2DDrawCommand, uploaded, bool);
    }
    void SS2DDrawCommand::toggle_uploaded()
    {
      set_uploaded(!is_uploaded());
    }

    void SS2DDrawCommand::set_uploaded(bool p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_uploaded

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_uploaded

      // Set new value
      TYPE_SOA(SS2DDrawCommand, uploaded, bool) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_uploaded

      // LOW_CODEGEN::END::CUSTOM:SETTER_uploaded

      broadcast_observable(N(uploaded));
    }

    uint64_t SS2DDrawCommand::get_canvas_handle() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:GETTER_canvas_handle

      return TYPE_SOA(SS2DDrawCommand, canvas_handle, uint64_t);
    }
    void SS2DDrawCommand::set_canvas_handle(uint64_t p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas_handle

      // Set new value
      TYPE_SOA(SS2DDrawCommand, canvas_handle, uint64_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas_handle

      // LOW_CODEGEN::END::CUSTOM:SETTER_canvas_handle

      broadcast_observable(N(canvas_handle));
    }

    void SS2DDrawCommand::mark_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty

      ms_Dirty.insert(get_id());
      // LOW_CODEGEN::END::CUSTOM:MARK_dirty
    }

    void SS2DDrawCommand::mark_z_dirty()
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:MARK_z_dirty

      SS2DCanvas l_Canvas = get_canvas_handle();
      l_Canvas.mark_z_dirty();
      // LOW_CODEGEN::END::CUSTOM:MARK_z_dirty
    }

    Low::Util::Name SS2DDrawCommand::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SS2DDrawCommand, name, Low::Util::Name);
    }
    void SS2DDrawCommand::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SS2DDrawCommand, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    SS2DDrawCommand SS2DDrawCommand::make(Low::Util::Name p_Name,
                                          SS2DCanvas p_Canvas,
                                          SS2DType p_Type)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

      SS2DDrawCommand l_DrawCommand = make(p_Name);

      l_DrawCommand.set_type(p_Type);
      l_DrawCommand.set_canvas_handle(p_Canvas.get_id());

      p_Canvas.get_draw_commands().push_back(l_DrawCommand);

      return l_DrawCommand;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
    }

    float SS2DDrawCommand::get_radius()
    {
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_radius

      return get_half_extents().x;
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_radius
    }

    void SS2DDrawCommand::set_radius(float p_Value)
    {
      Low::Util::HandleLock<SS2DDrawCommand> l_Lock(get_id());
      // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_set_radius

      set_half_extents_x(p_Value);
      // LOW_CODEGEN::END::CUSTOM:FUNCTION_set_radius
    }

    uint32_t SS2DDrawCommand::create_instance(
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
      LOW_ASSERT(l_FoundIndex,
                 "Budget blown for type SS2DDrawCommand");
      ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
      p_PageIndex = l_PageIndex;
      p_SlotIndex = l_SlotIndex;
      p_PageLock = std::move(l_PageLock);
      LOCK_UNLOCK(l_PagesLock);
      return l_Index;
    }

    u32 SS2DDrawCommand::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                 "Could not increase capacity for SS2DDrawCommand.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SS2DDrawCommand::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SS2DDrawCommand::get_page_for_index(const u32 p_Index,
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
