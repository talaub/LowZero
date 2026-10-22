#include "LowCliPackaging.h"

#include "LowCliCommands.h"
#include "LowCliOutput.h"
#include "LowCliProcess.h"
#include "LowCliSettings.h"
#include "LowCliSymbols.h"

#include <EASTL/algorithm.h>

#include <filesystem>
#include <mutex>
#include <string>

namespace Low {
  namespace Cli {
    namespace Packaging {
      namespace fs = std::filesystem;

      // How much of a failed tool's output is shown, the rest is
      // available with -v
      static const size_t g_MaxErrorLines = 20;

      // Relative to the project, build_project.js resolves it the
      // same way
      static const char *g_PackageBuildDir = ".low/build_package";
      static const char *g_PackageConfig = "RelWithDebInfo";

      // The feature flag set of packaged builds, see the options in
      // the engine's root CMakeLists.txt
      static const char *g_PackageDefines[] = {
          "-DLOW_RUNTIME_SHADER_COMPILATION=OFF",
          "-DLOW_VULKAN_VALIDATION=OFF",
          "-DLOW_HIDE_CONSOLE=ON",
      };

      // MSVC compiler/linker, MSBuild and CMake error lines. MSBuild
      // writes them to stdout, not stderr.
      static bool is_error_line(const Util::String &p_Line)
      {
        return p_Line.find(": error ") != Util::String::npos ||
               p_Line.find(": fatal error ") != Util::String::npos ||
               p_Line.find("CMake Error") != Util::String::npos;
      }

      static Util::List<Util::String>
      pick_error_details(const Util::List<Util::String> &p_Lines,
                         bool &p_OutTruncated)
      {
        Util::List<Util::String> l_Errors;
        size_t l_ErrorCount = 0;
        for (const Util::String &i_Line : p_Lines) {
          if (!is_error_line(i_Line)) {
            continue;
          }
          // MSBuild repeats every error in its summary at the end
          if (eastl::find(l_Errors.begin(), l_Errors.end(), i_Line) !=
              l_Errors.end()) {
            continue;
          }
          ++l_ErrorCount;
          // The first errors are the relevant ones, later ones are
          // often follow-up errors
          if (l_Errors.size() < g_MaxErrorLines) {
            l_Errors.push_back(i_Line);
          }
        }

        if (!l_Errors.empty()) {
          p_OutTruncated = l_ErrorCount > l_Errors.size();
          return l_Errors;
        }

        // Nothing recognizable, the end is where the error usually is
        const size_t l_First = p_Lines.size() > g_MaxErrorLines
                                   ? p_Lines.size() - g_MaxErrorLines
                                   : 0;
        p_OutTruncated = l_First > 0;
        return Util::List<Util::String>(p_Lines.begin() + l_First,
                                        p_Lines.end());
      }

      bool run_step(Util::StringView p_Label,
                    const Util::String &p_Executable,
                    const Util::List<Util::String> &p_Arguments,
                    const Util::String &p_WorkingDirectory)
      {
        std::mutex l_LinesMutex;
        Util::List<Util::String> l_Lines;

        int l_ExitCode = -1;
        {
          Output::Step l_Step(p_Label);

          const auto l_Collect = [&l_LinesMutex,
                                  &l_Lines](Util::StringView p_Line) {
            Output::debug(p_Line);
            std::lock_guard<std::mutex> l_Lock(l_LinesMutex);
            l_Lines.push_back(
                Util::String(p_Line.data(), p_Line.size()));
          };

          Process::Options l_Options;
          l_Options.executable = p_Executable;
          l_Options.arguments = p_Arguments;
          l_Options.workingDirectory = p_WorkingDirectory;
          l_Options.onStdoutLine = l_Collect;
          l_Options.onStderrLine = l_Collect;

          l_ExitCode = Process::run(l_Options);
          if (l_ExitCode != 0) {
            l_Step.fail();
          }
        }
        // Step ended

        if (l_ExitCode == 0) {
          return true;
        }

        const Util::String l_Label(p_Label.data(), p_Label.size());

        if (l_ExitCode == -1) {
          Output::error("Could not start " + p_Executable, {l_Label},
                        "make sure " + p_Executable +
                            " is installed and on PATH");
          return false;
        }

        bool l_Truncated = false;
        const Util::List<Util::String> l_Details =
            pick_error_details(l_Lines, l_Truncated);

        const bool l_Verbose = Output::get_config().verbosity ==
                               Output::Verbosity::VERBOSE;
        Output::error(
            l_Label + " failed (exit code " +
                Util::String(std::to_string(l_ExitCode).c_str()) +
                ")",
            l_Details,
            l_Truncated && !l_Verbose
                ? "run with -v for the full output"
                : "");
        return false;
      }

