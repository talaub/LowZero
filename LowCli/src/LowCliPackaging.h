#pragma once

#include <LowUtil.h>
namespace Low {
  namespace Cli {
    struct Context;
    namespace Packaging {
      /** Runs a tool like cmake under a spinner. Output is collected
       and only shown on failure (or always as debug output with -v)*/
      bool run_step(Util::StringView p_Label,
                    const Util::String &p_Executable,
                    const Util::List<Util::String> &p_Arguments,
                    const Util::String &p_WorkingDirectory);

      enum class GitState
      {
        Clean,
        Dirty,
        Unknown
      };

      GitState
      get_git_state(const Util::String &p_Repository,
                    const Util::List<Util::String> &p_Pathspecs = {});

      /** Deletes the contents and recreates the folder, reports
       errors itself*/
      bool clear_directory(const Util::String &p_Path);

      struct BundledOptions
      {
        bool allowDirty = false;
        // Falls back to git describe if empty. low release passes the
        // tag it is about to create
        Util::String buildId;
        // Empty: low config get symbol_store
        Util::String symbolStore;
        // low release checks the project itself before it bumps the
        // version in project.yaml, which would count as a change here
        bool checkProjectState = true;
      };

      /** Builds the project and the engine with the package flags and
       * fills <project>/engine, <project>/bin and the launcher*/
      int package_bundled(Context &p_Context,
                          const BundledOptions &p_Options);
    } // namespace Packaging
  } // namespace Cli
} // namespace Low
