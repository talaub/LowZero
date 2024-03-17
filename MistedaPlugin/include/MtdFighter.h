#pragma once

#include "MtdPluginApi.h"

#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"

#include "LowCoreEntity.h"

#include "MtdAbility.h"

// LOW_CODEGEN:BEGIN:CUSTOM:HEADER_CODE
#include "MtdStatusEffect.h"
// LOW_CODEGEN::END::CUSTOM:HEADER_CODE

namespace Mtd {
  namespace Component {
    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_CODE

    struct MISTEDA_API FighterData
    {
      Low::Util::List<Mtd::Ability> deck;
      uint32_t mana;
      int hp;
      Low::Util::List<Mtd::StatusEffectInstance> status_effects;
      Low::Core::Entity entity;
      Low::Util::UniqueId unique_id;

      static size_t get_size()
      {
        return sizeof(FighterData);
      }
    };

    struct MISTEDA_API Fighter : public Low::Util::Handle
    {
    public:
      static uint8_t *ms_Buffer;
      static Low::Util::Instances::Slot *ms_Slots;

      static Low::Util::List<Fighter> ms_LivingInstances;

      const static uint16_t TYPE_ID;

      Fighter();
      Fighter(uint64_t p_Id);
      Fighter(Fighter &p_Copy);

      static Fighter make(Low::Core::Entity p_Entity);
      static Low::Util::Handle _make(Low::Util::Handle p_Entity);
      explicit Fighter(const Fighter &p_Copy)
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
      static Fighter *living_instances()
      {
        return ms_LivingInstances.data();
      }

      static Fighter find_by_index(uint32_t p_Index);

      bool is_alive() const;

      static uint32_t get_capacity();

      void serialize(Low::Util::Yaml::Node &p_Node) const;

      Fighter duplicate(Low::Core::Entity p_Entity) const;
      static Fighter duplicate(Fighter p_Handle,
                               Low::Core::Entity p_Entity);
      static Low::Util::Handle _duplicate(Low::Util::Handle p_Handle,
                                          Low::Util::Handle p_Entity);

      static void serialize(Low::Util::Handle p_Handle,
                            Low::Util::Yaml::Node &p_Node);
      static Low::Util::Handle
      deserialize(Low::Util::Yaml::Node &p_Node,
                  Low::Util::Handle p_Creator);
      static bool is_alive(Low::Util::Handle p_Handle)
      {
        return p_Handle.get_type() == Fighter::TYPE_ID &&
               p_Handle.check_alive(ms_Slots, get_capacity());
      }

      static void destroy(Low::Util::Handle p_Handle)
      {
        _LOW_ASSERT(is_alive(p_Handle));
        Fighter l_Fighter = p_Handle.get_id();
        l_Fighter.destroy();
      }

      Low::Util::List<Mtd::Ability> &get_deck() const;
      void set_deck(Low::Util::List<Mtd::Ability> &p_Value);

      uint32_t get_mana() const;
      void set_mana(uint32_t p_Value);

      int get_hp() const;
      void set_hp(int p_Value);

      Low::Util::List<Mtd::StatusEffectInstance> &
      get_status_effects() const;

      Low::Core::Entity get_entity() const;
      void set_entity(Low::Core::Entity p_Value);

      Low::Util::UniqueId get_unique_id() const;

      Mtd::Ability draw();
      void deal_damage(uint32_t p_Amount);
      void heal(uint32_t p_Amount);
      void apply_status_effect(Mtd::StatusEffect p_StatusEffect,
                               int p_Duration,
                               Mtd::Component::Fighter p_Caster);

    private:
      static uint32_t ms_Capacity;
      static uint32_t create_instance();
      static void increase_budget();
      void set_status_effects(
          Low::Util::List<Mtd::StatusEffectInstance> &p_Value);
      void set_unique_id(Low::Util::UniqueId p_Value);
    };

    // LOW_CODEGEN:BEGIN:CUSTOM:NAMESPACE_AFTER_STRUCT_CODE
    // LOW_CODEGEN::END::CUSTOM:NAMESPACE_AFTER_STRUCT_CODE

  } // namespace Component
} // namespace Mtd
