#include "LowRendererSkeletalAnimation.h"

#include <algorithm>

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE

// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Low {
  namespace Renderer {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    const uint16_t SkeletalAnimation::TYPE_ID = 29;
    uint32_t SkeletalAnimation::ms_Capacity = 0u;
    uint8_t *SkeletalAnimation::ms_Buffer = 0;
    std::shared_mutex SkeletalAnimation::ms_BufferMutex;
    Low::Util::Instances::Slot *SkeletalAnimation::ms_Slots = 0;
    Low::Util::List<SkeletalAnimation>
        SkeletalAnimation::ms_LivingInstances =
            Low::Util::List<SkeletalAnimation>();

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
      WRITE_LOCK(l_Lock);
      uint32_t l_Index = create_instance();

      SkeletalAnimation l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, duration,
                        float) = 0.0f;
      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, ticks_per_second,
                        float) = 0.0f;
      new (&ACCESSOR_TYPE_SOA(
          l_Handle, SkeletalAnimation, channels,
          Util::List<Util::Resource::AnimationChannel>))
          Util::List<Util::Resource::AnimationChannel>();
      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, name,
                        Low::Util::Name) = Low::Util::Name(0u);
      LOCK_UNLOCK(l_Lock);

      l_Handle.set_name(p_Name);

      ms_LivingInstances.push_back(l_Handle);

      // LOW_CODEGEN:BEGIN:CUSTOM:MAKE

      // LOW_CODEGEN::END::CUSTOM:MAKE

      return l_Handle;
    }

    void SkeletalAnimation::destroy()
    {
      LOW_ASSERT(is_alive(), "Cannot destroy dead object");

      // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY

      // LOW_CODEGEN::END::CUSTOM:DESTROY

      WRITE_LOCK(l_Lock);
      ms_Slots[this->m_Data.m_Index].m_Occupied = false;
      ms_Slots[this->m_Data.m_Index].m_Generation++;

      const SkeletalAnimation *l_Instances = living_instances();
      bool l_LivingInstanceFound = false;
      for (uint32_t i = 0u; i < living_count(); ++i) {
        if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
          ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
          l_LivingInstanceFound = true;
          break;
        }
      }
    }

    void SkeletalAnimation::initialize()
    {
      WRITE_LOCK(l_Lock);
      // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE

      // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

      ms_Capacity = Low::Util::Config::get_capacity(
          N(LowRenderer), N(SkeletalAnimation));

      initialize_buffer(&ms_Buffer, SkeletalAnimationData::get_size(),
                        get_capacity(), &ms_Slots);
      LOCK_UNLOCK(l_Lock);

      LOW_PROFILE_ALLOC(type_buffer_SkeletalAnimation);
      LOW_PROFILE_ALLOC(type_slots_SkeletalAnimation);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkeletalAnimation);
      l_TypeInfo.typeId = TYPE_ID;
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkeletalAnimation::is_alive;
      l_TypeInfo.destroy = &SkeletalAnimation::destroy;
      l_TypeInfo.serialize = &SkeletalAnimation::serialize;
      l_TypeInfo.deserialize = &SkeletalAnimation::deserialize;
      l_TypeInfo.find_by_index = &SkeletalAnimation::_find_by_index;
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
            offsetof(SkeletalAnimationData, duration);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
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
            offsetof(SkeletalAnimationData, ticks_per_second);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::FLOAT;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
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
            offsetof(SkeletalAnimationData, channels);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UNKNOWN;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
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
            offsetof(SkeletalAnimationData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.handleType = 0;
        l_PropertyInfo.get_return =
            [](Low::Util::Handle p_Handle) -> void const * {
          SkeletalAnimation l_Handle = p_Handle.get_id();
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
      WRITE_LOCK(l_Lock);
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_SkeletalAnimation);
      LOW_PROFILE_FREE(type_slots_SkeletalAnimation);
      LOCK_UNLOCK(l_Lock);
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
      l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

      return l_Handle;
    }

    bool SkeletalAnimation::is_alive() const
    {
      READ_LOCK(l_Lock);
      return m_Data.m_Type == SkeletalAnimation::TYPE_ID &&
             check_alive(ms_Slots, SkeletalAnimation::get_capacity());
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
      for (auto it = ms_LivingInstances.begin();
           it != ms_LivingInstances.end(); ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
      return 0ull;
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

    float SkeletalAnimation::get_duration() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_duration

      // LOW_CODEGEN::END::CUSTOM:GETTER_duration

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(SkeletalAnimation, duration, float);
    }
    void SkeletalAnimation::set_duration(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_duration

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_duration

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(SkeletalAnimation, duration, float) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_duration

      // LOW_CODEGEN::END::CUSTOM:SETTER_duration
    }

    float SkeletalAnimation::get_ticks_per_second() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:GETTER_ticks_per_second

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(SkeletalAnimation, ticks_per_second, float);
    }
    void SkeletalAnimation::set_ticks_per_second(float p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_ticks_per_second

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(SkeletalAnimation, ticks_per_second, float) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_ticks_per_second

      // LOW_CODEGEN::END::CUSTOM:SETTER_ticks_per_second
    }

    Util::List<Util::Resource::AnimationChannel> &
    SkeletalAnimation::get_channels() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_channels

      // LOW_CODEGEN::END::CUSTOM:GETTER_channels

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(SkeletalAnimation, channels,
                      Util::List<Util::Resource::AnimationChannel>);
    }

    Low::Util::Name SkeletalAnimation::get_name() const
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name

      // LOW_CODEGEN::END::CUSTOM:GETTER_name

      READ_LOCK(l_ReadLock);
      return TYPE_SOA(SkeletalAnimation, name, Low::Util::Name);
    }
    void SkeletalAnimation::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name

      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      WRITE_LOCK(l_WriteLock);
      TYPE_SOA(SkeletalAnimation, name, Low::Util::Name) = p_Value;
      LOCK_UNLOCK(l_WriteLock);

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name

      // LOW_CODEGEN::END::CUSTOM:SETTER_name
    }

    uint32_t SkeletalAnimation::create_instance()
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

    void SkeletalAnimation::increase_budget()
    {
      uint32_t l_Capacity = get_capacity();
      uint32_t l_CapacityIncrease =
          std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0,
                 "Could not increase capacity");

      uint8_t *l_NewBuffer =
          (uint8_t *)malloc((l_Capacity + l_CapacityIncrease) *
                            sizeof(SkeletalAnimationData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletalAnimationData, duration) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletalAnimationData, duration) *
                       (l_Capacity)],
            l_Capacity * sizeof(float));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData,
                                     ticks_per_second) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData,
                                   ticks_per_second) *
                          (l_Capacity)],
               l_Capacity * sizeof(float));
      }
      {
        for (auto it = ms_LivingInstances.begin();
             it != ms_LivingInstances.end(); ++it) {
          SkeletalAnimation i_SkeletalAnimation = *it;

          auto *i_ValPtr =
              new (&l_NewBuffer
                       [offsetof(SkeletalAnimationData, channels) *
                            (l_Capacity + l_CapacityIncrease) +
                        (it->get_index() *
                         sizeof(Util::List<
                                Util::Resource::AnimationChannel>))])
                  Util::List<Util::Resource::AnimationChannel>();
          *i_ValPtr = ACCESSOR_TYPE_SOA(
              i_SkeletalAnimation, SkeletalAnimation, channels,
              Util::List<Util::Resource::AnimationChannel>);
        }
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, name) *
                          (l_Capacity)],
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

      LOW_LOG_DEBUG
          << "Auto-increased budget for SkeletalAnimation from "
          << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
          << LOW_LOG_END;
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE

    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

  } // namespace Renderer
} // namespace Low
