#include "LowCoreUiElement.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"
#include "LowUtilObserverManager.h"

#include "LowCoreUiDisplay.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      const uint16_t Element::TYPE_ID = 37;
      uint32_t Element::ms_Capacity = 0u;
      uint32_t Element::ms_PageSize = 0u;
      Low::Util::SharedMutex Element::ms_LivingMutex;
      Low::Util::SharedMutex Element::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          Element::ms_PagesLock(Element::ms_PagesMutex,
                                std::defer_lock);
      Low::Util::List<Element> Element::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Element::ms_Pages;

      Element::Element() : Low::Util::Handle(0ull)
      {
      }
      Element::Element(uint64_t p_Id) : Low::Util::Handle(p_Id)
      {
      }
      Element::Element(Element &p_Copy)
          : Low::Util::Handle(p_Copy.m_Id)
      {
      }

      Low::Util::Handle Element::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Element Element::make(Low::Util::Name p_Name)
      {
        return make(p_Name, 0ull);
      }

      Element Element::make(Low::Util::Name p_Name,
                            Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        Element l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Element::TYPE_ID;

        l_PageLock.unlock();

        Low::Util::HandleLock<Element> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, Element, components,
            SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)))
            Util::Map<uint16_t, Util::Handle>();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Element, view,
                                   Low::Core::UI::View))
            Low::Core::UI::View();
        ACCESSOR_TYPE_SOA(l_Handle, Element, click_passthrough,
                          bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Element, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        if (p_UniqueId > 0ull) {
          l_Handle.set_unique_id(p_UniqueId);
        } else {
          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
        }
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Element::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<Element> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          Util::List<Element> l_Children;
          if (get_display().is_alive()) {
            for (Component::Display i_Child :
                 get_display().get_children()) {
              l_Children.push_back(i_Child.get_element());
            }
          }

          for (Element i_Child : l_Children) {
            i_Child.destroy();
          }

          Util::List<uint16_t> l_ComponentTypes;
          for (auto it = get_components().begin();
               it != get_components().end(); ++it) {
            if (has_component(it->first)) {
              l_ComponentTypes.push_back(it->first);
            }
          }

          for (auto it = l_ComponentTypes.begin();
               it != l_ComponentTypes.end(); ++it) {
            Util::Handle i_Handle = get_component(*it);
            Util::RTTI::TypeInfo &i_TypeInfo =
                Util::Handle::get_type_info(*it);
            if (i_TypeInfo.is_alive(i_Handle)) {
              i_TypeInfo.destroy(i_Handle);
            }
          }

          if (get_view().is_alive()) {
            get_view().remove_element(*this);
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

      void Element::initialize()
      {
        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Element));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Element::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Element);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Element::is_alive;
        l_TypeInfo.destroy = &Element::destroy;
        l_TypeInfo.serialize = &Element::serialize;
        l_TypeInfo.deserialize = &Element::deserialize;
        l_TypeInfo.find_by_index = &Element::_find_by_index;
        l_TypeInfo.notify = &Element::_notify;
        l_TypeInfo.find_by_name = &Element::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Element::_make;
        l_TypeInfo.duplicate_default = &Element::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Element::living_instances);
        l_TypeInfo.get_living_count = &Element::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: components
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(components);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Element::Data, components);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            l_Handle.get_components();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, components,
                SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            *((Util::Map<uint16_t, Util::Handle> *)p_Data) =
                l_Handle.get_components();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: components
        }
        {
          // Property: view
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Element::Data, view);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::UI::View::TYPE_ID;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            l_Handle.get_view();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Element, view,
                                              Low::Core::UI::View);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_view(*(Low::Core::UI::View *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            *((Low::Core::UI::View *)p_Data) = l_Handle.get_view();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: view
        }
        {
          // Property: click_passthrough
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(click_passthrough);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Element::Data, click_passthrough);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            l_Handle.is_click_passthrough();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, click_passthrough, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_click_passthrough(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            *((bool *)p_Data) = l_Handle.is_click_passthrough();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: click_passthrough
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Element::Data, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Element::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Element, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Element l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<Element> l_HandleLock(l_Handle);
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
          l_FunctionInfo.handleType = Element::TYPE_ID;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Name);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::NAME;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_View);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = Low::Core::UI::View::TYPE_ID;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make
        }
        {
          // Function: get_component
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_component);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_TypeId);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT16;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_component
        }
        {
          // Function: add_component
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(add_component);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_Component);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: add_component
        }
        {
          // Function: remove_component
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(remove_component);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ComponentType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT16;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: remove_component
        }
        {
          // Function: has_component
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(has_component);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_FunctionInfo.handleType = 0;
          {
            Low::Util::RTTI::ParameterInfo l_ParameterInfo;
            l_ParameterInfo.name = N(p_ComponentType);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::UINT16;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: has_component
        }
        {
          // Function: get_display
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_display);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::UI::Component::Display::TYPE_ID;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: get_display
        }
        {
          // Function: serialize
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(serialize);
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
            l_ParameterInfo.name = N(p_AddHandles);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: serialize
        }
        {
          // Function: serialize_hierarchy
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(serialize_hierarchy);
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
            l_ParameterInfo.name = N(p_AddHandles);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::BOOL;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: serialize_hierarchy
        }
        {
          // Function: deserialize_hierarchy
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(deserialize_hierarchy);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType = UI::Element::TYPE_ID;
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
            l_ParameterInfo.name = N(p_Creator);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType = 0;
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: deserialize_hierarchy
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Element::cleanup()
      {
        Low::Util::List<Element> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Element::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Element Element::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Element l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Element::TYPE_ID;

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

      Element Element::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Element l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Element::TYPE_ID;

        return l_Handle;
      }

      bool Element::is_alive() const
      {
        if (m_Data.m_Type != Element::TYPE_ID) {
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
        return m_Data.m_Type == Element::TYPE_ID &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Element::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Element::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Element Element::find_by_name(Low::Util::Name p_Name)
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

      Element Element::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        Element l_Element = make(p_Name);

        for (auto it = get_components().begin();
             it != get_components().end(); ++it) {
          Util::RTTI::TypeInfo &i_ComponentTypeInfo =
              Util::Handle::get_type_info(it->first);

          i_ComponentTypeInfo.duplicate_component(it->second,
                                                  l_Element);
        }

        Component::Display l_Display = get_display();

        for (u32 i = 0; i < l_Display.get_children().size(); ++i) {
          Component::Display i_ChildDisplay =
              l_Display.get_children()[i];

          Element i_CopiedElement =
              i_ChildDisplay.get_element().duplicate(
                  i_ChildDisplay.get_element().get_name());

          i_CopiedElement.get_display().set_parent(
              l_Element.get_display());
        }

        l_Element.set_click_passthrough(is_click_passthrough());
        l_Element.get_display().set_parent(l_Display.get_parent());
        get_view().add_element(l_Element);

        return l_Element;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Element Element::duplicate(Element p_Handle,
                                 Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Element::_duplicate(Low::Util::Handle p_Handle,
                          Low::Util::Name p_Name)
      {
        Element l_Element = p_Handle.get_id();
        return l_Element.duplicate(p_Name);
      }

      void Element::serialize(Low::Util::Yaml::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        serialize(p_Node, false);
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Element::serialize(Low::Util::Handle p_Handle,
                              Low::Util::Yaml::Node &p_Node)
      {
        Element l_Element = p_Handle.get_id();
        l_Element.serialize(p_Node);
      }

      Low::Util::Handle
      Element::deserialize(Low::Util::Yaml::Node &p_Node,
                           Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        View l_View = p_Creator.get_id();

        if (!l_View.is_alive()) {
          if (p_Node["view"]) {
            l_View = Util::find_handle_by_unique_id(
                         p_Node["view"].as<uint64_t>())
                         .get_id();
          }
        }

        Element l_Element =
            Element::make(LOW_YAML_AS_NAME(p_Node["name"]));

        p_Node["_handle"] = l_Element.get_id();

        // Parse the old unique id and assign it again (need
        // to remove the auto generated uid first)
        if (p_Node["unique_id"]) {
          Util::remove_unique_id(l_Element.get_unique_id());
          l_Element.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Util::register_unique_id(l_Element.get_unique_id(),
                                   l_Element);
        }

        l_View.add_element(l_Element);

        Util::Yaml::Node &l_ComponentsNode = p_Node["components"];

        for (auto it = l_ComponentsNode.begin();
             it != l_ComponentsNode.end(); ++it) {
          Util::Yaml::Node &i_ComponentNode = *it;
          Util::RTTI::TypeInfo &i_TypeInfo =
              Util::Handle::get_type_info(
                  i_ComponentNode["type"].as<uint16_t>());

          i_ComponentNode["_handle"] =
              i_TypeInfo
                  .deserialize(i_ComponentNode["properties"],
                               l_Element)
                  .get_id();
        }

        return l_Element;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void Element::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Element::observe(Low::Util::Name p_Observable,
                           Low::Util::Function<void(Low::Util::Handle,
                                                    Low::Util::Name)>
                               p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Element::observe(Low::Util::Name p_Observable,
                           Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Element::notify(Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Element::_notify(Low::Util::Handle p_Observer,
                            Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
      {
        Element l_Element = p_Observer.get_id();
        l_Element.notify(p_Observed, p_Observable);
      }

      Util::Map<uint16_t, Util::Handle> &
      Element::get_components() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_components

        // LOW_CODEGEN::END::CUSTOM:GETTER_components

        return TYPE_SOA(
            Element, components,
            SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
      }

      Low::Core::UI::View Element::get_view() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view

        // LOW_CODEGEN::END::CUSTOM:GETTER_view

        return TYPE_SOA(Element, view, Low::Core::UI::View);
      }
      void Element::set_view(Low::Core::UI::View p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view

        // Set new value
        TYPE_SOA(Element, view, Low::Core::UI::View) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view

        if (get_display().is_alive()) {
          // If a parent gets a new view assigned all children get
          // moved to that view as well
          for (u64 i_ChildId : get_display().get_children()) {
            Component::Display i_ChildDisplay = i_ChildId;
            if (p_Value.is_alive()) {
              p_Value.add_element(i_ChildDisplay.get_element());
            }
          }
        }
        // LOW_CODEGEN::END::CUSTOM:SETTER_view

        broadcast_observable(N(view));
      }

      bool Element::is_click_passthrough() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_click_passthrough

        // LOW_CODEGEN::END::CUSTOM:GETTER_click_passthrough

        return TYPE_SOA(Element, click_passthrough, bool);
      }
      void Element::toggle_click_passthrough()
      {
        set_click_passthrough(!is_click_passthrough());
      }

      void Element::set_click_passthrough(bool p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_click_passthrough

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_click_passthrough

        // Set new value
        TYPE_SOA(Element, click_passthrough, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_click_passthrough

        // LOW_CODEGEN::END::CUSTOM:SETTER_click_passthrough

        broadcast_observable(N(click_passthrough));
      }

      Low::Util::UniqueId Element::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Element, unique_id, Low::Util::UniqueId);
      }
      void Element::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Element, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      Low::Util::Name Element::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Element, name, Low::Util::Name);
      }
      void Element::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<Element> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Element, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Element Element::make(Low::Util::Name p_Name,
                            Low::Core::UI::View p_View)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make

        Element l_Element = Element::make(p_Name);
        p_View.add_element(l_Element);
        return l_Element;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make
      }

      uint64_t Element::get_component(uint16_t p_TypeId) const
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_component

        if (get_components().find(p_TypeId) ==
            get_components().end()) {
          return ~0ull;
        }
        return get_components()[p_TypeId].get_id();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_component
      }

      void Element::add_component(Low::Util::Handle &p_Component)
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_component

        Util::Handle l_ExistingComponent =
            get_component(p_Component.get_type());
        Util::RTTI::TypeInfo l_ComponentTypeInfo =
            get_type_info(p_Component.get_type());

        LOW_ASSERT(l_ComponentTypeInfo.uiComponent,
                   "Can only add ui components to an element");
        LOW_ASSERT(!l_ComponentTypeInfo.is_alive(l_ExistingComponent),
                   "An element can only hold one component "
                   "of a given type");

        l_ComponentTypeInfo.properties[N(element)].set(p_Component,
                                                       this);

        get_components()[p_Component.get_type()] =
            p_Component.get_id();
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_component
      }

      void Element::remove_component(uint16_t p_ComponentType)
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_component

        LOW_ASSERT(has_component(p_ComponentType),
                   "Cannot remove component from element. This "
                   "element does not "
                   "have a component of the specified type");

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_ComponentType);

        l_TypeInfo.destroy(get_components()[p_ComponentType]);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_component
      }

      bool Element::has_component(uint16_t p_ComponentType)
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_has_component

        if (get_components().find(p_ComponentType) ==
            get_components().end()) {
          return false;
        }

        Util::Handle l_Handle = get_components()[p_ComponentType];

        Util::RTTI::TypeInfo &l_TypeInfo =
            Util::Handle::get_type_info(p_ComponentType);

        return l_TypeInfo.is_alive(l_Handle);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_has_component
      }

      Low::Core::UI::Component::Display Element::get_display() const
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_display

        _LOW_ASSERT(is_alive());
        return get_component(UI::Component::Display::TYPE_ID);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_display
      }

      void Element::serialize(Util::Yaml::Node &p_Node,
                              bool p_AddHandles) const
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize

        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();
        p_Node["unique_id"] = get_unique_id();
        if (p_AddHandles) {
          p_Node["handle"] = get_id();
        }
        p_Node["view"] = 0;
        if (get_view().is_alive()) {
          p_Node["view"] = get_view().get_unique_id();
        }

        for (auto it = get_components().begin();
             it != get_components().end(); ++it) {
          Util::Yaml::Node i_Node;
          i_Node["type"] = it->first;
          if (p_AddHandles) {
            i_Node["handle"] = it->second.get_id();
          }

          Util::RTTI::TypeInfo &i_TypeInfo =
              Util::Handle::get_type_info(it->first);
          Util::Yaml::Node i_PropertiesNode;
          i_TypeInfo.serialize(it->second, i_PropertiesNode);
          i_Node["properties"] = i_PropertiesNode;
          p_Node["components"].push_back(i_Node);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize
      }

      void Element::serialize_hierarchy(Util::Yaml::Node &p_Node,
                                        bool p_AddHandles) const
      {
        Low::Util::HandleLock<Element> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_serialize_hierarchy

        serialize(p_Node, p_AddHandles);

        Component::Display l_Display = get_display();

        for (auto it = l_Display.get_children().begin();
             it != l_Display.get_children().end(); ++it) {
          Component::Display i_Child = *it;

          Util::Yaml::Node i_Node;
          i_Child.get_element().serialize_hierarchy(i_Node,
                                                    p_AddHandles);

          p_Node["children"].push_back(i_Node);
        }
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_serialize_hierarchy
      }

      UI::Element
      Element::deserialize_hierarchy(Util::Yaml::Node &p_Node,
                                     Util::Handle p_Creator)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_deserialize_hierarchy

        Element l_Element =
            (Element)deserialize(p_Node, p_Creator).get_id();

        if (p_Node["children"]) {
          for (uint32_t i = 0; i < p_Node["children"].size(); ++i) {
            Element::deserialize_hierarchy(p_Node["children"][i],
                                           p_Creator);
          }
        }

        return l_Element;
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_deserialize_hierarchy
      }

      uint32_t Element::create_instance(
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

      u32 Element::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Element.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Element::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Element::get_page_for_index(const u32 p_Index,
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
