#include "LowCoreUiWidgetAsset.h"

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
#include "LowUtilAssetManager.h"
#include "LowUtilString.h"
#include "LowCoreUiDisplay.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 WidgetAsset::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          WidgetAsset::IDENTIFIER(LOW_NAME(1181529166),
                                  LOW_NAME(2149613867));
      uint32_t WidgetAsset::ms_Capacity = 0u;
      uint32_t WidgetAsset::ms_PageSize = 0u;
      Low::Util::SharedMutex WidgetAsset::ms_LivingMutex;
      Low::Util::SharedMutex WidgetAsset::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          WidgetAsset::ms_PagesLock(WidgetAsset::ms_PagesMutex,
                                    std::defer_lock);
      Low::Util::List<WidgetAsset> WidgetAsset::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          WidgetAsset::ms_Pages;

      Low::Util::Handle WidgetAsset::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      WidgetAsset WidgetAsset::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        WidgetAsset l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = WidgetAsset::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, WidgetAsset, state,
                                   Low::Core::UI::LoadState))
            Low::Core::UI::LoadState();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, WidgetAsset, content,
            Low::Util::List<Low::Core::UI::ElementDescriptor>))
            Low::Util::List<Low::Core::UI::ElementDescriptor>();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, WidgetAsset, controller,
                                   Low::Core::UI::Controller))
            Low::Core::UI::Controller();
        ACCESSOR_TYPE_SOA(l_Handle, WidgetAsset,
                          has_custom_controller, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, WidgetAsset, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        l_Handle.set_state(LoadState::Unloaded);
        l_Handle.set_local_element_id_counter(1);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void WidgetAsset::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

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

      void WidgetAsset::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(WidgetAsset));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowCore),
                                                      N(WidgetAsset));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, WidgetAsset::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(WidgetAsset);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &WidgetAsset::is_alive;
        l_TypeInfo.destroy = &WidgetAsset::destroy;
        l_TypeInfo.serialize = &WidgetAsset::serialize;
        l_TypeInfo.deserialize = &WidgetAsset::deserialize;
        l_TypeInfo.find_by_index = &WidgetAsset::_find_by_index;
        l_TypeInfo.notify = &WidgetAsset::_notify;
        l_TypeInfo.post_load = &WidgetAsset::_post_load;
        l_TypeInfo.find_by_name = &WidgetAsset::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &WidgetAsset::_make;
        l_TypeInfo.duplicate_default = &WidgetAsset::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &WidgetAsset::living_instances);
        l_TypeInfo.get_living_count = &WidgetAsset::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: state
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(state);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, state);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_state();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetAsset, state,
                Low::Core::UI::LoadState);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.set_state(*(Low::Core::UI::LoadState *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((Low::Core::UI::LoadState *)p_Data) =
                l_Handle.get_state();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: state
        }
        {
          // Property: content
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(content);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, content);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_content();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetAsset, content,
                Low::Util::List<Low::Core::UI::ElementDescriptor>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.set_content(
                *(Low::Util::List<Low::Core::UI::ElementDescriptor> *)
                    p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((Low::Util::List<Low::Core::UI::ElementDescriptor> *)
                  p_Data) = l_Handle.get_content();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: content
        }
        {
          // Property: path
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(path);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, path);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_path();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetAsset, path, Low::Util::String);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((Low::Util::String *)p_Data) = l_Handle.get_path();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: path
        }
        {
          // Property: controller
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(controller);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, controller);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::UI::Controller::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_controller();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetAsset, controller,
                Low::Core::UI::Controller);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.set_controller(
                *(Low::Core::UI::Controller *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((Low::Core::UI::Controller *)p_Data) =
                l_Handle.get_controller();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: controller
        }
        {
          // Property: has_custom_controller
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(has_custom_controller);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, has_custom_controller);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.has_custom_controller();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetAsset, has_custom_controller, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.has_custom_controller(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.has_custom_controller();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: has_custom_controller
        }
        {
          // Property: local_element_id_counter
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(local_element_id_counter);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, local_element_id_counter);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
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
          // End property: local_element_id_counter
        }
        {
          // Property: custom_controller_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(custom_controller_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, custom_controller_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_custom_controller_id();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, WidgetAsset,
                                              custom_controller_id,
                                              uint64_t);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.set_custom_controller_id(*(uint64_t *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((uint64_t *)p_Data) =
                l_Handle.get_custom_controller_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: custom_controller_id
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetAsset::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, WidgetAsset,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetAsset l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetAsset l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetAsset> l_HandleLock(l_Handle);
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: get_next_local_id
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_next_local_id);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_FunctionInfo.handleType = 0;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_next_local_id
        }
        {
          // Function: parse_content
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(parse_content);
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
          // End function: parse_content
        }
        {
          // Function: parse_element
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(parse_element);
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
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Descriptor);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: parse_element
        }
        {
          // Function: spawn_instance
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(spawn_instance);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = WidgetInstance::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Canvas);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Renderer::UiCanvas::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: spawn_instance
        }
        {
          // Function: spawn_element
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(spawn_element);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::UI::Element::type_id();
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Instance);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::UI::WidgetInstance::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Canvas);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Renderer::UiCanvas::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Descriptor);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Parent);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::UI::Element::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: spawn_element
        }
        {
          // Function: fill_element_descriptor
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(fill_element_descriptor);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Element);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::UI::Element::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Descriptor);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: fill_element_descriptor
        }
        {
          // Function: fill_content_from_instance
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(fill_content_from_instance);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Instance);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::UI::WidgetInstance::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: fill_content_from_instance
        }
        {
          // Function: serialize_element_descriptor
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(serialize_element_descriptor);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Descriptor);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Node);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: serialize_element_descriptor
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        {
          Util::AssetManager::TypeRegistratorBuilder l_Builder(
              N(UiWidget), IDENTIFIER);
          l_Builder.auto_initialize(true)
              .initialize_on_startup(true)
              .add_asset_suffix(".uiwidget.yaml");
          l_Builder
              .add_initialize_directory(Util::get_project().dataPath,
                                        true)
              .supports_loading(true)
              .is_loadable([](Util::Handle p_Handle) -> bool {
                WidgetAsset l_Asset = p_Handle.get_id();
                return l_Asset.is_alive() &&
                       l_Asset.get_state() == LoadState::Unloaded;
              })
              .load_path_property_name(N(path));
          l_Builder
              .initializer([](const Util::String p_Path)
                               -> Util::Handle {
                const Util::String l_FileName =
                    Util::PathHelper::get_base_name_no_ext(p_Path);
                WidgetAsset l_Asset =
                    WidgetAsset::make(LOW_NAME(l_FileName.c_str()));
                l_Asset.set_path(p_Path);
                return l_Asset.get_id();
              })
              .creatable()
              .creator([](const Util::Name p_Name,
                          const Util::String p_Path) -> Util::Handle {
                WidgetAsset l_Asset = WidgetAsset::make(p_Name);
                l_Asset.set_path(p_Path);

                return l_Asset.get_id();
              });

          Util::AssetManager::register_asset_type(l_Builder.build());
        }
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void WidgetAsset::cleanup()
      {
        Low::Util::List<WidgetAsset> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle WidgetAsset::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      WidgetAsset WidgetAsset::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        WidgetAsset l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = WidgetAsset::ms_TypeId;

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

      WidgetAsset WidgetAsset::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        WidgetAsset l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = WidgetAsset::ms_TypeId;

        return l_Handle;
      }

      bool WidgetAsset::is_alive() const
      {
        if (m_Data.m_Type != WidgetAsset::ms_TypeId) {
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
        return m_Data.m_Type == WidgetAsset::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t WidgetAsset::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      WidgetAsset::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      WidgetAsset WidgetAsset::find_by_name(Low::Util::Name p_Name)
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

      WidgetAsset WidgetAsset::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      WidgetAsset WidgetAsset::duplicate(WidgetAsset p_Handle,
                                         Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      WidgetAsset::_duplicate(Low::Util::Handle p_Handle,
                              Low::Util::Name p_Name)
      {
        WidgetAsset l_WidgetAsset = p_Handle.get_id();
        return l_WidgetAsset.duplicate(p_Name);
      }

      void
      WidgetAsset::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        p_Node["name"] = get_name();
        Util::Serial::Node &l_ContentNode =
            p_Node["content"]["elements"];

        for (ElementDescriptor &i_Element : get_content()) {
          Util::Serial::Node i_ElementNode;
          serialize_element_descriptor(i_Element, i_ElementNode);

          l_ContentNode.push_back(i_ElementNode);
        }

        if (get_controller().is_alive()) {
          p_Node["controller"] = get_controller().get_name();
          p_Node["custom_controller"] = has_custom_controller();
          if (has_custom_controller()) {
            p_Node["custom_controller_id"] =
                Util::U64Id{get_custom_controller_id()};
          }
        }

        p_Node["local_element_id_counter"] =
            Util::U64Id{get_local_element_id_counter()};
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void WidgetAsset::serialize(Low::Util::Handle p_Handle,
                                  Low::Util::Serial::Node &p_Node)
      {
        WidgetAsset l_WidgetAsset = p_Handle.get_id();
        l_WidgetAsset.serialize(p_Node);
      }

      Low::Util::Handle
      WidgetAsset::deserialize(Low::Util::Serial::Node &p_Node,
                               Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        return Low::Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void WidgetAsset::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      WidgetAsset::observe(Low::Util::Name p_Observable,
                           Low::Util::Function<void(Low::Util::Handle,
                                                    Low::Util::Name)>
                               p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 WidgetAsset::observe(Low::Util::Name p_Observable,
                               Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void WidgetAsset::notify(Low::Util::Handle p_Observed,
                               Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void WidgetAsset::_notify(Low::Util::Handle p_Observer,
                                Low::Util::Handle p_Observed,
                                Low::Util::Name p_Observable)
      {
        WidgetAsset l_WidgetAsset = p_Observer.get_id();
        l_WidgetAsset.notify(p_Observed, p_Observable);
      }

      void WidgetAsset::post_load(Low::Util::Serial::Node &p_Node)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:POST_LOAD
        set_state(LoadState::Loaded);

        if (p_Node["local_element_id_counter"]) {
          set_local_element_id_counter(
              p_Node["local_element_id_counter"].as<Util::U64Id>());
        } else {
          set_local_element_id_counter(1);
        }

        get_content().clear();
        if (p_Node["content"]) {
          parse_content(p_Node["content"]);
        }

        if (p_Node["controller"]) {
          if (p_Node["custom_controller"]) {
            has_custom_controller(
                p_Node["custom_controller"].as<bool>());

            if (p_Node["custom_controller_id"]) {
              set_custom_controller_id(
                  p_Node["custom_controller_id"].as<Util::U64Id>());
            }
          }
          Util::Name l_ControllerName =
              p_Node["controller"].as<Util::Name>();
          Controller l_Controller =
              Controller::find_by_name(l_ControllerName);
          if (l_Controller.is_alive()) {
            set_controller(l_Controller);
          } else {
            Util::resolve_handle_reference_by_name(
                get_id(), N(controller), l_ControllerName);
          }
        }

        // LOW_CODEGEN::END::CUSTOM:POST_LOAD
      }

      Low::Core::UI::LoadState WidgetAsset::get_state() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_state
        // LOW_CODEGEN::END::CUSTOM:GETTER_state

        return TYPE_SOA(WidgetAsset, state, Low::Core::UI::LoadState);
      }
      void WidgetAsset::set_state(Low::Core::UI::LoadState p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_state
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_state

        // Set new value
        TYPE_SOA(WidgetAsset, state, Low::Core::UI::LoadState) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_state
        // LOW_CODEGEN::END::CUSTOM:SETTER_state

        broadcast_observable(N(state));
      }

      Low::Util::List<Low::Core::UI::ElementDescriptor> &
      WidgetAsset::get_content() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_content
        // LOW_CODEGEN::END::CUSTOM:GETTER_content

        return TYPE_SOA(
            WidgetAsset, content,
            Low::Util::List<Low::Core::UI::ElementDescriptor>);
      }
      void WidgetAsset::set_content(
          Low::Util::List<Low::Core::UI::ElementDescriptor> &p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_content
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_content

        // Set new value
        TYPE_SOA(WidgetAsset, content,
                 Low::Util::List<Low::Core::UI::ElementDescriptor>) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_content
        // LOW_CODEGEN::END::CUSTOM:SETTER_content

        broadcast_observable(N(content));
      }

      Low::Util::String WidgetAsset::get_path() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_path
        // LOW_CODEGEN::END::CUSTOM:GETTER_path

        return TYPE_SOA(WidgetAsset, path, Low::Util::String);
      }
      void WidgetAsset::set_path(const char *p_Value)
      {
        Low::Util::String l_Val(p_Value);
        set_path(l_Val);
      }

      void WidgetAsset::set_path(Low::Util::String p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_path
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_path

        // Set new value
        TYPE_SOA(WidgetAsset, path, Low::Util::String) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_path
        // LOW_CODEGEN::END::CUSTOM:SETTER_path

        broadcast_observable(N(path));
      }

      Low::Core::UI::Controller WidgetAsset::get_controller() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_controller
        // LOW_CODEGEN::END::CUSTOM:GETTER_controller

        return TYPE_SOA(WidgetAsset, controller,
                        Low::Core::UI::Controller);
      }
      void
      WidgetAsset::set_controller(Low::Core::UI::Controller p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_controller
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_controller

        // Set new value
        TYPE_SOA(WidgetAsset, controller, Low::Core::UI::Controller) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_controller
        // LOW_CODEGEN::END::CUSTOM:SETTER_controller

        broadcast_observable(N(controller));
      }

      bool WidgetAsset::has_custom_controller() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_has_custom_controller
        // LOW_CODEGEN::END::CUSTOM:GETTER_has_custom_controller

        return TYPE_SOA(WidgetAsset, has_custom_controller, bool);
      }
      void WidgetAsset::toggle_has_custom_controller()
      {
        has_custom_controller(!has_custom_controller());
      }

      void WidgetAsset::has_custom_controller(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_has_custom_controller
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_has_custom_controller

        // Set new value
        TYPE_SOA(WidgetAsset, has_custom_controller, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_has_custom_controller
        // LOW_CODEGEN::END::CUSTOM:SETTER_has_custom_controller

        broadcast_observable(N(has_custom_controller));
      }

      uint64_t WidgetAsset::get_local_element_id_counter() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_local_element_id_counter
        // LOW_CODEGEN::END::CUSTOM:GETTER_local_element_id_counter

        return TYPE_SOA(WidgetAsset, local_element_id_counter,
                        uint64_t);
      }
      void WidgetAsset::set_local_element_id_counter(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_local_element_id_counter
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_local_element_id_counter

        // Set new value
        TYPE_SOA(WidgetAsset, local_element_id_counter, uint64_t) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_local_element_id_counter
        // LOW_CODEGEN::END::CUSTOM:SETTER_local_element_id_counter

        broadcast_observable(N(local_element_id_counter));
      }

      uint64_t WidgetAsset::get_custom_controller_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_custom_controller_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_custom_controller_id

        return TYPE_SOA(WidgetAsset, custom_controller_id, uint64_t);
      }
      void WidgetAsset::set_custom_controller_id(uint64_t p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_custom_controller_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_custom_controller_id

        // Set new value
        TYPE_SOA(WidgetAsset, custom_controller_id, uint64_t) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_custom_controller_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_custom_controller_id

        broadcast_observable(N(custom_controller_id));
      }

      Low::Util::Name WidgetAsset::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(WidgetAsset, name, Low::Util::Name);
      }
      void WidgetAsset::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(WidgetAsset, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint64_t WidgetAsset::get_next_local_id()
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_next_local_id
        u64 l_Id = 0;
        do {
          l_Id = get_local_element_id_counter();
          LOW_LOG_DEBUG << "ID UPDATE: " << l_Id << LOW_LOG_END;
          set_local_element_id_counter(
              get_local_element_id_counter() + 1);
        } while (l_Id == 0);

        return l_Id;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_next_local_id
      }

      void WidgetAsset::parse_content(Low::Util::Serial::Node &p_Node)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_parse_content
        Util::Serial::Node &l_ElementsNode = p_Node["elements"];
        if (l_ElementsNode) {
          for (u32 i = 0; i < l_ElementsNode.size(); ++i) {
            ElementDescriptor i_Descriptor;
            parse_element(l_ElementsNode[i], i_Descriptor);

            get_content().push_back(i_Descriptor);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_parse_content
      }

      void WidgetAsset::parse_element(
          Low::Util::Serial::Node &p_Node,
          Low::Core::UI::ElementDescriptor &p_Descriptor)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_parse_element
        p_Descriptor.name = p_Node["name"].as<Util::Name>();
        p_Descriptor.local_id = p_Node["local_id"].as<Util::U64Id>();

        Util::Serial::Node &l_ComponentsNode = p_Node["components"];
        if (l_ComponentsNode) {
          for (u32 i = 0; i < l_ComponentsNode.size(); ++i) {
            ComponentDescriptor i_Descriptor;
            i_Descriptor.typeId = Util::Handle::type_id(
                Util::TypeIdentifier::from_string(
                    l_ComponentsNode[i]["type"].as<Util::String>()));
            i_Descriptor.data = l_ComponentsNode[i]["data"];

            p_Descriptor.components.push_back(i_Descriptor);
          }
        }

        Util::Serial::Node &l_ChildrenNode = p_Node["children"];
        if (l_ChildrenNode) {
          for (u32 i = 0; i < l_ChildrenNode.size(); ++i) {
            ElementDescriptor i_Descriptor;
            parse_element(l_ChildrenNode[i], i_Descriptor);

            p_Descriptor.children.push_back(i_Descriptor);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_parse_element
      }

      WidgetInstance
      WidgetAsset::spawn_instance(Low::Renderer::UiCanvas p_Canvas)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_instance
        WidgetInstance l_Instance = WidgetInstance::make(get_name());

        if (get_controller().is_alive()) {
          l_Instance.set_controller_instance(
              get_controller().spawn_instance());
        }

        Element l_RootElement = Element::make(N(root), p_Canvas);
        l_RootElement.set_widget_instance(l_Instance);
        Component::Display l_RootDisplay =
            Component::Display::make(l_RootElement);
        l_RootElement.set_click_passthrough(true);

        l_Instance.set_root(l_RootElement);

        for (ElementDescriptor &i_ElementDescriptor : get_content()) {
          Element i_Element =
              spawn_element(l_Instance, p_Canvas, i_ElementDescriptor,
                            l_RootElement);
          i_Element.set_widget_instance(l_Instance.get_id());

          l_Instance.get_elements().push_back(i_Element);
        }

        return l_Instance;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_instance
      }

      Low::Core::UI::Element WidgetAsset::spawn_element(
          Low::Core::UI::WidgetInstance p_Instance,
          Low::Renderer::UiCanvas p_Canvas,
          Low::Core::UI::ElementDescriptor &p_Descriptor,
          Low::Core::UI::Element p_Parent)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_element
        Element l_Element =
            Element::make(p_Descriptor.name, p_Canvas);
        l_Element.set_local_id(p_Descriptor.local_id);
        for (ComponentDescriptor &i_ComponentDescriptor :
             p_Descriptor.components) {
          Util::RTTI::TypeInfo &i_ComponentType =
              Handle::get_type_info(i_ComponentDescriptor.typeId);

          Util::Handle i_Component = i_ComponentType.deserialize(
              i_ComponentDescriptor.data, l_Element);

          if (i_Component.get_type() ==
              Component::Display::type_id()) {
            Component::Display i_Display = i_Component;
            i_Display.set_parent(p_Parent.get_display().get_id());
          }
        }
        for (ElementDescriptor &i_Descriptor :
             p_Descriptor.children) {
          p_Instance.get_elements().push_back(spawn_element(
              p_Instance, p_Canvas, i_Descriptor, l_Element));
        }

        return l_Element;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_element
      }

      void WidgetAsset::fill_element_descriptor(
          Low::Core::UI::Element p_Element,
          Low::Core::UI::ElementDescriptor &p_Descriptor)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_fill_element_descriptor
        Component::Display l_Display = p_Element.get_display();
        p_Descriptor.name = p_Element.get_name();
        p_Descriptor.local_id = p_Element.get_local_id();
        if (p_Descriptor.local_id == 0) {
          p_Descriptor.local_id = get_next_local_id();
        }

        for (auto &i_ComponentEntry : p_Element.get_components()) {
          Util::Handle i_Component = i_ComponentEntry.second;
          Util::RTTI::TypeInfo &i_ComponentType =
              Util::Handle::get_type_info(i_ComponentEntry.first);

          ComponentDescriptor i_ComponentDescriptor;
          i_ComponentDescriptor.typeId = i_Component.get_type();
          i_ComponentType.serialize(i_Component,
                                    i_ComponentDescriptor.data);

          i_ComponentDescriptor.data.remove("_unique_id");
          i_ComponentDescriptor.data.remove("unique_id");

          p_Descriptor.components.push_back(i_ComponentDescriptor);
        }

        for (Component::Display i_Display :
             l_Display.get_children()) {
          Element i_Element = i_Display.get_element();
          ElementDescriptor i_ElementDescriptor;
          fill_element_descriptor(i_Element, i_ElementDescriptor);

          p_Descriptor.children.push_back(i_ElementDescriptor);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_fill_element_descriptor
      }

      void WidgetAsset::fill_content_from_instance(
          Low::Core::UI::WidgetInstance p_Instance)
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_fill_content_from_instance
        get_content().clear();

        Component::Display l_RootDisplay =
            p_Instance.get_root().get_display();

        for (Component::Display i_Display :
             l_RootDisplay.get_children()) {
          Element i_Element = i_Display.get_element();
          ElementDescriptor i_Descriptor;

          fill_element_descriptor(i_Element, i_Descriptor);
          get_content().push_back(i_Descriptor);
        }

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_fill_content_from_instance
      }

      void WidgetAsset::serialize_element_descriptor(
          const Low::Core::UI::ElementDescriptor &p_Descriptor,
          Low::Util::Serial::Node &p_Node) const
      {
        Low::Util::HandleLock<WidgetAsset> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_element_descriptor
        p_Node["name"] = p_Descriptor.name;
        p_Node["local_id"] = Util::U64Id{p_Descriptor.local_id};
        Util::Serial::Node &i_ComponentsNode = p_Node["components"];
        for (const ComponentDescriptor &i_Component :
             p_Descriptor.components) {
          Util::Serial::Node i_ComponentNode;
          i_ComponentNode["type"] =
              Util::Handle::identifier(i_Component.typeId);
          i_ComponentNode["data"] = i_Component.data;
          if (i_ComponentNode["data"]["_unique_id"]) {
            i_ComponentNode["data"].remove("_unique_id");
          }

          i_ComponentsNode.push_back(i_ComponentNode);
        }
        if (!p_Descriptor.children.empty()) {
          Util::Serial::Node &l_ChildrenNode = p_Node["children"];
          for (const ElementDescriptor &i_Element :
               p_Descriptor.children) {
            Util::Serial::Node i_ElementNode;
            serialize_element_descriptor(i_Element, i_ElementNode);
            l_ChildrenNode.push_back(i_ElementNode);
          }
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_element_descriptor
      }

      uint32_t WidgetAsset::create_instance(
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

      u32 WidgetAsset::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for WidgetAsset.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, WidgetAsset::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool WidgetAsset::get_page_for_index(const u32 p_Index,
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
