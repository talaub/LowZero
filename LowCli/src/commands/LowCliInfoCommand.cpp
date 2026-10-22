#include "LowCliCommands.h"

#include "LowCliOutput.h"

#include "LowUtil.h"

namespace Low {
  namespace Cli {
    namespace Commands {
      void register_info(CLI::App &p_App)
      {
        CLI::App *l_Command = p_App.add_subcommand(
            "info", "Show which engine this low belongs to");

        add_command(l_Command, [](Context &p_Context) {
          if (!p_Context.require_engine_root()) {
            return 1;
          }

          Output::result("executable: " + Util::get_executable_directory());
          Output::result("engine:     " + p_Context.engineRoot);
          Output::result("project:    " + (p_Context.projectRoot.empty()
                                               ? Util::String("-")
                                               : p_Context.projectRoot));
          return 0;
        });
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
