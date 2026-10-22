#pragma once

#include "LowCliEvents.h"

// Status and progress go to stderr, results to stdout. With --json every
// call is written as an NDJSON event to stdout instead of being rendered.

namespace Low {
  namespace Cli {
    namespace Output {
      enum class Verbosity
      {
        QUIET,
        NORMAL,
        VERBOSE
      };

      struct Config
      {
        Verbosity verbosity = Verbosity::NORMAL;
        bool plain = false;
        bool json = false;
        bool noColor = false;
      };

      void initialize(const Config &p_Config);
      // Prints the end summary if any step or task ran
      void finish(int p_ExitCode);

      const Config &get_config();

      void emit(const Event &p_Event);

      void section(Util::StringView p_Label);

      void debug(Util::StringView p_Message);
      void detail(Util::StringView p_Message);
      void info(Util::StringView p_Message);
      void success(Util::StringView p_Message);
      void warn(Util::StringView p_Message,
                const Util::List<Util::String> &p_Details = {},
                Util::StringView p_Hint = "");
      void error(Util::StringView p_Message,
                 const Util::List<Util::String> &p_Details = {},
                 Util::StringView p_Hint = "");

      void result(Util::StringView p_Message);

      // p_Total < 0 shows a spinner, otherwise a progress bar
      void task_begin(Util::StringView p_Id, Util::StringView p_Label,
                      int64_t p_Total = -1);
      void task_update(Util::StringView p_Id, int64_t p_Done,
                       Util::StringView p_Label = "");
      void task_end(Util::StringView p_Id,
                    TaskStatus p_Status = TaskStatus::OK);

      // Scoped task: spinner while alive, ends with ✔ and the duration
      // unless fail() was called
      class Step
      {
      public:
        explicit Step(Util::StringView p_Label);
        ~Step();

        Step(const Step &) = delete;
        Step &operator=(const Step &) = delete;

        void fail();

      private:
        Util::String m_Id;
        bool m_Failed = false;
      };
    } // namespace Output
  } // namespace Cli
} // namespace Low
