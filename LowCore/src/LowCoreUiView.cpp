#include "LowCoreUiView.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

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
      uint8_t *View::ms_Buffer = 0;
      Low::Util::Instances::Slot *View::ms_Slots = 0;
      Low::Util::List<View> View::ms_LivingInstances =
          Low::Util::List<View>();

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
        uint32_t l_Index = create_instance();

        View l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        ACCESSOR_TYPE_SOA(l_Handle, View, loaded, bool) = false;
        new (&ACCESSOR_TYPE_SOA(l_Handle, View, elements,
                                Util::Set<Util::UniqueId>))
            Util::Set<Util::UniqueId>();
        ACCESSOR_TYPE_SOA(l_Handle, View, internal, bool) = false;
        ACCESSOR_TYPE_SOA(l_Handle, View, view_template, bool) =
            false;
        new (&ACCESSOR_TYPE_SOA(l_Handle, View, pixel_position,
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

        l_Handle.set_unique_id(
            Low::Util::generate_unique_id(l_Handle.get_id()));
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

        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
        // LOW_CODEGEN::END::CUSTOM:DESTROY

        Low::Util::remove_unique_id(get_unique_id());

        ms_Slots[this->m_Data.m_Index].m_Occupied = false;
        ms_Slots[this->m_Data.m_Index].m_Generation++;

        const View *l_Instances = living_instances();
        bool l_LivingInstanceFound = false;
        for (uint32_t i = 0u; i < living_count(); ++i) {
          if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
            ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
            l_LivingInstanceFound = true;
            break;
          }
        }
      }

      void View::initialize()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity =
            Low::Util::Config::get_capacity(N(LowCore), N(View));

        initialize_buffer(&ms_Buffer, ViewData::get_size(),
                          get_capacity(), &ms_Slots);

        LOW_PROFILE_ALLOC(type_buffer_View);
        LOW_PROFILE_ALLOC(type_slots_View);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(View);
        l_TypeInfo.typeId = TYPE_ID;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &View::is_alive;
        l_TypeInfo.destroy = &View::destroy;
        l_TypeInfo.serialize = &View::serialize;
        l_TypeInfo.deserialize = &View::deserialize;
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
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(loaded);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, loaded);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.is_loaded();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, loaded,
                                              bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_loaded(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(elements);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, elements);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.get_elements();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, elements, Util::Set<Util::UniqueId>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(internal);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, internal);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.is_internal();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              internal, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(view_template);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(ViewData, view_template);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.is_view_template();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              view_template, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_view_template(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(pixel_position);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(ViewData, pixel_position);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::VECTOR2;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.pixel_position();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, pixel_position, Low::Math::Vector2);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.pixel_position(*(Low::Math::Vector2 *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(rotation);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(ViewData, rotation);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.rotation();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              rotation, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.rotation(*(float *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(scale_multiplier);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset =
              offsetof(ViewData, scale_multiplier);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.scale_multiplier();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, scale_multiplier, float);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.scale_multiplier(*(float *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(unique_id);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(ViewData, unique_id);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT64;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.get_unique_id();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, View, unique_id, Low::Util::UniqueId);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(transform_dirty);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(ViewData, transform_dirty);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.is_transform_dirty();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View,
                                              transform_dirty, bool);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_transform_dirty(*(bool *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        {
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = true;
          l_PropertyInfo.dataOffset = offsetof(ViewData, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.get =
              [](Low::Util::Handle p_Handle) -> void const * {
            View l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, View, name,
                                              Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            View l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        }
        Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
      }

      void View::cleanup()
      {
        Low::Util::List<View> l_Instances = ms_LivingInstances;
        for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
          l_Instances[i].destroy();
        }
        free(ms_Buffer);
        free(ms_Slots);

        LOW_PROFILE_FREE(type_buffer_View);
        LOW_PROFILE_FREE(type_slots_View);
      }

      View View::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        View l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
        l_Handle.m_Data.m_Type = View::TYPE_ID;

        return l_Handle;
      }

      bool View::is_alive() const
      {
        return m_Data.m_Type == View::TYPE_ID &&
               check_alive(ms_Slots, View::get_capacity());
      }

      uint32_t View::get_capacity()
      {
        return ms_Capacity;
      }

      View View::find_by_name(Low::Util::Name p_Name)
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          if (it->get_name() == p_Name) {
            return *it;
          }
        }
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
        p_Node["unique_id"] = get_unique_id();
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
        View l_Handle = View::make(N(View));

        if (p_Node["unique_id"]) {
          Low::Util::remove_unique_id(l_Handle.get_unique_id());
          l_Handle.set_unique_id(p_Node["unique_id"].as<uint64_t>());
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());
        }

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

      bool View::is_loaded() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:GETTER_loaded

        return TYPE_SOA(View, loaded, bool);
      }
      void View::set_loaded(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_loaded

        // Set new value
        TYPE_SOA(View, loaded, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_loaded
        // LOW_CODEGEN::END::CUSTOM:SETTER_loaded
      }

      Util::Set<Util::UniqueId> &View::get_elements() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_elements
        // LOW_CODEGEN::END::CUSTOM:GETTER_elements

        return TYPE_SOA(View, elements, Util::Set<Util::UniqueId>);
      }

      bool View::is_internal() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal
        // LOW_CODEGEN::END::CUSTOM:GETTER_internal

        return TYPE_SOA(View, internal, bool);
      }
      void View::set_internal(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

        // Set new value
        TYPE_SOA(View, internal, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal
        // LOW_CODEGEN::END::CUSTOM:SETTER_internal
      }

      bool View::is_view_template() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_view_template
        // LOW_CODEGEN::END::CUSTOM:GETTER_view_template

        return TYPE_SOA(View, view_template, bool);
      }
      void View::set_view_template(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_view_template
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_view_template

        // Set new value
        TYPE_SOA(View, view_template, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_view_template
        // LOW_CODEGEN::END::CUSTOM:SETTER_view_template
      }

      Low::Math::Vector2 &View::pixel_position() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_position
        // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_position

        return TYPE_SOA(View, pixel_position, Low::Math::Vector2);
      }
      void View::pixel_position(Low::Math::Vector2 &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_position
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_position

        if (pixel_position() != p_Value) {
          // Set dirty flags
          TYPE_SOA(View, transform_dirty, bool) = true;

          // Set new value
          TYPE_SOA(View, pixel_position, Low::Math::Vector2) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_position
          // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_position
        }
      }

      float View::rotation() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation
        // LOW_CODEGEN::END::CUSTOM:GETTER_rotation

        return TYPE_SOA(View, rotation, float);
      }
      void View::rotation(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation

        if (rotation() != p_Value) {
          // Set dirty flags
          TYPE_SOA(View, transform_dirty, bool) = true;

          // Set new value
          TYPE_SOA(View, rotation, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation
          // LOW_CODEGEN::END::CUSTOM:SETTER_rotation
        }
      }

      float View::scale_multiplier() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_scale_multiplier
        // LOW_CODEGEN::END::CUSTOM:GETTER_scale_multiplier

        return TYPE_SOA(View, scale_multiplier, float);
      }
      void View::scale_multiplier(float p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scale_multiplier
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_scale_multiplier

        if (scale_multiplier() != p_Value) {
          // Set dirty flags
          TYPE_SOA(View, transform_dirty, bool) = true;

          // Set new value
          TYPE_SOA(View, scale_multiplier, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale_multiplier
          // LOW_CODEGEN::END::CUSTOM:SETTER_scale_multiplier
        }
      }

      Low::Util::UniqueId View::get_unique_id() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

        return TYPE_SOA(View, unique_id, Low::Util::UniqueId);
      }
      void View::set_unique_id(Low::Util::UniqueId p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

        // Set new value
        TYPE_SOA(View, unique_id, Low::Util::UniqueId) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
        // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
      }

      bool View::is_transform_dirty() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_transform_dirty
        // LOW_CODEGEN::END::CUSTOM:GETTER_transform_dirty

        return TYPE_SOA(View, transform_dirty, bool);
      }
      void View::set_transform_dirty(bool p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_transform_dirty
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_transform_dirty

        // Set new value
        TYPE_SOA(View, transform_dirty, bool) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_transform_dirty
        // LOW_CODEGEN::END::CUSTOM:SETTER_transform_dirty
      }

      Low::Util::Name View::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(View, name, Low::Util::Name);
      }
      void View::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(View, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
        // LOW_CODEGEN::END::CUSTOM:SETTER_name
      }

      void View::serialize_elements(Util::Yaml::Node &p_Node)
      {
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
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_remove_element
        // LOW_CODEGEN::END::CUSTOM:FUNCTION_remove_element
      }

      void View::load_elements()
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_load_elements
        LOW_ASSERT(is_alive(), "Cannot load dead view");
        LOW_ASSERT(!is_loaded(), "View is already loaded");

        set_loaded(true);

        Util::String l_Path =
            Util::String(LOW_DATA_PATH) + "\\assets\\ui_views\\";
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

      uint32_t View::create_instance()
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

      void View::increase_budget()
      {
        uint32_t l_Capacity = get_capacity();
        uint32_t l_CapacityIncrease =
            std::max(std::min(l_Capacity, 64u), 1u);
        l_CapacityIncrease =
            std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

        LOW_ASSERT(l_CapacityIncrease > 0,
                   "Could not increase capacity");

        uint8_t *l_NewBuffer = (uint8_t *)malloc(
            (l_Capacity + l_CapacityIncrease) * sizeof(ViewData));
        Low::Util::Instances::Slot *l_NewSlots =
            (Low::Util::Instances::Slot *)malloc(
                (l_Capacity + l_CapacityIncrease) *
                sizeof(Low::Util::Instances::Slot));

        memcpy(l_NewSlots, ms_Slots,
               l_Capacity * sizeof(Low::Util::Instances::Slot));
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewData, loaded) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewData, loaded) * (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        {
          for (auto it = ms_LivingInstances.begin();
               it != ms_LivingInstances.end(); ++it) {
            auto *i_ValPtr = new (
                &l_NewBuffer[offsetof(ViewData, elements) *
                                 (l_Capacity + l_CapacityIncrease) +
                             (it->get_index() *
                              sizeof(Util::Set<Util::UniqueId>))])
                Util::Set<Util::UniqueId>();
            *i_ValPtr = it->get_elements();
          }
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewData, internal) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewData, internal) * (l_Capacity)],
              l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, view_template) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, view_template) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, pixel_position) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, pixel_position) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Math::Vector2));
        }
        {
          memcpy(
              &l_NewBuffer[offsetof(ViewData, rotation) *
                           (l_Capacity + l_CapacityIncrease)],
              &ms_Buffer[offsetof(ViewData, rotation) * (l_Capacity)],
              l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, scale_multiplier) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, scale_multiplier) *
                            (l_Capacity)],
                 l_Capacity * sizeof(float));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, unique_id) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, unique_id) *
                            (l_Capacity)],
                 l_Capacity * sizeof(Low::Util::UniqueId));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, transform_dirty) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, transform_dirty) *
                            (l_Capacity)],
                 l_Capacity * sizeof(bool));
        }
        {
          memcpy(&l_NewBuffer[offsetof(ViewData, name) *
                              (l_Capacity + l_CapacityIncrease)],
                 &ms_Buffer[offsetof(ViewData, name) * (l_Capacity)],
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

        LOW_LOG_DEBUG << "Auto-increased budget for View from "
                      << l_Capacity << " to "
                      << (l_Capacity + l_CapacityIncrease)
                      << LOW_LOG_END;
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    } // namespace UI
  }   // namespace Core
} // namespace Low
