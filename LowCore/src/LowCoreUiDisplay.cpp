#include "LowCoreUiDisplay.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowMathQuaternionUtil.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        const uint16_t Display::TYPE_ID = 39;
        uint32_t Display::ms_Capacity = 0u;
        uint8_t *Display::ms_Buffer = 0;
        Low::Util::Instances::Slot *Display::ms_Slots = 0;
        Low::Util::List<Display> Display::ms_LivingInstances =
            Low::Util::List<Display>();

        Display::Display() : Low::Util::Handle(0ull)
        {
        }
        Display::Display(uint64_t p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Display::Display(Display &p_Copy)
            : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        Low::Util::Handle Display::_make(Low::Util::Handle p_Element)
        {
          Low::Core::UI::Element l_Element = p_Element.get_id();
          LOW_ASSERT(l_Element.is_alive(),
                     "Cannot create component for dead element");
          return make(l_Element).get_id();
        }

        Display Display::make(Low::Core::UI::Element p_Element)
        {
          uint32_t l_Index = create_instance();

          Display l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Slots[l_Index].m_Generation;
          l_Handle.m_Data.m_Type = Display::TYPE_ID;

          new (&ACCESSOR_TYPE_SOA(l_Handle, Display, pixel_position,
                                  Low::Math::Vector2))
              Low::Math::Vector2();
          ACCESSOR_TYPE_SOA(l_Handle, Display, rotation, float) =
              0.0f;
          new (&ACCESSOR_TYPE_SOA(l_Handle, Display, pixel_scale,
                                  Low::Math::Vector2))
              Low::Math::Vector2();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Display, children,
                                  Low::Util::List<uint64_t>))
              Low::Util::List<uint64_t>();
          new (&ACCESSOR_TYPE_SOA(
              l_Handle, Display, absolute_pixel_position,
              Low::Math::Vector2)) Low::Math::Vector2();
          ACCESSOR_TYPE_SOA(l_Handle, Display, absolute_rotation,
                            float) = 0.0f;
          new (&ACCESSOR_TYPE_SOA(
              l_Handle, Display, absolute_pixel_scale,
              Low::Math::Vector2)) Low::Math::Vector2();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Display, world_matrix,
                                  Low::Math::Matrix4x4))
              Low::Math::Matrix4x4();
          ACCESSOR_TYPE_SOA(l_Handle, Display, world_updated, bool) =
              false;
          new (&ACCESSOR_TYPE_SOA(l_Handle, Display, element,
                                  Low::Core::UI::Element))
              Low::Core::UI::Element();
          ACCESSOR_TYPE_SOA(l_Handle, Display, dirty, bool) = false;
          ACCESSOR_TYPE_SOA(l_Handle, Display, world_dirty, bool) =
              false;

          l_Handle.set_element(p_Element);
          p_Element.add_component(l_Handle);

          ms_LivingInstances.push_back(l_Handle);

          l_Handle.set_unique_id(
              Low::Util::generate_unique_id(l_Handle.get_id()));
          Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                        l_Handle.get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
          // LOW_CODEGEN::END::CUSTOM:MAKE

          return l_Handle;
        }

        void Display::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // Doing this to remove the transform from the list of
          // children
          set_parent(0);
          // LOW_CODEGEN::END::CUSTOM:DESTROY

          Low::Util::remove_unique_id(get_unique_id());

          ms_Slots[this->m_Data.m_Index].m_Occupied = false;
          ms_Slots[this->m_Data.m_Index].m_Generation++;

          const Display *l_Instances = living_instances();
          bool l_LivingInstanceFound = false;
          for (uint32_t i = 0u; i < living_count(); ++i) {
            if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
              ms_LivingInstances.erase(ms_LivingInstances.begin() +
                                       i);
              l_LivingInstanceFound = true;
              break;
            }
          }
        }

        void Display::initialize()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Display));

          initialize_buffer(&ms_Buffer, DisplayData::get_size(),
                            get_capacity(), &ms_Slots);

          LOW_PROFILE_ALLOC(type_buffer_Display);
          LOW_PROFILE_ALLOC(type_slots_Display);

          Low::Util::RTTI::TypeInfo l_TypeInfo;
          l_TypeInfo.name = N(Display);
          l_TypeInfo.typeId = TYPE_ID;
          l_TypeInfo.get_capacity = &get_capacity;
          l_TypeInfo.is_alive = &Display::is_alive;
          l_TypeInfo.destroy = &Display::destroy;
          l_TypeInfo.serialize = &Display::serialize;
          l_TypeInfo.deserialize = &Display::deserialize;
          l_TypeInfo.find_by_index = &Display::_find_by_index;
          l_TypeInfo.make_default = nullptr;
          l_TypeInfo.make_component = &Display::_make;
          l_TypeInfo.duplicate_default = nullptr;
          l_TypeInfo.duplicate_component = &Display::_duplicate;
          l_TypeInfo.get_living_instances = reinterpret_cast<
              Low::Util::RTTI::LivingInstancesGetter>(
              &Display::living_instances);
          l_TypeInfo.get_living_count = &Display::living_count;
          l_TypeInfo.component = false;
          l_TypeInfo.uiComponent = true;
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(pixel_position);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, pixel_position);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.pixel_position();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                pixel_position,
                                                Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.pixel_position(*(Low::Math::Vector2 *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(rotation);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, rotation);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.rotation();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                rotation, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.rotation(*(float *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(pixel_scale);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, pixel_scale);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.pixel_scale();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, pixel_scale, Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.pixel_scale(*(Low::Math::Vector2 *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(layer);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(DisplayData, layer);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.layer();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                layer, uint32_t);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.layer(*(uint32_t *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(parent);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(DisplayData, parent);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_parent();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                parent, uint64_t);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.set_parent(*(uint64_t *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(parent_uid);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, parent_uid);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_parent_uid();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                parent_uid, uint64_t);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(children);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, children);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_children();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, children,
                  Low::Util::List<uint64_t>);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(absolute_pixel_position);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, absolute_pixel_position);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_absolute_pixel_position();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, absolute_pixel_position,
                  Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(absolute_rotation);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, absolute_rotation);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_absolute_rotation();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, absolute_rotation, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(absolute_pixel_scale);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, absolute_pixel_scale);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_absolute_pixel_scale();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                absolute_pixel_scale,
                                                Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(absolute_layer);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, absolute_layer);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT32;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_absolute_layer();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, absolute_layer, uint32_t);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(world_matrix);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, world_matrix);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_world_matrix();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                world_matrix,
                                                Low::Math::Matrix4x4);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(world_updated);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, world_updated);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.is_world_updated();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                world_updated, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.set_world_updated(*(bool *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_element();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, element, Low::Core::UI::Element);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.set_element(*(Low::Core::UI::Element *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(unique_id);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Display, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(dirty);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(DisplayData, dirty);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.is_dirty();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                dirty, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.set_dirty(*(bool *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(world_dirty);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(DisplayData, world_dirty);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Display l_Handle = p_Handle.get_id();
              l_Handle.is_world_dirty();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Display,
                                                world_dirty, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Display l_Handle = p_Handle.get_id();
              l_Handle.set_world_dirty(*(bool *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::FunctionInfo l_FunctionInfo;
            l_FunctionInfo.name = N(recalculate_world_transform);
            l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
            l_FunctionInfo.handleType = 0;
            l_TypeInfo.functions[l_FunctionInfo.name] =
                l_FunctionInfo;
          }
          {
            Low::Util::RTTI::FunctionInfo l_FunctionInfo;
            l_FunctionInfo.name = N(get_absolute_layer_float);
            l_FunctionInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_FunctionInfo.handleType = 0;
            l_TypeInfo.functions[l_FunctionInfo.name] =
                l_FunctionInfo;
          }
          {
            Low::Util::RTTI::FunctionInfo l_FunctionInfo;
            l_FunctionInfo.name = N(point_is_in_bounding_box);
            l_FunctionInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_FunctionInfo.handleType = 0;
            {
              Low::Util::RTTI::ParameterInfo l_ParameterInfo;
              l_ParameterInfo.name = N(p_Point);
              l_ParameterInfo.type =
                  Low::Util::RTTI::PropertyType::VECTOR2;
              l_ParameterInfo.handleType = 0;
              l_FunctionInfo.parameters.push_back(l_ParameterInfo);
            }
            l_TypeInfo.functions[l_FunctionInfo.name] =
                l_FunctionInfo;
          }
          Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
        }

        void Display::cleanup()
        {
          Low::Util::List<Display> l_Instances = ms_LivingInstances;
          for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
            l_Instances[i].destroy();
          }
          free(ms_Buffer);
          free(ms_Slots);

          LOW_PROFILE_FREE(type_buffer_Display);
          LOW_PROFILE_FREE(type_slots_Display);
        }

        Low::Util::Handle Display::_find_by_index(uint32_t p_Index)
        {
          return find_by_index(p_Index).get_id();
        }

        Display Display::find_by_index(uint32_t p_Index)
        {
          LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

          Display l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Generation =
              ms_Slots[p_Index].m_Generation;
          l_Handle.m_Data.m_Type = Display::TYPE_ID;

          return l_Handle;
        }

        bool Display::is_alive() const
        {
          return m_Data.m_Type == Display::TYPE_ID &&
                 check_alive(ms_Slots, Display::get_capacity());
        }

        uint32_t Display::get_capacity()
        {
          return ms_Capacity;
        }

        Display
        Display::duplicate(Low::Core::UI::Element p_Element) const
        {
          _LOW_ASSERT(is_alive());

          Display l_Handle = make(p_Element);
          l_Handle.pixel_position(pixel_position());
          l_Handle.rotation(rotation());
          l_Handle.pixel_scale(pixel_scale());
          l_Handle.layer(layer());
          l_Handle.set_dirty(is_dirty());
          l_Handle.set_world_dirty(is_world_dirty());

          // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
          return 0;
          // LOW_CODEGEN::END::CUSTOM:DUPLICATE

          return l_Handle;
        }

        Display Display::duplicate(Display p_Handle,
                                   Low::Core::UI::Element p_Element)
        {
          return p_Handle.duplicate(p_Element);
        }

        Low::Util::Handle
        Display::_duplicate(Low::Util::Handle p_Handle,
                            Low::Util::Handle p_Element)
        {
          Display l_Display = p_Handle.get_id();
          Low::Core::UI::Element l_Element = p_Element.get_id();
          return l_Display.duplicate(l_Element);
        }

        void Display::serialize(Low::Util::Yaml::Node &p_Node) const
        {
          _LOW_ASSERT(is_alive());

          Low::Util::Serialization::serialize(
              p_Node["pixel_position"], pixel_position());
          p_Node["rotation"] = rotation();
          Low::Util::Serialization::serialize(p_Node["pixel_scale"],
                                              pixel_scale());
          p_Node["layer"] = layer();
          p_Node["parent_uid"] = get_parent_uid();
          p_Node["unique_id"] = get_unique_id();

          // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
          // LOW_CODEGEN::END::CUSTOM:SERIALIZER
        }

        void Display::serialize(Low::Util::Handle p_Handle,
                                Low::Util::Yaml::Node &p_Node)
        {
          Display l_Display = p_Handle.get_id();
          l_Display.serialize(p_Node);
        }

        Low::Util::Handle
        Display::deserialize(Low::Util::Yaml::Node &p_Node,
                             Low::Util::Handle p_Creator)
        {
          Display l_Handle = Display::make(p_Creator.get_id());

          if (p_Node["unique_id"]) {
            Low::Util::remove_unique_id(l_Handle.get_unique_id());
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<uint64_t>());
            Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                          l_Handle.get_id());
          }

          if (p_Node["pixel_position"]) {
            l_Handle.pixel_position(
                Low::Util::Serialization::deserialize_vector2(
                    p_Node["pixel_position"]));
          }
          if (p_Node["rotation"]) {
            l_Handle.rotation(p_Node["rotation"].as<float>());
          }
          if (p_Node["pixel_scale"]) {
            l_Handle.pixel_scale(
                Low::Util::Serialization::deserialize_vector2(
                    p_Node["pixel_scale"]));
          }
          if (p_Node["layer"]) {
            l_Handle.layer(p_Node["layer"].as<uint32_t>());
          }
          if (p_Node["parent_uid"]) {
            l_Handle.set_parent_uid(
                p_Node["parent_uid"].as<uint64_t>());
          }
          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        Low::Math::Vector2 &Display::pixel_position() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_position
          // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_position

          return TYPE_SOA(Display, pixel_position,
                          Low::Math::Vector2);
        }
        void Display::pixel_position(Low::Math::Vector2 &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_position
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_position

          if (pixel_position() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, pixel_position, Low::Math::Vector2) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_position
            // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_position
          }
        }

        float Display::rotation() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_rotation
          // LOW_CODEGEN::END::CUSTOM:GETTER_rotation

          return TYPE_SOA(Display, rotation, float);
        }
        void Display::rotation(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation

          if (rotation() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, rotation, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation
            // LOW_CODEGEN::END::CUSTOM:SETTER_rotation
          }
        }

        Low::Math::Vector2 &Display::pixel_scale() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pixel_scale
          // LOW_CODEGEN::END::CUSTOM:GETTER_pixel_scale

          return TYPE_SOA(Display, pixel_scale, Low::Math::Vector2);
        }
        void Display::pixel_scale(Low::Math::Vector2 &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pixel_scale
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_pixel_scale

          if (pixel_scale() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, pixel_scale, Low::Math::Vector2) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pixel_scale
            // LOW_CODEGEN::END::CUSTOM:SETTER_pixel_scale
          }
        }

        uint32_t Display::layer() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_layer
          // LOW_CODEGEN::END::CUSTOM:GETTER_layer

          return TYPE_SOA(Display, layer, uint32_t);
        }
        void Display::layer(uint32_t p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_layer
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_layer

          if (layer() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, layer, uint32_t) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_layer
            // LOW_CODEGEN::END::CUSTOM:SETTER_layer
          }
        }

        uint64_t Display::get_parent() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent
          // LOW_CODEGEN::END::CUSTOM:GETTER_parent

          return TYPE_SOA(Display, parent, uint64_t);
        }
        void Display::set_parent(uint64_t p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent
          Display l_Parent = get_parent();
          if (l_Parent.is_alive()) {
            for (auto it = l_Parent.get_children().begin();
                 it != l_Parent.get_children().end();) {
              if (*it == get_id()) {
                it = l_Parent.get_children().erase(it);
              } else {
                ++it;
              }
            }
          }
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent

          if (get_parent() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, parent, uint64_t) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent
            Display l_Parent(p_Value);
            if (l_Parent.is_alive()) {
              set_parent_uid(l_Parent.get_unique_id());
              l_Parent.get_children().push_back(get_id());
            } else {
              set_parent_uid(0);
            }
            // LOW_CODEGEN::END::CUSTOM:SETTER_parent
          }
        }

        uint64_t Display::get_parent_uid() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_parent_uid
          // LOW_CODEGEN::END::CUSTOM:GETTER_parent_uid

          return TYPE_SOA(Display, parent_uid, uint64_t);
        }
        void Display::set_parent_uid(uint64_t p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_parent_uid
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_parent_uid

          if (get_parent_uid() != p_Value) {
            // Set dirty flags
            TYPE_SOA(Display, dirty, bool) = true;
            TYPE_SOA(Display, world_dirty, bool) = true;

            // Set new value
            TYPE_SOA(Display, parent_uid, uint64_t) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_parent_uid
            // LOW_CODEGEN::END::CUSTOM:SETTER_parent_uid
          }
        }

        Low::Util::List<uint64_t> &Display::get_children() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_children
          // LOW_CODEGEN::END::CUSTOM:GETTER_children

          return TYPE_SOA(Display, children,
                          Low::Util::List<uint64_t>);
        }

        Low::Math::Vector2 &Display::get_absolute_pixel_position()
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_absolute_pixel_position
          recalculate_world_transform();
          // LOW_CODEGEN::END::CUSTOM:GETTER_absolute_pixel_position

          return TYPE_SOA(Display, absolute_pixel_position,
                          Low::Math::Vector2);
        }
        void Display::set_absolute_pixel_position(
            Low::Math::Vector2 &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_absolute_pixel_position
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_absolute_pixel_position

          // Set new value
          TYPE_SOA(Display, absolute_pixel_position,
                   Low::Math::Vector2) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_absolute_pixel_position
          // LOW_CODEGEN::END::CUSTOM:SETTER_absolute_pixel_position
        }

        float Display::get_absolute_rotation()
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_absolute_rotation
          recalculate_world_transform();
          // LOW_CODEGEN::END::CUSTOM:GETTER_absolute_rotation

          return TYPE_SOA(Display, absolute_rotation, float);
        }
        void Display::set_absolute_rotation(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_absolute_rotation
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_absolute_rotation

          // Set new value
          TYPE_SOA(Display, absolute_rotation, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_absolute_rotation
          // LOW_CODEGEN::END::CUSTOM:SETTER_absolute_rotation
        }

        Low::Math::Vector2 &Display::get_absolute_pixel_scale()
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_absolute_pixel_scale
          recalculate_world_transform();
          // LOW_CODEGEN::END::CUSTOM:GETTER_absolute_pixel_scale

          return TYPE_SOA(Display, absolute_pixel_scale,
                          Low::Math::Vector2);
        }
        void
        Display::set_absolute_pixel_scale(Low::Math::Vector2 &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_absolute_pixel_scale
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_absolute_pixel_scale

          // Set new value
          TYPE_SOA(Display, absolute_pixel_scale,
                   Low::Math::Vector2) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_absolute_pixel_scale
          // LOW_CODEGEN::END::CUSTOM:SETTER_absolute_pixel_scale
        }

        uint32_t Display::get_absolute_layer()
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_absolute_layer
          recalculate_world_transform();
          // LOW_CODEGEN::END::CUSTOM:GETTER_absolute_layer

          return TYPE_SOA(Display, absolute_layer, uint32_t);
        }
        void Display::set_absolute_layer(uint32_t p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_absolute_layer
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_absolute_layer

          // Set new value
          TYPE_SOA(Display, absolute_layer, uint32_t) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_absolute_layer
          // LOW_CODEGEN::END::CUSTOM:SETTER_absolute_layer
        }

        Low::Math::Matrix4x4 &Display::get_world_matrix()
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_matrix
          recalculate_world_transform();
          // LOW_CODEGEN::END::CUSTOM:GETTER_world_matrix

          return TYPE_SOA(Display, world_matrix,
                          Low::Math::Matrix4x4);
        }
        void Display::set_world_matrix(Low::Math::Matrix4x4 &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_matrix
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_matrix

          // Set new value
          TYPE_SOA(Display, world_matrix, Low::Math::Matrix4x4) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_matrix
          // LOW_CODEGEN::END::CUSTOM:SETTER_world_matrix
        }

        bool Display::is_world_updated() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:GETTER_world_updated

          return TYPE_SOA(Display, world_updated, bool);
        }
        void Display::set_world_updated(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_updated

          // Set new value
          TYPE_SOA(Display, world_updated, bool) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:SETTER_world_updated
        }

        Low::Core::UI::Element Display::get_element() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_element
          // LOW_CODEGEN::END::CUSTOM:GETTER_element

          return TYPE_SOA(Display, element, Low::Core::UI::Element);
        }
        void Display::set_element(Low::Core::UI::Element p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_element
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_element

          // Set new value
          TYPE_SOA(Display, element, Low::Core::UI::Element) =
              p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_element
          // LOW_CODEGEN::END::CUSTOM:SETTER_element
        }

        Low::Util::UniqueId Display::get_unique_id() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

          return TYPE_SOA(Display, unique_id, Low::Util::UniqueId);
        }
        void Display::set_unique_id(Low::Util::UniqueId p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

          // Set new value
          TYPE_SOA(Display, unique_id, Low::Util::UniqueId) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
        }

        bool Display::is_dirty() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

          return TYPE_SOA(Display, dirty, bool);
        }
        void Display::set_dirty(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

          // Set new value
          TYPE_SOA(Display, dirty, bool) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:SETTER_dirty
        }

        bool Display::is_world_dirty() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_dirty
          if (get_element().get_view().is_alive() &&
              get_element().get_view().is_transform_dirty()) {
            return true;
          }

          if (TYPE_SOA(Display, world_dirty, bool)) {
            return TYPE_SOA(Display, world_dirty, bool);
          }

          Display l_Parent = get_parent();

          if (l_Parent.is_alive()) {
            return l_Parent.is_world_dirty() ||
                   l_Parent.is_world_updated();
          }
          // LOW_CODEGEN::END::CUSTOM:GETTER_world_dirty

          return TYPE_SOA(Display, world_dirty, bool);
        }
        void Display::set_world_dirty(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_dirty
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_dirty

          // Set new value
          TYPE_SOA(Display, world_dirty, bool) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_dirty
          // LOW_CODEGEN::END::CUSTOM:SETTER_world_dirty
        }

        void Display::recalculate_world_transform()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_recalculate_world_transform
          LOW_ASSERT(is_alive(), "Cannot calculate world position of "
                                 "dead ui display");

          if (!is_world_dirty() || is_world_updated()) {
            return;
          }

          if (!Display(get_parent()).is_alive() &&
              get_parent_uid() != 0) {
            set_parent(
                Util::find_handle_by_unique_id(get_parent_uid())
                    .get_id());
          }

          Element l_Element = get_element();
          Math::Vector2 l_Position = pixel_position();
          float l_Rotation = rotation();
          Math::Vector2 l_Scale = pixel_scale();
          uint32_t l_Layer = layer();

          Display l_Parent = get_parent();

          Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

          if (l_Parent.is_alive()) {
            l_Position *= l_Element.get_view().scale_multiplier();
            if (l_Parent.is_world_dirty()) {
              l_Parent.recalculate_world_transform();
            }

            Math::Vector2 l_ParentPosition =
                l_Parent.get_absolute_pixel_position();
            float l_ParentRotation = l_Parent.get_absolute_rotation();
            Math::Vector2 l_ParentScale =
                l_Parent.get_absolute_pixel_scale();
            uint32_t l_ParentLayer = l_Parent.get_absolute_layer();

            l_Position += l_ParentPosition;
            l_Rotation += l_ParentRotation;
            // Could lead to some unwanted stretching
            // TODO: Introduce overall multiplier variable
            // on display to scale everything while keeping
            // dimensions l_Scale *= l_ParentScale;
            l_Layer += l_ParentLayer;
          } else {
            l_Position += l_Element.get_view().pixel_position();
            l_Rotation += l_Element.get_view().rotation();
            l_Layer += l_Element.get_view().layer_offset();
          }

          l_Scale *= l_Element.get_view().scale_multiplier();

          set_absolute_pixel_position(l_Position);
          set_absolute_rotation(l_Rotation);
          set_absolute_pixel_scale(l_Scale);
          set_absolute_layer(l_Layer);

          l_LocalMatrix = glm::translate(
              l_LocalMatrix,
              Math::Vector3(l_Position.x, l_Position.y + l_Scale.y,
                            get_absolute_layer_float()));
          l_LocalMatrix *= glm::toMat4(Math::VectorUtil::from_euler(
              Math::Vector3(0.0f, 0.0f, l_Rotation)));
          l_LocalMatrix =
              glm::scale(l_LocalMatrix,
                         Math::Vector3(l_Scale.x, l_Scale.y, 1.0f));

          set_world_matrix(l_LocalMatrix);

          set_world_dirty(false);
          set_world_updated(true);
          // LOW_CODEGEN::END::CUSTOM:FUNCTION_recalculate_world_transform
        }

        float Display::get_absolute_layer_float()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_get_absolute_layer_float
          float l_AbsoluteLayer =
              (float)TYPE_SOA(Display, absolute_layer, uint32_t);
          return l_AbsoluteLayer * 0.1f;
          // LOW_CODEGEN::END::CUSTOM:FUNCTION_get_absolute_layer_float
        }

        bool
        Display::point_is_in_bounding_box(Low::Math::Vector2 &p_Point)
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_point_is_in_bounding_box
          Math::Vector2 l_Position = get_absolute_pixel_position();
          Math::Vector2 l_Size = get_absolute_pixel_scale();

          if (p_Point.x < l_Position.x || p_Point.y < l_Position.y) {
            return false;
          }

          if (p_Point.x > (l_Position.x + l_Size.x)) {
            return false;
          }

          if (p_Point.y > (l_Position.y + l_Size.y)) {
            return false;
          }

          return true;
          // LOW_CODEGEN::END::CUSTOM:FUNCTION_point_is_in_bounding_box
        }

        uint32_t Display::create_instance()
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

        void Display::increase_budget()
        {
          uint32_t l_Capacity = get_capacity();
          uint32_t l_CapacityIncrease =
              std::max(std::min(l_Capacity, 64u), 1u);
          l_CapacityIncrease = std::min(l_CapacityIncrease,
                                        LOW_UINT32_MAX - l_Capacity);

          LOW_ASSERT(l_CapacityIncrease > 0,
                     "Could not increase capacity");

          uint8_t *l_NewBuffer =
              (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                                sizeof(DisplayData));
          Low::Util::Instances::Slot *l_NewSlots =
              (Low::Util::Instances::Slot *)malloc(
                  (l_Capacity + l_CapacityIncrease) *
                  sizeof(Low::Util::Instances::Slot));

          memcpy(l_NewSlots, ms_Slots,
                 l_Capacity * sizeof(Low::Util::Instances::Slot));
          {
            memcpy(
                &l_NewBuffer[offsetof(DisplayData, pixel_position) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(DisplayData, pixel_position) *
                           (l_Capacity)],
                l_Capacity * sizeof(Low::Math::Vector2));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, rotation) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, rotation) *
                              (l_Capacity)],
                   l_Capacity * sizeof(float));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, pixel_scale) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, pixel_scale) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Math::Vector2));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, layer) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, layer) *
                              (l_Capacity)],
                   l_Capacity * sizeof(uint32_t));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, parent) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, parent) *
                              (l_Capacity)],
                   l_Capacity * sizeof(uint64_t));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, parent_uid) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, parent_uid) *
                              (l_Capacity)],
                   l_Capacity * sizeof(uint64_t));
          }
          {
            for (auto it = ms_LivingInstances.begin();
                 it != ms_LivingInstances.end(); ++it) {
              auto *i_ValPtr = new (
                  &l_NewBuffer[offsetof(DisplayData, children) *
                                   (l_Capacity + l_CapacityIncrease) +
                               (it->get_index() *
                                sizeof(Low::Util::List<uint64_t>))])
                  Low::Util::List<uint64_t>();
              *i_ValPtr = it->get_children();
            }
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData,
                                         absolute_pixel_position) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData,
                                       absolute_pixel_position) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Math::Vector2));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(DisplayData,
                                      absolute_rotation) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(DisplayData, absolute_rotation) *
                           (l_Capacity)],
                l_Capacity * sizeof(float));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData,
                                         absolute_pixel_scale) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData,
                                       absolute_pixel_scale) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Math::Vector2));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(DisplayData, absolute_layer) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(DisplayData, absolute_layer) *
                           (l_Capacity)],
                l_Capacity * sizeof(uint32_t));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, world_matrix) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, world_matrix) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Math::Matrix4x4));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, world_updated) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, world_updated) *
                              (l_Capacity)],
                   l_Capacity * sizeof(bool));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, element) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, element) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Core::UI::Element));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, unique_id) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, unique_id) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Util::UniqueId));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, dirty) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, dirty) *
                              (l_Capacity)],
                   l_Capacity * sizeof(bool));
          }
          {
            memcpy(&l_NewBuffer[offsetof(DisplayData, world_dirty) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(DisplayData, world_dirty) *
                              (l_Capacity)],
                   l_Capacity * sizeof(bool));
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

          LOW_LOG_DEBUG << "Auto-increased budget for Display from "
                        << l_Capacity << " to "
                        << (l_Capacity + l_CapacityIncrease)
                        << LOW_LOG_END;
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
