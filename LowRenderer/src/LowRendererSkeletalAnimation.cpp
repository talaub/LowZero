#include "LowRendererSkeletalAnimation.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

namespace Low {
  namespace Renderer {
    const uint16_t SkeletalAnimation::TYPE_ID = 29;
    uint32_t SkeletalAnimation::ms_Capacity = 0u;
    uint8_t *SkeletalAnimation::ms_Buffer = 0;
    Low::Util::Instances::Slot *SkeletalAnimation::ms_Slots = 0;
    Low::Util::List<SkeletalAnimation> SkeletalAnimation::ms_LivingInstances =
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
      uint32_t l_Index = create_instance();

      SkeletalAnimation l_Handle;
      l_Handle.m_Data.m_Index = l_Index;
      l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
      l_Handle.m_Data.m_Type = SkeletalAnimation::TYPE_ID;

      ACCESSOR_TYPE_SOA(l_Handle, SkeletalAnimation, name, Low::Util::Name) =
          Low::Util::Name(0u);

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
      _LOW_ASSERT(l_LivingInstanceFound);
    }

    void SkeletalAnimation::initialize()
    {
      ms_Capacity =
          Low::Util::Config::get_capacity(N(LowRenderer), N(SkeletalAnimation));

      initialize_buffer(&ms_Buffer, SkeletalAnimationData::get_size(),
                        get_capacity(), &ms_Slots);

      LOW_PROFILE_ALLOC(type_buffer_SkeletalAnimation);
      LOW_PROFILE_ALLOC(type_slots_SkeletalAnimation);

      Low::Util::RTTI::TypeInfo l_TypeInfo;
      l_TypeInfo.name = N(SkeletalAnimation);
      l_TypeInfo.get_capacity = &get_capacity;
      l_TypeInfo.is_alive = &SkeletalAnimation::is_alive;
      l_TypeInfo.destroy = &SkeletalAnimation::destroy;
      l_TypeInfo.serialize = &SkeletalAnimation::serialize;
      l_TypeInfo.deserialize = &SkeletalAnimation::deserialize;
      l_TypeInfo.make_component = nullptr;
      l_TypeInfo.make_default = &SkeletalAnimation::_make;
      l_TypeInfo.get_living_instances =
          reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
              &SkeletalAnimation::living_instances);
      l_TypeInfo.get_living_count = &SkeletalAnimation::living_count;
      l_TypeInfo.component = false;
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(skeletal_animation_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, skeletal_animation_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            skeletal_animation_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_skeletal_animation_start(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(channel_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, channel_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            channel_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_channel_start(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(channel_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, channel_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            channel_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_channel_count(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(position_key_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, position_key_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            position_key_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_position_key_start(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(position_key_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, position_key_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            position_key_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_position_key_count(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rotation_key_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, rotation_key_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            rotation_key_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_rotation_key_start(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(rotation_key_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, rotation_key_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            rotation_key_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_rotation_key_count(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(scale_key_start);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, scale_key_start);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            scale_key_start, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_scale_key_start(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(scale_key_count);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset =
            offsetof(SkeletalAnimationData, scale_key_count);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation,
                                            scale_key_count, uint32_t);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_scale_key_count(*(uint32_t *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      {
        Low::Util::RTTI::PropertyInfo l_PropertyInfo;
        l_PropertyInfo.name = N(name);
        l_PropertyInfo.editorProperty = false;
        l_PropertyInfo.dataOffset = offsetof(SkeletalAnimationData, name);
        l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
        l_PropertyInfo.get = [](Low::Util::Handle p_Handle) -> void const * {
          return (void *)&ACCESSOR_TYPE_SOA(p_Handle, SkeletalAnimation, name,
                                            Low::Util::Name);
        };
        l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                                const void *p_Data) -> void {
          SkeletalAnimation l_Handle = p_Handle.get_id();
          l_Handle.set_name(*(Low::Util::Name *)p_Data);
        };
        l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
      }
      Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
    }

    void SkeletalAnimation::cleanup()
    {
      Low::Util::List<SkeletalAnimation> l_Instances = ms_LivingInstances;
      for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
        l_Instances[i].destroy();
      }
      free(ms_Buffer);
      free(ms_Slots);

      LOW_PROFILE_FREE(type_buffer_SkeletalAnimation);
      LOW_PROFILE_FREE(type_slots_SkeletalAnimation);
    }

    SkeletalAnimation SkeletalAnimation::find_by_index(uint32_t p_Index)
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
      return m_Data.m_Type == SkeletalAnimation::TYPE_ID &&
             check_alive(ms_Slots, SkeletalAnimation::get_capacity());
    }

    uint32_t SkeletalAnimation::get_capacity()
    {
      return ms_Capacity;
    }

    SkeletalAnimation SkeletalAnimation::find_by_name(Low::Util::Name p_Name)
    {
      for (auto it = ms_LivingInstances.begin(); it != ms_LivingInstances.end();
           ++it) {
        if (it->get_name() == p_Name) {
          return *it;
        }
      }
    }

    void SkeletalAnimation::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      _LOW_ASSERT(is_alive());

      p_Node["skeletal_animation_start"] = get_skeletal_animation_start();
      p_Node["channel_start"] = get_channel_start();
      p_Node["channel_count"] = get_channel_count();
      p_Node["position_key_start"] = get_position_key_start();
      p_Node["position_key_count"] = get_position_key_count();
      p_Node["rotation_key_start"] = get_rotation_key_start();
      p_Node["rotation_key_count"] = get_rotation_key_count();
      p_Node["scale_key_start"] = get_scale_key_start();
      p_Node["scale_key_count"] = get_scale_key_count();
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

      if (p_Node["skeletal_animation_start"]) {
        l_Handle.set_skeletal_animation_start(
            p_Node["skeletal_animation_start"].as<uint32_t>());
      }
      if (p_Node["channel_start"]) {
        l_Handle.set_channel_start(p_Node["channel_start"].as<uint32_t>());
      }
      if (p_Node["channel_count"]) {
        l_Handle.set_channel_count(p_Node["channel_count"].as<uint32_t>());
      }
      if (p_Node["position_key_start"]) {
        l_Handle.set_position_key_start(
            p_Node["position_key_start"].as<uint32_t>());
      }
      if (p_Node["position_key_count"]) {
        l_Handle.set_position_key_count(
            p_Node["position_key_count"].as<uint32_t>());
      }
      if (p_Node["rotation_key_start"]) {
        l_Handle.set_rotation_key_start(
            p_Node["rotation_key_start"].as<uint32_t>());
      }
      if (p_Node["rotation_key_count"]) {
        l_Handle.set_rotation_key_count(
            p_Node["rotation_key_count"].as<uint32_t>());
      }
      if (p_Node["scale_key_start"]) {
        l_Handle.set_scale_key_start(p_Node["scale_key_start"].as<uint32_t>());
      }
      if (p_Node["scale_key_count"]) {
        l_Handle.set_scale_key_count(p_Node["scale_key_count"].as<uint32_t>());
      }
      if (p_Node["name"]) {
        l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
      }

      // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
      // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

      return l_Handle;
    }

    uint32_t SkeletalAnimation::get_skeletal_animation_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, skeletal_animation_start, uint32_t);
    }
    void SkeletalAnimation::set_skeletal_animation_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_skeletal_animation_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_skeletal_animation_start

      // Set new value
      TYPE_SOA(SkeletalAnimation, skeletal_animation_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_skeletal_animation_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_skeletal_animation_start
    }

    uint32_t SkeletalAnimation::get_channel_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, channel_start, uint32_t);
    }
    void SkeletalAnimation::set_channel_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_channel_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_channel_start

      // Set new value
      TYPE_SOA(SkeletalAnimation, channel_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_channel_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_channel_start
    }

    uint32_t SkeletalAnimation::get_channel_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, channel_count, uint32_t);
    }
    void SkeletalAnimation::set_channel_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_channel_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_channel_count

      // Set new value
      TYPE_SOA(SkeletalAnimation, channel_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_channel_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_channel_count
    }

    uint32_t SkeletalAnimation::get_position_key_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, position_key_start, uint32_t);
    }
    void SkeletalAnimation::set_position_key_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position_key_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position_key_start

      // Set new value
      TYPE_SOA(SkeletalAnimation, position_key_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position_key_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_position_key_start
    }

    uint32_t SkeletalAnimation::get_position_key_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, position_key_count, uint32_t);
    }
    void SkeletalAnimation::set_position_key_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_position_key_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_position_key_count

      // Set new value
      TYPE_SOA(SkeletalAnimation, position_key_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_position_key_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_position_key_count
    }

    uint32_t SkeletalAnimation::get_rotation_key_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, rotation_key_start, uint32_t);
    }
    void SkeletalAnimation::set_rotation_key_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation_key_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation_key_start

      // Set new value
      TYPE_SOA(SkeletalAnimation, rotation_key_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation_key_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_rotation_key_start
    }

    uint32_t SkeletalAnimation::get_rotation_key_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, rotation_key_count, uint32_t);
    }
    void SkeletalAnimation::set_rotation_key_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_rotation_key_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_rotation_key_count

      // Set new value
      TYPE_SOA(SkeletalAnimation, rotation_key_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_rotation_key_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_rotation_key_count
    }

    uint32_t SkeletalAnimation::get_scale_key_start() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, scale_key_start, uint32_t);
    }
    void SkeletalAnimation::set_scale_key_start(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scale_key_start
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_scale_key_start

      // Set new value
      TYPE_SOA(SkeletalAnimation, scale_key_start, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale_key_start
      // LOW_CODEGEN::END::CUSTOM:SETTER_scale_key_start
    }

    uint32_t SkeletalAnimation::get_scale_key_count() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, scale_key_count, uint32_t);
    }
    void SkeletalAnimation::set_scale_key_count(uint32_t p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_scale_key_count
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_scale_key_count

      // Set new value
      TYPE_SOA(SkeletalAnimation, scale_key_count, uint32_t) = p_Value;

      // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_scale_key_count
      // LOW_CODEGEN::END::CUSTOM:SETTER_scale_key_count
    }

    Low::Util::Name SkeletalAnimation::get_name() const
    {
      _LOW_ASSERT(is_alive());
      return TYPE_SOA(SkeletalAnimation, name, Low::Util::Name);
    }
    void SkeletalAnimation::set_name(Low::Util::Name p_Value)
    {
      _LOW_ASSERT(is_alive());

      // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
      // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

      // Set new value
      TYPE_SOA(SkeletalAnimation, name, Low::Util::Name) = p_Value;

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
      uint32_t l_CapacityIncrease = std::max(std::min(l_Capacity, 64u), 1u);
      l_CapacityIncrease =
          std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

      LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

      uint8_t *l_NewBuffer = (uint8_t *)malloc(
          (l_Capacity + l_CapacityIncrease) * sizeof(SkeletalAnimationData));
      Low::Util::Instances::Slot *l_NewSlots =
          (Low::Util::Instances::Slot *)malloc(
              (l_Capacity + l_CapacityIncrease) *
              sizeof(Low::Util::Instances::Slot));

      memcpy(l_NewSlots, ms_Slots,
             l_Capacity * sizeof(Low::Util::Instances::Slot));
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData,
                                     skeletal_animation_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData,
                                   skeletal_animation_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, channel_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, channel_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, channel_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, channel_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletalAnimationData, position_key_start) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletalAnimationData, position_key_start) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletalAnimationData, position_key_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletalAnimationData, position_key_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletalAnimationData, rotation_key_start) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletalAnimationData, rotation_key_start) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(
            &l_NewBuffer[offsetof(SkeletalAnimationData, rotation_key_count) *
                         (l_Capacity + l_CapacityIncrease)],
            &ms_Buffer[offsetof(SkeletalAnimationData, rotation_key_count) *
                       (l_Capacity)],
            l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, scale_key_start) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, scale_key_start) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, scale_key_count) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, scale_key_count) *
                          (l_Capacity)],
               l_Capacity * sizeof(uint32_t));
      }
      {
        memcpy(&l_NewBuffer[offsetof(SkeletalAnimationData, name) *
                            (l_Capacity + l_CapacityIncrease)],
               &ms_Buffer[offsetof(SkeletalAnimationData, name) * (l_Capacity)],
               l_Capacity * sizeof(Low::Util::Name));
      }
      for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease; ++i) {
        l_NewSlots[i].m_Occupied = false;
        l_NewSlots[i].m_Generation = 0;
      }
      free(ms_Buffer);
      free(ms_Slots);
      ms_Buffer = l_NewBuffer;
      ms_Slots = l_NewSlots;
      ms_Capacity = l_Capacity + l_CapacityIncrease;

      LOW_LOG_DEBUG << "Auto-increased budget for SkeletalAnimation from "
                    << l_Capacity << " to " << (l_Capacity + l_CapacityIncrease)
                    << LOW_LOG_END;
    }
  } // namespace Renderer
} // namespace Low
