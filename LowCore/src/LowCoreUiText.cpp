#include "LowCoreUiText.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        const uint16_t Text::TYPE_ID = 42;
        uint32_t Text::ms_Capacity = 0u;
        uint8_t *Text::ms_Buffer = 0;
        Low::Util::Instances::Slot *Text::ms_Slots = 0;
        Low::Util::List<Text> Text::ms_LivingInstances =
            Low::Util::List<Text>();

        Text::Text() : Low::Util::Handle(0ull)
        {
        }
        Text::Text(uint64_t p_Id) : Low::Util::Handle(p_Id)
        {
        }
        Text::Text(Text &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
        {
        }

        Low::Util::Handle Text::_make(Low::Util::Handle p_Element)
        {
          Low::Core::UI::Element l_Element = p_Element.get_id();
          LOW_ASSERT(l_Element.is_alive(),
                     "Cannot create component for dead element");
          return make(l_Element).get_id();
        }

        Text Text::make(Low::Core::UI::Element p_Element)
        {
          uint32_t l_Index = create_instance();

          Text l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Slots[l_Index].m_Generation;
          l_Handle.m_Data.m_Type = Text::TYPE_ID;

          new (&ACCESSOR_TYPE_SOA(l_Handle, Text, text, Util::String))
              Util::String();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Text, font, Core::Font))
              Core::Font();
          new (&ACCESSOR_TYPE_SOA(l_Handle, Text, color, Math::Color))
              Math::Color();
          ACCESSOR_TYPE_SOA(l_Handle, Text, size, float) = 0.0f;
          new (&ACCESSOR_TYPE_SOA(l_Handle, Text, element,
                                  Low::Core::UI::Element))
              Low::Core::UI::Element();

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

        void Text::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          // LOW_CODEGEN::END::CUSTOM:DESTROY

          Low::Util::remove_unique_id(get_unique_id());

          ms_Slots[this->m_Data.m_Index].m_Occupied = false;
          ms_Slots[this->m_Data.m_Index].m_Generation++;

          const Text *l_Instances = living_instances();
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

        void Text::initialize()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Text));

          initialize_buffer(&ms_Buffer, TextData::get_size(),
                            get_capacity(), &ms_Slots);

          LOW_PROFILE_ALLOC(type_buffer_Text);
          LOW_PROFILE_ALLOC(type_slots_Text);

          Low::Util::RTTI::TypeInfo l_TypeInfo;
          l_TypeInfo.name = N(Text);
          l_TypeInfo.typeId = TYPE_ID;
          l_TypeInfo.get_capacity = &get_capacity;
          l_TypeInfo.is_alive = &Text::is_alive;
          l_TypeInfo.destroy = &Text::destroy;
          l_TypeInfo.serialize = &Text::serialize;
          l_TypeInfo.deserialize = &Text::deserialize;
          l_TypeInfo.make_default = nullptr;
          l_TypeInfo.make_component = &Text::_make;
          l_TypeInfo.get_living_instances = reinterpret_cast<
              Low::Util::RTTI::LivingInstancesGetter>(
              &Text::living_instances);
          l_TypeInfo.get_living_count = &Text::living_count;
          l_TypeInfo.component = false;
          l_TypeInfo.uiComponent = true;
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(text);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(TextData, text);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_text();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, text,
                                                Util::String);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_text(*(Util::String *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(font);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(TextData, font);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType = Core::Font::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_font();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, font,
                                                Core::Font);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_font(*(Core::Font *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(color);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(TextData, color);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_color();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, color,
                                                Math::Color);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_color(*(Math::Color *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(size);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(TextData, size);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_size();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, size,
                                                float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_size(*(float *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(TextData, element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::TYPE_ID;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_element();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Text, element, Low::Core::UI::Element);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_element(*(Low::Core::UI::Element *)p_Data);
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          {
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(unique_id);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(TextData, unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.get =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Text, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
          }
          Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
        }

        void Text::cleanup()
        {
          Low::Util::List<Text> l_Instances = ms_LivingInstances;
          for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
            l_Instances[i].destroy();
          }
          free(ms_Buffer);
          free(ms_Slots);

          LOW_PROFILE_FREE(type_buffer_Text);
          LOW_PROFILE_FREE(type_slots_Text);
        }

        Text Text::find_by_index(uint32_t p_Index)
        {
          LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

          Text l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Generation =
              ms_Slots[p_Index].m_Generation;
          l_Handle.m_Data.m_Type = Text::TYPE_ID;

          return l_Handle;
        }

        bool Text::is_alive() const
        {
          return m_Data.m_Type == Text::TYPE_ID &&
                 check_alive(ms_Slots, Text::get_capacity());
        }

        uint32_t Text::get_capacity()
        {
          return ms_Capacity;
        }

        void Text::serialize(Low::Util::Yaml::Node &p_Node) const
        {
          _LOW_ASSERT(is_alive());

          p_Node["text"] = get_text().c_str();
          if (get_font().is_alive()) {
            get_font().serialize(p_Node["font"]);
          }
          Low::Util::Serialization::serialize(p_Node["color"],
                                              get_color());
          p_Node["size"] = get_size();
          p_Node["unique_id"] = get_unique_id();

          // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
          // LOW_CODEGEN::END::CUSTOM:SERIALIZER
        }

        void Text::serialize(Low::Util::Handle p_Handle,
                             Low::Util::Yaml::Node &p_Node)
        {
          Text l_Text = p_Handle.get_id();
          l_Text.serialize(p_Node);
        }

        Low::Util::Handle
        Text::deserialize(Low::Util::Yaml::Node &p_Node,
                          Low::Util::Handle p_Creator)
        {
          Text l_Handle = Text::make(p_Creator.get_id());

          if (p_Node["unique_id"]) {
            Low::Util::remove_unique_id(l_Handle.get_unique_id());
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<uint64_t>());
            Low::Util::register_unique_id(l_Handle.get_unique_id(),
                                          l_Handle.get_id());
          }

          if (p_Node["text"]) {
            l_Handle.set_text(LOW_YAML_AS_STRING(p_Node["text"]));
          }
          if (p_Node["font"]) {
            l_Handle.set_font(Core::Font::deserialize(
                                  p_Node["font"], l_Handle.get_id())
                                  .get_id());
          }
          if (p_Node["color"]) {
            l_Handle.set_color(
                Low::Util::Serialization::deserialize_vector4(
                    p_Node["color"]));
          }
          if (p_Node["size"]) {
            l_Handle.set_size(p_Node["size"].as<float>());
          }
          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        Util::String &Text::get_text() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_text
          // LOW_CODEGEN::END::CUSTOM:GETTER_text

          return TYPE_SOA(Text, text, Util::String);
        }
        void Text::set_text(Util::String &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_text
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_text

          // Set new value
          TYPE_SOA(Text, text, Util::String) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_text
          // LOW_CODEGEN::END::CUSTOM:SETTER_text
        }

        Core::Font Text::get_font() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font
          // LOW_CODEGEN::END::CUSTOM:GETTER_font

          return TYPE_SOA(Text, font, Core::Font);
        }
        void Text::set_font(Core::Font p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_font

          // Set new value
          TYPE_SOA(Text, font, Core::Font) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font
          // LOW_CODEGEN::END::CUSTOM:SETTER_font
        }

        Math::Color &Text::get_color() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color
          // LOW_CODEGEN::END::CUSTOM:GETTER_color

          return TYPE_SOA(Text, color, Math::Color);
        }
        void Text::set_color(Math::Color &p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

          // Set new value
          TYPE_SOA(Text, color, Math::Color) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color
          // LOW_CODEGEN::END::CUSTOM:SETTER_color
        }

        float Text::get_size() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size
          // LOW_CODEGEN::END::CUSTOM:GETTER_size

          return TYPE_SOA(Text, size, float);
        }
        void Text::set_size(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_size

          // Set new value
          TYPE_SOA(Text, size, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size
          // LOW_CODEGEN::END::CUSTOM:SETTER_size
        }

        Low::Core::UI::Element Text::get_element() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_element
          // LOW_CODEGEN::END::CUSTOM:GETTER_element

          return TYPE_SOA(Text, element, Low::Core::UI::Element);
        }
        void Text::set_element(Low::Core::UI::Element p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_element
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_element

          // Set new value
          TYPE_SOA(Text, element, Low::Core::UI::Element) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_element
          // LOW_CODEGEN::END::CUSTOM:SETTER_element
        }

        Low::Util::UniqueId Text::get_unique_id() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

          return TYPE_SOA(Text, unique_id, Low::Util::UniqueId);
        }
        void Text::set_unique_id(Low::Util::UniqueId p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

          // Set new value
          TYPE_SOA(Text, unique_id, Low::Util::UniqueId) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id
        }

        uint32_t Text::create_instance()
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

        void Text::increase_budget()
        {
          uint32_t l_Capacity = get_capacity();
          uint32_t l_CapacityIncrease =
              std::max(std::min(l_Capacity, 64u), 1u);
          l_CapacityIncrease = std::min(l_CapacityIncrease,
                                        LOW_UINT32_MAX - l_Capacity);

          LOW_ASSERT(l_CapacityIncrease > 0,
                     "Could not increase capacity");

          uint8_t *l_NewBuffer = (uint8_t *)malloc(
              (l_Capacity + l_CapacityIncrease) * sizeof(TextData));
          Low::Util::Instances::Slot *l_NewSlots =
              (Low::Util::Instances::Slot *)malloc(
                  (l_Capacity + l_CapacityIncrease) *
                  sizeof(Low::Util::Instances::Slot));

          memcpy(l_NewSlots, ms_Slots,
                 l_Capacity * sizeof(Low::Util::Instances::Slot));
          {
            memcpy(
                &l_NewBuffer[offsetof(TextData, text) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(TextData, text) * (l_Capacity)],
                l_Capacity * sizeof(Util::String));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(TextData, font) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(TextData, font) * (l_Capacity)],
                l_Capacity * sizeof(Core::Font));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(TextData, color) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(TextData, color) * (l_Capacity)],
                l_Capacity * sizeof(Math::Color));
          }
          {
            memcpy(
                &l_NewBuffer[offsetof(TextData, size) *
                             (l_Capacity + l_CapacityIncrease)],
                &ms_Buffer[offsetof(TextData, size) * (l_Capacity)],
                l_Capacity * sizeof(float));
          }
          {
            memcpy(&l_NewBuffer[offsetof(TextData, element) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(TextData, element) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Core::UI::Element));
          }
          {
            memcpy(&l_NewBuffer[offsetof(TextData, unique_id) *
                                (l_Capacity + l_CapacityIncrease)],
                   &ms_Buffer[offsetof(TextData, unique_id) *
                              (l_Capacity)],
                   l_Capacity * sizeof(Low::Util::UniqueId));
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

          LOW_LOG_DEBUG << "Auto-increased budget for Text from "
                        << l_Capacity << " to "
                        << (l_Capacity + l_CapacityIncrease)
                        << LOW_LOG_END;
        }
      } // namespace Component
    }   // namespace UI
  }     // namespace Core
} // namespace Low
