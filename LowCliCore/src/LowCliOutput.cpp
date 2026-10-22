#include "LowCliOutput.h"

#include "LowCliTerminal.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <thread>

namespace Low {
  namespace Cli {
    namespace Output {
      using Clock = std::chrono::steady_clock;
      using Terminal::Color;

      struct LiveTask
      {
        Util::String id;
        Util::String label;
        Util::String detail;
        int64_t done = -1;
        int64_t total = -1;
        Clock::time_point start;
        int lastReportedQuarter = 0;
      };

      Config g_Config;
      std::mutex g_Mutex;

      Util::List<LiveTask> g_Tasks;
      uint32_t g_LiveLineCount = 0;
      bool g_Animate = false;

      std::thread g_RenderThread;
      std::condition_variable g_RenderSignal;
      std::atomic<bool> g_RenderRunning{false};

      Clock::time_point g_StartTime;
      bool g_HadTasks = false;
      uint32_t g_ErrorCount = 0;
      Util::List<Util::String> g_Warnings;
      std::atomic<uint32_t> g_StepCounter{0};

      static uint32_t display_width(Util::StringView p_Text)
      {
        uint32_t l_Width = 0;
        for (char i_Char : p_Text) {
          if ((static_cast<unsigned char>(i_Char) & 0xC0) != 0x80) {
            ++l_Width;
          }
        }
        return l_Width;
      }

      static Util::String truncate(Util::StringView p_Text,
                                   uint32_t p_MaxWidth)
      {
        if (display_width(p_Text) <= p_MaxWidth) {
          return Util::String(p_Text.data(), p_Text.size());
        }
        if (p_MaxWidth == 0) {
          return "";
        }

        uint32_t l_Width = 0;
        size_t l_Cut = 0;
        for (size_t i = 0; i < p_Text.size(); ++i) {
          if ((static_cast<unsigned char>(p_Text[i]) & 0xC0) != 0x80) {
            if (l_Width == p_MaxWidth - 1) {
              l_Cut = i;
              break;
            }
            ++l_Width;
          }
        }

        Util::String l_Result(p_Text.data(), l_Cut);
        l_Result += Terminal::get_capabilities().unicode ? "\xE2\x80\xA6"
                                                         : "~";
        return l_Result;
      }

      static Util::String format_duration(double p_Seconds)
      {
        char l_Buffer[32];
        if (p_Seconds < 1.0) {
          snprintf(l_Buffer, sizeof(l_Buffer), "%dms",
                   static_cast<int>(p_Seconds * 1000.0));
        } else if (p_Seconds < 60.0) {
          snprintf(l_Buffer, sizeof(l_Buffer), "%.1fs", p_Seconds);
        } else {
          const int l_Total = static_cast<int>(p_Seconds);
          snprintf(l_Buffer, sizeof(l_Buffer), "%dm %02ds", l_Total / 60,
                   l_Total % 60);
        }
        return l_Buffer;
      }

      static double seconds_since(Clock::time_point p_Start)
      {
        return std::chrono::duration<double>(Clock::now() - p_Start)
            .count();
      }

      static Util::String to_string(int64_t p_Value)
      {
        char l_Buffer[32];
        snprintf(l_Buffer, sizeof(l_Buffer), "%lld",
                 static_cast<long long>(p_Value));
        return l_Buffer;
      }

      static LiveTask *find_task(const Util::String &p_Id)
      {
        for (LiveTask &i_Task : g_Tasks) {
          if (i_Task.id == p_Id) {
            return &i_Task;
          }
        }
        return nullptr;
      }

      // Symbol in front of a line. Unicode symbols like the warning sign
      // render wider than one column in many fonts, so they get more room.
      static Util::String icon(const char *p_Symbol, Color p_Color)
      {
        return Terminal::colorize(p_Symbol, p_Color) +
               (Terminal::get_capabilities().unicode ? "  " : " ");
      }

      // --- Live region (only used when animating) ---------------------
      //
      // The region is never cleared before redrawing, that would flash.
      // Instead the cursor moves back to its first line and every line is
      // overwritten, each followed by "clear to end of line". Whatever is
      // left below the new frame is cleared at the end.

