#include "LowRendererVkSS2DCanvasBackend.h"

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

      u16 SS2DCanvasBackend::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          SS2DCanvasBackend::IDENTIFIER(LOW_NAME(509652687),
                                        LOW_NAME(4084333416));
      uint32_t SS2DCanvasBackend::ms_Capacity = 0u;
      uint32_t SS2DCanvasBackend::ms_PageSize = 0u;
      Low::Util::SharedMutex SS2DCanvasBackend::ms_LivingMutex;
      Low::Util::SharedMutex SS2DCanvasBackend::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          SS2DCanvasBackend::ms_PagesLock(
              SS2DCanvasBackend::ms_PagesMutex, std::defer_lock);
      Low::Util::List<SS2DCanvasBackend>
          SS2DCanvasBackend::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          SS2DCanvasBackend::ms_Pages;

      Low::Util::Handle
      SS2DCanvasBackend::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      SS2DCanvasBackend
      SS2DCanvasBackend::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        SS2DCanvasBackend l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = SS2DCanvasBackend::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<SS2DCanvasBackend> l_HandleLock(
            l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, SS2DCanvasBackend,
                                   canvas_data, VkDescriptorSet))
            VkDescriptorSet();
        ACCESSOR_TYPE_SOA(l_Handle, SS2DCanvasBackend, name,
                          Low::Util::Name) = Low::Util::Name(0u);

        l_Handle.set_name(p_Name);

        {
          Low::Util::UniqueLock<Low::Util::SharedMutex> l_LivingLock(
              ms_LivingMutex);
          ms_LivingInstances.push_back(l_Handle);
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

        // LOW_CODEGEN::END::CUSTOM:MAKE

        return l_Handle;
      }

      void SS2DCanvasBackend::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<SS2DCanvasBackend> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          // LOW_CODEGEN::END::CUSTOM:DESTROY
        }

        broadcast_observable(OBSERVABLE_DESTROY);

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

      void SS2DCanvasBackend::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowRenderer2), N(SS2DCanvasBackend));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowRenderer2), N(SS2DCanvasBackend));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, SS2DCanvasBackend::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(SS2DCanvasBackend);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &SS2DCanvasBackend::is_alive;
        l_TypeInfo.destroy = &SS2DCanvasBackend::destroy;
        l_TypeInfo.serialize = &SS2DCanvasBackend::serialize;
        l_TypeInfo.deserialize = &SS2DCanvasBackend::deserialize;
        l_TypeInfo.find_by_index = &SS2DCanvasBackend::_find_by_index;
        l_TypeInfo.notify = &SS2DCanvasBackend::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &SS2DCanvasBackend::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &SS2DCanvasBackend::_make;
        l_TypeInfo.duplicate_default = &SS2DCanvasBackend::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &SS2DCanvasBackend::living_instances);
        l_TypeInfo.get_living_count =
            &SS2DCanvasBackend::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: canvas_data
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(canvas_data);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(SS2DCanvasBackend::Data, canvas_data);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<SS2DCanvasBackend> l_HandleLock(
                l_Handle);
            l_Handle.get_canvas_data();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, SS2DCanvasBackend, canvas_data,
                VkDescriptorSet);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            l_Handle.set_canvas_data(*(VkDescriptorSet *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<SS2DCanvasBackend> l_HandleLock(
                l_Handle);
            *((VkDescriptorSet *)p_Data) = l_Handle.get_canvas_data();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: canvas_data
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(SS2DCanvasBackend::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<SS2DCanvasBackend> l_HandleLock(
                l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, SS2DCanvasBackend, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            SS2DCanvasBackend l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<SS2DCanvasBackend> l_HandleLock(
                l_Handle);
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

      void SS2DCanvasBackend::cleanup()
      {
        Low::Util::List<SS2DCanvasBackend> l_Instances =
            ms_LivingInstances;
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

      Low::Util::Handle
      SS2DCanvasBackend::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      SS2DCanvasBackend
      SS2DCanvasBackend::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        SS2DCanvasBackend l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = SS2DCanvasBackend::ms_TypeId;

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

      SS2DCanvasBackend
      SS2DCanvasBackend::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        SS2DCanvasBackend l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = SS2DCanvasBackend::ms_TypeId;

        return l_Handle;
      }

      bool SS2DCanvasBackend::is_alive() const
      {
        if (m_Data.m_Type != SS2DCanvasBackend::ms_TypeId) {
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
        return m_Data.m_Type == SS2DCanvasBackend::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t SS2DCanvasBackend::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      SS2DCanvasBackend::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      SS2DCanvasBackend
      SS2DCanvasBackend::find_by_name(Low::Util::Name p_Name)
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

      SS2DCanvasBackend
      SS2DCanvasBackend::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        SS2DCanvasBackend l_Handle = make(p_Name);
        l_Handle.set_canvas_data(get_canvas_data());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        // LOW_CODEGEN::END::CUSTOM:DUPLICATE

        return l_Handle;
      }

      SS2DCanvasBackend
      SS2DCanvasBackend::duplicate(SS2DCanvasBackend p_Handle,
                                   Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      SS2DCanvasBackend::_duplicate(Low::Util::Handle p_Handle,
                                    Low::Util::Name p_Name)
      {
        SS2DCanvasBackend l_SS2DCanvasBackend = p_Handle.get_id();
        return l_SS2DCanvasBackend.duplicate(p_Name);
      }

      void SS2DCanvasBackend::serialize(
          Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        p_Node["name"] = get_name().c_str();

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void
      SS2DCanvasBackend::serialize(Low::Util::Handle p_Handle,
                                   Low::Util::Serial::Node &p_Node)
      {
        SS2DCanvasBackend l_SS2DCanvasBackend = p_Handle.get_id();
        l_SS2DCanvasBackend.serialize(p_Node);
      }

      Low::Util::Handle
      SS2DCanvasBackend::deserialize(Low::Util::Serial::Node &p_Node,
                                     Low::Util::Handle p_Creator)
      {
        SS2DCanvasBackend l_Handle =
            SS2DCanvasBackend::make(N(SS2DCanvasBackend));

        if (p_Node["canvas_data"]) {
        }
        if (p_Node["name"]) {
          l_Handle.set_name(p_Node["name"].as<Low::Util::Name>());
        }

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

        return l_Handle;
      }

      void SS2DCanvasBackend::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 SS2DCanvasBackend::observe(
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

      u64
      SS2DCanvasBackend::observe(Low::Util::Name p_Observable,
                                 Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void SS2DCanvasBackend::notify(Low::Util::Handle p_Observed,
                                     Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void SS2DCanvasBackend::_notify(Low::Util::Handle p_Observer,
                                      Low::Util::Handle p_Observed,
                                      Low::Util::Name p_Observable)
      {
        SS2DCanvasBackend l_SS2DCanvasBackend = p_Observer.get_id();
        l_SS2DCanvasBackend.notify(p_Observed, p_Observable);
      }

      VkDescriptorSet SS2DCanvasBackend::get_canvas_data() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<SS2DCanvasBackend> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_canvas_data

        // LOW_CODEGEN::END::CUSTOM:GETTER_canvas_data

        return TYPE_SOA(SS2DCanvasBackend, canvas_data,
                        VkDescriptorSet);
      }
      void SS2DCanvasBackend::set_canvas_data(VkDescriptorSet p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<SS2DCanvasBackend> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_canvas_data

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_canvas_data

        // Set new value
        TYPE_SOA(SS2DCanvasBackend, canvas_data, VkDescriptorSet) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_canvas_data

        // LOW_CODEGEN::END::CUSTOM:SETTER_canvas_data

        broadcast_observable(N(canvas_data));
      }

      Low::Util::Name SS2DCanvasBackend::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<SS2DCanvasBackend> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(SS2DCanvasBackend, name, Low::Util::Name);
      }
      void SS2DCanvasBackend::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<SS2DCanvasBackend> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(SS2DCanvasBackend, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t SS2DCanvasBackend::create_instance(
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

      u32 SS2DCanvasBackend::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT(
            (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
            "Could not increase capacity for SS2DCanvasBackend.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, SS2DCanvasBackend::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool SS2DCanvasBackend::get_page_for_index(const u32 p_Index,
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
