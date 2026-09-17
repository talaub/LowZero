#include "LowCoreUiLayout.h"

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
#include "LowCoreUiDisplay.h"
#include "LowCoreUiScreen.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Core {
    namespace UI {
      namespace Component {
        // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
        // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

        u16 Layout::ms_TypeId = 0;
        const Low::Util::TypeIdentifier
            Layout::IDENTIFIER(LOW_NAME(1181529166),
                               LOW_NAME(1033268948));
        uint32_t Layout::ms_Capacity = 0u;
        uint32_t Layout::ms_PageSize = 0u;
        Low::Util::List<Layout> Layout::ms_LivingInstances;
        Low::Util::List<Low::Util::Instances::Page *>
            Layout::ms_Pages;

        Low::Util::Handle Layout::_make(Low::Util::Handle p_Element)
        {
          Low::Core::UI::Element l_Element = p_Element.get_id();
          LOW_ASSERT(l_Element.is_alive(),
                     "Cannot create component for dead element");
          return make(l_Element).get_id();
        }

        Layout Layout::make(Low::Core::UI::Element p_Element)
        {
          return make(p_Element, 0ull);
        }

        Layout Layout::make(Low::Core::UI::Element p_Element,
                            Low::Util::UniqueId p_UniqueId)
        {
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          uint32_t l_Index =
              create_instance(l_PageIndex, l_SlotIndex);

          Layout l_Handle;
          l_Handle.m_Data.m_Index = l_Index;
          l_Handle.m_Data.m_Generation =
              ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
          l_Handle.m_Data.m_Type = Layout::ms_TypeId;

          ACCESSOR_TYPE_SOA(l_Handle, Layout, margin_left, float) =
              0.0f;
          ACCESSOR_TYPE_SOA(l_Handle, Layout, margin_top, float) =
              0.0f;
          ACCESSOR_TYPE_SOA(l_Handle, Layout, margin_right, float) =
              0.0f;
          ACCESSOR_TYPE_SOA(l_Handle, Layout, margin_bottom, float) =
              0.0f;
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Layout, size_mode,
                                     LayoutSizeMode))
              LayoutSizeMode();
          ACCESSOR_TYPE_SOA(l_Handle, Layout, aspect_ratio, float) =
              0.0f;
          ACCESSOR_TYPE_SOA(l_Handle, Layout, world_updated, bool) =
              false;
          new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Layout, element,
                                     Low::Core::UI::Element))
              Low::Core::UI::Element();
          ACCESSOR_TYPE_SOA(l_Handle, Layout, dirty, bool) = false;
          ACCESSOR_TYPE_SOA(l_Handle, Layout, world_dirty, bool) =
              false;

          l_Handle.set_element(p_Element);
          p_Element.add_component(l_Handle);

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
          // LOW_CODEGEN::END::CUSTOM:MAKE

          return l_Handle;
        }

        void Layout::destroy()
        {
          LOW_ASSERT(is_alive(), "Cannot destroy dead object");

          {
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

        void Layout::initialize()
        {
          const Low::Util::TypeIdentifier l_IdentifierNames(
              N(LowCore), N(Layout));

          // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
          // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

          ms_Capacity =
              Low::Util::Config::get_capacity(N(LowCore), N(Layout));

          ms_PageSize = Low::Math::Util::clamp(
              Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
          {
            u32 l_Capacity = 0u;
            while (l_Capacity < ms_Capacity) {
              Low::Util::Instances::Page *i_Page =
                  new Low::Util::Instances::Page;
              Low::Util::Instances::initialize_page(
                  i_Page, Layout::Data::get_size(), ms_PageSize);
              ms_Pages.push_back(i_Page);
              l_Capacity += ms_PageSize;
            }
            ms_Capacity = l_Capacity;
          }

          Low::Util::RTTI::TypeInfo l_TypeInfo;
          l_TypeInfo.name = N(Layout);
          l_TypeInfo.typeId = ms_TypeId;
          l_TypeInfo.get_capacity = &get_capacity;
          l_TypeInfo.is_alive = &Layout::is_alive;
          l_TypeInfo.destroy = &Layout::destroy;
          l_TypeInfo.serialize = &Layout::serialize;
          l_TypeInfo.deserialize = &Layout::deserialize;
          l_TypeInfo.find_by_index = &Layout::_find_by_index;
          l_TypeInfo.notify = &Layout::_notify;
          l_TypeInfo.post_load = nullptr;
          l_TypeInfo.make_default = nullptr;
          l_TypeInfo.make_component = &Layout::_make;
          l_TypeInfo.duplicate_default = nullptr;
          l_TypeInfo.duplicate_component = &Layout::_duplicate;
          l_TypeInfo.get_living_instances = reinterpret_cast<
              Low::Util::RTTI::LivingInstancesGetter>(
              &Layout::living_instances);
          l_TypeInfo.get_living_count = &Layout::living_count;
          l_TypeInfo.component = false;
          l_TypeInfo.uiComponent = true;
          {
            // Property: anchor_min
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(anchor_min);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, anchor_min);
            l_PropertyInfo.size = sizeof(Layout::Data::anchor_min);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_anchor_min();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, anchor_min, Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_anchor_min(*(Low::Math::Vector2 *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((Low::Math::Vector2 *)p_Data) =
                  l_Handle.get_anchor_min();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: anchor_min
          }
          {
            // Property: anchor_max
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(anchor_max);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, anchor_max);
            l_PropertyInfo.size = sizeof(Layout::Data::anchor_max);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_anchor_max();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, anchor_max, Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_anchor_max(*(Low::Math::Vector2 *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((Low::Math::Vector2 *)p_Data) =
                  l_Handle.get_anchor_max();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: anchor_max
          }
          {
            // Property: margin_left
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(margin_left);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, margin_left);
            l_PropertyInfo.size = sizeof(Layout::Data::margin_left);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_margin_left();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                margin_left, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_margin_left(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((float *)p_Data) = l_Handle.get_margin_left();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: margin_left
          }
          {
            // Property: margin_top
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(margin_top);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, margin_top);
            l_PropertyInfo.size = sizeof(Layout::Data::margin_top);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_margin_top();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                margin_top, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_margin_top(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((float *)p_Data) = l_Handle.get_margin_top();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: margin_top
          }
          {
            // Property: margin_right
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(margin_right);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, margin_right);
            l_PropertyInfo.size = sizeof(Layout::Data::margin_right);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_margin_right();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                margin_right, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_margin_right(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((float *)p_Data) = l_Handle.get_margin_right();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: margin_right
          }
          {
            // Property: margin_bottom
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(margin_bottom);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, margin_bottom);
            l_PropertyInfo.size = sizeof(Layout::Data::margin_bottom);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_margin_bottom();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                margin_bottom, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_margin_bottom(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((float *)p_Data) = l_Handle.get_margin_bottom();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: margin_bottom
          }
          {
            // Property: size_mode
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(size_mode);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, size_mode);
            l_PropertyInfo.size = sizeof(Layout::Data::size_mode);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::ENUM;
            l_PropertyInfo.handleType =
                LayoutSizeModeEnum::get_enum_id();
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_size_mode();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, size_mode, LayoutSizeMode);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_size_mode(*(LayoutSizeMode *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((LayoutSizeMode *)p_Data) = l_Handle.get_size_mode();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: size_mode
          }
          {
            // Property: fixed_size
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(fixed_size);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, fixed_size);
            l_PropertyInfo.size = sizeof(Layout::Data::fixed_size);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_fixed_size();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, fixed_size, Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_fixed_size(*(Low::Math::Vector2 *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((Low::Math::Vector2 *)p_Data) =
                  l_Handle.get_fixed_size();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: fixed_size
          }
          {
            // Property: pivot
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(pivot);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset = offsetof(Layout::Data, pivot);
            l_PropertyInfo.size = sizeof(Layout::Data::pivot);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::VECTOR2;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_pivot();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, pivot, Low::Math::Vector2);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_pivot(*(Low::Math::Vector2 *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((Low::Math::Vector2 *)p_Data) = l_Handle.get_pivot();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: pivot
          }
          {
            // Property: aspect_ratio
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(aspect_ratio);
            l_PropertyInfo.editorProperty = true;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, aspect_ratio);
            l_PropertyInfo.size = sizeof(Layout::Data::aspect_ratio);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::FLOAT;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_aspect_ratio();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                aspect_ratio, float);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_aspect_ratio(*(float *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((float *)p_Data) = l_Handle.get_aspect_ratio();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: aspect_ratio
          }
          {
            // Property: world_updated
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(world_updated);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, world_updated);
            l_PropertyInfo.size = sizeof(Layout::Data::world_updated);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.is_world_updated();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                world_updated, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_world_updated(*(bool *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((bool *)p_Data) = l_Handle.is_world_updated();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: world_updated
          }
          {
            // Property: element
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(element);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, element);
            l_PropertyInfo.size = sizeof(Layout::Data::element);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::HANDLE;
            l_PropertyInfo.handleType =
                Low::Core::UI::Element::IDENTIFIER;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_element();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, element, Low::Core::UI::Element);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_element(*(Low::Core::UI::Element *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
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
                offsetof(Layout::Data, unique_id);
            l_PropertyInfo.size = sizeof(Layout::Data::unique_id);
            l_PropertyInfo.type =
                Low::Util::RTTI::PropertyType::UINT64;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.get_unique_id();
              return (void *)&ACCESSOR_TYPE_SOA(
                  p_Handle, Layout, unique_id, Low::Util::UniqueId);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {};
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((Low::Util::UniqueId *)p_Data) =
                  l_Handle.get_unique_id();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: unique_id
          }
          {
            // Property: dirty
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(dirty);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset = offsetof(Layout::Data, dirty);
            l_PropertyInfo.size = sizeof(Layout::Data::dirty);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.is_dirty();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                dirty, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_dirty(*(bool *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((bool *)p_Data) = l_Handle.is_dirty();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: dirty
          }
          {
            // Property: world_dirty
            Low::Util::RTTI::PropertyInfo l_PropertyInfo;
            l_PropertyInfo.name = N(world_dirty);
            l_PropertyInfo.editorProperty = false;
            l_PropertyInfo.dataOffset =
                offsetof(Layout::Data, world_dirty);
            l_PropertyInfo.size = sizeof(Layout::Data::world_dirty);
            l_PropertyInfo.type = Low::Util::RTTI::PropertyType::BOOL;
            l_PropertyInfo.handleType = 0;
            l_PropertyInfo.get_return =
                [](Low::Util::Handle p_Handle) -> void const * {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.is_world_dirty();
              return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Layout,
                                                world_dirty, bool);
            };
            l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                    const void *p_Data) -> void {
              Layout l_Handle = p_Handle.get_id();
              l_Handle.set_world_dirty(*(bool *)p_Data);
            };
            l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                    void *p_Data) {
              Layout l_Handle = p_Handle.get_id();
              *((bool *)p_Data) = l_Handle.is_world_dirty();
            };
            l_TypeInfo.properties[l_PropertyInfo.name] =
                l_PropertyInfo;
            // End property: world_dirty
          }
          {
            // Function: resolve
            Low::Util::RTTI::FunctionInfo l_FunctionInfo;
            l_FunctionInfo.name = N(resolve);
            l_FunctionInfo.type = Low::Util::RTTI::PropertyType::VOID;
            l_FunctionInfo.handleType = 0;
            l_TypeInfo.functions[l_FunctionInfo.name] =
                l_FunctionInfo;
            // End function: resolve
          }
          ms_TypeId = Low::Util::Handle::register_type_info(
              IDENTIFIER, l_TypeInfo);
          // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE
          // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
        }

        void Layout::cleanup()
        {
          Low::Util::List<Layout> l_Instances = ms_LivingInstances;
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

        Low::Util::Handle Layout::_find_by_index(uint32_t p_Index)
        {
          return find_by_index(p_Index).get_id();
        }

        Layout Layout::find_by_index(uint32_t p_Index)
        {
          LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

          Layout l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Type = Layout::ms_TypeId;

          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          if (!get_page_for_index(p_Index, l_PageIndex,
                                  l_SlotIndex)) {
            l_Handle.m_Data.m_Generation = 0;
          }
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
          l_Handle.m_Data.m_Generation =
              l_Page->slots[l_SlotIndex].m_Generation;

          return l_Handle;
        }

        Layout Layout::create_handle_by_index(u32 p_Index)
        {
          if (p_Index < get_capacity()) {
            return find_by_index(p_Index);
          }

          Layout l_Handle;
          l_Handle.m_Data.m_Index = p_Index;
          l_Handle.m_Data.m_Generation = 0;
          l_Handle.m_Data.m_Type = Layout::ms_TypeId;

          return l_Handle;
        }

        bool Layout::is_alive() const
        {
          if (m_Data.m_Type != Layout::ms_TypeId) {
            return false;
          }
          u32 l_PageIndex = 0;
          u32 l_SlotIndex = 0;
          if (!get_page_for_index(get_index(), l_PageIndex,
                                  l_SlotIndex)) {
            return false;
          }
          Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
          return m_Data.m_Type == Layout::ms_TypeId &&
                 l_Page->slots[l_SlotIndex].m_Occupied &&
                 l_Page->slots[l_SlotIndex].m_Generation ==
                     m_Data.m_Generation;
        }

        uint32_t Layout::get_capacity()
        {
          return ms_Capacity;
        }

        Layout
        Layout::duplicate(Low::Core::UI::Element p_Element) const
        {
          _LOW_ASSERT(is_alive());

          Layout l_Handle = make(p_Element);
          l_Handle.set_anchor_min(get_anchor_min());
          l_Handle.set_anchor_max(get_anchor_max());
          l_Handle.set_margin_left(get_margin_left());
          l_Handle.set_margin_top(get_margin_top());
          l_Handle.set_margin_right(get_margin_right());
          l_Handle.set_margin_bottom(get_margin_bottom());
          l_Handle.set_size_mode(get_size_mode());
          l_Handle.set_fixed_size(get_fixed_size());
          l_Handle.set_pivot(get_pivot());
          l_Handle.set_aspect_ratio(get_aspect_ratio());
          l_Handle.set_dirty(is_dirty());
          l_Handle.set_world_dirty(is_world_dirty());

          // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
          // LOW_CODEGEN::END::CUSTOM:DUPLICATE

          return l_Handle;
        }

        Layout Layout::duplicate(Layout p_Handle,
                                 Low::Core::UI::Element p_Element)
        {
          return p_Handle.duplicate(p_Element);
        }

        Low::Util::Handle
        Layout::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Handle p_Element)
        {
          Layout l_Layout = p_Handle.get_id();
          Low::Core::UI::Element l_Element = p_Element.get_id();
          return l_Layout.duplicate(l_Element);
        }

        void Layout::serialize(Low::Util::Serial::Node &p_Node) const
        {
          _LOW_ASSERT(is_alive());

          p_Node["anchor_min"] = get_anchor_min();
          p_Node["anchor_max"] = get_anchor_max();
          p_Node["margin_left"] = get_margin_left();
          p_Node["margin_top"] = get_margin_top();
          p_Node["margin_right"] = get_margin_right();
          p_Node["margin_bottom"] = get_margin_bottom();
          Low::Util::Serial::serialize_enum(
              p_Node["size_mode"], LayoutSizeModeEnum::get_enum_id(),
              static_cast<uint8_t>(get_size_mode()));
          p_Node["fixed_size"] = get_fixed_size();
          p_Node["pivot"] = get_pivot();
          p_Node["aspect_ratio"] = get_aspect_ratio();
          p_Node["_unique_id"] = Low::Util::U64Id{get_unique_id()};

          // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
          // LOW_CODEGEN::END::CUSTOM:SERIALIZER
        }

        void Layout::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Serial::Node &p_Node)
        {
          Layout l_Layout = p_Handle.get_id();
          l_Layout.serialize(p_Node);
        }

        Low::Util::Handle
        Layout::deserialize(Low::Util::Serial::Node &p_Node,
                            Low::Util::Handle p_Creator)
        {
          Low::Util::UniqueId l_HandleUniqueId = 0ull;
          if (p_Node["unique_id"]) {
            l_HandleUniqueId = p_Node["unique_id"].as<uint64_t>();
          } else if (p_Node["_unique_id"]) {
            l_HandleUniqueId = Low::Util::string_to_hash(
                p_Node["_unique_id"].as<Low::Util::String>());
          }

          Layout l_Handle =
              Layout::make(p_Creator.get_id(), l_HandleUniqueId);

          if (p_Node["anchor_min"]) {
            l_Handle.set_anchor_min(
                p_Node["anchor_min"].as<Low::Math::Vector2>());
          }
          if (p_Node["anchor_max"]) {
            l_Handle.set_anchor_max(
                p_Node["anchor_max"].as<Low::Math::Vector2>());
          }
          if (p_Node["margin_left"]) {
            l_Handle.set_margin_left(
                p_Node["margin_left"].as<float>());
          }
          if (p_Node["margin_top"]) {
            l_Handle.set_margin_top(p_Node["margin_top"].as<float>());
          }
          if (p_Node["margin_right"]) {
            l_Handle.set_margin_right(
                p_Node["margin_right"].as<float>());
          }
          if (p_Node["margin_bottom"]) {
            l_Handle.set_margin_bottom(
                p_Node["margin_bottom"].as<float>());
          }
          if (p_Node["size_mode"]) {
            l_Handle.set_size_mode(static_cast<LayoutSizeMode>(
                Low::Util::Serial::deserialize_enum(
                    p_Node["size_mode"])));
          }
          if (p_Node["fixed_size"]) {
            l_Handle.set_fixed_size(
                p_Node["fixed_size"].as<Low::Math::Vector2>());
          }
          if (p_Node["pivot"]) {
            l_Handle.set_pivot(
                p_Node["pivot"].as<Low::Math::Vector2>());
          }
          if (p_Node["aspect_ratio"]) {
            l_Handle.set_aspect_ratio(
                p_Node["aspect_ratio"].as<float>());
          }
          if (p_Node["unique_id"]) {
            l_Handle.set_unique_id(
                p_Node["unique_id"].as<Low::Util::UniqueId>());
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
          // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

          return l_Handle;
        }

        void Layout::broadcast_observable(
            Low::Util::Name p_Observable) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          Low::Util::notify(l_Key);
        }

        u64
        Layout::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        u64 Layout::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
        {
          Low::Util::ObserverKey l_Key;
          l_Key.handleId = get_id();
          l_Key.observableName = p_Observable.m_Index;

          return Low::Util::observe(l_Key, p_Observer);
        }

        void Layout::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
          // LOW_CODEGEN::END::CUSTOM:NOTIFY
        }

        void Layout::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
        {
          Layout l_Layout = p_Observer.get_id();
          l_Layout.notify(p_Observed, p_Observable);
        }

        Low::Math::Vector2 Layout::get_anchor_min() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_anchor_min
          // LOW_CODEGEN::END::CUSTOM:GETTER_anchor_min

          return TYPE_SOA(Layout, anchor_min, Low::Math::Vector2);
        }
        void Layout::set_anchor_min(float p_X, float p_Y)
        {
          Low::Math::Vector2 l_Val(p_X, p_Y);
          set_anchor_min(l_Val);
        }

        void Layout::set_anchor_min_x(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_anchor_min();
          l_Value.x = p_Value;
          set_anchor_min(l_Value);
        }

        void Layout::set_anchor_min_y(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_anchor_min();
          l_Value.y = p_Value;
          set_anchor_min(l_Value);
        }

        void Layout::set_anchor_min(Low::Math::Vector2 p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_anchor_min
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_anchor_min

          if (get_anchor_min() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, anchor_min, Low::Math::Vector2) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_anchor_min
            // LOW_CODEGEN::END::CUSTOM:SETTER_anchor_min

            broadcast_observable(N(anchor_min));
          }
        }

        Low::Math::Vector2 Layout::get_anchor_max() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_anchor_max
          // LOW_CODEGEN::END::CUSTOM:GETTER_anchor_max

          return TYPE_SOA(Layout, anchor_max, Low::Math::Vector2);
        }
        void Layout::set_anchor_max(float p_X, float p_Y)
        {
          Low::Math::Vector2 l_Val(p_X, p_Y);
          set_anchor_max(l_Val);
        }

        void Layout::set_anchor_max_x(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_anchor_max();
          l_Value.x = p_Value;
          set_anchor_max(l_Value);
        }

        void Layout::set_anchor_max_y(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_anchor_max();
          l_Value.y = p_Value;
          set_anchor_max(l_Value);
        }

        void Layout::set_anchor_max(Low::Math::Vector2 p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_anchor_max
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_anchor_max

          if (get_anchor_max() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, anchor_max, Low::Math::Vector2) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_anchor_max
            // LOW_CODEGEN::END::CUSTOM:SETTER_anchor_max

            broadcast_observable(N(anchor_max));
          }
        }

        float Layout::get_margin_left() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_margin_left
          // LOW_CODEGEN::END::CUSTOM:GETTER_margin_left

          return TYPE_SOA(Layout, margin_left, float);
        }
        void Layout::set_margin_left(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_margin_left
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_margin_left

          if (get_margin_left() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, margin_left, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_margin_left
            // LOW_CODEGEN::END::CUSTOM:SETTER_margin_left

            broadcast_observable(N(margin_left));
          }
        }

        float Layout::get_margin_top() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_margin_top
          // LOW_CODEGEN::END::CUSTOM:GETTER_margin_top

          return TYPE_SOA(Layout, margin_top, float);
        }
        void Layout::set_margin_top(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_margin_top
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_margin_top

          if (get_margin_top() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, margin_top, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_margin_top
            // LOW_CODEGEN::END::CUSTOM:SETTER_margin_top

            broadcast_observable(N(margin_top));
          }
        }

        float Layout::get_margin_right() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_margin_right
          // LOW_CODEGEN::END::CUSTOM:GETTER_margin_right

          return TYPE_SOA(Layout, margin_right, float);
        }
        void Layout::set_margin_right(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_margin_right
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_margin_right

          if (get_margin_right() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, margin_right, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_margin_right
            // LOW_CODEGEN::END::CUSTOM:SETTER_margin_right

            broadcast_observable(N(margin_right));
          }
        }

        float Layout::get_margin_bottom() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_margin_bottom
          // LOW_CODEGEN::END::CUSTOM:GETTER_margin_bottom

          return TYPE_SOA(Layout, margin_bottom, float);
        }
        void Layout::set_margin_bottom(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_margin_bottom
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_margin_bottom

          if (get_margin_bottom() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, margin_bottom, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_margin_bottom
            // LOW_CODEGEN::END::CUSTOM:SETTER_margin_bottom

            broadcast_observable(N(margin_bottom));
          }
        }

        LayoutSizeMode Layout::get_size_mode() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_size_mode
          // LOW_CODEGEN::END::CUSTOM:GETTER_size_mode

          return TYPE_SOA(Layout, size_mode, LayoutSizeMode);
        }
        void Layout::set_size_mode(LayoutSizeMode p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_size_mode
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_size_mode

          if (get_size_mode() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, size_mode, LayoutSizeMode) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_size_mode
            // LOW_CODEGEN::END::CUSTOM:SETTER_size_mode

            broadcast_observable(N(size_mode));
          }
        }

        Low::Math::Vector2 Layout::get_fixed_size() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_fixed_size
          // LOW_CODEGEN::END::CUSTOM:GETTER_fixed_size

          return TYPE_SOA(Layout, fixed_size, Low::Math::Vector2);
        }
        void Layout::set_fixed_size(float p_X, float p_Y)
        {
          Low::Math::Vector2 l_Val(p_X, p_Y);
          set_fixed_size(l_Val);
        }

        void Layout::set_fixed_size_x(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_fixed_size();
          l_Value.x = p_Value;
          set_fixed_size(l_Value);
        }

        void Layout::set_fixed_size_y(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_fixed_size();
          l_Value.y = p_Value;
          set_fixed_size(l_Value);
        }

        void Layout::set_fixed_size(Low::Math::Vector2 p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_fixed_size
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_fixed_size

          if (get_fixed_size() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, fixed_size, Low::Math::Vector2) =
                p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_fixed_size
            // LOW_CODEGEN::END::CUSTOM:SETTER_fixed_size

            broadcast_observable(N(fixed_size));
          }
        }

        Low::Math::Vector2 Layout::get_pivot() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_pivot
          // LOW_CODEGEN::END::CUSTOM:GETTER_pivot

          return TYPE_SOA(Layout, pivot, Low::Math::Vector2);
        }
        void Layout::set_pivot(float p_X, float p_Y)
        {
          Low::Math::Vector2 l_Val(p_X, p_Y);
          set_pivot(l_Val);
        }

        void Layout::set_pivot_x(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_pivot();
          l_Value.x = p_Value;
          set_pivot(l_Value);
        }

        void Layout::set_pivot_y(float p_Value)
        {
          Low::Math::Vector2 l_Value = get_pivot();
          l_Value.y = p_Value;
          set_pivot(l_Value);
        }

        void Layout::set_pivot(Low::Math::Vector2 p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_pivot
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_pivot

          if (get_pivot() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, pivot, Low::Math::Vector2) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_pivot
            // LOW_CODEGEN::END::CUSTOM:SETTER_pivot

            broadcast_observable(N(pivot));
          }
        }

        float Layout::get_aspect_ratio() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_aspect_ratio
          // LOW_CODEGEN::END::CUSTOM:GETTER_aspect_ratio

          return TYPE_SOA(Layout, aspect_ratio, float);
        }
        void Layout::set_aspect_ratio(float p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_aspect_ratio
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_aspect_ratio

          if (get_aspect_ratio() != p_Value) {
            // Set dirty flags
            mark_dirty();
            mark_world_dirty();

            // Set new value
            TYPE_SOA(Layout, aspect_ratio, float) = p_Value;

            // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_aspect_ratio
            // LOW_CODEGEN::END::CUSTOM:SETTER_aspect_ratio

            broadcast_observable(N(aspect_ratio));
          }
        }

        bool Layout::is_world_updated() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:GETTER_world_updated

          return TYPE_SOA(Layout, world_updated, bool);
        }
        void Layout::toggle_world_updated()
        {
          set_world_updated(!is_world_updated());
        }

        void Layout::set_world_updated(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_updated

          // Set new value
          TYPE_SOA(Layout, world_updated, bool) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_updated
          // LOW_CODEGEN::END::CUSTOM:SETTER_world_updated

          broadcast_observable(N(world_updated));
        }

        Low::Core::UI::Element Layout::get_element() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_element
          // LOW_CODEGEN::END::CUSTOM:GETTER_element

          return TYPE_SOA(Layout, element, Low::Core::UI::Element);
        }
        void Layout::set_element(Low::Core::UI::Element p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_element
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_element

          // Set new value
          TYPE_SOA(Layout, element, Low::Core::UI::Element) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_element
          // LOW_CODEGEN::END::CUSTOM:SETTER_element

          broadcast_observable(N(element));
        }

        Low::Util::UniqueId Layout::get_unique_id() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:GETTER_unique_id

          return TYPE_SOA(Layout, unique_id, Low::Util::UniqueId);
        }
        void Layout::set_unique_id(Low::Util::UniqueId p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_unique_id

          // Set new value
          TYPE_SOA(Layout, unique_id, Low::Util::UniqueId) = p_Value;

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_unique_id
          // LOW_CODEGEN::END::CUSTOM:SETTER_unique_id

          broadcast_observable(N(unique_id));
        }

        bool Layout::is_dirty() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:GETTER_dirty

          return TYPE_SOA(Layout, dirty, bool);
        }
        void Layout::toggle_dirty()
        {
          set_dirty(!is_dirty());
        }

        void Layout::set_dirty(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_dirty

          // Set new value
          TYPE_SOA(Layout, dirty, bool) = p_Value;

          if (p_Value) {
            mark_dirty();
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_dirty
          // LOW_CODEGEN::END::CUSTOM:SETTER_dirty

          broadcast_observable(N(dirty));
        }

        void Layout::mark_dirty()
        {
          if (!is_dirty()) {
            TYPE_SOA(Layout, dirty, bool) = true;
            // LOW_CODEGEN:BEGIN:CUSTOM:MARK_dirty
            // LOW_CODEGEN::END::CUSTOM:MARK_dirty
          }
        }

        bool Layout::is_world_dirty() const
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_world_dirty

          if (TYPE_SOA(Layout, world_dirty, bool)) {
            return true;
          }

          Element l_Element = get_element();
          Component::Display l_ParentDisplay =
              l_Element.get_display().get_parent();

          if (l_ParentDisplay.is_alive()) {
            Element l_ParentElement = l_ParentDisplay.get_element();
            if (l_ParentElement.has_component(Layout::type_id())) {
              Layout l_ParentLayout =
                  l_ParentElement.get_component(Layout::type_id());
              return l_ParentLayout.is_world_dirty() ||
                     l_ParentLayout.is_world_updated();
            }
            return false;
          }

          Screen l_Screen = l_Element.get_cached_screen();
          if (l_Screen.is_alive()) {
            return l_Screen.is_dirty();
          }

          return false;
          // LOW_CODEGEN::END::CUSTOM:GETTER_world_dirty

          return TYPE_SOA(Layout, world_dirty, bool);
        }
        void Layout::toggle_world_dirty()
        {
          set_world_dirty(!is_world_dirty());
        }

        void Layout::set_world_dirty(bool p_Value)
        {
          _LOW_ASSERT(is_alive());

          // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_world_dirty
          // LOW_CODEGEN::END::CUSTOM:PRESETTER_world_dirty

          // Set new value
          TYPE_SOA(Layout, world_dirty, bool) = p_Value;

          if (p_Value) {
            mark_world_dirty();
          }

          // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_world_dirty
          // LOW_CODEGEN::END::CUSTOM:SETTER_world_dirty

          broadcast_observable(N(world_dirty));
        }

        void Layout::mark_world_dirty()
        {
          if (!is_world_dirty()) {
            TYPE_SOA(Layout, world_dirty, bool) = true;
            // LOW_CODEGEN:BEGIN:CUSTOM:MARK_world_dirty
            // LOW_CODEGEN::END::CUSTOM:MARK_world_dirty
          }
        }

        void Layout::resolve()
        {
          // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_resolve

          LOW_ASSERT(is_alive(), "Cannot resolve dead layout");

          if (!is_world_dirty() || is_world_updated()) {
            return;
          }

          Element l_Element = get_element();
          Component::Display l_Display = l_Element.get_display();
          Component::Display l_ParentDisplay = l_Display.get_parent();

          Math::Vector2 l_ParentSize(0.0f, 0.0f);

          if (l_ParentDisplay.is_alive()) {
            Element l_ParentElement = l_ParentDisplay.get_element();
            if (l_ParentElement.has_component(Layout::type_id())) {
              Layout l_ParentLayout =
                  l_ParentElement.get_component(Layout::type_id());
              if (l_ParentLayout.is_world_dirty()) {
                l_ParentLayout.resolve();
              }
            }
            l_ParentSize = l_ParentDisplay.pixel_scale();
          } else {
            Screen l_Screen = l_Element.get_cached_screen();
            if (l_Screen.is_alive()) {
              l_ParentSize = l_Screen.pixel_size();
            }
          }

          Math::Vector2 l_AnchorMin = get_anchor_min();
          Math::Vector2 l_AnchorMax = get_anchor_max();

          Math::Vector2 l_RectMin(l_ParentSize.x * l_AnchorMin.x,
                                  l_ParentSize.y * l_AnchorMin.y);
          Math::Vector2 l_RectMax(l_ParentSize.x * l_AnchorMax.x,
                                  l_ParentSize.y * l_AnchorMax.y);

          bool l_StretchX = l_AnchorMin.x != l_AnchorMax.x;
          bool l_StretchY = l_AnchorMin.y != l_AnchorMax.y;

          Math::Vector2 l_Position(0.0f, 0.0f);
          Math::Vector2 l_Size(0.0f, 0.0f);

          if (l_StretchX) {
            l_Position.x = l_RectMin.x + get_margin_left();
            l_Size.x =
                (l_RectMax.x - get_margin_right()) - l_Position.x;
          }
          if (l_StretchY) {
            l_Position.y = l_RectMin.y + get_margin_top();
            l_Size.y =
                (l_RectMax.y - get_margin_bottom()) - l_Position.y;
          }

          if (!l_StretchX || !l_StretchY) {
            Math::Vector2 l_FixedSize = get_fixed_size();

            if (get_size_mode() == LayoutSizeMode::Aspect) {
              float l_Ratio = get_aspect_ratio();
              if (l_Ratio > 0.0f) {
                if (!l_StretchX && l_StretchY) {
                  l_FixedSize.x = l_Size.y * l_Ratio;
                } else if (l_StretchX && !l_StretchY) {
                  l_FixedSize.y = l_Size.x / l_Ratio;
                } else {
                  l_FixedSize.y = l_FixedSize.x / l_Ratio;
                }
              }
            }

            Math::Vector2 l_Pivot = get_pivot();

            if (!l_StretchX) {
              l_Size.x = l_FixedSize.x;
              l_Position.x = l_RectMin.x - l_Size.x * l_Pivot.x;
            }
            if (!l_StretchY) {
              l_Size.y = l_FixedSize.y;
              l_Position.y = l_RectMin.y - l_Size.y * l_Pivot.y;
            }
          }

          l_Display.pixel_position(l_Position);
          l_Display.pixel_scale(l_Size);

          set_world_dirty(false);
          set_world_updated(true);
          // LOW_CODEGEN::END::CUSTOM:FUNCTION_resolve
        }

        uint32_t Layout::create_instance(u32 &p_PageIndex,
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

        u32 Layout::create_page()
        {
          const u32 l_Capacity = get_capacity();
          LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                     "Could not increase capacity for Layout.");

          Low::Util::Instances::Page *l_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              l_Page, Layout::Data::get_size(), ms_PageSize);
          ms_Pages.push_back(l_Page);

          ms_Capacity = l_Capacity + l_Page->size;
          return ms_Pages.size() - 1;
        }

        bool Layout::get_page_for_index(const u32 p_Index,
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
    }   // namespace UI
  }     // namespace Core
} // namespace Low