      static void rewind_live(Util::String &p_Frame)
      {
        if (g_LiveLineCount == 0) {
          return;
        }

        p_Frame += "\r";
        if (g_LiveLineCount > 1) {
          p_Frame += "\x1b[" + to_string(g_LiveLineCount - 1) + "A";
        }
        g_LiveLineCount = 0;
      }

      static Util::String pad_left(const Util::String &p_Text,
                                   uint32_t p_Width)
      {
        const uint32_t l_TextWidth = display_width(p_Text);
        if (l_TextWidth >= p_Width) {
          return p_Text;
        }
        return Util::String(p_Width - l_TextWidth, ' ') + p_Text;
      }

      static Util::String pad_right(const Util::String &p_Text,
                                    uint32_t p_Width)
      {
        const uint32_t l_TextWidth = display_width(p_Text);
        if (l_TextWidth >= p_Width) {
          return p_Text;
        }
        return p_Text + Util::String(p_Width - l_TextWidth, ' ');
      }

      // Layout: spinner, label, bar or elapsed time, detail. Everything
      // before the detail has a fixed width per task, so the changing
      // detail text never moves the bar. p_LabelWidth is shared by all
      // live tasks to line up their bars.
      static Util::String render_task_line(const LiveTask &p_Task,
                                           uint32_t p_LabelWidth)
      {
        const Terminal::Symbols &l_Symbols = Terminal::get_symbols();

        const uint64_t l_Frame =
            static_cast<uint64_t>(seconds_since(g_StartTime) * 12.5);
        const char *l_Spinner =
            l_Symbols.spinner[l_Frame % l_Symbols.spinner.size()];

        const uint32_t l_Width = Terminal::get_width() - 1;

        Util::String l_Line =
            icon(l_Spinner, Color::CYAN) +
            pad_right(truncate(p_Task.label, p_LabelWidth), p_LabelWidth);
        uint32_t l_Used = 3 + p_LabelWidth;

        if (p_Task.total < 0) {
          const Util::String l_Elapsed =
              pad_left(format_duration(seconds_since(p_Task.start)), 7);
          l_Line += "  " + Terminal::colorize(l_Elapsed, Color::DIM);
          l_Used += 2 + display_width(l_Elapsed);
        } else {
          const uint32_t l_BarWidth = 20;
          const int64_t l_Done = p_Task.done < 0 ? 0 : p_Task.done;
          const uint32_t l_Filled =
              p_Task.total > 0
                  ? static_cast<uint32_t>(
                        (l_Done > p_Task.total ? p_Task.total : l_Done) *
                        l_BarWidth / p_Task.total)
                  : 0;

          Util::String l_Full;
          Util::String l_Empty;
          for (uint32_t i = 0; i < l_BarWidth; ++i) {
            if (i < l_Filled) {
              l_Full += l_Symbols.barFull;
            } else {
              l_Empty += l_Symbols.barEmpty;
            }
          }

          // Done is padded to the width of total so the count keeps its
          // width too
          const Util::String l_Total = to_string(p_Task.total);
          const Util::String l_Count =
              pad_left(to_string(l_Done), display_width(l_Total)) + "/" +
              l_Total;
          l_Line += "  " + Terminal::colorize(l_Full, Color::CYAN) +
                    Terminal::colorize(l_Empty, Color::DIM) + " " +
                    Terminal::colorize(l_Count, Color::DIM);
          l_Used += 3 + l_BarWidth + display_width(l_Count);
        }

        if (!p_Task.detail.empty() && l_Width > l_Used + 8) {
          l_Line += "  " + Terminal::colorize(
                               truncate(p_Task.detail, l_Width - l_Used - 2),
                               Color::DIM);
        }

        return l_Line;
      }

      static void draw_live(Util::String &p_Frame)
      {
        // Longest label, but at most a third of the terminal so the bars
        // still fit
        const uint32_t l_MaxLabelWidth = Terminal::get_width() / 3;
        uint32_t l_LabelWidth = 0;
        for (const LiveTask &i_Task : g_Tasks) {
          const uint32_t l_TaskWidth = display_width(i_Task.label);
          if (l_TaskWidth > l_LabelWidth) {
            l_LabelWidth = l_TaskWidth;
          }
        }
        if (l_LabelWidth > l_MaxLabelWidth) {
          l_LabelWidth = l_MaxLabelWidth;
        }

        for (size_t i = 0; i < g_Tasks.size(); ++i) {
          if (i > 0) {
            p_Frame += "\n";
          }
          p_Frame += render_task_line(g_Tasks[i], l_LabelWidth) + "\x1b[K";
        }
        p_Frame += "\x1b[J";
        g_LiveLineCount = static_cast<uint32_t>(g_Tasks.size());
      }

