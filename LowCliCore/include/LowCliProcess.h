#pragma once

#include "LowUtilContainers.h"

namespace Low {
  namespace Cli {
    namespace Process {
      struct Options
      {
        // Looked up on PATH if not absolute. On Windows only .exe files are
        // found that way, so wrappers like npm.cmd need cmd.exe /c.
        Util::String executable;
        Util::List<Util::String> arguments;
        Util::String workingDirectory;

        // Called per line without the line break, from reader threads
        Util::Function<void(Util::StringView)> onStdoutLine;
        Util::Function<void(Util::StringView)> onStderrLine;
      };

      // Blocks until the process exited and both streams are drained.
      // Returns the exit code, -1 if the process could not be started.
      int run(const Options &p_Options);

      // Also inherited by every child started afterwards
      void set_environment_variable(Util::StringView p_Name,
                                    Util::StringView p_Value);

      // Kills all running children including their child processes
      void kill_all();

      struct EventRunResult
      {
        int exitCode = -1;
        Util::List<Util::String> stderrLines;
      };

      // Runs a child that reports through NDJSON events on stdout (see
      // LowCliEvents.h). Plain stdout lines are shown as details, stderr
      // is collected (and shown with -v) so the caller can attach it to an
      // error message.
      EventRunResult run_with_events(const Options &p_Options);
    } // namespace Process
  } // namespace Cli
} // namespace Low
