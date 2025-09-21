#include "LowCoreUiView.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowUtilFileIO.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t View::TYPE_ID = 38;
      uint32_t View::ms_Capacity = 0u;
      uint32_t View::ms_PageSize = 0u;
      Low::Util::SharedMutex View::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          View::ms_PagesLock(View::ms_PagesMutex, std::defer_lock);
      Low::Util::List<View> View::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> View::ms_Pages;

      View::View() : Low::Util::Handle(0ull)
      {
      }
      View::View(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      View::View(View &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle View::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      View View::make(Low::Util::Name p_Name)
      {
        return make(p_Name, 0ull);
      }

      View View::make(Low::Util::Name p_Name,
                      Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        View l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<View> l_HandleLock(l_Handle);

        ACCESSOR_TYPE_SOA(l_Handle, View, loaded, bool) = false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, View, elements,
                                   Util::Set<Util::UniqueId>))
            Util::Set<Util::UniqueId>();
        ACCESSOR_TYPE_SOA(l_Handle, View, internal, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, View, view_template, bool) =
            false;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, View, pixel_position,
                                   Low::Math::Vector2))
            Low::Math::Vector2();
        ACCESSOR_TYPE_SOA(l_Handle, View, rotation, float) = 0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, View, scale_multiplier, float) =
            0.0f;
        ACCESSOR_TYPE_SOA(l_Handle, View, transform_dirty, bool) =
            false;
        ACCESSOR_TYPE_SOA(l_Handle, View, name, Low::Util::Name) =
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

        l_Handle.set_internal(false);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void View::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<View> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          Util::List<Element> l_ElementsToDelete;
          for (u64 i_ElementId : get_elements()) {
            Element i_Element =
                Util::find_handle_by_unique_id(i_ElementId);

            if (!i_Element.is_alive()) {
              continue;
            }

            Component::Display i_Parent = 0;

            if (i_Element.get_display().is_alive()) {
              i_Parent = i_Element.get_display().get_parent();
            }

            if (!i_Parent.is_alive()) {
              l_ElementsToDelete.push_back(i_Element);
            }
          }

          for (Element i_Element : l_ElementsToDelete) {
            i_Element.destroy();
          }
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
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

      void View::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(View));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, View::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(View);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &View::is_alive;
        l_TypeInfo.destroy = &View::destroy;
        l_TypeInfo.serialize = &View::serialize;
        l_TypeInfo.deserialize = &View::deserialize;
        l_TypeInfo.find_by_index = &View::_find_by_index;
        l_TypeInfo.notify = &View::_notify;
        l_TypeInfo.find_by_name = &View::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &View::_make;
        l_TypeInfo.duplicate_default = &View::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &View::living_instances);
        l_TypeInfo.get_living_count = &View::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: loaded
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(loaded);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(View::Data, loaded);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.is_loaded();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, loaded,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_loaded(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_loaded();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: loaded
        }
        {
          // Property: elements
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(elements);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(View::Data, elements);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.get_elements();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, elements, Util::Set<Util::UniqueId>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((Util::Set<Util::UniqueId> *)p_Data) =
                l_Handle.get_elements();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: elements
        }
        {
          // Property: internal
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(internal);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(View::Data, internal);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.is_internal();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              internal, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_internal();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: internal
        }
        {
          // Property: view_template
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view_template);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(View::Data, view_template);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.is_view_template();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              view_template, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_view_template(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_view_template();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: view_template
        }
        {
          // Property: pixel_position
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pixel_position);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(View::Data, pixel_position);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR2;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.pixel_position();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, pixel_position, Low::Math::Vector2);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.pixel_position(*(Low::Math::Vector2 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((Low::Math::Vector2 *)p_Data) =
                l_Handle.pixel_position();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: pixel_position
        }
        {
          // Property: rotation
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rotation);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(View::Data, rotation);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.rotation();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              rotation, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.rotation(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((float *)p_Data) = l_Handle.rotation();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: rotation
        }
        {
          // Property: scale_multiplier
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(scale_multiplier);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(View::Data, scale_multiplier);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.scale_multiplier();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, scale_multiplier, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.scale_multiplier(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((float *)p_Data) = l_Handle.scale_multiplier();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: scale_multiplier
        }
        {
          // Property: layer_offset
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(layer_offset);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(View::Data, layer_offset);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.layer_offset();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              layer_offset, uint32_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.layer_offset(*(uint32_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((uint32_t *)p_Data) = l_Handle.layer_offset();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: layer_offset
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(View::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: transform_dirty
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(transform_dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(View::Data, transform_dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.is_transform_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              transform_dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_transform_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_transform_dirty();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: transform_dirty
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(View::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            View l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<View> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: serialize_elements
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(serialize_elements);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Node);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: serialize_elements
        }
        {
          // Function: add_element
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(add_element);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Element);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Element::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: add_element
        }
        {
          // Function: remove_element
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(remove_element);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Element);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Element::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: remove_element
        }
        {
          // Function: load_elements
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(load_elements);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: load_elements
        }
        {
          // Function: unload_elements
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(unload_elements);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: unload_elements
        }
        {
          // Function: spawn_instance
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(spawn_instance);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Low::Core::UI::View::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: spawn_instance
        }
        {
          // Function: find_element_by_name
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(find_element_by_name);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = Low::Core::UI::Element::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: find_element_by_name
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void View::cleanup()
      {
        Low::Util::List<View> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle View::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      View View::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        View l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

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

      View View::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        View l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        return l_Handle;
      }

      bool View::is_alive() const
      {
        if (m_Data.m_Type != View::TYPE_ID) {
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
        return m_Data.m_Type == View::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t View::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle View::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      View View::find_by_name(Low::Util::Name p_Name)
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

      View View::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        LOW_ASSERT(is_loaded(), "Cannot duplicate unloaded view");

        View l_ClonedView = make(p_Name);
        l_ClonedView.set_view_template(is_view_template());
        l_ClonedView.set_loaded(true);
        l_ClonedView.set_internal(is_internal());

        Util::Set<Util::UniqueId> l_Elements = get_elements();

        for (Util::UniqueId i_UniqueId : l_Elements) {
          Element i_Element =
              Util::find_handle_by_unique_id(i_UniqueId).get_id();

          if (!i_Element.is_alive()) {
            continue;
          }

          Component::Display i_Parent =
              i_Element.get_display().get_parent();

          if (i_Parent.is_alive()) {
            continue;
          }

          Element i_CopiedElement =
              i_Element.duplicate(i_Element.get_name());
          l_ClonedView.add_element(i_CopiedElement);
        }

        return l_ClonedView;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      View View::duplicate(View p_Handle, Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle View::_duplicate(Low::Util::Handle p_Handle,
                                         Low::Util::Name p_Name)
      {
        View l_View = p_Handle.get_id();
        return l_View.duplicate(p_Name);
      }

      void View::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["view_template"] = is_view_template();
        Low::Util::Serialization::serialize(p_Node["pixel_position"],
                                            pixel_position());
        p_Node["rotation"] = rotation();
        p_Node["scale_multiplier"] = scale_multiplier();
        p_Node["layer_offset"] = layer_offset();
        p_Node["_unique_id"] =
            Low::Util::hash_to_string(get_unique_id()).c_str();
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void View::serialize(Low::Util::Handle p_Handle,
                           Low::Util::Yaml::Node &p_Node)
      {
        View l_View = p_Handle.get_id();
        l_View.serialize(p_Node);
      }

      Low::Util::Handle
      View::deserialize(Low::Util::Yaml::Node &p_Node,
                        Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              LOW_YAML_AS_STRING(p_Node["_unique_id"]));
        }

        View l_Handle = View::make(N(View), l_HandleUniqueId);

        if (p_Node["view_template"]) {
          l_Handle.set_view_template(
              p_Node["view_template"].as<bool>());
        }
        if (p_Node["pixel_position"]) {
          l_Handle.pixel_position(
              Low::Util::Serialization::deserialize_vector2(
                  p_Node["pixel_position"]));
        }
        if (p_Node["rotation"]) {
          l_Handle.rotation(p_Node["rotation"].as<float>());
        }
        if (p_Node["scale_multiplier"]) {
          l_Handle.scale_multiplier(
              p_Node["scale_multiplier"].as<float>());
        }
        if (p_Node["layer_offset"]) {
          l_Handle.layer_offset(
              p_Node["layer_offset"].as<uint32_t>());
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
      View::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 View::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 View::observe(Low::Util::Name p_Observable,
                        Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void View::notify(Low::Util::Handle p_Observed,
                        Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void View::_notify(Low::Util::Handle p_Observer,
                         Low::Util::Handle p_Observed,
                         Low::Util::Name p_Observable)
      {
        View l_View = p_Observer.get_id();
        l_View.notify(p_Observed, p_Observable);
      }

      bool View::is_loaded() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded

        // LOW_CODEGEN::END::CUSTOM:GETTER_loaded

        return TYPE_SOA(View, loaded, bool);
      }
      void View::toggle_loaded()
      {
        set_loaded(!is_loaded());
      }

      void View::set_loaded(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

        // Set new value
        TYPE_SOA(View, loaded, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded

        // LOW_CODEGEN::END::CUSTOM:SETTER_loaded

        broadcast_observable(N(loaded));
      }

      Util::Set<Util::UniqueId> &View::get_elements() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_elements

        // LOW_CODEGEN::END::CUSTOM:GETTER_elements

        return TYPE_SOA(View, elements, Util::Set<Util::UniqueId>);
      }

      bool View::is_internal() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal

        // LOW_CODEGEN::END::CUSTOM:GETTER_internal

        return TYPE_SOA(View, internal, bool);
      }
      void View::toggle_internal()
      {
        set_internal(!is_internal());
      }

      void View::set_internal(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

        // Set new value
        TYPE_SOA(View, internal, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal

        // LOW_CODEGEN::END::CUSTOM:SETTER_internal

        broadcast_observable(N(internal));
      }

      bool View::is_view_template() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_template

        // LOW_CODEGEN::END::CUSTOM:GETTER_view_template

        return TYPE_SOA(View, view_template, bool);
      }
      void View::toggle_view_template()
      {
        set_view_template(!is_view_template());
      }

      void View::set_view_template(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_template

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_template

        // Set new value
        TYPE_SOA(View, view_template, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_template

        // LOW_CODEGEN::END::CUSTOM:SETTER_view_template

        broadcast_observable(N(view_template));
      }

      Low::Math::Vector2 &View::pixel_position() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_position

        // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_position

        return TYPE_SOA(View, pixel_position, Low::Math::Vector2);
      }
      void View::pixel_position(float p_X, float p_Y)
      {
        Low::Math::Vector2 l_Val(p_X, p_Y);
        pixel_position(l_Val);
      }

      void View::pixel_position_x(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_position();
        l_Value.x = p_Value;
        pixel_position(l_Value);
      }

      void View::pixel_position_y(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_position();
        l_Value.y = p_Value;
        pixel_position(l_Value);
      }

      void View::pixel_position(Low::Math::Vector2 &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_position

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_position

        if (pixel_position() != p_Value) {
          // Set dirty flags
          mark_transform_dirty();

          // Set new value
          TYPE_SOA(View, pixel_position, Low::Math::Vector2) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_position

          // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_position

          broadcast_observable(N(pixel_position));
        }
      }

      float View::rotation() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation

        // LOW_CODEGEN::END::CUSTOM:GETTER_rotation

        return TYPE_SOA(View, rotation, float);
      }
      void View::rotation(float p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation

        if (rotation() != p_Value) {
          // Set dirty flags
          mark_transform_dirty();

          // Set new value
          TYPE_SOA(View, rotation, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation

          // LOW_CODEGEN::END::CUSTOM:SETTER_rotation

          broadcast_observable(N(rotation));
        }
      }

      float View::scale_multiplier() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_scale_multiplier

        // LOW_CODEGEN::END::CUSTOM:GETTER_scale_multiplier

        return TYPE_SOA(View, scale_multiplier, float);
      }
      void View::scale_multiplier(float p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scale_multiplier

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_scale_multiplier

        if (scale_multiplier() != p_Value) {
          // Set dirty flags
          mark_transform_dirty();

          // Set new value
          TYPE_SOA(View, scale_multiplier, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale_multiplier

          // LOW_CODEGEN::END::CUSTOM:SETTER_scale_multiplier

          broadcast_observable(N(scale_multiplier));
        }
      }

      uint32_t View::layer_offset() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_layer_offset

        // LOW_CODEGEN::END::CUSTOM:GETTER_layer_offset

        return TYPE_SOA(View, layer_offset, uint32_t);
      }
      void View::layer_offset(uint32_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_layer_offset

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_layer_offset

        if (layer_offset() != p_Value) {
          // Set dirty flags
          mark_transform_dirty();

          // Set new value
          TYPE_SOA(View, layer_offset, uint32_t) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_layer_offset

          // LOW_CODEGEN::END::CUSTOM:SETTER_layer_offset

          broadcast_observable(N(layer_offset));
        }
      }

      Low::Util::UniqueId View::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(View, unique_id, Low::Util::UniqueId);
      }
      void View::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(View, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool View::is_transform_dirty() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transform_dirty

        // LOW_CODEGEN::END::CUSTOM:GETTER_transform_dirty

        return TYPE_SOA(View, transform_dirty, bool);
      }
      void View::toggle_transform_dirty()
      {
        set_transform_dirty(!is_transform_dirty());
      }

      void View::set_transform_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transform_dirty

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_transform_dirty

        // Set new value
        TYPE_SOA(View, transform_dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transform_dirty

        // LOW_CODEGEN::END::CUSTOM:SETTER_transform_dirty

        broadcast_observable(N(transform_dirty));
      }

      void View::mark_transform_dirty()
      {
        if (!is_transform_dirty()) {
          TYPE_SOA(View, transform_dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_transform_dirty
          // LOW_CODEGEN::END::CUSTOM:MARK_transform_dirty
        }
      }

      Low::Util::Name View::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(View, name, Low::Util::Name);
      }
      void View::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<View> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(View, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      void View::serialize_elements(Util::Yaml::Node &p_Node)
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_elements

        for (auto it = get_elements().begin();
             it != get_elements().end(); ++it) {
          Element i_Element =
              Util::find_handle_by_unique_id(*it).get_id();
          if (i_Element.is_alive()) {
            Util::Yaml::Node i_Node;
            i_Element.serialize(i_Node);
            p_Node["elements"].push_back(i_Node);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_elements
      }

      void View::add_element(Element p_Element)
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_element

        if (p_Element.get_view().is_alive()) {
          p_Element.get_view().remove_element(p_Element);
        }

        p_Element.set_view(*this);
        get_elements().insert(p_Element.get_unique_id());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_element
      }

      void View::remove_element(Element p_Element)
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_element

        p_Element.set_view(0);
        get_elements().erase(p_Element.get_unique_id());
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_element
      }

      void View::load_elements()
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load_elements

        LOW_ASSERT(is_alive(), "Cannot load dead view");
        LOW_ASSERT(!is_loaded(), "View is already loaded");

        set_loaded(true);

        Util::String l_Path =
            Util::get_project().dataPath + "\\assets\\ui_views\\";
        l_Path += std::to_string(get_unique_id()).c_str();
        l_Path += ".elements.yaml";

        if (!Util::FileIO::file_exists_sync(l_Path.c_str())) {
          return;
        }

        Util::Yaml::Node l_RootNode =
            Util::Yaml::load_file(l_Path.c_str());
        Util::Yaml::Node &l_ElementsNode = l_RootNode["elements"];

        for (auto it = l_ElementsNode.begin();
             it != l_ElementsNode.end(); ++it) {
          Util::Yaml::Node &i_ElementNode = *it;
          Element::deserialize(i_ElementNode, *this);
        }

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_load_elements
      }

      void View::unload_elements()
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_unload_elements

        LOW_ASSERT(is_alive(), "Cannot unload dead view");
        LOW_ASSERT(is_loaded(),
                   "Cannot unload view that is not loaded");

        set_loaded(false);

        while (!get_elements().empty()) {
          Element i_Element =
              Util::find_handle_by_unique_id(*get_elements().begin())
                  .get_id();
          i_Element.destroy();
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_unload_elements
      }

      Low::Core::UI::View View::spawn_instance(Low::Util::Name p_Name)
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_instance

        _LOW_ASSERT(is_alive());
        LOW_ASSERT(is_view_template(),
                   "Cannot spawn instances of UI-Views that are not "
                   "marked as templates.");

        View l_View = duplicate(p_Name);

        l_View.set_view_template(false);
        l_View.set_internal(true);

        return l_View;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_instance
      }

      Low::Core::UI::Element
      View::find_element_by_name(Low::Util::Name p_Name)
      {
        Low::Util::HandleLock<View> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_find_element_by_name

        _LOW_ASSERT(is_alive());
        _LOW_ASSERT(is_loaded());

        for (u64 i_ElementId : get_elements()) {
          Element l_Element =
              Util::find_handle_by_unique_id(i_ElementId);
          if (l_Element.get_name() == p_Name) {
            return l_Element;
          }
        }

        return 0;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_find_element_by_name
      }

      uint32_t View::create_instance(
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
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
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

      u32 View::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for View.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, View::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool View::get_page_for_index(const u32 p_Index,
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

    } // namespace UI
  } // namespace Core
} // namespace Low