      // One write per frame. The ?2026 markers ask the terminal to show
      // the frame at once, terminals that don't know them ignore them.
      static void write_frame(const Util::String &p_Frame)
      {
        Terminal::write_stderr("\x1b[?2026h" + p_Frame + "\x1b[?2026l");
      }

      static void redraw_live()
      {
        Util::String l_Frame;
        rewind_live(l_Frame);
        draw_live(l_Frame);
        write_frame(l_Frame);
      }

      static void render_loop()
      {
        std::unique_lock<std::mutex> l_Lock(g_Mutex);
        while (g_RenderRunning) {
          g_RenderSignal.wait_for(l_Lock, std::chrono::milliseconds(50));
          if (!g_RenderRunning) {
            break;
          }
          if (!g_Tasks.empty()) {
            redraw_live();
          }
        }
      }

      // --- Line output ------------------------------------------------

      static void print_line(Util::StringView p_Line)
      {
        if (!g_Animate) {
          Util::String l_Line(p_Line.data(), p_Line.size());
          Terminal::write_stderr(l_Line + "\n");
          return;
        }

        // The line overwrites the live region, which is drawn again below
        Util::String l_Frame;
        rewind_live(l_Frame);
        for (char i_Char : p_Line) {
          if (i_Char == '\n') {
            l_Frame += "\x1b[K";
          }
          l_Frame += i_Char;
        }
        l_Frame += "\x1b[K\n";
        draw_live(l_Frame);
        write_frame(l_Frame);
      }

      static void print_block(const Util::String &p_Head,
                              const Util::List<Util::String> &p_Details,
                              const Util::String &p_Hint)
      {
        const Terminal::Symbols &l_Symbols = Terminal::get_symbols();

        Util::String l_Block = p_Head;
        for (const Util::String &i_Detail : p_Details) {
          l_Block += "\n  " + Terminal::colorize(l_Symbols.pipe, Color::DIM) +
                     " " + i_Detail;
        }
        if (!p_Hint.empty()) {
          l_Block +=
              "\n  " + Terminal::colorize("hint:", Color::CYAN) + " " + p_Hint;
        }
        print_line(l_Block);
      }

      // "── Label ──────" across the terminal, capped so it doesn't get
      // silly on wide terminals
      static Util::String render_section(const Util::String &p_Label)
      {
        const char *l_Rule = Terminal::get_symbols().rule;
        const uint32_t l_LineWidth = eastl::min(Terminal::get_width() - 1, 72u);

        const Util::String l_Label = truncate(p_Label, l_LineWidth - 8);
        const uint32_t l_Used = 4 + display_width(l_Label);

        Util::String l_Left;
        Util::String l_Right;
        for (uint32_t i = 0; i < 2; ++i) {
          l_Left += l_Rule;
        }
        for (uint32_t i = l_Used; i < l_LineWidth; ++i) {
          l_Right += l_Rule;
        }

        return Terminal::colorize(l_Left, Color::DIM) + " " +
               Terminal::colorize(l_Label, Color::BOLD) + " " +
               Terminal::colorize(l_Right, Color::DIM);
      }

      static bool is_visible(LogLevel p_Level)
      {
        switch (g_Config.verbosity) {
        case Verbosity::QUIET:
          return p_Level >= LogLevel::WARNING;
        case Verbosity::NORMAL:
          return p_Level >= LogLevel::DETAIL;
        case Verbosity::VERBOSE:
          return true;
        }
        return true;
      }

