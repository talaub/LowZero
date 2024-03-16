#include "MtdCflatScripting.h"

#include "LowCoreCflatScripting.h"

#include "LowUtilContainers.h"
#include "MtdCombat.h"

#include "CflatGlobal.h"
#include "Cflat.h"
#include "CflatHelper.h"

// REGISTER_CFLAT_INCLUDES_BEGIN
#include "MtdAbility.h"
#include "MtdFighter.h"
// REGISTER_CFLAT_INCLUDES_END

namespace Mtd {
  namespace Scripting {
    Low::Util::Map<Low::Util::String, Cflat::Struct *> g_CflatStructs;

    void register_types();

    static void register_combat()
    {
      {
        Cflat::Namespace *l_Namespace =
            Low::Core::Scripting::get_environment()->requestNamespace(
                "Mtd");
        CflatRegisterEnumClass(l_Namespace, CombatState);
        CflatEnumClassAddValue(l_Namespace, CombatState,
                               FIGHTER_1_TURN);
        CflatEnumClassAddValue(l_Namespace, CombatState,
                               FIGHTER_2_TURN);
      }
    }

    void initialize()
    {
      register_combat();
      register_types();
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

        CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
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

        CflatRegisterSTLVectorCustom(l_UtilNamespace, Low::Util::List,
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
          Mtd::Component::Fighter, Low::Core::Entity, get_entity);
      CflatStructAddMethodVoidParams1(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, void, set_entity,
          Low::Core::Entity);

      CflatStructAddMethodReturn(
          Low::Core::Scripting::get_environment(),
          Mtd::Component::Fighter, Mtd::Ability, draw);
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
      preregister_types();

      register_mistedaplugin_ability();
      register_mistedaplugin_fighter();
    }
    // REGISTER_CFLAT_END
  } // namespace Scripting
} // namespace Mtd
