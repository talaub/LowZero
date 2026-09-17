#include "LowCoreUiScreen.h"

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
#include "LowRendererUiCanvas.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "LowCoreUiWidgetAsset.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Screen::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Screen::IDENTIFIER(LOW_NAME(1181529166),
                             LOW_NAME(3638584326));
      uint32_t Screen::ms_Capacity = 0u;
      uint32_t Screen::ms_PageSize = 0u;
      Low::Util::List<Screen> Screen::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *> Screen::ms_Pages;

      Low::Util::Handle Screen::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Screen Screen::make(Low::Util::Name p_Name)
      {
        return make(p_Name, 0ull);
      }

      Screen Screen::make(Low::Util::Name p_Name,
                          Low::Util::UniqueId p_UniqueId)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Screen l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Screen::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Screen, elements,
                                   Util::List<Core::UI::Element>))
            Util::List<Core::UI::Element>();
        ACCESSOR_TYPE_SOA(l_Handle, Screen, zoom, float) = 0.0f;
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Screen, canvas,
                                   Low::Renderer::UiCanvas))
            Low::Renderer::UiCanvas();
        ACCESSOR_TYPE_SOA(l_Handle, Screen, dirty, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, Screen, name, Low::Util::Name) =
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
        Renderer::UiCanvas l_Canvas =
            Renderer::UiCanvas::make(p_Name);
        l_Handle.set_canvas(l_Canvas);
        l_Handle.zoom(1.0);
        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Screen::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          for (Element i_Element : get_elements()) {
            if (i_Element.is_alive()) {
              i_Element.destroy_with_hierarchy();
            }
          }

          get_canvas().destroy();
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

        Low::Util::remove_unique_id(get_unique_id());

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        _LOW_ASSERT(get_page_for_index(get_index(), l_PageIndex,
                                       l_SlotIndex));
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

        l_Page->slots[l_SlotIndex].m_Occupied = false;
        l_Page->slots[l_SlotIndex].m_Generation++;

        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end();) {
          if (it->get_id() == get_id()) {
            it = ms_LivingInstances.erase(it);
          } else {
            it++;
          }
        }
      }

      void Screen::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(N(LowCore),
                                                          N(Screen));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(Screen));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Screen::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Screen);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Screen::is_alive;
        l_TypeInfo.destroy = &Screen::destroy;
        l_TypeInfo.serialize = &Screen::serialize;
        l_TypeInfo.deserialize = &Screen::deserialize;
        l_TypeInfo.find_by_index = &Screen::_find_by_index;
        l_TypeInfo.notify = &Screen::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Screen::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Screen::_make;
        l_TypeInfo.duplicate_default = &Screen::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Screen::living_instances);
        l_TypeInfo.get_living_count = &Screen::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: elements
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(elements);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Screen::Data, elements);
          l_PropertyInfo.size = sizeof(Screen::Data::elements);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.get_elements();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Screen, elements,
                Util::List<Core::UI::Element>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Util::List<Core::UI::Element> *)p_Data) =
                l_Handle.get_elements();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: elements
        }
        {
          // Property: pixel_position
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pixel_position);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Screen::Data, pixel_position);
          l_PropertyInfo.size = sizeof(Screen::Data::pixel_position);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR2;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.pixel_position();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Screen, pixel_position, Low::Math::Vector2);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.pixel_position(*(Low::Math::Vector2 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Low::Math::Vector2 *)p_Data) =
                l_Handle.pixel_position();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: pixel_position
        }
        {
          // Property: pixel_size
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pixel_size);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(Screen::Data, pixel_size);
          l_PropertyInfo.size = sizeof(Screen::Data::pixel_size);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR2;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.pixel_size();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Screen, pixel_size, Low::Math::Vector2);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.pixel_size(*(Low::Math::Vector2 *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Low::Math::Vector2 *)p_Data) = l_Handle.pixel_size();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: pixel_size
        }
        {
          // Property: zoom
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(zoom);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Screen::Data, zoom);
          l_PropertyInfo.size = sizeof(Screen::Data::zoom);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.zoom();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Screen, zoom,
                                              float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.zoom(*(float *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((float *)p_Data) = l_Handle.zoom();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: zoom
        }
        {
          // Property: canvas
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(canvas);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Screen::Data, canvas);
          l_PropertyInfo.size = sizeof(Screen::Data::canvas);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Renderer::UiCanvas::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.get_canvas();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Screen, canvas, Low::Renderer::UiCanvas);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.set_canvas(*(Low::Renderer::UiCanvas *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Low::Renderer::UiCanvas *)p_Data) =
                l_Handle.get_canvas();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: canvas
        }
        {
          // Property: unique_id
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Screen::Data, unique_id);
          l_PropertyInfo.size = sizeof(Screen::Data::unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, Screen, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Low::Util::UniqueId *)p_Data) =
                l_Handle.get_unique_id();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: unique_id
        }
        {
          // Property: dirty
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Screen::Data, dirty);
          l_PropertyInfo.size = sizeof(Screen::Data::dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.is_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Screen, dirty,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.set_dirty(*(bool *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((bool *)p_Data) = l_Handle.is_dirty();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: dirty
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(Screen::Data, name);
          l_PropertyInfo.size = sizeof(Screen::Data::name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Screen, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Screen l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Screen l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        {
          // Function: make_from_widget_asset
          Low::Util::RTTI::FunctionInfo l_FunctionInfo;
          l_FunctionInfo.name = N(make_from_widget_asset);
          l_FunctionInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_FunctionInfo.handleType =
              Low::Core::UI::Screen::type_id();
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
            l_ParameterInfo.name = N(p_WidgetAsset);
            l_ParameterInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_ParameterInfo.handleType =
                Low::Core::UI::WidgetAsset::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: make_from_widget_asset
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
            l_ParameterInfo.handleType =
                Low::Core::UI::Element::type_id();
            l_FunctionInfo.parameters.push_back(l_ParameterInfo);
          }
          l_TypeInfo.functions[l_FunctionInfo.name] = l_FunctionInfo;
          // End function: add_element
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void Screen::cleanup()
      {
        Low::Util::List<Screen> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        for (auto it = ms_Pages.begin(); it != ms_Pages.end();) {
          Low::Util::Instances::Page *i_Page = *it;
          free(i_Page->buffer);
          free(i_Page->slots);
          delete i_Page;
          it = ms_Pages.erase(it);
        }

        ms_Capacity = 0;
      }

      Low::Util::Handle Screen::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Screen Screen::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Screen l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Screen::ms_TypeId;

        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(p_Index, l_PageIndex, l_SlotIndex)) {
          l_Handle.m_Data.m_Generation = 0;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        l_Handle.m_Data.m_Generation =
            l_Page->slots[l_SlotIndex].m_Generation;

        return l_Handle;
      }

      Screen Screen::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Screen l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Screen::ms_TypeId;

        return l_Handle;
      }

      bool Screen::is_alive() const
      {
        if (m_Data.m_Type != Screen::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Screen::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Screen::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle Screen::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Screen Screen::find_by_name(Low::Util::Name p_Name)
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

      Screen Screen::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      Screen Screen::duplicate(Screen p_Handle,
                               Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle Screen::_duplicate(Low::Util::Handle p_Handle,
                                           Low::Util::Name p_Name)
      {
        Screen l_Screen = p_Handle.get_id();
        return l_Screen.duplicate(p_Name);
      }

      void Screen::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["pixel_position"] = pixel_position();
        p_Node["pixel_size"] = pixel_size();
        p_Node["zoom"] = zoom();
        if (get_canvas().is_alive()) {
          get_canvas().serialize(p_Node["canvas"]);
        }
        p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Screen::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Serial::Node &p_Node)
      {
        Screen l_Screen = p_Handle.get_id();
        l_Screen.serialize(p_Node);
      }

      Low::Util::Handle
      Screen::deserialize(Low::Util::Serial::Node &p_Node,
                          Low::Util::Handle p_Creator)
      {
        Low::Util::UniqueId l_HandleUniqueId = 0ull;
        if (p_Node["unique_id"]) {
          l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
        } else if (p_Node["_unique_id"]) {
          l_HandleUniqueId = Low::Util::string_to_hash(
              p_Node["_unique_id"].as<Low::Util::String>());
        }

        Screen l_Handle = Screen::make(N(Screen), l_HandleUniqueId);

        if (p_Node["pixel_position"]) {
          l_Handle.pixel_position(
              p_Node["pixel_position"].as<Low::Math::Vector2>());
        }
        if (p_Node["pixel_size"]) {
          l_Handle.pixel_size(
              p_Node["pixel_size"].as<Low::Math::Vector2>());
        }
        if (p_Node["zoom"]) {
          l_Handle.zoom(p_Node["zoom"].as<float>());
        }
        if (p_Node["canvas"]) {
          l_Handle.set_canvas(Low::Renderer::UiCanvas::deserialize(
                                  p_Node["canvas"], l_Handle.get_id())
                                  .get_id());
        }
        if (p_Node["unique_id"]) {
          l_Handle.set_unique_id(
              p_Node["unique_id"].as<Low::Util::UniqueId>());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void
      Screen::broadcast_observable(Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 Screen::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Screen::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Screen::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Screen::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
      {
        Screen l_Screen = p_Observer.get_id();
        l_Screen.notify(p_Observed, p_Observable);
      }

      Util::List<Core::UI::Element> &Screen::get_elements() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_elements
        // LOW_CODEGEN::END::CUSTOM:GETTER_elements

        return TYPE_SOA(Screen, elements,
                        Util::List<Core::UI::Element>);
      }

      Low::Math::Vector2 Screen::pixel_position() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_position
        // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_position

        return TYPE_SOA(Screen, pixel_position, Low::Math::Vector2);
      }
      void Screen::pixel_position(float p_X, float p_Y)
      {
        Low::Math::Vector2 l_Val(p_X, p_Y);
        pixel_position(l_Val);
      }

      void Screen::pixel_position_x(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_position();
        l_Value.x = p_Value;
        pixel_position(l_Value);
      }

      void Screen::pixel_position_y(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_position();
        l_Value.y = p_Value;
        pixel_position(l_Value);
      }

      void Screen::pixel_position(Low::Math::Vector2 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_position
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_position

        if (pixel_position() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Screen, pixel_position, Low::Math::Vector2) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_position
          // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_position

          broadcast_observable(N(pixel_position));
        }
      }

      Low::Math::Vector2 Screen::pixel_size() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_size
        // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_size

        return TYPE_SOA(Screen, pixel_size, Low::Math::Vector2);
      }
      void Screen::pixel_size(float p_X, float p_Y)
      {
        Low::Math::Vector2 l_Val(p_X, p_Y);
        pixel_size(l_Val);
      }

      void Screen::pixel_size_x(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_size();
        l_Value.x = p_Value;
        pixel_size(l_Value);
      }

      void Screen::pixel_size_y(float p_Value)
      {
        Low::Math::Vector2 l_Value = pixel_size();
        l_Value.y = p_Value;
        pixel_size(l_Value);
      }

      void Screen::pixel_size(Low::Math::Vector2 p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_size
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_size

        if (pixel_size() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Screen, pixel_size, Low::Math::Vector2) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_size
          // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_size

          broadcast_observable(N(pixel_size));
        }
      }

      float Screen::zoom() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_zoom
        // LOW_CODEGEN::END::CUSTOM:GETTER_zoom

        return TYPE_SOA(Screen, zoom, float);
      }
      void Screen::zoom(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_zoom
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_zoom

        if (zoom() != p_Value) {
          // Set dirty flags
          mark_dirty();

          // Set new value
          TYPE_SOA(Screen, zoom, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_zoom
          // LOW_CODEGEN::END::CUSTOM:SETTER_zoom

          broadcast_observable(N(zoom));
        }
      }

      Low::Renderer::UiCanvas Screen::get_canvas() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas
        // LOW_CODEGEN::END::CUSTOM:GETTER_canvas

        return TYPE_SOA(Screen, canvas, Low::Renderer::UiCanvas);
      }
      void Screen::set_canvas(Low::Renderer::UiCanvas p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas

        // Set new value
        TYPE_SOA(Screen, canvas, Low::Renderer::UiCanvas) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas
        // LOW_CODEGEN::END::CUSTOM:SETTER_canvas

        broadcast_observable(N(canvas));
      }

      Low::Util::UniqueId Screen::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(Screen, unique_id, Low::Util::UniqueId);
      }
      void Screen::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(Screen, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

        broadcast_observable(N(unique_id));
      }

      bool Screen::is_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

        return TYPE_SOA(Screen, dirty, bool);
      }
      void Screen::toggle_dirty()
      {
        set_dirty(!is_dirty());
      }

      void Screen::set_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

        // Set new value
        TYPE_SOA(Screen, dirty, bool) = p_Value;

        if (p_Value) {
          mark_dirty();
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

        broadcast_observable(N(dirty));
      }

      void Screen::mark_dirty()
      {
        if (!is_dirty()) {
          TYPE_SOA(Screen, dirty, bool) = true;
          // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
          // LOW_CODEGEN::END::CUSTOM:MARK_dirty
        }
      }

      Low::Util::Name Screen::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Screen, name, Low::Util::Name);
      }
      void Screen::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Screen, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      Low::Core::UI::Screen Screen::make_from_widget_asset(
          Low::Util::Name p_Name,
          Low::Core::UI::WidgetAsset p_WidgetAsset)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_make_from_widget_asset
        Screen l_Screen = make(p_Name);
        WidgetInstance l_Instance =
            p_WidgetAsset.spawn_instance(l_Screen.get_canvas());

        l_Screen.add_element(l_Instance.get_root());

        return l_Screen;

        // LOW_CODEGEN::END::CUSTOM:FUNCTION_make_from_widget_asset
      }

      void Screen::add_element(Low::Core::UI::Element p_Element)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_add_element

        LOW_ASSERT(!p_Element.get_display().get_parent().is_alive(),
                   "Can only assign root elements to screens");

        p_Element.update_screen(get_id());
        get_elements().push_back(p_Element);
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_add_element
      }

      uint32_t Screen::create_instance(u32 &p_PageIndex,
                                       u32 &p_SlotIndex)
      {
        u32 l_Index = 0;
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        bool l_FoundIndex = false;

        for (; !l_FoundIndex && l_PageIndex < ms_Pages.size();
             ++l_PageIndex) {
          for (l_SlotIndex = 0;
               l_SlotIndex < ms_Pages[l_PageIndex]->size;
               ++l_SlotIndex) {
            if (!ms_Pages[l_PageIndex]
                     ->slots[l_SlotIndex]
                     .m_Occupied) {
              l_FoundIndex = true;
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
        }
        ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied = true;
        p_PageIndex = l_PageIndex;
        p_SlotIndex = l_SlotIndex;
        return l_Index;
      }

      u32 Screen::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Screen.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Screen::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Screen::get_page_for_index(const u32 p_Index,
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
  }   // namespace Core
} // namespace Low
