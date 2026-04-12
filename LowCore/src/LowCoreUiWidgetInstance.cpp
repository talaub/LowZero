#include "LowCoreUiWidgetInstance.h"

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
      // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

      // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

      u16 WidgetInstance::ms_TypeId = 0;
      const Low::Util::TypeIdentifier
          WidgetInstance::IDENTIFIER(LOW_NAME(1181529166),
                                     LOW_NAME(205868524));
      uint32_t WidgetInstance::ms_Capacity = 0u;
      uint32_t WidgetInstance::ms_PageSize = 0u;
      Low::Util::SharedMutex WidgetInstance::ms_LivingMutex;
      Low::Util::SharedMutex WidgetInstance::ms_PagesMutex;
      Low::Util::UniqueLock<Low::Util::SharedMutex>
          WidgetInstance::ms_PagesLock(WidgetInstance::ms_PagesMutex,
                                       std::defer_lock);
      Low::Util::List<WidgetInstance>
          WidgetInstance::ms_LivingInstances;
      Low::Util::List<Low::Util::Instances::Page *>
          WidgetInstance::ms_Pages;

      Low::Util::Handle WidgetInstance::_make(Low::Util::Name p_Name)
      {
        return make(p_Name).get_id();
      }

      WidgetInstance WidgetInstance::make(Low::Util::Name p_Name)
      {
        u32 l_PageIndex = 0;
        u32 l_SlotIndex = 0;
        Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
        uint32_t l_Index =
            create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

        WidgetInstance l_Handle;
        l_Handle.m_Data.m_Index = l_Index;
        l_Handle.m_Data.m_Generation =
            ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
        l_Handle.m_Data.m_Type = WidgetInstance::ms_TypeId;

        l_PageLock.unlock();

        Low::Util::HandleLock<WidgetInstance> l_HandleLock(l_Handle);

        new (ACCESSOR_TYPE_SOA_PTR(l_Handle, WidgetInstance, root,
                                   Low::Core::UI::Element))
            Low::Core::UI::Element();
        new (ACCESSOR_TYPE_SOA_PTR(
            l_Handle, WidgetInstance, elements,
            Low::Util::List<Low::Core::UI::Element>))
            Low::Util::List<Low::Core::UI::Element>();
        ACCESSOR_TYPE_SOA(l_Handle, WidgetInstance, name,
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

      void WidgetInstance::destroy()
      {
        LOW_ASSERT(is_alive(), "Cannot destroy dead object");

        {
          Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());
          // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

          get_root().destroy_with_hierarchy();

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

      void WidgetInstance::initialize()
      {
        const Low::Util::TypeIdentifier l_IdentifierNames(
            N(LowCore), N(WidgetInstance));

        LOCK_PAGES_WRITE(l_PagesLock);
        // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

        // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

        ms_Capacity = Low::Util::Config::get_capacity(
            N(LowCore), N(WidgetInstance));

        ms_PageSize = Low::Math::Util::clamp(
            Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
        {
          u32 l_Capacity = 0u;
          while (l_Capacity < ms_Capacity) {
            Low::Util::Instances::Page *i_Page =
                new Low::Util::Instances::Page;
            Low::Util::Instances::initialize_page(
                i_Page, WidgetInstance::Data::get_size(),
                ms_PageSize);
            ms_Pages.push_back(i_Page);
            l_Capacity += ms_PageSize;
          }
          ms_Capacity = l_Capacity;
        }
        LOCK_UNLOCK(l_PagesLock);

        Low::Util::RTTI::TypeInfo l_TypeInfo;
        l_TypeInfo.name = N(WidgetInstance);
        l_TypeInfo.typeId = ms_TypeId;
        l_TypeInfo.get_capacity = &get_capacity;
        l_TypeInfo.is_alive = &WidgetInstance::is_alive;
        l_TypeInfo.destroy = &WidgetInstance::destroy;
        l_TypeInfo.serialize = &WidgetInstance::serialize;
        l_TypeInfo.deserialize = &WidgetInstance::deserialize;
        l_TypeInfo.find_by_index = &WidgetInstance::_find_by_index;
        l_TypeInfo.notify = &WidgetInstance::_notify;
        l_TypeInfo.post_load = nullptr;
        l_TypeInfo.find_by_name = &WidgetInstance::_find_by_name;
        l_TypeInfo.make_component = nullptr;
        l_TypeInfo.make_default = &WidgetInstance::_make;
        l_TypeInfo.duplicate_default = &WidgetInstance::_duplicate;
        l_TypeInfo.duplicate_component = nullptr;
        l_TypeInfo.get_living_instances =
            reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
                &WidgetInstance::living_instances);
        l_TypeInfo.get_living_count = &WidgetInstance::living_count;
        l_TypeInfo.component = false;
        l_TypeInfo.uiComponent = false;
        {
          // Property: root
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(root);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetInstance::Data, root);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::HANDLE;
          l_PropertyInfo.handleType =
              Low::Core::UI::Element::type_id();
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_root();
            return (void *)&ACCESSOR_TYPE_SOA(p_Handle,
                                              WidgetInstance, root,
                                              Low::Core::UI::Element);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetInstance l_Handle = p_Handle.get_id();
            l_Handle.set_root(*(Low::Core::UI::Element *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
                l_Handle);
            *((Low::Core::UI::Element *)p_Data) = l_Handle.get_root();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: root
        }
        {
          // Property: elements
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(elements);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetInstance::Data, elements);
          l_PropertyInfo.type =
              Low::Util::RTTI::PropertyType::UNKNOWN;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_elements();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetInstance, elements,
                Low::Util::List<Low::Core::UI::Element>);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {};
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
                l_Handle);
            *((Low::Util::List<Low::Core::UI::Element> *)p_Data) =
                l_Handle.get_elements();
          };
          l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
          // End property: elements
        }
        {
          // Property: name
          Low::Util::RTTI::PropertyInfo l_PropertyInfo;
          l_PropertyInfo.name = N(name);
          l_PropertyInfo.editorProperty = false;
          l_PropertyInfo.dataOffset =
              offsetof(WidgetInstance::Data, name);
          l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
          l_PropertyInfo.handleType = 0;
          l_PropertyInfo.get_return =
              [](Low::Util::Handle p_Handle) -> void const * {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
                l_Handle);
            l_Handle.get_name();
            return (void *)&ACCESSOR_TYPE_SOA(
                p_Handle, WidgetInstance, name, Low::Util::Name);
          };
          l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                  const void *p_Data) -> void {
            WidgetInstance l_Handle = p_Handle.get_id();
            l_Handle.set_name(*(Low::Util::Name *)p_Data);
          };
          l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                  void *p_Data) {
            WidgetInstance l_Handle = p_Handle.get_id();
            Low::Util::HandleLock<WidgetInstance> l_HandleLock(
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

      void WidgetInstance::cleanup()
      {
        Low::Util::List<WidgetInstance> l_Instances =
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
      WidgetInstance::_find_by_index(uint32_t p_Index)
      {
        return find_by_index(p_Index).get_id();
      }

      WidgetInstance WidgetInstance::find_by_index(uint32_t p_Index)
      {
        LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

        WidgetInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Type = WidgetInstance::ms_TypeId;

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

      WidgetInstance
      WidgetInstance::create_handle_by_index(u32 p_Index)
      {
        if (p_Index < get_capacity()) {
          return find_by_index(p_Index);
        }

        WidgetInstance l_Handle;
        l_Handle.m_Data.m_Index = p_Index;
        l_Handle.m_Data.m_Generation = 0;
        l_Handle.m_Data.m_Type = WidgetInstance::ms_TypeId;

        return l_Handle;
      }

      bool WidgetInstance::is_alive() const
      {
        if (m_Data.m_Type != WidgetInstance::ms_TypeId) {
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
        return m_Data.m_Type == WidgetInstance::ms_TypeId &&
               l_Page->slots[l_SlotIndex].m_Occupied &&
               l_Page->slots[l_SlotIndex].m_Generation ==
                   m_Data.m_Generation;
      }

      uint32_t WidgetInstance::get_capacity()
      {
        return ms_Capacity;
      }

      Low::Util::Handle
      WidgetInstance::_find_by_name(Low::Util::Name p_Name)
      {
        return find_by_name(p_Name).get_id();
      }

      WidgetInstance
      WidgetInstance::find_by_name(Low::Util::Name p_Name)
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

      WidgetInstance
      WidgetInstance::duplicate(Low::Util::Name p_Name) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

        LOW_ASSERT_WARN(false, "Not implemented");
        return 0;
        // LOW_CODEGEN::END::CUSTOM:DUPLICATE
      }

      WidgetInstance
      WidgetInstance::duplicate(WidgetInstance p_Handle,
                                Low::Util::Name p_Name)
      {
        return p_Handle.duplicate(p_Name);
      }

      Low::Util::Handle
      WidgetInstance::_duplicate(Low::Util::Handle p_Handle,
                                 Low::Util::Name p_Name)
      {
        WidgetInstance l_WidgetInstance = p_Handle.get_id();
        return l_WidgetInstance.duplicate(p_Name);
      }

      void
      WidgetInstance::serialize(Low::Util::Serial::Node &p_Node) const
      {
        _LOW_ASSERT(is_alive());

        // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

        // LOW_CODEGEN::END::CUSTOM:SERIALIZER
      }

      void WidgetInstance::serialize(Low::Util::Handle p_Handle,
                                     Low::Util::Serial::Node &p_Node)
      {
        WidgetInstance l_WidgetInstance = p_Handle.get_id();
        l_WidgetInstance.serialize(p_Node);
      }

      Low::Util::Handle
      WidgetInstance::deserialize(Low::Util::Serial::Node &p_Node,
                                  Low::Util::Handle p_Creator)
      {

        // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

        return Low::Util::Handle::DEAD;
        // LOW_CODEGEN::END::CUSTOM:DESERIALIZER
      }

      void WidgetInstance::broadcast_observable(
          Low::Util::Name p_Observable) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        Low::Util::notify(l_Key);
      }

      u64 WidgetInstance::observe(
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

      u64 WidgetInstance::observe(Low::Util::Name p_Observable,
                                  Low::Util::Handle p_Observer) const
      {
        Low::Util::ObserverKey l_Key;
        l_Key.handleId = get_id();
        l_Key.observableName = p_Observable.m_Index;

        return Low::Util::observe(l_Key, p_Observer);
      }

      void WidgetInstance::notify(Low::Util::Handle p_Observed,
                                  Low::Util::Name p_Observable)
      {
        // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY

        // LOW_CODEGEN::END::CUSTOM:NOTIFY
      }

      void WidgetInstance::_notify(Low::Util::Handle p_Observer,
                                   Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
      {
        WidgetInstance l_WidgetInstance = p_Observer.get_id();
        l_WidgetInstance.notify(p_Observed, p_Observable);
      }

      Low::Core::UI::Element WidgetInstance::get_root() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_root

        // LOW_CODEGEN::END::CUSTOM:GETTER_root

        return TYPE_SOA(WidgetInstance, root, Low::Core::UI::Element);
      }
      void WidgetInstance::set_root(Low::Core::UI::Element p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_root

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_root

        // Set new value
        TYPE_SOA(WidgetInstance, root, Low::Core::UI::Element) =
            p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_root

        // LOW_CODEGEN::END::CUSTOM:SETTER_root

        broadcast_observable(N(root));
      }

      Low::Util::List<Low::Core::UI::Element> &
      WidgetInstance::get_elements() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_elements

        // LOW_CODEGEN::END::CUSTOM:GETTER_elements

        return TYPE_SOA(WidgetInstance, elements,
                        Low::Util::List<Low::Core::UI::Element>);
      }

      Low::Util::Name WidgetInstance::get_name() const
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

        // LOW_CODEGEN::END::CUSTOM:GETTER_name

        return TYPE_SOA(WidgetInstance, name, Low::Util::Name);
      }
      void WidgetInstance::set_name(Low::Util::Name p_Value)
      {
        _LOW_ASSERT(is_alive());
        Low::Util::HandleLock<WidgetInstance> l_Lock(get_id());

        // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

        // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

        // Set new value
        TYPE_SOA(WidgetInstance, name, Low::Util::Name) = p_Value;

        // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

        // LOW_CODEGEN::END::CUSTOM:SETTER_name

        broadcast_observable(N(name));
      }

      uint32_t WidgetInstance::create_instance(
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

      u32 WidgetInstance::create_page()
      {
        const u32 l_Capacity = get_capacity();
        LOW_ASSERT((l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
                   "Could not increase capacity for WidgetInstance.");

        Low::Util::Instances::Page *l_Page =
            new Low::Util::Instances::Page;
        Low::Util::Instances::initialize_page(
            l_Page, WidgetInstance::Data::get_size(), ms_PageSize);
        ms_Pages.push_back(l_Page);

        ms_Capacity = l_Capacity + l_Page->size;
        return ms_Pages.size() - 1;
      }

      bool WidgetInstance::get_page_for_index(const u32 p_Index,
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
  } // namespace Core
} // namespace Low
