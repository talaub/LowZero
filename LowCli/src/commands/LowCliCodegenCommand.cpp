#include "LowCliCommands.h"

#include "LowCliEngineRegistry.h"
#include "LowCliOutput.h"

namespace Low {
  namespace Cli {
    namespace Commands {
      static bool is_inside_engine(const Context &p_Context)
      {
        if (p_Context.engineRoot.empty()) {
          return false;
        }

        std::filesystem::path l_Path =
            to_path(get_current_directory()).lexically_normal();
        while (true) {
          if (EngineRegistry::paths_equal(from_path(l_Path),
                                          p_Context.engineRoot)) {
            return true;
          }
          if (!l_Path.has_parent_path() ||
              l_Path.parent_path() == l_Path) {
            return false;
          }
          l_Path = l_Path.parent_path();
        }
      }

      static int codegen_engine(Context &p_Context)
      {
        if (!p_Context.require_engine_source()) {
          return 1;
        }

        int l_ExitCode = run_engine_script(
            p_Context, "generate_types.js", {}, p_Context.engineRoot,
            "Generating the engine types failed");
        if (l_ExitCode != 0) {
          return l_ExitCode;
        }

        l_ExitCode = run_engine_script(
            p_Context, "generate_scripting_api_as.js", {},
            p_Context.engineRoot,
            "Generating the AngelScript bindings failed");
        if (l_ExitCode != 0) {
          return l_ExitCode;
        }

        Output::success("Engine code generated");
        return 0;
      }

      static int codegen_project(Context &p_Context)
      {
        if (!p_Context.require_engine_root()) {
          return 1;
        }
        if (!p_Context.require_project_root()) {
          return 1;
        }

        const int l_ExitCode = run_engine_script(
            p_Context, "generate_types.js", {p_Context.projectRoot},
            p_Context.projectRoot,
            "Generating the project types failed");
        if (l_ExitCode != 0) {
          return l_ExitCode;
        }

        Output::success("Project code generated");
        return 0;
      }

      static int codegen(Context &p_Context)
      {
        if (!p_Context.projectRoot.empty()) {
          return codegen_project(p_Context);
        }
        if (is_inside_engine(p_Context)) {
          return codegen_engine(p_Context);
        }

        Output::error("Nothing to generate here",
                      {"no project.yaml in this folder or its parents",
                       "and this folder is not inside the engine"},
                      "run it inside a project, pass -p or use "
                      "`low codegen engine`");
        return 1;
      }

      void register_codegen(CLI::App &p_App)
      {
        CLI::App *l_Codegen = p_App.add_subcommand(
            "codegen", "Generate C++ from the type yamls of the project "
                       "or, inside the engine, of the engine");
        l_Codegen->require_subcommand(0, 1);
        add_command(l_Codegen, codegen);

        CLI::App *l_Engine = l_Codegen->add_subcommand(
            "engine", "Generate the engine's types and AngelScript "
                      "bindings from anywhere");
        add_command(l_Engine, codegen_engine);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
