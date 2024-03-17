#pragma once

#include "MtdPluginApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "LowCoreUiView.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Mtd {
  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
  namespace Component {
    struct Fighter;
  }
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

  struct MISTEDA_API StatusEffectData
  {
    Low::Util::String title;
    Low::Util::String description;
    Low::Util::String start_turn_function;
    Low::Util::Name name;

    static size_t get_size()
    {
      return sizeof(StatusEffectData);
    }
  };

  struct MISTEDA_API StatusEffect : public Low::Util::Handle
  {
  public:
    static uint8_t *ms_Buffer;
    static Low::Util::Instances::Slot *ms_Slots;

    static Low::Util::List<StatusEffect> ms_LivingInstances;

    const static uint16_t TYPE_ID;

    StatusEffect();
    StatusEffect(uint64_t p_Id);
    StatusEffect(StatusEffect &p_Copy);

    static StatusEffect make(Low::Util::Name p_Name);
    static Low::Util::Handle _make(Low::Util::Name p_Name);
    explicit StatusEffect(const StatusEffect &p_Copy)
        : Low::Util::Handle(p_Copy.m_Id)
    {
    }

    void destroy();

    static void initialize();
    static void cleanup();

    static uint32_t living_count()
    {
      return static_cast<uint32_t>(ms_LivingInstances.size());
    }
    static StatusEffect *living_instances()
    {
      return ms_LivingInstances.data();
    }

    static StatusEffect find_by_index(uint32_t p_Index);

    bool is_alive() const;

    static uint32_t get_capacity();

    void serialize(Low::Util::Yaml::Node &p_Node) const;

    StatusEffect duplicate(Low::Util::Name p_Name) const;
    static StatusEffect duplicate(StatusEffect p_Handle,
                                  Low::Util::Name p_Name);
    static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                        Low::Util::Name p_Name);

    static StatusEffect find_by_name(Low::Util::Name p_Name);

    static void serialize(Low::Util::Handle p_Handle,
                          Low::Util::Yaml::Node &p_Node);
    static Low::Util::Handle
    deserialize(Low::Util::Yaml::Node &p_Node,
                Low::Util::Handle p_Creator);
    static bool is_alive(Low::Util::Handle p_Handle)
    {
      return p_Handle.get_type() == StatusEffect::TYPE_ID &&
             p_Handle.check_alive(ms_Slots, get_capacity());
    }

    static void destroy(Low::Util::Handle p_Handle)
    {
      _LOW_ASSERT(is_alive(p_Handle));
      StatusEffect l_StatusEffect = p_Handle.get_id();
      l_StatusEffect.destroy();
    }

    Low::Util::String &get_title() const;
    void set_title(Low::Util::String &p_Value);

    Low::Util::String &get_description() const;
    void set_description(Low::Util::String &p_Value);

    Low::Util::String &get_start_turn_function() const;
    void set_start_turn_function(Low::Util::String &p_Value);

    Low::Util::Name get_name() const;
    void set_name(Low::Util::Name p_Value);

    void execute(Mtd::Component::Fighter p_Caster,
                 Mtd::Component::Fighter p_Target, int p_Remaining);

  private:
    static uint32_t ms_Capacity;
    static uint32_t create_instance();
    static void increase_budget();
  };

  // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
  struct StatusEffectInstance
  {
    StatusEffect statusEffect;
    Low::Core::UI::View view;
    int remainingRounds;
    Low::Util::Handle caster;
  };
  // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

} // namespace Mtd