      GitState
      get_git_state(const Util::String &p_Repository,
                    const Util::List<Util::String> &p_Pathspecs)
      {
        Process::Options l_Options;
        l_Options.executable = "git";
        // --porcelain is the stable machine readable format, it does
        // not change with the git version or the user's language
        l_Options.arguments = {"-C", p_Repository, "status",
                               "--porcelain"};
        if (!p_Pathspecs.empty()) {
          // Everything after -- is a path, never an option
          l_Options.arguments.push_back("--");
          for (const Util::String &i_Pathspec : p_Pathspecs) {
            l_Options.arguments.push_back(i_Pathspec);
          }
        }

        // stdout is read on the calling thread, stderr on another one
        bool l_HasChanges = false;
        l_Options.onStdoutLine =
            [&l_HasChanges](Util::StringView p_Line) {
              if (!p_Line.empty()) {
                l_HasChanges = true;
              }
            };
        l_Options.onStderrLine = [](Util::StringView p_Line) {
          Output::debug(p_Line);
        };

        // -1: git is not installed, 128: not a git repository
        const int l_ExitCode = Process::run(l_Options);
        if (l_ExitCode != 0) {
          return GitState::Unknown;
        }

        return l_HasChanges ? GitState::Dirty : GitState::Clean;
      }

      bool clear_directory(const Util::String &p_Path)
      {
        const fs::path l_Path = to_path(p_Path);

        std::error_code l_Error;
        fs::remove_all(l_Path, l_Error);
        if (!l_Error) {
          fs::create_directories(l_Path, l_Error);
        }

        if (l_Error) {
          // Windows keeps loaded DLLs locked, so a running editor
          // makes this fail halfway through
          Output::error(
              "Could not clear " + p_Path,
              {l_Error.message().c_str()},
              "close programs that use files in it (e.g. the "
              "editor, lowder.exe) and try again");
          return false;
        }

        return true;
      }

      // Collects dirty repositories, an unknown state is only a
      // warning so projects without git can still be packaged
      static void
      check_git_state(const Util::String &p_Name,
                      const Util::String &p_Repository,
                      const GitState p_State,
                      Util::List<Util::String> &p_OutDirty)
      {
        if (p_State == GitState::Dirty) {
          p_OutDirty.push_back(p_Name + " (" + p_Repository + ")");
        } else if (p_State == GitState::Unknown) {
          Output::warn(
              "Could not check the git state of the " + p_Name,
              {p_Repository}, "the build id will be unknown");
        }
      }

      // Refuses to delete a folder with content that does not look
      // like an earlier package, e.g. a project's own engine/ folder
      static bool
      can_replace_engine_folder(const Util::String &p_Path)
      {
        const fs::path l_Path = to_path(p_Path);
        std::error_code l_Error;
        if (!fs::exists(l_Path, l_Error) ||
            fs::is_empty(l_Path, l_Error)) {
          return true;
        }
        return fs::is_directory(l_Path / "bin", l_Error) ||
               fs::is_directory(l_Path / "LowData", l_Error);
      }

      // --config has to match the build: the Visual Studio generator
      // keeps all configs side by side in one build dir
      static bool
      install_component(const Util::String &p_BuildDir,
                        const char *p_Component,
                        const Util::String &p_Prefix,
                        const Util::String &p_WorkingDirectory)
      {
        return run_step(
            Util::String("Installing ") + p_Component, "cmake",
            {"--install", p_BuildDir, "--config", g_PackageConfig,
             "--component", p_Component, "--prefix", p_Prefix},
            p_WorkingDirectory);
      }

