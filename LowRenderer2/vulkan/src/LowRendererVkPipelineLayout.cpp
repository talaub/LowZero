#include "LowRendererVkPipelineLayout.h"

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

#include "LowRendererVulkan.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 PipelineLayout::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          PipelineLayout::IDENTIFIER(LOW_NAME(509652687),
                                     LOW_NAME(3518960863));
      uint32_t PipelineLayout::ms_Capacity = 0u;
      uint32_t PipelineLayout::ms_PageSize = 0u;
      Low::Util::List<PipelineLayout>
          PipelineLayout::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          PipelineLayout::ms_Pages;

      Low::Util::Handle PipelineLayout::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      PipelineLayout PipelineLayout::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        PipelineLayout l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = PipelineLayout::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, PipelineLayout, internal,
                                   VkPipelineLayout))
            VkPipelineLayout();
        ACCESSOR_TYPE_SOA(l_Handle, PipelineLayout, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void PipelineLayout::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          VkPipelineLayout l_Layout = get();
          vkDestroyPipelineLayout(Global::get_device(), l_Layout,
                                  nullptr);
          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

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

      void PipelineLayout::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowRenderer2), N(PipelineLayout));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowRenderer2), N(PipelineLayout));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, PipelineLayout::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(PipelineLayout);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &PipelineLayout::is_alive;
        l_TypeInfo.destroy = &PipelineLayout::destroy;
        l_TypeInfo.serialize = &PipelineLayout::serialize;
        l_TypeInfo.deserialize = &PipelineLayout::deserialize;
        l_TypeInfo.find_by_index = &PipelineLayout::_find_by_index;
        l_TypeInfo.notify = &PipelineLayout::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &PipelineLayout::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &PipelineLayout::_make;
        l_TypeInfo.duplicate_default = &PipelineLayout::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &PipelineLayout::living_instances);
        l_TypeInfo.get_living_count = &PipelineLayout::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: internal
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(internal);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PipelineLayout::Data, internal);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PipelineLayout l_Handle = p_Handle.get_id();
            l_Handle.get();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PipelineLayout, internal, VkPipelineLayout);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PipelineLayout l_Handle = p_Handle.get_id();
            l_Handle.set(*(VkPipelineLayout *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PipelineLayout l_Handle = p_Handle.get_id();
            *((VkPipelineLayout *)p_Data) = l_Handle.get();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: internal
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(PipelineLayout::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            PipelineLayout l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, PipelineLayout, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            PipelineLayout l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            PipelineLayout l_Handle = p_Handle.get_id();
            *((Low::Util::Name *)p_Data) = l_Handle.get_name();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: name
        }
        ms_TypeId = Low::Util::Handle::register_type_info(IDENTIFIER,
                                                          l_TypeInfo);
        // LOW_CODEGEN:BEGIN:CUSTOM:POSTINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:POSTINITIALIZE
      }

      void PipelineLayout::cleanup()
      {
        Low::Util::List<PipelineLayout> l_Instances =
            ms_LivingInstances;
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

      Low::Util::Handle
      PipelineLayout::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      PipelineLayout PipelineLayout::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        PipelineLayout l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = PipelineLayout::ms_TypeId;

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

      PipelineLayout
      PipelineLayout::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        PipelineLayout l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = PipelineLayout::ms_TypeId;

        return l_Handle;
      }

      bool PipelineLayout::is_alive() const
      {
        if (m_Data.m_Type != PipelineLayout::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == PipelineLayout::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t PipelineLayout::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      PipelineLayout::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      PipelineLayout
      PipelineLayout::find_by_name(Low::Util::Name p_Name)
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

      PipelineLayout
      PipelineLayout::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        PipelineLayout l_Handle = make(p_Name);
        l_Handle.set(get());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      PipelineLayout
      PipelineLayout::duplicate(PipelineLayout p_Handle,
                                Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      PipelineLayout::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Name p_Name)
      {
        PipelineLayout l_PipelineLayout = p_Handle.get_id();
        return l_PipelineLayout.duplicate(p_Name);
      }

      void
      PipelineLayout::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void PipelineLayout::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
      {
        PipelineLayout l_PipelineLayout = p_Handle.get_id();
        l_PipelineLayout.serialize(p_Node);
      }

      Low::Util::Handle
      PipelineLayout::deserialize(Low::Util::Serial::Node &p_Node,
                                  Low::Util::Handle p_Creator)
      {
        PipelineLayout l_Handle =
            PipelineLayout::make(N(PipelineLayout));

        if (p_Node["internal"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void PipelineLayout::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 PipelineLayout::observe(
          Low::Util::Name p_Observable,
          Low::Util::Function<void(Low::Util::Handle,
                                   Low::Util::Name)>
              p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 PipelineLayout::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void PipelineLayout::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void PipelineLayout::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
      {
        PipelineLayout l_PipelineLayout = p_Observer.get_id();
        l_PipelineLayout.notify(p_Observed, p_Observable);
      }

      VkPipelineLayout &PipelineLayout::get() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal

        // LOW_CODEGEN::END::CUSTOM:GETTER_internal

        return TYPE_SOA(PipelineLayout, internal, VkPipelineLayout);
      }
      void PipelineLayout::set(VkPipelineLayout &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

        // Set new value
        TYPE_SOA(PipelineLayout, internal, VkPipelineLayout) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal

        // LOW_CODEGEN::END::CUSTOM:SETTER_internal

        broadcast_observable(N(internal));
      }

      Low::Util::Name PipelineLayout::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(PipelineLayout, name, Low::Util::Name);
      }
      void PipelineLayout::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(PipelineLayout, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t PipelineLayout::create_instance(u32 &p_PageIndex,
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

      u32 PipelineLayout::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for PipelineLayout.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, PipelineLayout::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool PipelineLayout::get_page_for_index(const u32 p_Index,
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

    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
