#include "MtdAbility.h"

#include <algorithm>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"
#include "LowUtilSerialization.h"

#include "LowCoreUiText.h"
#include "LowCoreUiImage.h"
#include "LowCoreUiView.h"
#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"

// LOW_CODEGEN:BEGIN:CUSTOM:SOURCE_CODE
#include "MtdFighter.h"
#include "LowCoreCflatScripting.h"
// LOW_CODEGEN::END::CUSTOM:SOURCE_CODE

namespace Mtd {
  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

  const uint16_t Ability::TYPE_ID = 45;
  uint32_t Ability::ms_Capacity = 0u;
  uint8_t *Ability::ms_Buffer = 0;
  Low::Util::Instances::Slot *Ability::ms_Slots = 0;
  Low::Util::List<Ability> Ability::ms_LivingInstances =
      Low::Util::List<Ability>();

  Ability::Ability() : Low::Util::Handle(0ull)
  {
  }
  Ability::Ability(uint64_t p_Id) : Low::Util::Handle(p_Id)
  {
  }
  Ability::Ability(Ability &p_Copy) : Low::Util::Handle(p_Copy.m_Id)
  {
  }

  Low::Util::Handle Ability::_make(Low::Util::Name p_Name)
  {
    return make(p_Name).get_id();
  }

  Ability Ability::make(Low::Util::Name p_Name)
  {
    uint32_t l_Index = create_instance();

    Ability l_Handle;
    l_Handle.m_Data.m_Index = l_Index;
    l_Handle.m_Data.m_Generation = ms_Slots[l_Index].m_Generation;
    l_Handle.m_Data.m_Type = Ability::TYPE_ID;

    new (&ACCESSOR_TYPE_SOA(l_Handle, Ability, title,
                            Low::Util::String)) Low::Util::String();
    new (&ACCESSOR_TYPE_SOA(l_Handle, Ability, description,
                            Low::Util::String)) Low::Util::String();
    new (&ACCESSOR_TYPE_SOA(l_Handle, Ability, execute_function_name,
                            Low::Util::String)) Low::Util::String();
    ACCESSOR_TYPE_SOA(l_Handle, Ability, name, Low::Util::Name) =
        Low::Util::Name(0u);

    l_Handle.set_name(p_Name);

    ms_LivingInstances.push_back(l_Handle);

    // LOW_CODEGEN:BEGIN:CUSTOM:MAKE
    // LOW_CODEGEN::END::CUSTOM:MAKE

    return l_Handle;
  }

  void Ability::destroy()
  {
    LOW_ASSERT(is_alive(), "Cannot destroy dead object");

    // LOW_CODEGEN:BEGIN:CUSTOM:DESTROY
    // LOW_CODEGEN::END::CUSTOM:DESTROY

    ms_Slots[this->m_Data.m_Index].m_Occupied = false;
    ms_Slots[this->m_Data.m_Index].m_Generation++;

    const Ability *l_Instances = living_instances();
    bool l_LivingInstanceFound = false;
    for (uint32_t i = 0u; i < living_count(); ++i) {
      if (l_Instances[i].m_Data.m_Index == m_Data.m_Index) {
        ms_LivingInstances.erase(ms_LivingInstances.begin() + i);
        l_LivingInstanceFound = true;
        break;
      }
    }
  }

