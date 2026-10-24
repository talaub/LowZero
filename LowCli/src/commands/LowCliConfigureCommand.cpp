#include "LowCliCommands.h"

#include "LowCliProcess.h"
#include "LowCliOutput.h"

#include "LowUtilFileIO.h"

namespace Low {
  namespace Cli {

    // How much of the script's stderr is shown when it fails, the
    // rest is available with -v
    static const size_t g_MaxErrorLines = 20;

    int run_engine_script(const Context &p_Context,
                          const char *p_ScriptName,
                          const Util::List<Util::String> &p_Arguments,
                          const Util::String &p_WorkingDirectory,
                          const char *p_FailureMessage)
    {
      const Util::String l_Script = p_Context.engineRoot +
                                    "/LowScripts/codegenerator/" +
                                    p_ScriptName;
      if (!Util::FileIO::file_exists_sync(l_Script.c_str())) {
        Output::error("The engine is missing one of its scripts",
                      {"missing " + l_Script},
                      "check that the engine is complete");
        return 1;
      }

      Process::Options l_Options;
      l_Options.executable = "node";
      l_Options.arguments = {l_Script};
      l_Options.workingDirectory = p_WorkingDirectory;

      for (const Util::String &i_Arg : p_Arguments) {
        l_Options.arguments.push_back(i_Arg);
      }

      const Process::EventRunResult l_Result =
          Process::run_with_events(l_Options);

      if (l_Result.exitCode == -1) {
        Output::error(
            "Could not start node",
            {"node is needed to run the engine's scripts"},
            "install Node.js and make sure node is on PATH");
        return 1;
      }

      if (l_Result.exitCode != 0) {
        // The end of stderr is where the actual error usually is
        Util::List<Util::String> l_Details;
        const size_t l_First =
            l_Result.stderrLines.size() > g_MaxErrorLines
                ? l_Result.stderrLines.size() - g_MaxErrorLines
                : 0;
        for (size_t i = l_First; i < l_Result.stderrLines.size();
             ++i) {
          l_Details.push_back(l_Result.stderrLines[i]);
        }

        Output::error(p_FailureMessage, l_Details,
                      l_First > 0 ? "run with -v for the full output"
                                  : "");
        return l_Result.exitCode;
      }

      return 0;
    }

    int run_project_configure(
        Context &p_Context,
        const Util::List<Util::String> &p_ExtraArguments)
    {
      if (!p_Context.require_engine_root()) {
        return 1;
      }
      if (!p_Context.require_project_root()) {
        return 1;
      }

      Util::List<Util::String> l_Arguments = {p_Context.projectRoot};
      for (const Util::String &i_Arg : p_ExtraArguments) {
        l_Arguments.push_back(i_Arg);
      }

      const int l_ExitCode = run_engine_script(
          p_Context, "build_project.js", l_Arguments,
          p_Context.projectRoot, "Configuring the project failed");
      if (l_ExitCode != 0) {
        return l_ExitCode;
      }

      Output::success("Project configured");
      return 0;
    }

    namespace Commands {
      static int configure_project(Context &p_Context)
      {
        return run_project_configure(p_Context, {});
      }

      void register_configure(CLI::App &p_App)
      {
        CLI::App *l_Configure = p_App.add_subcommand(
            "configure", "Used to generate cmake files for a project "
                         "and run the cmake configure command.");

        add_command(l_Configure, configure_project);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
