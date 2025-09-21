#include "LowRendererSkeletalAnimation.h"

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
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t SkeletalAnimation::TYPE_ID = 29;
    uint32_t SkeletalAnimation::ms_Capacity = 0u;
    uint32_t SkeletalAnimation::ms_PageSize = 0u;
    Low::Util::SharedMutex SkeletalAnimation::ms_PagesMutex;
    Low::Util::UniqueLock<Low::Util::SharedMutex>
        SkeletalAnimation::ms_PagesLock(
            SkeletalAnimation::ms_PagesMutex, std::defer_lock);
    Low::Util::List<SkeletalAnimation>
        SkeletalAnimation::ms_LivingInstances;
    Low::Util::List<Low::Util::Instances::Page *>
        SkeletalAnimation::ms_Pages;

    SkeletalAnimation::SkeletalAnimation() : Low::Util::Handle(0ull)
    {
    }
    SkeletalAnimation::SkeletalAnimation(uint64_t p_Id)
        : Low::Util::Handle(p_Id)
    {
    }
    SkeletalAnimation::SkeletalAnimation(SkeletalAnimation &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    Low::Util::Handle SkeletalAnimation::_make(Low::Util::Name p_Name)
    {
      return make(p_Name).get_id();
    }

    SkeletalAnimation SkeletalAnimation::make(Low::Util::Name p_Name)
    {
      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock;
      uint32_t l_Index =
          create_instance(l_PageIndex, l_SlotIndex, l_PageLock);

      SkeletalAnimation l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation =
          ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Generation;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

      l_PageLock.unlock();

      Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(l_Handle);

      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, duration,
                        float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, ticks_per_second,
                        float) = 0.0f;
      new (ACCESSOR_TYPE_SOA_PTR(
          l_Handle, SkeletalAnimation, channels,
          Util::List<Util::Resource::AnimationChannel>))
          Util::List<Util::Resource::AnimationChannel>();
      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, name,
                        Low::Util::Name) = Low::Util::Name(0u);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SkeletalAnimation::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      {
        Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());
        // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

        // LOW_CODEGEN::END::CUSTOM:DESTROY
      }

      broadcast_observable(OBSERVABLE_DESTROY);

      u32 l_PageIndex = 0;
      u32 l_SlotIndex = 0;
      _LOW_ASSERT(
          get_page_for_index(get_index(), l_PageIndex, l_SlotIndex));
      Low::Util::Instances::Page *l_Page = ms_Pages[l_PageIndex];

      Low::Util::UniqueLock<Low::Util::Mutex> l_PageLock(
          l_Page->mutex);
      l_Page->slots[l_SlotIndex].m_Occupied = false;
      l_Page->slots[l_SlotIndex].m_Generation++;

      ms_PagesLock.lock();
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end();) {
        if (it->get_id() == get_id()) {
          it = ms_LivingInstances.erase(it);
        } else {
          it++;
        }
      }
      ms_PagesLock.unlock();
    }

    void SkeletalAnimation::initialize()
    {
      LOCK_PAGES_WRITE(l_PagesLock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer), N(SkeletalAnimation));

      ms_PageSize = Low::Math::Util::clamp(
          Low::Math::Util::next_power_of_two(ms_Capacity), 8, 32);
      {
        u32 l_Capacity = 0u;
        while (l_Capacity < ms_Capacity) {
          Low::Util::Instances::Page *i_Page =
              new Low::Util::Instances::Page;
          Low::Util::Instances::initialize_page(
              i_Page, SkeletalAnimation::Data::get_size(),
              ms_PageSize);
          ms_Pages.push_back(i_Page);
          l_Capacity += ms_PageSize;
        }
        ms_Capacity = l_Capacity;
      }
      LOCK_UNLOCK(l_PagesLock);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkeletalAnimation);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkeletalAnimation::is_alive;
      l_TypeInfo.destroy = &SkeletalAnimation::destroy;
      l_TypeInfo.serialize = &SkeletalAnimation::serialize;
      l_TypeInfo.deserialize = &SkeletalAnimation::deserialize;
      l_TypeInfo.find_by_index = &SkeletalAnimation::_find_by_index;
      l_TypeInfo.notify = &SkeletalAnimation::_notify;
      l_TypeInfo.find_by_name = &SkeletalAnimation::_find_by_name;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SkeletalAnimation::_make;
      l_TypeInfo.duplicate_default = &SkeletalAnimation::_duplicate;
      l_TypeInfo.duplicate_component = nullptr;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SkeletalAnimation::living_instances);
      l_TypeInfo.get_living_count = &SkeletalAnimation::living_count;
      l_TypeInfo.component = false;
      l_TypeInfo.uiComponent = false;
      {
        // Property: duration
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(duration);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimation::Data, duration);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          l_Handle.get_duration();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletalAnimation, duration, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_duration(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          *((float *)p_Data) = l_Handle.get_duration();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: duration
      }
      {
        // Property: ticks_per_second
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(ticks_per_second);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimation::Data, ticks_per_second);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          l_Handle.get_ticks_per_second();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletalAnimation, ticks_per_second, float);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_ticks_per_second(*(float *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          *((float *)p_Data) = l_Handle.get_ticks_per_second();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: ticks_per_second
      }
      {
        // Property: channels
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(channels);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimation::Data, channels);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          l_Handle.get_channels();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletalAnimation, channels,
              Util::List<Util::Resource::AnimationChannel>);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {};
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          *((Util::List<Util::Resource::AnimationChannel> *)p_Data) =
              l_Handle.get_channels();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: channels
      }
      {
        // Property: name
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimation::Data, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          l_Handle.get_name();
          return (void *)&ACCESSOR_TYPE_SOA(
              p_Handle, SkeletalAnimation, name, Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle,
                                void *p_Data) {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          Low::Util::HandleLock<SkeletalAnimation> l_HandleLock(
              l_Handle);
          *((Low::Util::Name *)p_Data) = l_Handle.get_name();
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
        // End property: name
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void SkeletalAnimation::cleanup()
    {
      Low::Util::List<SkeletalAnimation> l_Instances =
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
    SkeletalAnimation::_find_by_index(uint32_t p_Index)
    {
      return find_by_index(p_Index).get_id();
    }

    SkeletalAnimation
    SkeletalAnimation::find_by_index(uint32_t p_Index)
    {
      LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

      SkeletalAnimation l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

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

    SkeletalAnimation
    SkeletalAnimation::create_handle_by_index(u32 p_Index)
    {
      if (p_Index < get_capacity()) {
        return find_by_index(p_Index);
      }

      SkeletalAnimation l_Handle;
      l_Handle.m_Data.m_Index = p_Index;
      l_Handle.m_Data.m_Generation = 0;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

      return l_Handle;
    }

    bool SkeletalAnimation::is_alive() const
    {
      if (m_Data.m_Type != SkeletalAnimation::TYPE_ID) {
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
      return m_Data.m_Type == SkeletalAnimation::TYPE_ID &&
             l_Page->slots[l_SlotIndex].m_Occupied &&
             l_Page->slots[l_SlotIndex].m_Generation ==
                 m_Data.m_Generation;
    }

    uint32_t SkeletalAnimation::get_capacity()
    {
      return ms_Capacity;
    }

    Low::Util::Handle
    SkeletalAnimation::_find_by_name(Low::Util::Name p_Name)
    {
      return find_by_name(p_Name).get_id();
    }

    SkeletalAnimation
    SkeletalAnimation::find_by_name(Low::Util::Name p_Name)
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

    SkeletalAnimation
    SkeletalAnimation::duplicate(Low::Util::Name p_Name) const
    {
      _LOW_ASSERT(is_alive());

      SkeletalAnimation l_Handle = make(p_Name);
      l_Handle.set_duration(get_duration());
      l_Handle.set_ticks_per_second(get_ticks_per_second());

      // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE

      // LOW_CODEGEN::END::CUSTOM:DUPLICATE

      return l_Handle;
    }

    SkeletalAnimation
    SkeletalAnimation::duplicate(SkeletalAnimation p_Handle,
                                 Low::Util::Name p_Name)
    {
      return p_Handle.duplicate(p_Name);
    }

    Low::Util::Handle
    SkeletalAnimation::_duplicate(Low::Util::Handle p_Handle,
                                  Low::Util::Name p_Name)
    {
      SkeletalAnimation l_SkeletalAnimation = p_Handle.get_id();
      return l_SkeletalAnimation.duplicate(p_Name);
    }

    void
    SkeletalAnimation::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["duration"] = get_duration();
      p_Node["ticks_per_second"] = get_ticks_per_second();
      p_Node["name"] = get_name().c_str();

      // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER

      // LOW_CODEGEN::END::CUSTOM:SERIALIZER
    }

    void SkeletalAnimation::serialize(Low::Util::Handle p_Handle,
                                      Low::Util::Yaml::Node &p_Node)
    {
      SkeletalAnimation l_SkeletalAnimation = p_Handle.get_id();
      l_SkeletalAnimation.serialize(p_Node);
    }

    Low::Util::Handle
    SkeletalAnimation::deserialize(Low::Util::Yaml::Node &p_Node,
                                   Low::Util::Handle p_Creator)
    {
      SkeletalAnimation l_Handle =
          SkeletalAnimation::make(N(SkeletalAnimation));

      if (p_Node["duration"]) {
        l_Handle.set_duration(p_Node["duration"].as<float>());
      }
      if (p_Node["ticks_per_second"]) {
        l_Handle.set_ticks_per_second(
            p_Node["ticks_per_second"].as<float>());
      }
      if (p_Node["channels"]) {
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER

      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    void SkeletalAnimation::broadcast_observable(
        Low::Util::Name p_Observable) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      Low::Util::notify(l_Key);
    }

    u64 SkeletalAnimation::observe(
        Low::Util::Name p_Observable,
        Low::Util::Function<void(Low::Util::Handle, Low::Util::Name)>
            p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    u64 SkeletalAnimation::observe(Low::Util::Name p_Observable,
                                   Low::Util::Handle p_Observer) const
    {
      Low::Util::ObserverKey l_Key;
      l_Key.handleId = get_id();
      l_Key.observableName = p_Observable.m_Index;

      return Low::Util::observe(l_Key, p_Observer);
    }

    void SkeletalAnimation::notify(Low::Util::Handle p_Observed,
                                   Low::Util::Name p_Observable)
    {
      // LOW_CODEGEN:BEGIN:CUSTOM:NOTIFY
      // LOW_CODEGEN::END::CUSTOM:NOTIFY
    }

    void SkeletalAnimation::_notify(Low::Util::Handle p_Observer,
                                    Low::Util::Handle p_Observed,
                                    Low::Util::Name p_Observable)
    {
      SkeletalAnimation l_SkeletalAnimation = p_Observer.get_id();
      l_SkeletalAnimation.notify(p_Observed, p_Observable);
    }

    float SkeletalAnimation::get_duration() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_duration

      // LOW_CODEGEN::END::CUSTOM:GETTER_duration

      return TYPE_SOA(SkeletalAnimation, duration, float);
    }
    void SkeletalAnimation::set_duration(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_duration

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_duration

      // Set new value
      TYPE_SOA(SkeletalAnimation, duration, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_duration

      // LOW_CODEGEN::END::CUSTOM:SETTER_duration

      broadcast_observable(N(duration));
    }

    float SkeletalAnimation::get_ticks_per_second() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:GETTER_ticks_per_second

      return TYPE_SOA(SkeletalAnimation, ticks_per_second, float);
    }
    void SkeletalAnimation::set_ticks_per_second(float p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_ticks_per_second

      // Set new value
      TYPE_SOA(SkeletalAnimation, ticks_per_second, float) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:SETTER_ticks_per_second

      broadcast_observable(N(ticks_per_second));
    }

    Util::List<Util::Resource::AnimationChannel> &
    SkeletalAnimation::get_channels() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_channels

      // LOW_CODEGEN::END::CUSTOM:GETTER_channels

      return TYPE_SOA(SkeletalAnimation, channels,
                      Util::List<Util::Resource::AnimationChannel>);
    }

    Low::Util::Name SkeletalAnimation::get_name() const
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      return TYPE_SOA(SkeletalAnimation, name, Low::Util::Name);
    }
    void SkeletalAnimation::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());
      Low::Util::HandleLock<SkeletalAnimation> l_Lock(get_id());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SkeletalAnimation, name, Low::Util::Name) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name

      broadcast_observable(N(name));
    }

    uint32_t SkeletalAnimation::create_instance(
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
          if (!ms_Pages[l_PageIndex]->slots[l_SlotIndex].m_Occupied) {
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

    u32 SkeletalAnimation::create_page()
    {
      const u32 l_Capacity = get_capacity();
      LOW_ASSERT(
          (l_Capacity + ms_PageSize) < LOW_UINT32_MAX,
          "Could not increase capacity for SkeletalAnimation.");

      Low::Util::Instances::Page *l_Page =
          new Low::Util::Instances::Page;
      Low::Util::Instances::initialize_page(
          l_Page, SkeletalAnimation::Data::get_size(), ms_PageSize);
      ms_Pages.push_back(l_Page);

      ms_Capacity = l_Capacity + l_Page->size;
      return ms_Pages.size() - 1;
    }

    bool SkeletalAnimation::get_page_for_index(const u32 p_Index,
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

  } // namespace Renderer
} // namespace Low
