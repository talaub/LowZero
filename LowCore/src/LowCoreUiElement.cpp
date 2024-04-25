#include "LowCoreUiElement.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

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
      uint8_t *Element::ms_Buffer = 0;
      Low::Util::Instances::Slot *Element::ms_Slots = 0;
      Low::Util::List<Element> Element::ms_LivingInstances =
          Low::Util::List<Element>();

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
        uint32_t l_Index = create_instance();

        Element l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = Element::TYPE_ID;

        new (&ACCESSOR_TYPE_SOA(
            l_Handle, Element, components,
            SINGLE_ARG(Util::Map<uint16_t, Util::Handle>)))
            Util::Map<uint16_t, Util::Handle>();
        new (&ACCESSOR_TYPE_SOA(l_Handle, Element, view,
                                Low::Core::UI::View))
            Low::Core::UI::View();
        ACCESSOR_TYPE_SOA(l_Handle, Element, click_passthrough,
                          bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Element, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
        Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                      l_Handle.get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Element::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

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

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const Element *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void Element::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Element));

        initialize_buffer(&ms_Buffer, ElementData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_Element);
        LOW_PROFILE_ALLOC(type_slots_Element);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Element);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Element::is_alive;
        l_TypeInfo.destroy = &Element::destroy;
        l_TypeInfo.serialize = &Element::serialize;
        l_TypeInfo.deserialize = &Element::deserialize;
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
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(components);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ElementData, components);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            l_Handle.get_components();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, components,
                SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ElementData, view);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = Low::Core::UI::View::TYPE_ID;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            l_Handle.get_view();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Element, view,
                                              Low::Core::UI::View);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_view(*(Low::Core::UI::View *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(click_passthrough);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ElementData, click_passthrough);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            l_Handle.is_click_passthrough();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, click_passthrough, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_click_passthrough(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ElementData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Element, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(ElementData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            Element l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Element, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Element l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
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
        }
        {
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
        }
        {
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
        }
        {
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
        }
        {
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
        }
        {
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(get_display);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::UI::Component::Display::TYPE_ID;
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
        }
        {
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
        }
        {
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
        }
        {
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
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void Element::cleanup()
      {
        Low::Util::List<Element> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_Element);
        LOW_PROFILE_FREE(type_slots_Element);
      }

      Element Element::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Element l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = Element::TYPE_ID;

        return l_Handle;
      }

      bool Element::is_alive() const
      {
        return m_Data.m_Type == Element::TYPE_ID &&
               check_alive(ms_Slots, Element::get_capacity());
      }

      uint32_t Element::get_capacity()
      {
        return ms_Capacity;
      }

      Element Element::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
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

      Util::Map<uint16_t, Util::Handle> &
      Element::get_components() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_components
        // LOW_CODEGEN::END::CUSTOM:GETTER_components

        return TYPE_SOA(
            Element, components,
            SINGLE_ARG(Util::Map<uint16_t, Util::Handle>));
      }

      Low::Core::UI::View Element::get_view() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view
        // LOW_CODEGEN::END::CUSTOM:GETTER_view

        return TYPE_SOA(Element, view, Low::Core::UI::View);
      }
      void Element::set_view(Low::Core::UI::View p_Value)
      {
        _LOW_ASSERT(is_alive());

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
      }

      bool Element::is_click_passthrough() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_click_passthrough
        // LOW_CODEGEN::END::CUSTOM:GETTER_click_passthrough

        return TYPE_SOA(Element, click_passthrough, bool);
      }
      void Element::set_click_passthrough(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_click_passthrough
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_click_passthrough

        // Set new value
        TYPE_SOA(Element, click_passthrough, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_click_passthrough
        // LOW_CODEGEN::END::CUSTOM:SETTER_click_passthrough
      }

      Low::Util::UniqueId Element::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Element, unique_id, Low::Util::UniqueId);
      }
      void Element::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Element, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      Low::Util::Name Element::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Element, name, Low::Util::Name);
      }
      void Element::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Element, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
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
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_display
        _LOW_ASSERT(is_alive());
        return get_component(UI::Component::Display::TYPE_ID);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_display
      }

      void Element::serialize(Util::Yaml::Node &p_Node,
                              bool p_AddHandles) const
      {
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

      uint32_t Element::create_instance()
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

      void Element::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ElementData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(ElementData, components) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(Util::Map<uint16_t,
                                               Util::Handle>))])
                Util::Map<uint16_t, Util::Handle>();
            *i_ValPtr = it->get_components();
          }
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ElementData, view) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ElementData, view) * (l_Capacity)],
              l_Capacity * sizeof(Low::Core::UI::View));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ElementData, click_passthrough) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ElementData, click_passthrough) *
                         (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ElementData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ElementData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ElementData, name) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ElementData, name) * (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for Element from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace UI
  }   // namespace Core
} // namespace Low
