#include "MtdStatusEffect.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "MtdFighter.h"
#include "LowCoreCflatScripting.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Mtd {
  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

  const uint16_t StatusEffect::TYPE_ID = 47;
  uint32_t StatusEffect::ms_Capacity = 0u;
  uint8_t *StatusEffect::ms_Buffer = 0;
  Low::Util::Instances::Slot *StatusEffect::ms_Slots = 0;
  Low::Util::List<StatusEffect> StatusEffect::ms_LivingInstances =
      Low::Util::List<StatusEffect>();

  StatusEffect::StatusEffect() : Low::Util::Handle(0ull)
  {
  }
  StatusEffect::StatusEffect(uint64_t p_Id) : Low::Util::Handle(p_Id)
  {
  }
  StatusEffect::StatusEffect(StatusEffect &p_Copy)
      : Low::Util::Handle(p_Copy.m_Id)
  {
  }

  Low::Util::Handle StatusEffect::_make(Low::Util::Name p_Name)
  {
    return make(p_Name).get_id();
  }

  StatusEffect StatusEffect::make(Low::Util::Name p_Name)
  {
    uint32_t l_Index = create_instance();

    StatusEffect l_Handle;
    l_Handle.m_Data.m_Index = l_Index;
    l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
    l_Handle.m_Data.m_Type = StatusEffect::TYPE_ID;

    new (&ACCESSOR_TYPE_SOA(l_Handle, StatusEffect, title,
                            Low::Util::String)) Low::Util::String();
    new (&ACCESSOR_TYPE_SOA(l_Handle, StatusEffect, description,
                            Low::Util::String)) Low::Util::String();
    new (&ACCESSOR_TYPE_SOA(l_Handle, StatusEffect,
                            start_turn_function, Low::Util::String))
        Low::Util::String();
    ACCESSOR_TYPE_SOA(l_Handle, StatusEffect, name, Low::Util::Name) =
        Low::Util::Name(0u);

    l_Handle.set_name(p_Name);

    ms_LivingInstances.push_back(l_Handle);

    // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
    // LOW_CODEGEN::END::CUSTOM:MAKE

    return l_Handle;
  }

  void StatusEffect::destroy()
  {
    LOW_ASSERT(is_alive(), "Cannot destroy dead object");

    // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
    // LOW_CODEGEN::END::CUSTOM:DESTROY

    ms_Slots[this->m_Data.m_Index].m_Occupied = false;
    ms_Slots[this->m_Data.m_Index].m_Generation++;

    const StatusEffect *l_Instances = living_instances();
    bool l_LivingInstanceFound = false;
    for (uint32_t i = 0u; i < living_count(); ++i) {
      if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
        ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
        l_LivingInstanceFound = true;
        break;
      }
    }
  }

  void StatusEffect::initialize()
  {
    // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
    // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

    ms_Capacity = Low::Util::Config::get_capacity(N(MistedaPlugin),
                                                  N(StatusEffect));

    initialize_buffer(&ms_Buffer, StatusEffectData::get_size(),
                      get_capacity(), &ms_Slots);

    LOW_PROFILE_ALLOC(type_buffer_StatusEffect);
    LOW_PROFILE_ALLOC(type_slots_StatusEffect);

    Low::Util::RTTI::TypeInfo l_TypeInfo;
    l_TypeInfo.name = N(StatusEffect);
    l_TypeInfo.typeId = TYPE_ID;
    l_TypeInfo.get_capacity = &get_capacity;
    l_TypeInfo.is_alive = &StatusEffect::is_alive;
    l_TypeInfo.destroy = &StatusEffect::destroy;
    l_TypeInfo.serialize = &StatusEffect::serialize;
    l_TypeInfo.deserialize = &StatusEffect::deserialize;
    l_TypeInfo.make_component = nullptr;
    l_TypeInfo.make_default = &StatusEffect::_make;
    l_TypeInfo.duplicate_default = &StatusEffect::_duplicate;
    l_TypeInfo.duplicate_component = nullptr;
    l_TypeInfo.get_living_instances =
        reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
            &StatusEffect::living_instances);
    l_TypeInfo.get_living_count = &StatusEffect::living_count;
    l_TypeInfo.component = false;
    l_TypeInfo.uiComponent = false;
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(title);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset = offsetof(StatusEffectData, title);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.get_title();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, StatusEffect,
                                          title, Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.set_title(*(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(description);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset =
          offsetof(StatusEffectData, description);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.get_description();
        return (void *)&ACCESSOR_TYPE_SOA(
            p_Handle, StatusEffect, description, Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.set_description(*(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(start_turn_function);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset =
          offsetof(StatusEffectData, start_turn_function);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.get_start_turn_function();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, StatusEffect,
                                          start_turn_function,
                                          Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.set_start_turn_function(
            *(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(name);
      l_PropertyInfo.editorProperty = false;
      l_PropertyInfo.dataOffset = offsetof(StatusEffectData, name);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.get_name();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, StatusEffect,
                                          name, Low::Util::Name);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        StatusEffect l_Handle = p_Handle.get_id();
        l_Handle.set_name(*(Low::Util::Name *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
  }

  void StatusEffect::cleanup()
  {
    Low::Util::List<StatusEffect> l_Instances = ms_LivingInstances;
    for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
      l_Instances[i].destroy();
    }
    free(ms_Buffer);
    free(ms_Slots);

    LOW_PROFILE_FREE(type_buffer_StatusEffect);
    LOW_PROFILE_FREE(type_slots_StatusEffect);
  }

  StatusEffect StatusEffect::find_by_index(uint32_t p_Index)
  {
    LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

    StatusEffect l_Handle;
    l_Handle.m_Data.m_Index = p_Index;
    l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
    l_Handle.m_Data.m_Type = StatusEffect::TYPE_ID;

    return l_Handle;
  }

  bool StatusEffect::is_alive() const
  {
    return m_Data.m_Type == StatusEffect::TYPE_ID &&
           check_alive(ms_Slots, StatusEffect::get_capacity());
  }

  uint32_t StatusEffect::get_capacity()
  {
    return ms_Capacity;
  }

  StatusEffect StatusEffect::find_by_name(Low::Util::Name p_Name)
  {
    for (auto it = ms_LivingInstances.begin();
         it != ms_LivingInstances.end(); ++it) {
      if (it->get_name() == p_Name) {
        return *it;
      }
    }
  }

  StatusEffect StatusEffect::duplicate(Low::Util::Name p_Name) const
  {
    _LOW_ASSERT(is_alive());

    StatusEffect l_Handle = make(p_Name);
    l_Handle.set_title(get_title());
    l_Handle.set_description(get_description());
    l_Handle.set_start_turn_function(get_start_turn_function());

    // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
    // LOW_CODEGEN::END::CUSTOM:DUPLICATE

    return l_Handle;
  }

  StatusEffect StatusEffect::duplicate(StatusEffect p_Handle,
                                       Low::Util::Name p_Name)
  {
    return p_Handle.duplicate(p_Name);
  }

  Low::Util::Handle
  StatusEffect::_duplicate(Low::Util::Handle p_Handle,
                           Low::Util::Name p_Name)
  {
    StatusEffect l_StatusEffect = p_Handle.get_id();
    return l_StatusEffect.duplicate(p_Name);
  }

  void StatusEffect::serialize(Low::Util::Yaml::Node &p_Node) const
  {
    _LOW_ASSERT(is_alive());

    p_Node["title"] = get_title().c_str();
    p_Node["description"] = get_description().c_str();
    p_Node["start_turn_function"] = get_start_turn_function().c_str();
    p_Node["name"] = get_name().c_str();

    // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
    // LOW_CODEGEN::END::CUSTOM:SERIALIZER
  }

  void StatusEffect::serialize(Low::Util::Handle p_Handle,
                               Low::Util::Yaml::Node &p_Node)
  {
    StatusEffect l_StatusEffect = p_Handle.get_id();
    l_StatusEffect.serialize(p_Node);
  }

  Low::Util::Handle
  StatusEffect::deserialize(Low::Util::Yaml::Node &p_Node,
                            Low::Util::Handle p_Creator)
  {
    StatusEffect l_Handle = StatusEffect::make(N(StatusEffect));

    if (p_Node["title"]) {
      l_Handle.set_title(LOW_YAML_AS_STRING(p_Node["title"]));
    }
    if (p_Node["description"]) {
      l_Handle.set_description(
          LOW_YAML_AS_STRING(p_Node["description"]));
    }
    if (p_Node["start_turn_function"]) {
      l_Handle.set_start_turn_function(
          LOW_YAML_AS_STRING(p_Node["start_turn_function"]));
    }
    if (p_Node["name"]) {
      l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
    // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

    return l_Handle;
  }

  Low::Util::String &StatusEffect::get_title() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_title
    // LOW_CODEGEN::END::CUSTOM:GETTER_title

    return TYPE_SOA(StatusEffect, title, Low::Util::String);
  }
  void StatusEffect::set_title(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_title
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_title

    // Set new value
    TYPE_SOA(StatusEffect, title, Low::Util::String) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_title
    // LOW_CODEGEN::END::CUSTOM:SETTER_title
  }

  Low::Util::String &StatusEffect::get_description() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_description
    // LOW_CODEGEN::END::CUSTOM:GETTER_description

    return TYPE_SOA(StatusEffect, description, Low::Util::String);
  }
  void StatusEffect::set_description(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_description
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_description

    // Set new value
    TYPE_SOA(StatusEffect, description, Low::Util::String) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_description
    // LOW_CODEGEN::END::CUSTOM:SETTER_description
  }

  Low::Util::String &StatusEffect::get_start_turn_function() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_start_turn_function
    // LOW_CODEGEN::END::CUSTOM:GETTER_start_turn_function

    return TYPE_SOA(StatusEffect, start_turn_function,
                    Low::Util::String);
  }
  void
  StatusEffect::set_start_turn_function(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_start_turn_function
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_start_turn_function

    // Set new value
    TYPE_SOA(StatusEffect, start_turn_function, Low::Util::String) =
        p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_start_turn_function
    // LOW_CODEGEN::END::CUSTOM:SETTER_start_turn_function
  }

  Low::Util::Name StatusEffect::get_name() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
    // LOW_CODEGEN::END::CUSTOM:GETTER_name

    return TYPE_SOA(StatusEffect, name, Low::Util::Name);
  }
  void StatusEffect::set_name(Low::Util::Name p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

    // Set new value
    TYPE_SOA(StatusEffect, name, Low::Util::Name) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
    // LOW_CODEGEN::END::CUSTOM:SETTER_name
  }

  void StatusEffect::execute(Mtd::Component::Fighter p_Caster,
                             Mtd::Component::Fighter p_Target,
                             int p_Remaining)
  {
    // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
    _LOW_ASSERT(is_alive());

    if (get_start_turn_function().empty()) {
      LOW_LOG_WARN << "Trying to execute status effect " << get_name()
                   << " but start turn function is empty."
                   << LOW_LOG_END;
      return;
    }

    Cflat::Function *l_Function =
        Low::Core::Scripting::get_environment()->getFunction(
            get_start_turn_function().c_str());

    LOW_ASSERT(l_Function,
               "Could not find statuseffect start turn function");

    static const Cflat::TypeUsage kTypeUsageFighter =
        Low::Core::Scripting::get_environment()->getTypeUsage(
            "Mtd::Component::Fighter");
    static const Cflat::TypeUsage kTypeUsageInt =
        Low::Core::Scripting::get_environment()->getTypeUsage("int");

    Cflat::Value l_ReturnValue;
    CflatArgsVector(Cflat::Value) l_Arguments;

    Cflat::Value l_Caster;
    l_Caster.initExternal(kTypeUsageFighter);
    l_Caster.set(&p_Caster);
    l_Arguments.push_back(l_Caster);

    Cflat::Value l_Target;
    l_Target.initExternal(kTypeUsageFighter);
    l_Target.set(&p_Target);
    l_Arguments.push_back(l_Target);

    Cflat::Value l_Remaining;
    l_Remaining.initExternal(kTypeUsageInt);
    l_Remaining.set(&p_Remaining);
    l_Arguments.push_back(l_Remaining);

    l_Function->execute(l_Arguments, &l_ReturnValue);
    // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
  }

  uint32_t StatusEffect::create_instance()
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

  void StatusEffect::increase_budget()
  {
    uint32_t l_Capacity = get_capacity();
    uint32_t l_CapacityIncrease =
        std::max(std::min(l_Capacity, 64u), 1u);
    l_CapacityIncrease =
        std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

    LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

    uint8_t *l_NewBuffer = (uint8_t *)malloc(
        (l_Capacity + l_CapacityIncrease) * sizeof(StatusEffectData));
    Low::Util::Instances::Slot *l_NewSlots =
        (Low::Util::Instances::Slot *)malloc(
            (l_Capacity + l_CapacityIncrease) *
            sizeof(Low::Util::Instances::Slot));

    memcpy(l_NewSlots, ms_Slots,
           l_Capacity * sizeof(Low::Util::Instances::Slot));
    {
      memcpy(&l_NewBuffer[offsetof(StatusEffectData, title) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(StatusEffectData, title) *
                        (l_Capacity)],
             l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(&l_NewBuffer[offsetof(StatusEffectData, description) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(StatusEffectData, description) *
                        (l_Capacity)],
             l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(
          &l_NewBuffer[offsetof(StatusEffectData,
                                start_turn_function) *
                       (l_Capacity + l_CapacityIncrease)],
          &ms_Buffer[offsetof(StatusEffectData, start_turn_function) *
                     (l_Capacity)],
          l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(
          &l_NewBuffer[offsetof(StatusEffectData, name) *
                       (l_Capacity + l_CapacityIncrease)],
          &ms_Buffer[offsetof(StatusEffectData, name) * (l_Capacity)],
          l_Capacity * sizeof(Low::Util::Name));
    }
    for (uint32_t i = l_Capacity; i < l_Capacity + l_CapacityIncrease;
         ++i) {
      l_NewSlots[i].m_Occupied = false;
      l_NewSlots[i].m_Generation = 0;
    }
    free(ms_Buffer);
    free(ms_Slots);
    ms_Buffer = l_NewBuffer;
    ms_Slots = l_NewSlots;
    ms_Capacity = l_Capacity + l_CapacityIncrease;

    LOW_LOG_DEBUG << "Auto-increased budget for StatusEffect from "
                  << l_Capacity << " to "
                  << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
  }

  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

} // namespace Mtd
