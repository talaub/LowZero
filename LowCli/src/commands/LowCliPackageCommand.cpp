#include "LowCliCommands.h"

#include "LowCliPackaging.h"

#include <string>

namespace Low {
  namespace Cli {
    namespace Commands {
      // CLI11 writes the flags here during parse(), so this has to
      // outlive register_package
      static struct PackageBundledArgs
      {
        bool allowDirty = false;
        std::string symbolStore;
      } g_BundledArgs;

      static int package_bundled_command(Context &p_Context)
      {
        Packaging::BundledOptions l_Options;
        l_Options.allowDirty = g_BundledArgs.allowDirty;
        l_Options.symbolStore = g_BundledArgs.symbolStore.c_str();
        return Packaging::package_bundled(p_Context, l_Options);
      }

      void register_package(CLI::App &p_App)
      {
        CLI::App *l_Package = p_App.add_subcommand(
            "package", "Build distributable versions of the engine or "
                       "a project");
        l_Package->require_subcommand(1);

        CLI::App *l_Bundled = l_Package->add_subcommand(
            "bundled", "Build the project with the engine and put the "
                       "binaries into the project for designers");
        l_Bundled->add_flag(
            "--allow-dirty", g_BundledArgs.allowDirty,
            "Package even if the engine or the project modules have "
            "uncommitted changes");
        l_Bundled->add_option(
            "--symbol-store", g_BundledArgs.symbolStore,
            "Store the PDBs here instead of the configured symbol store "
            "(low config get symbol_store)");

        add_command(l_Bundled, package_bundled_command);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
