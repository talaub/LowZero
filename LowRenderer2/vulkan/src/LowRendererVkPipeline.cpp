#include "LowRendererVkPipeline.h"

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
  namespace Renderer {
    namespace Vulkan {
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 Pipeline::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          Pipeline::IDENTIFIER(LOW_NAME(509652687),
                               LOW_NAME(2223684495));
      uint32_t Pipeline::ms_Capacity = 0u;
      uint32_t Pipeline::ms_PageSize = 0u;
      Low::Util::List<Pipeline> Pipeline::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          Pipeline::ms_Pages;

      Low::Util::Handle Pipeline::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      Pipeline Pipeline::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        uint32_t l_Index = create_instance(l_PageIndex, l_SlotIndex);

        Pipeline l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = Pipeline::ms_TypeId;

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Pipeline, internal,
                                   VkPipeline)) VkPipeline();
        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, Pipeline, layout,
                                   PipelineLayout)) PipelineLayout();
        ACCESSOR_TYPE_SOA(l_Handle, Pipeline, name, Low::Util::Name) =
            Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        ms_LivingInstances.push_back(l_Handle);

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void Pipeline::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
          vkDestroyPipeline(Global::get_device(), get(), nullptr);
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

      void Pipeline::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowRenderer2), N(Pipeline));

        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(N(LowRenderer2),
                                                      N(Pipeline));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, Pipeline::Data::get_size(), ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(Pipeline);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &Pipeline::is_alive;
        l_TypeInfo.destroy = &Pipeline::destroy;
        l_TypeInfo.serialize = &Pipeline::serialize;
        l_TypeInfo.deserialize = &Pipeline::deserialize;
        l_TypeInfo.find_by_index = &Pipeline::_find_by_index;
        l_TypeInfo.notify = &Pipeline::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &Pipeline::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &Pipeline::_make;
        l_TypeInfo.duplicate_default = &Pipeline::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &Pipeline::living_instances);
        l_TypeInfo.get_living_count = &Pipeline::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: internal
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(internal);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Pipeline::Data, internal);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pipeline,
                                              internal, VkPipeline);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set(*(VkPipeline *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Pipeline l_Handle = p_Handle.get_id();
            *((VkPipeline *)p_Data) = l_Handle.get();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: internal
        }
        {
          // Property: layout
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(layout);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(Pipeline::Data, layout);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType = PipelineLayout::IDENTIFIER;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get_layout();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pipeline,
                                              layout, PipelineLayout);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set_layout(*(PipelineLayout *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Pipeline l_Handle = p_Handle.get_id();
            *((PipelineLayout *)p_Data) = l_Handle.get_layout();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: layout
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset = offsetof(Pipeline::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Pipeline,
                                              name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            Pipeline l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            Pipeline l_Handle = p_Handle.get_id();
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

      void Pipeline::cleanup()
      {
        Low::Util::List<Pipeline> l_Instances = ms_LivingInstances;
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

      Low::Util::Handle Pipeline::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      Pipeline Pipeline::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        Pipeline l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = Pipeline::ms_TypeId;

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

      Pipeline Pipeline::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        Pipeline l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = Pipeline::ms_TypeId;

        return l_Handle;
      }

      bool Pipeline::is_alive() const
      {
        if (m_Data.m_Type != Pipeline::ms_TypeId) {
          return false;
        }
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        if (!get_page_for_index(get_index(), l_PageIndex,
                                l_SlotIndex)) {
          return false;
        }
        Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];
        return m_Data.m_Type == Pipeline::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t Pipeline::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      Pipeline::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      Pipeline Pipeline::find_by_name(Low::Util::Name p_Name)
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

      Pipeline Pipeline::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        Pipeline l_Handle = make(p_Name);
        l_Handle.set(get());
        if (get_layout().is_alive()) {
          l_Handle.set_layout(get_layout());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      Pipeline Pipeline::duplicate(Pipeline p_Handle,
                                   Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      Pipeline::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
      {
        Pipeline l_Pipeline = p_Handle.get_id();
        return l_Pipeline.duplicate(p_Name);
      }

      void Pipeline::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        if (get_layout().is_alive()) {
          get_layout().serialize(p_Node["layout"]);
        }
        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void Pipeline::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Serial::Node &p_Node)
      {
        Pipeline l_Pipeline = p_Handle.get_id();
        l_Pipeline.serialize(p_Node);
      }

      Low::Util::Handle
      Pipeline::deserialize(Low::Util::Serial::Node &p_Node,
                            Low::Util::Handle p_Creator)
      {
        Pipeline l_Handle = Pipeline::make(N(Pipeline));

        if (p_Node["internal"]) {
        }
        if (p_Node["layout"]) {
          l_Handle.set_layout(PipelineLayout::deserialize(
                                  p_Node["layout"], l_Handle.get_id())
                                  .get_id());
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void Pipeline::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64
      Pipeline::observe(Low::Util::Name p_Observable,
                        Low::Util::Function<void(Low::Util::Handle,
                                                 Low::Util::Name)>
                            p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      u64 Pipeline::observe(Low::Util::Name p_Observable,
                            Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void Pipeline::notify(Low::Util::Handle p_Observed,
                            Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void Pipeline::_notify(Low::Util::Handle p_Observer,
                             Low::Util::Handle p_Observed,
                             Low::Util::Name p_Observable)
      {
        Pipeline l_Pipeline = p_Observer.get_id();
        l_Pipeline.notify(p_Observed, p_Observable);
      }

      VkPipeline &Pipeline::get() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_internal
        // LOW_CODEGEN::END::CUSTOM:GETTER_internal

        return TYPE_SOA(Pipeline, internal, VkPipeline);
      }
      void Pipeline::set(VkPipeline &p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_internal
        // LOW_CODEGEN::END::CUSTOM:PRESETTER_internal

        // Set new value
        TYPE_SOA(Pipeline, internal, VkPipeline) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_internal
        // LOW_CODEGEN::END::CUSTOM:SETTER_internal

        broadcast_observable(N(internal));
      }

      PipelineLayout Pipeline::get_layout() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_layout

        // LOW_CODEGEN::END::CUSTOM:GETTER_layout

        return TYPE_SOA(Pipeline, layout, PipelineLayout);
      }
      void Pipeline::set_layout(PipelineLayout p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_layout

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_layout

        // Set new value
        TYPE_SOA(Pipeline, layout, PipelineLayout) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_layout

        // LOW_CODEGEN::END::CUSTOM:SETTER_layout

        broadcast_observable(N(layout));
      }

      Low::Util::Name Pipeline::get_name() const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(Pipeline, name, Low::Util::Name);
      }
      void Pipeline::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(Pipeline, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t Pipeline::create_instance(u32 &p_PageIndex,
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

      u32 Pipeline::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for Pipeline.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, Pipeline::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool Pipeline::get_page_for_index(const u32 p_Index,
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