      static void render_log(const Event &p_Event)
      {
        const Terminal::Symbols &l_Symbols = Terminal::get_symbols();

        if (p_Event.level == LogLevel::WARNING) {
          g_Warnings.push_back(p_Event.message);
        } else if (p_Event.level == LogLevel::ERROR) {
          ++g_ErrorCount;
        }

        if (!is_visible(p_Event.level)) {
          return;
        }

        switch (p_Event.level) {
        case LogLevel::DEBUG:
          print_line("  " + Terminal::colorize(p_Event.message, Color::DIM));
          break;
        case LogLevel::DETAIL:
          print_line(
              "  " +
              Terminal::colorize(Util::String(l_Symbols.detail) + " " +
                                     p_Event.message,
                                 Color::DIM));
          break;
        case LogLevel::INFO:
          print_line(p_Event.message);
          break;
        case LogLevel::SUCCESS:
          print_line(icon(l_Symbols.success, Color::GREEN) + p_Event.message);
          break;
        case LogLevel::WARNING:
          print_block(icon(l_Symbols.warning, Color::YELLOW) + p_Event.message,
                      p_Event.details, p_Event.hint);
          break;
        case LogLevel::ERROR:
          print_block(icon(l_Symbols.error, Color::RED) +
                          Terminal::colorize(p_Event.message, Color::BOLD),
                      p_Event.details, p_Event.hint);
          break;
        }
      }

      static void render_task_end(const LiveTask &p_Task,
                                  TaskStatus p_Status)
      {
        const Terminal::Symbols &l_Symbols = Terminal::get_symbols();
        const Util::String l_Duration = Terminal::colorize(
            "(" + format_duration(seconds_since(p_Task.start)) + ")",
            Color::DIM);

        switch (p_Status) {
        case TaskStatus::OK:
          if (g_Config.verbosity != Verbosity::QUIET) {
            print_line(icon(l_Symbols.success, Color::GREEN) + p_Task.label +
                       " " + l_Duration);
          }
          break;
        case TaskStatus::FAILED:
          print_line(icon(l_Symbols.error, Color::RED) + p_Task.label + " " +
                     l_Duration);
          break;
        case TaskStatus::CANCELLED:
          print_line(icon(l_Symbols.warning, Color::YELLOW) + p_Task.label +
                     " " + Terminal::colorize("(cancelled)", Color::DIM));
          break;
        }
      }

      static void handle_task_begin(const Event &p_Event)
      {
        g_HadTasks = true;

        LiveTask *l_Existing = find_task(p_Event.id);
        LiveTask l_Task;
        l_Task.id = p_Event.id;
        l_Task.label = p_Event.label;
        l_Task.total = p_Event.total;
        l_Task.start = Clock::now();

        if (l_Existing) {
          *l_Existing = l_Task;
        } else {
          g_Tasks.push_back(l_Task);
        }

        if (g_Config.verbosity == Verbosity::QUIET) {
          return;
        }

        if (g_Animate) {
          if (g_Tasks.size() == 1) {
            Terminal::show_cursor(false);
          }
          redraw_live();
        } else {
          print_line(Terminal::colorize(
              Util::String(Terminal::get_symbols().detail) + " " +
                  p_Event.label + "...",
              Color::DIM));
        }
      }

      static void handle_task_update(const Event &p_Event)
      {
        LiveTask *l_Task = find_task(p_Event.id);
        if (!l_Task) {
          return;
        }

        if (p_Event.done >= 0) {
          l_Task->done = p_Event.done;
        }
        if (!p_Event.label.empty()) {
          l_Task->detail = p_Event.label;
        }

        // Without animation only report every quarter
        if (!g_Animate && g_Config.verbosity != Verbosity::QUIET &&
            l_Task->total > 0 && l_Task->done >= 0) {
          const int l_Quarter =
              static_cast<int>(l_Task->done * 4 / l_Task->total);
          if (l_Quarter > l_Task->lastReportedQuarter && l_Quarter < 4) {
            l_Task->lastReportedQuarter = l_Quarter;
            print_line(Terminal::colorize(
                "  " + l_Task->label + ": " + to_string(l_Quarter * 25) +
                    "%",
                Color::DIM));
          }
        }
      }

