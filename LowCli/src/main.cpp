#include "LowCliCommands.h"
#include "LowCliOutput.h"
#include "LowCliProcess.h"
#include "LowCliTerminal.h"

#include "LowUtil.h"

#include <CLI/CLI.hpp>

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

int main(int argc, char *argv[])
{
  using namespace Low;

  CLI::App l_App{"LowEngine command line tools", "low"};
  argv = l_App.ensure_utf8(argv);
  l_App.require_subcommand(1);
  // Lets the global flags also come after the subcommand (low
  // generate -v)
  l_App.fallthrough();

  bool l_Plain = false;
  bool l_Json = false;
  bool l_NoColor = false;
  bool l_Quiet = false;
  bool l_Verbose = false;

  l_App.add_flag("--plain", l_Plain,
                 "ASCII output without colors or animation");
  l_App.add_flag("--json", l_Json,
                 "Write NDJSON events to stdout instead of formatted "
                 "output");
  l_App.add_flag("--no-color", l_NoColor, "Disable colors");
  CLI::Option *l_QuietFlag =
      l_App.add_flag("-q,--quiet", l_Quiet,
                     "Only show warnings, errors and results");
  CLI::Option *l_VerboseFlag = l_App.add_flag(
      "-v,--verbose", l_Verbose,
      "Show debug output and stderr of child processes");
  l_QuietFlag->excludes(l_VerboseFlag);

  std::string l_ProjectPath;
  l_App.add_option("-p,--project", l_ProjectPath,
                   "Project folder or its project.yaml. Default: the "
                   "first parent folder with a project.yaml");

  Cli::Commands::register_all(l_App);

  const auto l_InitializeOutput = [&]() {
    Cli::Output::Config l_Config;
    l_Config.plain = l_Plain;
    l_Config.json = l_Json;
    l_Config.noColor = l_NoColor;
    if (l_Quiet) {
      l_Config.verbosity = Cli::Output::Verbosity::QUIET;
    } else if (l_Verbose) {
      l_Config.verbosity = Cli::Output::Verbosity::VERBOSE;
    }
    Cli::Output::initialize(l_Config);
  };

  try {
    l_App.parse(argc, argv);
  } catch (const CLI::ParseError &l_Error) {
    // CLI11 checks for a missing subcommand before it complains about
    // unknown arguments, so a typo like `low generate` would only say
    // "A subcommand is required"
    const std::vector<std::string> l_Remaining =
        l_App.remaining(true);
    if (!dynamic_cast<const CLI::RequiredError *>(&l_Error) ||
        l_Remaining.empty()) {
      return l_App.exit(l_Error);
    }

    // Global flags might not be applied yet at this point, so this
    // can fall back to the default output
    l_InitializeOutput();

    Util::String l_CommandPath = "low";
    CLI::App *l_Deepest = &l_App;
    while (!l_Deepest->get_subcommands().empty()) {
      l_Deepest = l_Deepest->get_subcommands().front();
      l_CommandPath += " ";
      l_CommandPath += l_Deepest->get_name().c_str();
    }

    const Util::String l_Unknown = l_Remaining.front().c_str();
    const bool l_IsOption = !l_Unknown.empty() && l_Unknown[0] == '-';
    Cli::Output::error(
        Util::String(l_IsOption ? "Unknown option: "
                                : "Unknown command: ") +
            l_Unknown,
        {},
        "run " + l_CommandPath + " --help to see what is available");

    const int l_ExitCode =
        static_cast<int>(CLI::ExitCodes::ExtrasError);
    Cli::Output::finish(l_ExitCode);
    return l_ExitCode;
  }

  l_InitializeOutput();

  Cli::Terminal::add_interrupt_callback(Cli::Process::kill_all);

  Cli::Process::set_environment_variable("LOW_CLI_EVENTS", "1");

  Cli::Context l_Context;
  l_Context.engineRoot =
      Util::find_engine_root(&l_Context.engineSource);

  // An explicit path that is not a project is an error, it must not
  // fall back to searching and silently pick some other project
  if (!l_ProjectPath.empty()) {
    l_Context.projectRoot =
        Util::resolve_project_path(l_ProjectPath.c_str());
    if (l_Context.projectRoot.empty()) {
      Cli::Output::error(
          Util::String("Not a LowEngine project: ") +
              l_ProjectPath.c_str(),
          {"there is no project.yaml in it"},
          "pass the project folder or its project.yaml");
      Cli::Output::finish(1);
      return 1;
    }
  } else {
    l_Context.projectRoot =
        Util::find_project_root(Cli::get_current_directory());
  }

  Cli::warn_on_engine_mismatch(l_Context);

  const int l_ExitCode = Cli::run_selected_command(l_Context);

  Cli::Output::finish(l_ExitCode);
  return l_ExitCode;
}