  void Ability::initialize()
  {
    // LOW_CODEGEN:BEGIN:CUSTOM:PREINITIALIZE
    // LOW_CODEGEN::END::CUSTOM:PREINITIALIZE

    ms_Capacity =
        Low::Util::Config::get_capacity(N(MistedaPlugin), N(Ability));

    initialize_buffer(&ms_Buffer, AbilityData::get_size(),
                      get_capacity(), &ms_Slots);

    LOW_PROFILE_ALLOC(type_buffer_Ability);
    LOW_PROFILE_ALLOC(type_slots_Ability);

    Low::Util::RTTI::TypeInfo l_TypeInfo;
    l_TypeInfo.name = N(Ability);
    l_TypeInfo.typeId = TYPE_ID;
    l_TypeInfo.get_capacity = &get_capacity;
    l_TypeInfo.is_alive = &Ability::is_alive;
    l_TypeInfo.destroy = &Ability::destroy;
    l_TypeInfo.serialize = &Ability::serialize;
    l_TypeInfo.deserialize = &Ability::deserialize;
    l_TypeInfo.make_component = nullptr;
    l_TypeInfo.make_default = &Ability::_make;
    l_TypeInfo.duplicate_default = &Ability::_duplicate;
    l_TypeInfo.duplicate_component = nullptr;
    l_TypeInfo.get_living_instances =
        reinterpret_cast<Low::Util::RTTI::LivingInstancesGetter>(
            &Ability::living_instances);
    l_TypeInfo.get_living_count = &Ability::living_count;
    l_TypeInfo.component = false;
    l_TypeInfo.uiComponent = false;
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(title);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset = offsetof(AbilityData, title);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.get_title();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Ability, title,
                                          Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.set_title(*(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(description);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset = offsetof(AbilityData, description);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.get_description();
        return (void *)&ACCESSOR_TYPE_SOA(
            p_Handle, Ability, description, Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.set_description(*(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(execute_function_name);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset =
          offsetof(AbilityData, execute_function_name);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::STRING;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.get_execute_function_name();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Ability,
                                          execute_function_name,
                                          Low::Util::String);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.set_execute_function_name(
            *(Low::Util::String *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(resource_cost);
      l_PropertyInfo.editorProperty = true;
      l_PropertyInfo.dataOffset =
          offsetof(AbilityData, resource_cost);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::UINT32;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.get_resource_cost();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Ability,
                                          resource_cost, uint32_t);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.set_resource_cost(*(uint32_t *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    {
      Low::Util::RTTI::PropertyInfo l_PropertyInfo;
      l_PropertyInfo.name = N(name);
      l_PropertyInfo.editorProperty = false;
      l_PropertyInfo.dataOffset = offsetof(AbilityData, name);
      l_PropertyInfo.type = Low::Util::RTTI::PropertyType::NAME;
      l_PropertyInfo.get =
          [](Low::Util::Handle p_Handle) -> void const * {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.get_name();
        return (void *)&ACCESSOR_TYPE_SOA(p_Handle, Ability, name,
                                          Low::Util::Name);
      };
      l_PropertyInfo.set = [](Low::Util::Handle p_Handle,
                              const void *p_Data) -> void {
        Ability l_Handle = p_Handle.get_id();
        l_Handle.set_name(*(Low::Util::Name *)p_Data);
      };
      l_TypeInfo.properties[l_PropertyInfo.name] = l_PropertyInfo;
    }
    Low::Util::Handle::register_type_info(TYPE_ID, l_TypeInfo);
  }

  void Ability::cleanup()
  {
    Low::Util::List<Ability> l_Instances = ms_LivingInstances;
    for (uint32_t i = 0u; i < l_Instances.size(); ++i) {
      l_Instances[i].destroy();
    }
    free(ms_Buffer);
    free(ms_Slots);

    LOW_PROFILE_FREE(type_buffer_Ability);
    LOW_PROFILE_FREE(type_slots_Ability);
  }

  Ability Ability::find_by_index(uint32_t p_Index)
  {
    LOW_ASSERT(p_Index < get_capacity(), "Index out of bounds");

    Ability l_Handle;
    l_Handle.m_Data.m_Index = p_Index;
    l_Handle.m_Data.m_Generation = ms_Slots[p_Index].m_Generation;
    l_Handle.m_Data.m_Type = Ability::TYPE_ID;

    return l_Handle;
  }

  bool Ability::is_alive() const
  {
    return m_Data.m_Type == Ability::TYPE_ID &&
           check_alive(ms_Slots, Ability::get_capacity());
  }

  uint32_t Ability::get_capacity()
  {
    return ms_Capacity;
  }

  Ability Ability::find_by_name(Low::Util::Name p_Name)
  {
    for (auto it = ms_LivingInstances.begin();
         it != ms_LivingInstances.end(); ++it) {
      if (it->get_name() == p_Name) {
        return *it;
      }
    }
  }

  Ability Ability::duplicate(Low::Util::Name p_Name) const
  {
    _LOW_ASSERT(is_alive());

    Ability l_Handle = make(p_Name);
    l_Handle.set_title(get_title());
    l_Handle.set_description(get_description());
    l_Handle.set_execute_function_name(get_execute_function_name());
    l_Handle.set_resource_cost(get_resource_cost());

    // LOW_CODEGEN:BEGIN:CUSTOM:DUPLICATE
    // LOW_CODEGEN::END::CUSTOM:DUPLICATE

    return l_Handle;
  }

  Ability Ability::duplicate(Ability p_Handle, Low::Util::Name p_Name)
  {
    return p_Handle.duplicate(p_Name);
  }

  Low::Util::Handle Ability::_duplicate(Low::Util::Handle p_Handle,
                                        Low::Util::Name p_Name)
  {
    Ability l_Ability = p_Handle.get_id();
    return l_Ability.duplicate(p_Name);
  }

  void Ability::serialize(Low::Util::Yaml::Node &p_Node) const
  {
    _LOW_ASSERT(is_alive());

    p_Node["title"] = get_title().c_str();
    p_Node["description"] = get_description().c_str();
    p_Node["execute_function_name"] =
        get_execute_function_name().c_str();
    p_Node["resource_cost"] = get_resource_cost();
    p_Node["name"] = get_name().c_str();

    // LOW_CODEGEN:BEGIN:CUSTOM:SERIALIZER
    // LOW_CODEGEN::END::CUSTOM:SERIALIZER
  }

  void Ability::serialize(Low::Util::Handle p_Handle,
                          Low::Util::Yaml::Node &p_Node)
  {
    Ability l_Ability = p_Handle.get_id();
    l_Ability.serialize(p_Node);
  }

  Low::Util::Handle
  Ability::deserialize(Low::Util::Yaml::Node &p_Node,
                       Low::Util::Handle p_Creator)
  {
    Ability l_Handle = Ability::make(N(Ability));

    if (p_Node["title"]) {
      l_Handle.set_title(LOW_YAML_AS_STRING(p_Node["title"]));
    }
    if (p_Node["description"]) {
      l_Handle.set_description(
          LOW_YAML_AS_STRING(p_Node["description"]));
    }
    if (p_Node["execute_function_name"]) {
      l_Handle.set_execute_function_name(
          LOW_YAML_AS_STRING(p_Node["execute_function_name"]));
    }
    if (p_Node["resource_cost"]) {
      l_Handle.set_resource_cost(
          p_Node["resource_cost"].as<uint32_t>());
    }
    if (p_Node["name"]) {
      l_Handle.set_name(LOW_YAML_AS_NAME(p_Node["name"]));
    }

    // LOW_CODEGEN:BEGIN:CUSTOM:DESERIALIZER
    // LOW_CODEGEN::END::CUSTOM:DESERIALIZER

    return l_Handle;
  }

  Low::Util::String &Ability::get_title() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_title
    // LOW_CODEGEN::END::CUSTOM:GETTER_title

    return TYPE_SOA(Ability, title, Low::Util::String);
  }
  void Ability::set_title(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_title
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_title

    // Set new value
    TYPE_SOA(Ability, title, Low::Util::String) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_title
    // LOW_CODEGEN::END::CUSTOM:SETTER_title
  }

  Low::Util::String &Ability::get_description() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_description
    // LOW_CODEGEN::END::CUSTOM:GETTER_description

    return TYPE_SOA(Ability, description, Low::Util::String);
  }
  void Ability::set_description(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_description
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_description

    // Set new value
    TYPE_SOA(Ability, description, Low::Util::String) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_description
    // LOW_CODEGEN::END::CUSTOM:SETTER_description
  }

  Low::Util::String &Ability::get_execute_function_name() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_execute_function_name
    // LOW_CODEGEN::END::CUSTOM:GETTER_execute_function_name

    return TYPE_SOA(Ability, execute_function_name,
                    Low::Util::String);
  }
  void Ability::set_execute_function_name(Low::Util::String &p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_execute_function_name
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_execute_function_name

    // Set new value
    TYPE_SOA(Ability, execute_function_name, Low::Util::String) =
        p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_execute_function_name
    // LOW_CODEGEN::END::CUSTOM:SETTER_execute_function_name
  }

  uint32_t Ability::get_resource_cost() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_resource_cost
    // LOW_CODEGEN::END::CUSTOM:GETTER_resource_cost

    return TYPE_SOA(Ability, resource_cost, uint32_t);
  }
  void Ability::set_resource_cost(uint32_t p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_resource_cost
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_resource_cost

    // Set new value
    TYPE_SOA(Ability, resource_cost, uint32_t) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_resource_cost
    // LOW_CODEGEN::END::CUSTOM:SETTER_resource_cost
  }

  Low::Util::Name Ability::get_name() const
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:GETTER_name
    // LOW_CODEGEN::END::CUSTOM:GETTER_name

    return TYPE_SOA(Ability, name, Low::Util::Name);
  }
  void Ability::set_name(Low::Util::Name p_Value)
  {
    _LOW_ASSERT(is_alive());

    // LOW_CODEGEN:BEGIN:CUSTOM:PRESETTER_name
    // LOW_CODEGEN::END::CUSTOM:PRESETTER_name

    // Set new value
    TYPE_SOA(Ability, name, Low::Util::Name) = p_Value;

    // LOW_CODEGEN:BEGIN:CUSTOM:SETTER_name
    // LOW_CODEGEN::END::CUSTOM:SETTER_name
  }

  Low::Core::UI::View Ability::spawn_card()
  {
    // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_spawn_card
    _LOW_ASSERT(is_alive());

    Low::Core::UI::View l_TemplateView =
        Low::Core::UI::View::find_by_name(N(CardTemplateView));

    Low::Core::UI::View l_AbilityView =
        l_TemplateView.spawn_instance(get_name());

    Low::Core::UI::Component::Text l_TitleText =
        l_AbilityView.find_element_by_name(N(cardtitle))
            .get_component(Low::Core::UI::Component::Text::TYPE_ID);
    l_TitleText.set_text(get_title());

    Low::Core::UI::Component::Text l_DescText =
        l_AbilityView.find_element_by_name(N(carddesc))
            .get_component(Low::Core::UI::Component::Text::TYPE_ID);
    l_DescText.set_text(get_description());

    Low::Core::UI::Component::Text l_ResourceText =
        l_AbilityView.find_element_by_name(N(resourcetxt))
            .get_component(Low::Core::UI::Component::Text::TYPE_ID);
    Low::Util::String l_ResourceCost =
        std::to_string(get_resource_cost()).c_str();
    l_ResourceText.set_text(l_ResourceCost);

    return l_AbilityView;
    // LOW_CODEGEN::END::CUSTOM:FUNCTION_spawn_card
  }

  void Ability::execute(Mtd::Component::Fighter p_Caster,
                        Mtd::Component::Fighter p_Target)
  {
    // LOW_CODEGEN:BEGIN:CUSTOM:FUNCTION_execute
    _LOW_ASSERT(is_alive());

    if (get_execute_function_name().empty()) {
      LOW_LOG_WARN << "Trying to execute ability " << get_name()
                   << " but execute function is empty."
                   << LOW_LOG_END;
      return;
    }

    Cflat::Function *l_Function =
        Low::Core::Scripting::get_environment()->getFunction(
            get_execute_function_name().c_str());

    LOW_ASSERT(l_Function, "Could not find ability execute function");

    static const Cflat::TypeUsage kTypeUsageFighter =
        Low::Core::Scripting::get_environment()->getTypeUsage(
            "Mtd::Component::Fighter");

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

    l_Function->execute(l_Arguments, &l_ReturnValue);
    // LOW_CODEGEN::END::CUSTOM:FUNCTION_execute
  }

  uint32_t Ability::create_instance()
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

  void Ability::increase_budget()
  {
    uint32_t l_Capacity = get_capacity();
    uint32_t l_CapacityIncrease =
        std::max(std::min(l_Capacity, 64u), 1u);
    l_CapacityIncrease =
        std::min(l_CapacityIncrease, LOW_UINT32_MAX - l_Capacity);

    LOW_ASSERT(l_CapacityIncrease > 0, "Could not increase capacity");

    uint8_t *l_NewBuffer = (uint8_t *)malloc(
        (l_Capacity + l_CapacityIncrease) * sizeof(AbilityData));
    Low::Util::Instances::Slot *l_NewSlots =
        (Low::Util::Instances::Slot *)malloc(
            (l_Capacity + l_CapacityIncrease) *
            sizeof(Low::Util::Instances::Slot));

    memcpy(l_NewSlots, ms_Slots,
           l_Capacity * sizeof(Low::Util::Instances::Slot));
    {
      memcpy(&l_NewBuffer[offsetof(AbilityData, title) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(AbilityData, title) * (l_Capacity)],
             l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(&l_NewBuffer[offsetof(AbilityData, description) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(AbilityData, description) *
                        (l_Capacity)],
             l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(
          &l_NewBuffer[offsetof(AbilityData, execute_function_name) *
                       (l_Capacity + l_CapacityIncrease)],
          &ms_Buffer[offsetof(AbilityData, execute_function_name) *
                     (l_Capacity)],
          l_Capacity * sizeof(Low::Util::String));
    }
    {
      memcpy(&l_NewBuffer[offsetof(AbilityData, resource_cost) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(AbilityData, resource_cost) *
                        (l_Capacity)],
             l_Capacity * sizeof(uint32_t));
    }
    {
      memcpy(&l_NewBuffer[offsetof(AbilityData, name) *
                          (l_Capacity + l_CapacityIncrease)],
             &ms_Buffer[offsetof(AbilityData, name) * (l_Capacity)],
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

    LOW_LOG_DEBUG << "Auto-increased budget for Ability from "
                  << l_Capacity << " to "
                  << (l_Capacity + l_CapacityIncrease) << LOW_LOG_END;
  }

  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_TYPE_CODE
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_TYPE_CODE

} // namespace Mtd