      // --symbol-store > low config (LOW_SYMBOL_STORE > settings.yaml >
      // default). Creates the folder right away, so a missing drive is
      // noticed before the long build and not after it
      static bool resolve_symbol_store(const Util::String &p_Override,
                                       Util::String &p_OutStore)
      {
        if (!p_Override.empty()) {
          std::error_code l_Error;
          const fs::path l_Path =
              fs::absolute(to_path(p_Override), l_Error);
          p_OutStore = l_Error ? p_Override
                               : from_path(l_Path.lexically_normal());
        } else {
          if (!Settings::initialize()) {
            Output::error("Could not read the settings file",
                          {Settings::get_settings_path()},
                          "fix or delete it, low config get shows the "
                          "settings");
            return false;
          }
          p_OutStore = Settings::get("symbol_store").value;
        }

        if (p_OutStore.empty()) {
          Output::error("No symbol store configured", {},
                        "set one with low config set symbol_store "
                        "<path>");
          return false;
        }

        std::error_code l_Error;
        fs::create_directories(to_path(p_OutStore), l_Error);
        if (l_Error) {
          Output::error("Could not create the symbol store",
                        {p_OutStore, l_Error.message().c_str()},
                        "check the path or change it with low config "
                        "set symbol_store <path>");
          return false;
        }
        return true;
      }

      static void report_symbols(const Symbols::StoreResult &p_Result,
                                 const Util::String &p_Store)
      {
        const u64 l_AddedMegabytes = p_Result.addedBytes / (1024 * 1024);
        Output::detail(
            "symbols: " +
            Util::String(std::to_string(p_Result.added).c_str()) +
            " new, " +
            Util::String(std::to_string(p_Result.existing).c_str()) +
            " already stored (+" +
            Util::String(std::to_string(l_AddedMegabytes).c_str()) +
            " MB) in " + p_Store);

        if (!p_Result.missing.empty()) {
          Output::warn("Some PDBs were not found", p_Result.missing,
                       "crashes in these binaries can't be symbolized");
        }
      }

