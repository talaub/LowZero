#pragma once

#include "LowUtilContainers.h"

#include <stdint.h>

namespace Low {
  namespace Cli {
    namespace Terminal {
      enum class Color
      {
        NONE,
        BOLD,
        DIM,
        RED,
        GREEN,
        YELLOW,
        CYAN
      };

      struct Symbols
      {
        const char *success;
        const char *error;
        const char *warning;
        const char *detail;
        const char *bullet;
        const char *pipe;
        const char *rule;
        const char *barFull;
        const char *barEmpty;
        Util::List<const char *> spinner;
      };

      struct Capabilities
      {
        bool stdoutIsTerminal = false;
        bool stderrIsTerminal = false;
        bool color = false;
        bool unicode = true;
      };

      // p_Plain: ASCII symbols and no colors. Colors are also off
      // when NO_COLOR is set or stderr is not a terminal.
      void initialize(bool p_Plain, bool p_NoColor);
      void cleanup();

      const Capabilities &get_capabilities();
      const Symbols &get_symbols();
      uint32_t get_width();

      Util::String colorize(Util::StringView p_Text,
                            Color p_Color);

      void write_stdout(Util::StringView p_Text);
      void write_stderr(Util::StringView p_Text);

      void show_cursor(bool p_Show);

      // Called on Ctrl+C before the process exits with code 130
      void add_interrupt_callback(Util::Function<void()> p_Callback);
    } // namespace Terminal
  } // namespace Cli
} // namespace Low
