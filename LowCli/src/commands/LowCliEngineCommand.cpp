#include "LowCliCommands.h"

#include "LowCliProcess.h"
#include "LowCliOutput.h"
#include "LowCliEngineRegistry.h"

#include <ctype.h>
#include <string>

namespace Low {
  namespace Cli {
    namespace Commands {
      static struct EngineRegisterArgs
      {
        std::string id;
        bool force = false;
      } g_RegisterArgs;

      static std::string validate_id(std::string &p_Id)
      {
        for (const char i_Char : p_Id) {
          if (!isalnum(static_cast<unsigned char>(i_Char)) &&
              i_Char != '.' && i_Char != '_' && i_Char != '-') {
            return "may only contain letters, digits, '.', '_' and '-'";
          }
        }
        if (p_Id.empty()) {
          return "must not be empty";
        }
        // project.yaml uses engine: bundled for the engine inside the
        // project
        if (p_Id == "bundled") {
          return "'bundled' is reserved for engines inside a project";
        }
        return "";
      }

      static bool write_entry(const Util::String &p_Id,
                              const Util::String &p_Path)
      {
        if (!EngineRegistry::force_set(p_Id, p_Path)) {
          Output::error("Could not write the engine registry",
                        {EngineRegistry::get_registry_path()},
                        "check that the folder is writable");
          return false;
        }
        Output::success("Engine registered.");
        return true;
      }

      static int engine_register(Context &p_Context)
      {
        if (!p_Context.require_engine_root()) {
          return 1;
        }
        if (!EngineRegistry::initialize()) {
          if (EngineRegistry::get_file_version() >
              EngineRegistry::FORMAT_VERSION) {
            Output::error(
                "The engine registry was written by a newer engine",
                {EngineRegistry::get_registry_path(),
                 "format version " +
                     Util::String(std::to_string(
                                      EngineRegistry::get_file_version())
                                      .c_str())},
                "register this engine with the low of a newer engine");
            return 1;
          }
          Output::error("Could not read the engine registry",
                        {EngineRegistry::get_registry_path()},
                        "fix or delete the file and register again");
          return 1;
        }

        const Util::String l_Id = g_RegisterArgs.id.c_str();

        Output::section("Register engine install");
        Output::detail(p_Context.engineRoot);
        Output::detail(Util::String("as ") + l_Id);

        if (EngineRegistry::has_key(l_Id)) {
          Output::debug("ID already exists");
          const Util::String l_Existing =
              EngineRegistry::get_key(l_Id);
          Output::debug("ID already registed for: " + l_Existing);
          if (!EngineRegistry::paths_equal(l_Existing,
                                           p_Context.engineRoot)) {
            Output::debug("This ID has already been registered "
                          "referencing a different path.");
            if (g_RegisterArgs.force) {
              Output::debug("Force is enabled so we override what's "
                            "already there.");
              if (!write_entry(l_Id, p_Context.engineRoot)) {
                return 1;
              }
            } else {
              Output::error("This ID has already been registered for "
                            "a different engine installation.",
                            {l_Existing},
                            "Choose a different ID or use --force to "
                            "override.");
              return 1;
            }
          } else {
            Output::debug("Paths are matching, nothing to do.");
            Output::info(
                "This path is already registered under the same ID.");
          }
        } else {
          Output::debug("ID does not yet exist in registry.");
          if (EngineRegistry::has_path(p_Context.engineRoot)) {
            const Util::String l_ExistingId =
                EngineRegistry::get_id_for_path(p_Context.engineRoot);
            Output::debug("Same path is already registered under a "
                          "different ID.");
            if (g_RegisterArgs.force) {
              Output::debug("Force is enabled, adding the entry.");
              if (!write_entry(l_Id, p_Context.engineRoot)) {
                return 1;
              }
            } else {
              Output::error("This path has already been registered "
                            "under a different ID.",
                            {l_ExistingId},
                            "You can force a second entry under a new "
                            "ID using --force.");
              return 1;
            }

          } else {
            if (!write_entry(l_Id, p_Context.engineRoot)) {
              return 1;
            }
          }
        }

        return 0;
      }

      void register_engine(CLI::App &p_App)
      {
        CLI::App *l_Engine = p_App.add_subcommand(
            "engine",
            "Manage the engines registered on this machine");
        l_Engine->require_subcommand(1);

        CLI::App *l_Register = l_Engine->add_subcommand(
            "register", "Register this engine under an id");
        l_Register
            ->add_option(
                "id", g_RegisterArgs.id,
                "Id that projects use to refer to this engine")
            ->required()
            ->check(CLI::Validator(validate_id, "ID"));
        l_Register->add_flag(
            "--force", g_RegisterArgs.force,
            "Overwrite the id if it points to another engine");

        add_command(l_Register, engine_register);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