      static void handle_task_end(const Event &p_Event)
      {
        for (size_t i = 0; i < g_Tasks.size(); ++i) {
          if (g_Tasks[i].id != p_Event.id) {
            continue;
          }

          const LiveTask l_Task = g_Tasks[i];
          g_Tasks.erase(g_Tasks.begin() + i);

          // When animating print_line also redraws the remaining tasks
          render_task_end(l_Task, p_Event.status);
          if (g_Animate && g_Tasks.empty()) {
            Terminal::show_cursor(true);
          }
          return;
        }
      }

      static void emit_locked(const Event &p_Event)
      {
        if (g_Config.json) {
          if (p_Event.type == EventType::LOG) {
            if (p_Event.level == LogLevel::WARNING) {
              g_Warnings.push_back(p_Event.message);
            } else if (p_Event.level == LogLevel::ERROR) {
              ++g_ErrorCount;
            }
          }
          Terminal::write_stdout(Events::to_json(p_Event) + "\n");
          return;
        }

        switch (p_Event.type) {
        case EventType::SECTION:
          if (g_Config.verbosity != Verbosity::QUIET) {
            print_line("\n" + render_section(p_Event.label));
          }
          break;
        case EventType::LOG:
          render_log(p_Event);
          break;
        case EventType::TASK_BEGIN:
          handle_task_begin(p_Event);
          break;
        case EventType::TASK_UPDATE:
          handle_task_update(p_Event);
          break;
        case EventType::TASK_END:
          handle_task_end(p_Event);
          break;
        case EventType::RESULT: {
          const bool l_SharesScreen =
              g_Animate && Terminal::get_capabilities().stdoutIsTerminal;
          if (l_SharesScreen) {
            Util::String l_Frame;
            rewind_live(l_Frame);
            Terminal::write_stderr(l_Frame + "\x1b[J");
          }
          Terminal::write_stdout(p_Event.message + "\n");
          if (l_SharesScreen) {
            redraw_live();
          }
          break;
        }
        }
      }

      static void on_interrupt()
      {
        // try_lock: on Linux the signal can arrive while this thread holds
        // the mutex
        std::unique_lock<std::mutex> l_Lock(g_Mutex, std::try_to_lock);
        if (l_Lock.owns_lock()) {
          Util::String l_Frame;
          if (g_Animate) {
            rewind_live(l_Frame);
            l_Frame += "\x1b[J";
          }
          if (!g_Config.json) {
            l_Frame += icon(Terminal::get_symbols().error, Color::RED) +
                       "Cancelled\n";
          }
          Terminal::write_stderr(l_Frame);
        } else {
          Terminal::write_stderr("\n");
        }
      }

      // --- Public API -------------------------------------------------

      void initialize(const Config &p_Config)
      {
        g_Config = p_Config;
        g_StartTime = Clock::now();

        Terminal::initialize(p_Config.plain || p_Config.json,
                             p_Config.noColor);

        g_Animate = !p_Config.json && !p_Config.plain &&
                    p_Config.verbosity != Verbosity::QUIET &&
                    Terminal::get_capabilities().stderrIsTerminal;

        Terminal::add_interrupt_callback(on_interrupt);

        if (g_Animate) {
          g_RenderRunning = true;
          g_RenderThread = std::thread(render_loop);
        }
      }

      void finish(int p_ExitCode)
      {
        if (g_RenderRunning) {
          {
            std::lock_guard<std::mutex> l_Lock(g_Mutex);
            g_RenderRunning = false;
          }
          g_RenderSignal.notify_all();
          g_RenderThread.join();
        }

        std::lock_guard<std::mutex> l_Lock(g_Mutex);

        while (!g_Tasks.empty()) {
          Event l_Event;
          l_Event.type = EventType::TASK_END;
          l_Event.id = g_Tasks.back().id;
          l_Event.status = TaskStatus::CANCELLED;
          handle_task_end(l_Event);
        }

        if (g_Config.json || !g_HadTasks ||
            g_Config.verbosity == Verbosity::QUIET) {
          return;
        }

        const Terminal::Symbols &l_Symbols = Terminal::get_symbols();
        const Util::String l_Time = format_duration(seconds_since(g_StartTime));

        Util::String l_Summary = "\n";
        if (p_ExitCode == 0) {
          l_Summary += icon(l_Symbols.success, Color::GREEN) +
                       Terminal::colorize("Done", Color::BOLD) + " in " +
                       l_Time;
        } else {
          l_Summary += icon(l_Symbols.error, Color::RED) +
                       Terminal::colorize("Failed", Color::BOLD) +
                       " after " + l_Time;
        }

        if (!g_Warnings.empty()) {
          l_Summary += ", " + to_string(g_Warnings.size()) +
                       (g_Warnings.size() == 1 ? " warning" : " warnings");
        }
        print_line(l_Summary);

        for (const Util::String &i_Warning : g_Warnings) {
          print_line("  " + icon(l_Symbols.warning, Color::YELLOW) +
                     i_Warning);
        }
      }

