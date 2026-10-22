#include "LowCliTerminal.h"

#include <atomic>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace Low {
  namespace Cli {
    namespace Terminal {
      Capabilities g_Capabilities;
      Symbols g_Symbols;
      std::mutex g_WriteMutex;
      std::atomic<bool> g_CursorHidden{false};
      bool g_Initialized = false;

      Util::List<Util::Function<void()>> g_InterruptCallbacks;
      std::atomic<bool> g_Interrupted{false};

#ifdef _WIN32
      UINT g_PreviousOutputCodepage = 0;
#endif

      static void handle_interrupt()
      {
        if (g_Interrupted.exchange(true)) {
          return;
        }

        for (auto &i_Callback : g_InterruptCallbacks) {
          i_Callback();
        }

        cleanup();
        fflush(stdout);
        fflush(stderr);
        _Exit(130);
      }

#ifdef _WIN32
      static BOOL WINAPI console_ctrl_handler(DWORD p_Type)
      {
        if (p_Type == CTRL_C_EVENT || p_Type == CTRL_BREAK_EVENT ||
            p_Type == CTRL_CLOSE_EVENT) {
          handle_interrupt();
          return TRUE;
        }
        return FALSE;
      }

      static bool enable_virtual_terminal(DWORD p_StdHandle)
      {
        HANDLE l_Handle = GetStdHandle(p_StdHandle);
        DWORD l_Mode = 0;
        if (!GetConsoleMode(l_Handle, &l_Mode)) {
          return false;
        }
        SetConsoleMode(l_Handle,
                       l_Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        return true;
      }
#else
      static void signal_handler(int)
      {
        handle_interrupt();
      }
#endif

      static void setup_symbols(bool p_Unicode)
      {
        if (p_Unicode) {
          // No ✔ ✖ ⚠: they have emoji variants, which some terminals draw
          // with a color emoji font that ignores our colors
          g_Symbols.success = "\xE2\x9C\x93";  // ✓
          g_Symbols.error = "\xE2\x9C\x97";    // ✗
          g_Symbols.warning = "\xE2\x96\xB2";  // ▲
          g_Symbols.detail = "\xE2\x80\xBA";   // ›
          g_Symbols.bullet = "\xE2\x80\xA2";   // •
          g_Symbols.pipe = "\xE2\x94\x82";     // │
          g_Symbols.rule = "\xE2\x94\x80";     // ─
          g_Symbols.barFull = "\xE2\x96\x88";  // █
          g_Symbols.barEmpty = "\xE2\x96\x91"; // ░
          g_Symbols.spinner = {
              "\xE2\xA0\x8B", "\xE2\xA0\x99", "\xE2\xA0\xB9",
              "\xE2\xA0\xB8", "\xE2\xA0\xBC", "\xE2\xA0\xB4",
              "\xE2\xA0\xA6", "\xE2\xA0\xA7", "\xE2\xA0\x87",
              "\xE2\xA0\x8F"};
        } else {
          g_Symbols.success = "[ok]";
          g_Symbols.error = "[x]";
          g_Symbols.warning = "[!]";
          g_Symbols.detail = ">";
          g_Symbols.bullet = "*";
          g_Symbols.pipe = "|";
          g_Symbols.rule = "-";
          g_Symbols.barFull = "#";
          g_Symbols.barEmpty = ".";
          g_Symbols.spinner = {"|", "/", "-", "\\"};
        }
      }

      void initialize(bool p_Plain, bool p_NoColor)
      {
#ifdef _WIN32
        g_PreviousOutputCodepage = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);

        g_Capabilities.stdoutIsTerminal =
            enable_virtual_terminal(STD_OUTPUT_HANDLE);
        g_Capabilities.stderrIsTerminal =
            enable_virtual_terminal(STD_ERROR_HANDLE);

        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
        g_Capabilities.stdoutIsTerminal = isatty(fileno(stdout));
        g_Capabilities.stderrIsTerminal = isatty(fileno(stderr));

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
#endif

        const char *l_NoColorEnv = getenv("NO_COLOR");
        const bool l_NoColorSet = l_NoColorEnv && l_NoColorEnv[0] != '\0';

        g_Capabilities.unicode = !p_Plain;
        g_Capabilities.color = !p_Plain && !p_NoColor && !l_NoColorSet &&
                               g_Capabilities.stderrIsTerminal;

        setup_symbols(g_Capabilities.unicode);

        // Restores the cursor even if a command returns from main early
        if (!g_Initialized) {
          atexit(cleanup);
        }
        g_Initialized = true;
      }

      void cleanup()
      {
        if (!g_Initialized) {
          return;
        }

        if (g_CursorHidden) {
          show_cursor(true);
        }

#ifdef _WIN32
        if (g_PreviousOutputCodepage != 0) {
          SetConsoleOutputCP(g_PreviousOutputCodepage);
        }
#endif
      }

      const Capabilities &get_capabilities()
      {
        return g_Capabilities;
      }

      const Symbols &get_symbols()
      {
        return g_Symbols;
      }

      uint32_t get_width()
      {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO l_Info;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE),
                                       &l_Info)) {
          return static_cast<uint32_t>(l_Info.srWindow.Right -
                                       l_Info.srWindow.Left + 1);
        }
#else
        winsize l_Size;
        if (ioctl(fileno(stderr), TIOCGWINSZ, &l_Size) == 0 &&
            l_Size.ws_col > 0) {
          return l_Size.ws_col;
        }
#endif
        return 80;
      }

      Util::String colorize(Util::StringView p_Text,
                            Color p_Color)
      {
        if (!g_Capabilities.color || p_Color == Color::NONE) {
          return Util::String(p_Text.data(), p_Text.size());
        }

        const char *l_Code = "";
        switch (p_Color) {
        case Color::BOLD:
          l_Code = "\x1b[1m";
          break;
        case Color::DIM:
          l_Code = "\x1b[2m";
          break;
        case Color::RED:
          l_Code = "\x1b[31m";
          break;
        case Color::GREEN:
          l_Code = "\x1b[32m";
          break;
        case Color::YELLOW:
          l_Code = "\x1b[33m";
          break;
        case Color::CYAN:
          l_Code = "\x1b[36m";
          break;
        default:
          break;
        }

        Util::String l_Result = l_Code;
        l_Result.append(p_Text.data(), p_Text.size());
        l_Result += "\x1b[0m";
        return l_Result;
      }

      void write_stdout(Util::StringView p_Text)
      {
        std::lock_guard<std::mutex> l_Lock(g_WriteMutex);
        fwrite(p_Text.data(), 1, p_Text.size(), stdout);
        fflush(stdout);
      }

      void write_stderr(Util::StringView p_Text)
      {
        std::lock_guard<std::mutex> l_Lock(g_WriteMutex);
        fwrite(p_Text.data(), 1, p_Text.size(), stderr);
        fflush(stderr);
      }

      void show_cursor(bool p_Show)
      {
        if (!g_Capabilities.stderrIsTerminal) {
          return;
        }
        g_CursorHidden = !p_Show;
        write_stderr(p_Show ? "\x1b[?25h" : "\x1b[?25l");
      }

      void add_interrupt_callback(Util::Function<void()> p_Callback)
      {
        g_InterruptCallbacks.push_back(p_Callback);
      }
    } // namespace Terminal
  } // namespace Cli
} // namespace Low
