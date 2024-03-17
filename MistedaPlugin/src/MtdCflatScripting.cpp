#include "MtdCflatScripting.h"

#include "LowCoreCflatScripting.h"

#include "LowUtilContainers.h"
#include "MtdCombat.h"

#include "CflatGlobal.h"
#include "Cflat.h"
#include "CflatHelper.h"

// REGISTER_CFLAT_INCLUDES_BEGIN
#include "MtdAbility.h"
#include "MtdStatusEffect.h"
#include "MtdFighter.h"
// REGISTER_CFLAT_INCLUDES_END

namespace Mtd {
  namespace Scripting {
    Low::Util::Map<Low::Util::String, Cflat::Struct *> g_CflatStructs;

    void setup();

    void initialize()
    {
      setup();
    }

    void cleanup()
    {
    }

    // REGISTER_CFLAT_BEGIN
    static void register_mistedaplugin_ability()
    {
      using namespace Low;
      using namespace Low::Core;
      using namespace ::Mtd;

      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd");

      Cflat::Struct *type = Scripting::g_CflatStructs["Mtd::Ability"];

      {
        Cflat::Namespace *l_UtilNamespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Low::Util");

        CflatRegisterSTLVectorCustom(
            Low::Core::Scripting::get_environment(), Low::Util::List,
            Mtd::Ability);
      }

      CflatStructAddConstructorParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          uint64_t);
      CflatStructAddStaticMember(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          uint16_t, TYPE_ID);
      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability, bool,
          is_alive);
      CflatStructAddMethodVoid(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          destroy);
      CflatStructAddStaticMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          uint32_t, get_capacity);
      CflatStructAddStaticMethodReturnParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Mtd::Ability, find_by_index, uint32_t);
      CflatStructAddStaticMethodReturnParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Mtd::Ability, find_by_name, Low::Util::Name);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Low::Util::String &, get_title);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          set_title, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Low::Util::String &, get_description);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          set_description, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Low::Util::String &, get_execute_function_name);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          set_execute_function_name, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          uint32_t, get_resource_cost);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          set_resource_cost, uint32_t);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Low::Util::Name, get_name);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          set_name, Low::Util::Name);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::Ability,
          Low::Core::UI::View, spawn_card);
      CflatStructAddMethodVoidParams2(
          Low::Core::Scripting::get_environment(), Mtd::Ability, void,
          execute, Mtd::Component::Fighter, Mtd::Component::Fighter);
    }

    static void register_mistedaplugin_statuseffect()
    {
      using namespace Low;
      using namespace Low::Core;
      using namespace ::Mtd;

      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd");

      Cflat::Struct *type =
          Scripting::g_CflatStructs["Mtd::StatusEffect"];

      {
        Cflat::Namespace *l_UtilNamespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Low::Util");

        CflatRegisterSTLVectorCustom(
            Low::Core::Scripting::get_environment(), Low::Util::List,
            Mtd::StatusEffect);
      }

      CflatStructAddConstructorParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          uint64_t);
      CflatStructAddStaticMember(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          uint16_t, TYPE_ID);
      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          bool, is_alive);
      CflatStructAddMethodVoid(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, destroy);
      CflatStructAddStaticMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          uint32_t, get_capacity);
      CflatStructAddStaticMethodReturnParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Mtd::StatusEffect, find_by_index, uint32_t);
      CflatStructAddStaticMethodReturnParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Mtd::StatusEffect, find_by_name, Low::Util::Name);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Low::Util::String &, get_title);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, set_title, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Low::Util::String &, get_description);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, set_description, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Low::Util::String &, get_start_turn_function);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, set_start_turn_function, Low::Util::String &);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          Low::Util::Name, get_name);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, set_name, Low::Util::Name);

      CflatStructAddMethodVoidParams3(
          Low::Core::Scripting::get_environment(), Mtd::StatusEffect,
          void, execute, Mtd::Component::Fighter,
          Mtd::Component::Fighter, int);
    }

    static void register_mistedaplugin_fighter()
    {
      using namespace Low;
      using namespace Low::Core;
      using namespace ::Mtd::Component;

      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd::Component");

      Cflat::Struct *type =
          Scripting::g_CflatStructs["Mtd::Component::Fighter"];

      {
        Cflat::Namespace *l_UtilNamespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Low::Util");

        CflatRegisterSTLVectorCustom(
            Low::Core::Scripting::get_environment(), Low::Util::List,
            Mtd::Component::Fighter);
      }

      CflatStructAddConstructorParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, uint64_t);
      CflatStructAddStaticMember(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, uint16_t, TYPE_ID);
      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, bool, is_alive);
      CflatStructAddMethodVoid(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, destroy);
      CflatStructAddStaticMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, uint32_t, get_capacity);
      CflatStructAddStaticMethodReturnParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, Mtd::Component::Fighter,
          find_by_index, uint32_t);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, uint32_t, get_mana);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, set_mana, uint32_t);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, int, get_hp);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, set_hp, int);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter,
          Low::Util::List<Mtd::StatusEffectInstance> &,
          get_status_effects);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, Low::Core::Entity, get_entity);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, set_entity,
          Low::Core::Entity);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, Mtd::Ability, draw);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, deal_damage, uint32_t);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, heal, uint32_t);
      CflatStructAddMethodVoidParams3(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, apply_status_effect,
          Mtd::StatusEffect, int, Mtd::Component::Fighter);
    }

    static void preregister_types()
    {
      using namespace Low::Core;

      {
        using namespace Mtd;
        Cflat::Namespace *l_Namespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Mtd");

        CflatRegisterStruct(l_Namespace, Ability);
        CflatStructAddBaseType(
            Low::Core::Scripting::get_environment(), Mtd::Ability,
            Low::Util::Handle);

        Scripting::g_CflatStructs["Mtd::Ability"] = type;
      }

      {
        using namespace Mtd;
        Cflat::Namespace *l_Namespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Mtd");

        CflatRegisterStruct(l_Namespace, StatusEffect);
        CflatStructAddBaseType(
            Low::Core::Scripting::get_environment(),
            Mtd::StatusEffect, Low::Util::Handle);

        Scripting::g_CflatStructs["Mtd::StatusEffect"] = type;
      }

      {
        using namespace Mtd::Component;
        Cflat::Namespace *l_Namespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Mtd::Component");

        CflatRegisterStruct(l_Namespace, Fighter);
        CflatStructAddBaseType(
            Low::Core::Scripting::get_environment(),
            Mtd::Component::Fighter, Low::Util::Handle);

        Scripting::g_CflatStructs["Mtd::Component::Fighter"] = type;
      }
    }
    static void register_types()
    {
      register_mistedaplugin_ability();
      register_mistedaplugin_statuseffect();
      register_mistedaplugin_fighter();
    }
    // REGISTER_CFLAT_END

    static void register_statuseffect_instance()
    {
      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd");

      {
        CflatRegisterStruct(l_Namespace, StatusEffectInstance);
        CflatStructAddMember(l_Namespace, StatusEffectInstance,
                             Low::Core::UI::View, view);
        CflatStructAddMember(l_Namespace, StatusEffectInstance, int,
                             remainingRounds);
        CflatStructAddMember(l_Namespace, StatusEffectInstance,
                             Low::Util::Handle, caster);
        CflatStructAddMember(l_Namespace, StatusEffectInstance,
                             StatusEffect, statusEffect);

        CflatRegisterSTLVectorCustom(
            Low::Core::Scripting::get_environment(), Low::Util::List,
            Mtd::StatusEffectInstance);
      }
    }

    static void register_combat()
    {
      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd");
      {
        CflatRegisterEnumClass(l_Namespace, CombatState);
        CflatEnumClassAddValue(l_Namespace, CombatState,
                               FIGHTER_1_TURN);
        CflatEnumClassAddValue(l_Namespace, CombatState,
                               FIGHTER_2_TURN);
      }

      {
        CflatRegisterEnumClass(l_Namespace, DamageType);
        CflatEnumClassAddValue(l_Namespace, DamageType, PHYSICAL);
        CflatEnumClassAddValue(l_Namespace, DamageType, SHADOW);
        CflatEnumClassAddValue(l_Namespace, DamageType, FIRE);
        CflatEnumClassAddValue(l_Namespace, DamageType, POISON);
        CflatEnumClassAddValue(l_Namespace, DamageType, ARCANE);
      }

      {
        CflatRegisterStruct(l_Namespace, CombatDelay);
        CflatStructAddMember(l_Namespace, CombatDelay, float,
                             remaining);
        CflatStructAddMember(l_Namespace, CombatDelay, CombatState,
                             nextState);
      }
    }

    static void register_hand_ability()
    {
      Cflat::Namespace *l_Namespace =
          Low::Core::Scripting::get_environment()->requestNamespace(
              "Mtd");
      CflatRegisterStruct(l_Namespace, HandAbility);
      CflatStructAddMember(Low::Core::Scripting::get_environment(),
                           Mtd::HandAbility, Mtd::Ability, ability);
      CflatStructAddMember(Low::Core::Scripting::get_environment(),
                           Mtd::HandAbility, Low::Core::UI::View,
                           view);

      CflatRegisterSTLVectorCustom(
          Low::Core::Scripting::get_environment(), Low::Util::List,
          Mtd::HandAbility);
    }

    static void setup()
    {
      preregister_types();
      register_statuseffect_instance();
      register_types();
      register_combat();
      register_hand_ability();
    }
  } // namespace Scripting
} // namespace Mtd