      const Config &get_config()
      {
        return g_Config;
      }

      void emit(const Event &p_Event)
      {
        std::lock_guard<std::mutex> l_Lock(g_Mutex);
        emit_locked(p_Event);
      }

      void section(Util::StringView p_Label)
      {
        Event l_Event;
        l_Event.type = EventType::SECTION;
        l_Event.label.assign(p_Label.data(), p_Label.size());
        emit(l_Event);
      }

      static void emit_log(LogLevel p_Level,
                           Util::StringView p_Message,
                           const Util::List<Util::String> &p_Details,
                           Util::StringView p_Hint)
      {
        Event l_Event;
        l_Event.type = EventType::LOG;
        l_Event.level = p_Level;
        l_Event.message.assign(p_Message.data(), p_Message.size());
        l_Event.details = p_Details;
        l_Event.hint.assign(p_Hint.data(), p_Hint.size());
        emit(l_Event);
      }

      void debug(Util::StringView p_Message)
      {
        emit_log(LogLevel::DEBUG, p_Message, {}, "");
      }

      void detail(Util::StringView p_Message)
      {
        emit_log(LogLevel::DETAIL, p_Message, {}, "");
      }

      void info(Util::StringView p_Message)
      {
        emit_log(LogLevel::INFO, p_Message, {}, "");
      }

      void success(Util::StringView p_Message)
      {
        emit_log(LogLevel::SUCCESS, p_Message, {}, "");
      }

      void warn(Util::StringView p_Message,
                const Util::List<Util::String> &p_Details,
                Util::StringView p_Hint)
      {
        emit_log(LogLevel::WARNING, p_Message, p_Details, p_Hint);
      }

      void error(Util::StringView p_Message,
                 const Util::List<Util::String> &p_Details,
                 Util::StringView p_Hint)
      {
        emit_log(LogLevel::ERROR, p_Message, p_Details, p_Hint);
      }

      void result(Util::StringView p_Message)
      {
        Event l_Event;
        l_Event.type = EventType::RESULT;
        l_Event.message.assign(p_Message.data(), p_Message.size());
        emit(l_Event);
      }

      void task_begin(Util::StringView p_Id, Util::StringView p_Label,
                      int64_t p_Total)
      {
        Event l_Event;
        l_Event.type = EventType::TASK_BEGIN;
        l_Event.id.assign(p_Id.data(), p_Id.size());
        l_Event.label.assign(p_Label.data(), p_Label.size());
        l_Event.total = p_Total;
        emit(l_Event);
      }

      void task_update(Util::StringView p_Id, int64_t p_Done,
                       Util::StringView p_Label)
      {
        Event l_Event;
        l_Event.type = EventType::TASK_UPDATE;
        l_Event.id.assign(p_Id.data(), p_Id.size());
        l_Event.done = p_Done;
        l_Event.label.assign(p_Label.data(), p_Label.size());
        emit(l_Event);
      }

      void task_end(Util::StringView p_Id, TaskStatus p_Status)
      {
        Event l_Event;
        l_Event.type = EventType::TASK_END;
        l_Event.id.assign(p_Id.data(), p_Id.size());
        l_Event.status = p_Status;
        emit(l_Event);
      }

      Step::Step(Util::StringView p_Label)
      {
        m_Id = "step:" + to_string(++g_StepCounter);
        task_begin(m_Id, p_Label);
      }

      Step::~Step()
      {
        task_end(m_Id, m_Failed ? TaskStatus::FAILED : TaskStatus::OK);
      }

      void Step::fail()
      {
        m_Failed = true;
      }
    } // namespace Output
  } // namespace Cli
} // namespace Low
