#include "LowCoreGameplaySystemScripting.h"
#include "LowCoreScriptClass.h"
#include "LowCoreScripting.h"
#include "LowUtilLogger.h"
#include "LowUtilYaml.h"
#include "angelscript.h"

#include "LowUtilAssert.h"
#include "LowCoreGameplaySystem.h"

namespace Low {
  namespace Core {
    namespace GameplaySystemScripting {
      void register_gameplay_system(asIScriptEngine *p_Engine)
      {
        int r = 0;

        r = p_Engine->RegisterInterface("GameplaySystem");
        LOW_ASSERT(r >= 0,
                   "Failed to register interface GameplaySystem");

        r = p_Engine->RegisterInterfaceMethod(
            "GameplaySystem", "void tick(float p_Delta)");
        LOW_ASSERT(r >= 0, "Failed to register GameplaySystem::tick");
        r = p_Engine->RegisterInterfaceMethod("GameplaySystem",
                                              "void mount()");
        LOW_ASSERT(r >= 0,
                   "Failed to register GameplaySystem::mount");

        r = p_Engine->RegisterInterfaceMethod("GameplaySystem",
                                              "void unmount()");
        LOW_ASSERT(r >= 0,
                   "Failed to register GameplaySystem::unmount");

        r = p_Engine->RegisterInterfaceMethod("GameplaySystem",
                                              "void resume()");
        LOW_ASSERT(r >= 0,
                   "Failed to register GameplaySystem::resume");

        r = p_Engine->RegisterInterfaceMethod("GameplaySystem",
                                              "void suspend()");
        LOW_ASSERT(r >= 0,
                   "Failed to register GameplaySystem::suspend");
      }

      void handle_class(ScriptClass p_Class, asITypeInfo *p_Type)
      {
        GameplaySystem l_System =
            GameplaySystem::find_by_scriptclass(p_Class);
        if (!l_System.is_alive()) {
          l_System = GameplaySystem::make_script(p_Class.get_name(),
                                                 p_Class);
        }
        for (const Util::Serial::Node &i_Annotation :
             p_Class.get_annotations()) {
          if (i_Annotation.is_dict()) {
            if (i_Annotation["DisplayName"]) {
              l_System.set_name(
                  i_Annotation["DisplayName"].as<Util::Name>());
            }
          }
        }
        l_System.get_exposed_variables().clear();

        for (const auto &[i_Name, i_Field] : p_Class.get_members()) {
          if (i_Field.has_annotation("GameModeExposed")) {
            l_System.get_exposed_variables()[i_Name] =
                i_Field.property_type;
          }
        }
      }

      void initialize()
      {
        Scripting::ClassInterfaceDefinition l_Definition;
        l_Definition.interface_name = N(GameplaySystem);
        l_Definition.register_types = &register_gameplay_system;
        l_Definition.class_handler = &handle_class;

        Scripting::register_class_interface_definition(l_Definition);
      }
    } // namespace GameplaySystemScripting
  } // namespace Core
} // namespace Low