      int package_bundled(Context &p_Context,
                          const BundledOptions &p_Options)
      {
        if (!p_Context.require_engine_root()) {
          return 1;
        }
        if (!p_Context.require_project_root()) {
          return 1;
        }

        // Needs the CMake files and build_project.js. engineSource
        // can't tell: the low of a source engine sits in <engine>/bin
        // next to LowData, the same layout as a packaged engine
        {
          std::error_code l_SourceError;
          const bool l_HasSource =
              fs::is_regular_file(
                  to_path(p_Context.engineRoot + "/CMakeLists.txt"),
                  l_SourceError) &&
              fs::is_regular_file(
                  to_path(
                      p_Context.engineRoot +
                      "/LowScripts/codegenerator/build_project.js"),
                  l_SourceError);
          if (!l_HasSource) {
            Output::error(
                "Packaging needs the engine source",
                {"this low belongs to " + p_Context.engineRoot,
                 "it has no CMakeLists.txt or LowScripts"},
                "set engine_override in .user/user.yaml to a "
                "source engine and run low through the shim");
            return 1;
          }
        }

        const Util::String l_ProjectFile =
            p_Context.projectRoot + "/project.yaml";
        const Util::String l_Name =
            read_yaml_scalar(l_ProjectFile, "name");
        if (l_Name.empty()) {
          Output::error("The project has no name", {l_ProjectFile},
                        "add name: to project.yaml");
          return 1;
        }

        Output::section("Packaging " + l_Name);

        // The bundle gets built either way, but only a project with
        // engine: bundled actually uses it
        const Util::String l_Engine =
            read_yaml_scalar(l_ProjectFile, "engine");
        if (l_Engine != "bundled") {
          Output::warn("project.yaml does not use the bundled engine",
                       {"engine: " + (l_Engine.empty()
                                          ? Util::String("<missing>")
                                          : l_Engine)},
                       "set engine: bundled so designers use the "
                       "packaged engine");
        }

        Util::List<Util::String> l_Dirty;
        check_git_state("engine", p_Context.engineRoot,
                        get_git_state(p_Context.engineRoot), l_Dirty);
        // Only what ends up in the module DLLs. Designer data and the
        // output of earlier packages (engine/, bin/) do not count
        if (p_Options.checkProjectState) {
          check_git_state("project modules", p_Context.projectRoot,
                          get_git_state(p_Context.projectRoot,
                                        {"modules", "project.yaml"}),
                          l_Dirty);
        }
        if (!l_Dirty.empty()) {
          if (!p_Options.allowDirty) {
            Output::error("Uncommitted changes", l_Dirty,
                          "commit them or pass --allow-dirty");
            return 1;
          }
          Output::warn("Packaging with uncommitted changes", l_Dirty);
        }

        const Util::String l_EngineDir =
            p_Context.projectRoot + "/engine";
        if (!can_replace_engine_folder(l_EngineDir)) {
          Output::error(
              "engine/ in the project is not a packaged engine",
              {l_EngineDir},
              "move it away, packaging replaces this folder");
          return 1;
        }

        Util::String l_SymbolStore;
        if (!resolve_symbol_store(p_Options.symbolStore, l_SymbolStore)) {
          return 1;
        }

        Util::List<Util::String> l_ConfigureArguments = {
            "--build-dir", g_PackageBuildDir};
        for (const char *i_Define : g_PackageDefines) {
          l_ConfigureArguments.push_back(i_Define);
        }
        if (!p_Options.buildId.empty()) {
          l_ConfigureArguments.push_back("--build-id");
          l_ConfigureArguments.push_back(p_Options.buildId);
        }
        if (run_project_configure(p_Context, l_ConfigureArguments) !=
            0) {
          return 1;
        }

        const Util::String l_BuildDir =
            p_Context.projectRoot + "/" + g_PackageBuildDir;

        // <Name> is the launch target of the generated project, it
        // depends on all modules and lowder. --parallel lets MSBuild
        // build several projects at once (/m)
        if (!run_step("Building " + l_Name, "cmake",
                      {"--build", l_BuildDir, "--config",
                       g_PackageConfig, "--target", l_Name,
                       "LowLauncher", "LowCli", "--parallel"},
                      p_Context.projectRoot)) {
          return 1;
        }

        // cmake --install only takes one component per call
        if (!clear_directory(l_EngineDir)) {
          return 1;
        }
        if (!install_component(l_BuildDir, "editor", l_EngineDir,
                               p_Context.projectRoot)) {
          return 1;
        }
        if (!install_component(l_BuildDir, "tools", l_EngineDir,
                               p_Context.projectRoot)) {
          return 1;
        }

        // DESTINATION bin of the generated install rules is relative
        // to the prefix, so the module DLLs land in <project>/bin
        const Util::String l_BinDir = p_Context.projectRoot + "/bin";
        if (!clear_directory(l_BinDir)) {
          return 1;
        }
        if (!install_component(l_BuildDir, "modules",
                               p_Context.projectRoot,
                               p_Context.projectRoot)) {
          return 1;
        }

        const Util::String l_Launcher =
            p_Context.projectRoot + "/" + l_Name + " Editor.exe";
        std::error_code l_Error;
        fs::copy_file(to_path(l_EngineDir + "/bin/lowlauncher.exe"),
                      to_path(l_Launcher),
                      fs::copy_options::overwrite_existing, l_Error);
        if (l_Error) {
          Output::error("Could not create the launcher",
                        {l_Launcher, l_Error.message().c_str()},
                        "close it if it is running and try again");
          return 1;
        }

        // Exactly the PDBs of the binaries that were just shipped,
        // found through the PDB path the linker wrote into each of them
        Symbols::StoreResult l_Symbols;
        if (!Symbols::store({l_EngineDir + "/bin", l_BinDir},
                            l_SymbolStore, l_Symbols)) {
          return 1;
        }

        Output::success("Packaged " + l_Name);
        Output::detail(l_EngineDir);
        Output::detail(l_BinDir);
        Output::detail(l_Launcher);
        report_symbols(l_Symbols, l_SymbolStore);
        return 0;
      }
    } // namespace Packaging
  } // namespace Cli
} // namespace Low
