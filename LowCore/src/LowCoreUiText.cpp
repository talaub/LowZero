#include "LowCoreUiText.h"

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

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        const uint16_t Text::TYPE_ID = 42;
        uint32_t Text::ms_Capacity = 0u;
        uint32_t Text::ms_PageSize = 0u;
        Low::Util::SharedMutex Text::ms_LivingMutex;
        Low::Util::SharedMutex Text::ms_PagesMutex;
        Low::Util::UniqueLock<Low::Util::SharedMutex>
            Text::ms_PagesLock(Text::ms_PagesMutex, std::defer_lock);
        Low::Util::List<Text> Text::ms_LivingInstances;
        Low::Util::List<Low::Util::Instances::Page *> Text::ms_Pages;

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
          return make(p_Element, 0ull);
        }

        Text Text::make(Low::Core::UI::Element p_Element,
                        Low::Util::UniqueId p_UniqueId)
        {
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
          uint32_t l_Index =
              create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

          Text l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
          l_Handle.m_Data.m_Type = Text::TYPE_ID;

          l_PageLock.unlock();

          Low::Util::HandleLock<Text> l_HandleLock(l_Handle);

          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Text, text,
                                     Low::Util::String))
              Low::Util::String();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Text, font,
                                     Low::Renderer::Font))
              Low::Renderer::Font();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Text, color,
                                     Low::Math::Color))
              Low::Math::Color();
          ACCESSOR_TYPE_SOA(l_Handle, Text, size, float) = 0.0f;
          new (ACCESSOR_TYPE_SOA_PTR(
              l_Handle, Text, content_fit_approach,
              TextContentFitOptions)) TextContentFitOptions();
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Text, element,
                                     Low::Core::UI::Element))
              Low::Core::UI::Element();

          l_Handle.set_element(p_Element);
          p_Element.add_component(l_Handle);

          {
            Low::Util::UniqueLock<Low::Util::SharedMutex>
                l_LivingLock(ms_LivingMutex);
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

        void Text::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          {
            Low::Util::HandleLock<Text> l_Lock(get_id());
            // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
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

        void Text::initialize()
        {
          LOCK_PAGES_WRITE(l_PagesLock);
          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Text));

          ms_PageSize = Low::Math::Util::clamp(
              Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
          {
            u32 l_Capacity = 0u;
            while (l_Capacity < ms_Capacity) {
              Low::Util::Instances::Page *i_Page =
                  new Low::Util::Instances::Page;
              Low::Util::Instances::initialize_page(
                  i_Page, Text::Data::get_size(), ms_PageSize);
              ms_Pages.push_back(i_Page);
              l_Capacity += ms_PageSize;
            }
            ms_Capacity = l_Capacity;
          }
          LOCK_UNLOCK(l_PagesLock);

          Low::Util::RTTI::TypeInfo l_TypeInfo;
          l_TypeInfo.name = N(Text);
          l_TypeInfo.typeId = TYPE_ID;
          l_TypeInfo.get_capacity = &get_capacity;
          l_TypeInfo.is_alive = &Text::is_alive;
          l_TypeInfo.destroy = &Text::destroy;
          l_TypeInfo.serialize = &Text::serialize;
          l_TypeInfo.deserialize = &Text::deserialize;
          l_TypeInfo.find_by_index = &Text::_find_by_index;
          l_TypeInfo.notify = &Text::_notify;
          l_TypeInfo.make_default = nullptr;
          l_TypeInfo.make_component = &Text::_make;
          l_TypeInfo.duplicate_default = nullptr;
          l_TypeInfo.duplicate_component = &Text::_duplicate;
          l_TypeInfo.get_living_instances = reinterpret_cast<
              Low::Util::RTTI::LivingInstancesGetter>(
              &Text::living_instances);
          l_TypeInfo.get_living_count = &Text::living_count;
          l_TypeInfo.component = false;
          l_TypeInfo.uiComponent = true;
          {
            // Property: text
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(text);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(Text::Data, text);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::STRING;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_text();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, text,
                                                Low::Util::String);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_text(*(Low::Util::String *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((Low::Util::String *)p_Data) = l_Handle.get_text();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: text
          }
          {
            // Property: font
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(font);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(Text::Data, font);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType = Low::Renderer::Font::TYPE_ID;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_font();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, font,
                                                Low::Renderer::Font);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_font(*(Low::Renderer::Font *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((Low::Renderer::Font *)p_Data) = l_Handle.get_font();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: font
          }
          {
            // Property: color
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(color);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(Text::Data, color);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_color();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, color,
                                                Low::Math::Color);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_color(*(Low::Math::Color *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((Low::Math::Color *)p_Data) = l_Handle.get_color();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: color
          }
          {
            // Property: size
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(size);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(Text::Data, size);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_size();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Text, size,
                                                float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_size(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((float *)p_Data) = l_Handle.get_size();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: size
          }
          {
            // Property: content_fit_approach
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(content_fit_approach);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Text::Data, content_fit_approach);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UNKNOWN;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_content_fit_approach();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Text, content_fit_approach,
                  TextContentFitOptions);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_content_fit_approach(
                  *(TextContentFitOptions *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((TextContentFitOptions *)p_Data) =
                  l_Handle.get_content_fit_approach();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: content_fit_approach
          }
          {
            // Property: element
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(Text::Data, element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::TYPE_ID;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_element();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Text, element, Low::Core::UI::Element);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Text l_Handle = p_Handle.get_id();
              l_Handle.set_element(*(Low::Core::UI::Element *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((Low::Core::UI::Element *)p_Data) =
                  l_Handle.get_element();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: element
          }
          {
            // Property: unique_id
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(unique_id);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Text::Data, unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Text, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Text l_Handle = p_Handle.get_id();
              Low::Util::HandleLock<Text> l_HandleLock(l_Handle);
              *((Low::Util::UniqueId *)p_Data) =
                  l_Handle.get_unique_id();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: unique_id
          }
          Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
        }

        void Text::cleanup()
        {
          Low::Util::List<Text> l_Instances = ms_LivingInstances;
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

        Low::Util::Handle Text::_find_by_index(uint32_t p_Index)
        {
          return find_by_index(p_Index).get_id();
        }

        Text Text::find_by_index(uint32_t p_Index)
        {
          LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

          Text l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Type = Text::TYPE_ID;

          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          if (!get_page_for_index(p_Index, l_PageIndex,
                                  l_SlotIndex)) {
            l_Handle.m_Data.m_Generation = 0;
          }
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
          Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
              l_Page->mutex);
          l_Handle.m_Data.m_Generation =
              l_Page->slots[l_SlotIndex].m_Generation;

          return l_Handle;
        }

        Text Text::create_handle_by_index(u32 p_Index)
        {
          if (p_Index < get_capacity()) {
            return find_by_index(p_Index);
          }

          Text l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Generation = 0;
          l_Handle.m_Data.m_Type = Text::TYPE_ID;

          return l_Handle;
        }

        bool Text::is_alive() const
        {
          if (m_Data.m_Type != Text::TYPE_ID) {
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
          return m_Data.m_Type == Text::TYPE_ID &&
                 l_Page->slots[l_SlotIndex].m_Occupied &&
                 l_Page->slots[l_SlotIndex].m_Generation ==
                     m_Data.m_Generation;
        }

        uint32_t Text::get_capacity()
        {
          return ms_Capacity;
        }

        Text Text::duplicate(Low::Core::UI::Element p_Element) const
        {
          _LOW_ASSERT(is_alive());

          Text l_Handle = make(p_Element);
          l_Handle.set_text(get_text());
          if (get_font().is_alive()) {
            l_Handle.set_font(get_font());
          }
          l_Handle.set_color(get_color());
          l_Handle.set_size(get_size());
          l_Handle.set_content_fit_approach(
              get_content_fit_approach());

          // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

          // LOW_CODEGEN::END::CUSTOM:DUPLICATE

          return l_Handle;
        }

        Text Text::duplicate(Text p_Handle,
                             Low::Core::UI::Element p_Element)
        {
          return p_Handle.duplicate(p_Element);
        }

        Low::Util::Handle
        Text::_duplicate(Low::Util::Handle p_Handle,
                         Low::Util::Handle p_Element)
        {
          Text l_Text = p_Handle.get_id();
          Low::Core::UI::Element l_Element = p_Element.get_id();
          return l_Text.duplicate(l_Element);
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
          p_Node["_unique_id"] =
              Low::Util::hash_to_string(get_unique_id()).c_str();

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
          Low::Util::UniqueId l_HandleUniqueId = 0ull;
          if (p_Node["unique_id"]) {
            l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
          } else if (p_Node["_unique_id"]) {
            l_HandleUniqueId = Low::Util::string_to_hash(
                LOW_YAML_AS_STRING(p_Node["_unique_id"]));
          }

          Text l_Handle =
              Text::make(p_Creator.get_id(), l_HandleUniqueId);

          if (p_Node["text"]) {
            l_Handle.set_text(LOW_YAML_AS_STRING(p_Node["text"]));
          }
          if (p_Node["font"]) {
            l_Handle.set_font(Low::Renderer::Font::deserialize(
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
          if (p_Node["content_fit_approach"]) {
          }
          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        void
        Text::broadcast_observable(Low::Util::Name p_Observable) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          Low::Util::notify(l_Key);
        }

        u64 Text::observe(Low::Util::Name p_Observable,
                          Low::Util::Function<void(Low::Util::Handle,
                                                   Low::Util::Name)>
                              p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        u64 Text::observe(Low::Util::Name p_Observable,
                          Low::Util::Handle p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        void Text::notify(Low::Util::Handle p_Observed,
                          Low::Util::Name p_Observable)
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
          // LOW_CODEGEN::END::CUSTOM:NOTIFY
        }

        void Text::_notify(Low::Util::Handle p_Observer,
                           Low::Util::Handle p_Observed,
                           Low::Util::Name p_Observable)
        {
          Text l_Text = p_Observer.get_id();
          l_Text.notify(p_Observed, p_Observable);
        }

        Low::Util::String &Text::get_text() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_text

          // LOW_CODEGEN::END::CUSTOM:GETTER_text

          return TYPE_SOA(Text, text, Low::Util::String);
        }
        void Text::set_text(const char *p_Value)
        {
          Low::Util::String l_Val(p_Value);
          set_text(l_Val);
        }

        void Text::set_text(Low::Util::String &p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_text

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_text

          // Set new value
          TYPE_SOA(Text, text, Low::Util::String) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_text

          // LOW_CODEGEN::END::CUSTOM:SETTER_text

          broadcast_observable(N(text));
        }

        Low::Renderer::Font Text::get_font() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_font

          // LOW_CODEGEN::END::CUSTOM:GETTER_font

          return TYPE_SOA(Text, font, Low::Renderer::Font);
        }
        void Text::set_font(Low::Renderer::Font p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_font
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_font

          // Set new value
          TYPE_SOA(Text, font, Low::Renderer::Font) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_font

          // LOW_CODEGEN::END::CUSTOM:SETTER_font

          broadcast_observable(N(font));
        }

        Low::Math::Color &Text::get_color() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_color

          // LOW_CODEGEN::END::CUSTOM:GETTER_color

          return TYPE_SOA(Text, color, Low::Math::Color);
        }
        void Text::set_color(Low::Math::Color &p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_color

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_color

          // Set new value
          TYPE_SOA(Text, color, Low::Math::Color) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_color

          if (p_Value.a < 0.0f) {
            Low::Math::Color l_Color = p_Value;
            p_Value.a = 0.0f;

            TYPE_SOA(Text, color, Low::Math::Color) = l_Color;
          }

          // LOW_CODEGEN::END::CUSTOM:SETTER_color

          broadcast_observable(N(color));
        }

        float Text::get_size() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size

          // LOW_CODEGEN::END::CUSTOM:GETTER_size

          return TYPE_SOA(Text, size, float);
        }
        void Text::set_size(float p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_size

          // Set new value
          TYPE_SOA(Text, size, float) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size

          // LOW_CODEGEN::END::CUSTOM:SETTER_size

          broadcast_observable(N(size));
        }

        TextContentFitOptions Text::get_content_fit_approach() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_content_fit_approach

          // LOW_CODEGEN::END::CUSTOM:GETTER_content_fit_approach

          return TYPE_SOA(Text, content_fit_approach,
                          TextContentFitOptions);
        }
        void
        Text::set_content_fit_approach(TextContentFitOptions p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_content_fit_approach

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_content_fit_approach

          // Set new value
          TYPE_SOA(Text, content_fit_approach,
                   TextContentFitOptions) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_content_fit_approach

          // LOW_CODEGEN::END::CUSTOM:SETTER_content_fit_approach

          broadcast_observable(N(content_fit_approach));
        }

        Low::Core::UI::Element Text::get_element() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_element

          // LOW_CODEGEN::END::CUSTOM:GETTER_element

          return TYPE_SOA(Text, element, Low::Core::UI::Element);
        }
        void Text::set_element(Low::Core::UI::Element p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_element

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_element

          // Set new value
          TYPE_SOA(Text, element, Low::Core::UI::Element) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_element

          // LOW_CODEGEN::END::CUSTOM:SETTER_element

          broadcast_observable(N(element));
        }

        Low::Util::UniqueId Text::get_unique_id() const
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id

          // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

          return TYPE_SOA(Text, unique_id, Low::Util::UniqueId);
        }
        void Text::set_unique_id(Low::Util::UniqueId p_Value)
        {
          _LOW_ASSERT(is_alive());
          Low::Util::HandleLock<Text> l_Lock(get_id());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id

          // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

          // Set new value
          TYPE_SOA(Text, unique_id, Low::Util::UniqueId) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id

          // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

          broadcast_observable(N(unique_id));
        }

        uint32_t Text::create_instance(
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

        u32 Text::create_page()
        {
          const u32 l_Capacity = get_capacity();
          LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                     "Could not increase capacity for Text.");

          Low::Util::Instances::Page *l_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              l_Page, Text::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(l_Page);

          ms_Capacity = l_Capacity + l_Page->size;
          return ms_Pages.size() - 1;
        }

        bool Text::get_page_for_index(const u32 p_Index,
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

      } // namespace Component
    } // namespace UI
  } // namespace Core
} // namespace Low
