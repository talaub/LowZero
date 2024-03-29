#include "MtdPlugin.h"

#include "MtdFighter.h"
#include "MtdCameraController.h"
#include "MtdTestSystem.h"
#include "MtdAbility.h"
#include "MtdCflatScripting.h"
#include "MtdStatusEffect.h"
#include "MtdAbilityType.h"

#include "LowCoreGameLoop.h"

#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Mtd {
  static void tick(float p_Delta, Low::Util::EngineState p_State)
  {
    System::Test::tick(p_Delta, p_State);
  }

  static void initialize_component_types()
  {
    Component::CameraController::initialize();
    Component::Fighter::initialize();
  }

  static void initialize_enums()
  {
    Mtd::AbilityTypeEnumHelper::initialize();
  }

  static void initialize_base_types()
  {
    Ability::initialize();
    StatusEffect::initialize();
  }

  static void initialize_types()
  {
    initialize_base_types();
    initialize_component_types();
  }

  static void load_abilities()
  {
    using namespace Low;
    Util::String l_Path =
        Util::String(LOW_DATA_PATH) + "\\assets\\ability";

    Util::List<Util::String> l_FilePaths;

    Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
    Util::String l_Ending = ".ability.yaml";

    for (Util::String &i_Path : l_FilePaths) {
      if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
        Util::Yaml::Node i_Node =
            Util::Yaml::load_file(i_Path.c_str());
        Ability::deserialize(i_Node, 0);
      }
    }
  }

  static void load_statuseffects()
  {
    using namespace Low;
    Util::String l_Path =
        Util::String(LOW_DATA_PATH) + "\\assets\\statuseffect";

    Util::List<Util::String> l_FilePaths;

    Util::FileIO::list_directory(l_Path.c_str(), l_FilePaths);
    Util::String l_Ending = ".statuseffect.yaml";

    for (Util::String &i_Path : l_FilePaths) {
      if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
        Util::Yaml::Node i_Node =
            Util::Yaml::load_file(i_Path.c_str());
        StatusEffect::deserialize(i_Node, 0);
      }
    }
  }

  static void load_assets()
  {
    load_abilities();
    load_statuseffects();
  }

  void initialize()
  {
    initialize_enums();
    initialize_types();

    Scripting::initialize();

    Low::Core::GameLoop::register_tick_callback(&tick);

    load_assets();
  }

  static void cleanup_component_types()
  {
    Component::Fighter::cleanup();
    Component::CameraController::cleanup();
  }

  static void cleanup_base_types()
  {
    Ability::cleanup();
    StatusEffect::cleanup();
  }

  static void cleanup_types()
  {
    cleanup_component_types();
    cleanup_base_types();
  }

  static void cleanup_enums()
  {
    Mtd::AbilityTypeEnumHelper::cleanup();
  }

  void cleanup()
  {
    cleanup_types();
    cleanup_enums();
  }
} // namespace Mtd

extern "C" int __declspec(dllexport) __stdcall plugin_initialize()
{
  Mtd::initialize();
  return 0;
}

extern "C" int __declspec(dllexport) __stdcall plugin_cleanup()
{
  Mtd::cleanup();
  return 0;
}
