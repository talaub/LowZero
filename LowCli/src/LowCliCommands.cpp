#include "LowCliCommands.h"

#include "LowCliEngineRegistry.h"
#include "LowCliOutput.h"

#include "LowUtilYaml.h"

#include <filesystem>
#include <fstream>
#include <stdlib.h>

namespace Low {
  namespace Cli {
    namespace fs = std::filesystem;

    fs::path to_path(Util::StringView p_Path)
    {
      return fs::path(std::u8string(
          reinterpret_cast<const char8_t *>(p_Path.data()),
          p_Path.size()));
    }

    Util::String from_path(const fs::path &p_Path)
    {
      const std::u8string l_Path = p_Path.generic_u8string();
      return Util::String(
          reinterpret_cast<const char *>(l_Path.data()),
          l_Path.size());
    }

    Util::String get_current_directory()
    {
      std::error_code l_Error;
      const fs::path l_Path = fs::current_path(l_Error);
      if (l_Error) {
        return "";
      }
      return from_path(l_Path);
    }

    Util::String read_yaml_scalar(const Util::String &p_File,
                                  const char *p_Key)
    {
      std::error_code l_Error;
      const fs::path l_Path = to_path(p_File);
      if (!fs::is_regular_file(l_Path, l_Error)) {
        return "";
      }
      try {
        std::ifstream l_Stream(l_Path, std::ios::binary);
        if (!l_Stream) {
          return "";
        }
        const Util::Yaml::Node l_Root = YAML::Load(l_Stream);
        const Util::Yaml::Node l_Value = l_Root[p_Key];
        if (l_Value && l_Value.IsScalar()) {
          return l_Value.Scalar().c_str();
        }
      } catch (const std::exception &) {
      }
      return "";
    }

    static Util::String get_engine_environment_override()
    {
#ifdef _WIN32
      const wchar_t *l_Engine = _wgetenv(L"LOW_ENGINE");
      if (l_Engine && *l_Engine) {
        return from_path(fs::path(l_Engine));
      }
#else
      const char *l_Engine = getenv("LOW_ENGINE");
      if (l_Engine && *l_Engine) {
        return l_Engine;
      }
#endif
      return "";
    }

    void warn_on_engine_mismatch(const Context &p_Context)
    {
      if (p_Context.engineRoot.empty() ||
          p_Context.projectRoot.empty()) {
        return;
      }

      // Same order as LowShim/src/main.cpp resolve_engine
      Util::String l_Expected = get_engine_environment_override();
      Util::String l_Source = "LOW_ENGINE environment variable";

      if (l_Expected.empty()) {
        l_Expected = read_yaml_scalar(p_Context.projectRoot +
                                          "/.user/user.yaml",
                                      "engine_override");
        l_Source = "engine_override in .user/user.yaml";
      }

      if (l_Expected.empty()) {
        const Util::String l_Engine = read_yaml_scalar(
            p_Context.projectRoot + "/project.yaml", "engine");
        if (l_Engine.empty()) {
          return;
        }
        l_Source = "engine: " + l_Engine + " in project.yaml";

        if (l_Engine == "bundled") {
          l_Expected = p_Context.projectRoot + "/engine";
        } else {
          if (!EngineRegistry::initialize()) {
            return;
          }
          l_Expected = EngineRegistry::get_key(l_Engine);
          if (l_Expected.empty()) {
            Output::warn("The project's engine is not registered on "
                         "this machine",
                         {l_Source},
                         "run Setup.bat in that engine or "
                         "`low engine register " +
                             l_Engine + "` with its low");
            return;
          }
        }
      }

      if (EngineRegistry::paths_equal(l_Expected,
                                      p_Context.engineRoot)) {
        return;
      }

      Output::warn(
          "This low belongs to a different engine than the "
          "project uses",
          {"running: " + p_Context.engineRoot,
           "project: " + l_Expected + " (" + l_Source + ")"},
          "run `low` through the shim on PATH or use the low "
          "of that engine");
    }

    struct RegisteredCommand
    {
      CLI::App *app;
      Util::Function<int(Context &)> run;
    };

    Util::List<RegisteredCommand> g_Commands;

    bool Context::require_engine_root() const
    {
      if (!engineRoot.empty()) {
        return true;
      }

      Output::error(
          "Could not find the engine this low belongs to",
          {"looked for LowData folder next to the bin folder",
           "and for a LowData folder in its parent folder"},
          "rebuild the engine from source or reinstall it");
      return false;
    }

    bool Context::require_project_root() const
    {
      if (!projectRoot.empty()) {
        return true;
      }

      Output::error(
          "Could not find the project root.",
          {"looked here for a project.yaml but could not find any.",
           "Also no specific path to a project was passed using",
           "the -p argument."},
          "execute this command in your project's root or pass the "
          "path to it using the -p flag.");
      return false;
    }

    bool Context::require_engine_source() const
    {
      if (!require_engine_root()) {
        return false;
      }
      if (engineSource == Util::EngineSource::Source) {
        return true;
      }

      Output::error("This command needs an engine built from source",
                    {"running: " + engineRoot,
                     "a packaged engine has no type yamls or sources"},
                    "use the low of a source engine");
      return false;
    }

    void add_command(CLI::App *p_App,
                     Util::Function<int(Context &)> p_Run)
    {
      g_Commands.push_back({p_App, p_Run});
    }

    int run_selected_command(Context &p_Context)
    {
      // get_subcommands() only returns subcommands that were parsed,
      // so the selected command is the parsed one without parsed
      // children
      for (RegisteredCommand &i_Command : g_Commands) {
        if (i_Command.app->parsed() &&
            i_Command.app->get_subcommands().empty()) {
          return i_Command.run(p_Context);
        }
      }

      Output::error("No command selected", {}, "run low --help");
      return 1;
    }

    namespace Commands {
      void register_all(CLI::App &p_App)
      {
        register_info(p_App);
        register_debug(p_App);
        register_configure(p_App);
        register_engine(p_App);
        register_package(p_App);
        register_config(p_App);
        register_release(p_App);
        register_codegen(p_App);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
