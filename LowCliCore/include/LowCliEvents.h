#pragma once

#include "LowUtilContainers.h"

#include <stdint.h>

// One event model for everything that ends up on the screen: Output calls,
// NDJSON lines from child processes (LowLens, codegen) and --json output.
//
// Wire format, one JSON object per line:
//   {"type":"section","label":"Generating project"}
//   {"type":"log","level":"info","msg":"...","details":["..."],"hint":"..."}
//   {"type":"task_begin","id":"types","label":"Generating types","total":42}
//   {"type":"task_update","id":"types","done":10,"label":"Mesh"}
//   {"type":"task_end","id":"types","status":"ok"}
//   {"type":"result","msg":"..."}
// levels: debug, detail, info, success, warn, error
// status: ok, failed, cancelled

namespace Low {
  namespace Cli {
    enum class EventType
    {
      SECTION,
      LOG,
      TASK_BEGIN,
      TASK_UPDATE,
      TASK_END,
      RESULT
    };

    // Ordered by severity, the verbosity filter compares them
    enum class LogLevel
    {
      DEBUG,
      DETAIL,
      INFO,
      SUCCESS,
      WARNING,
      ERROR
    };

    enum class TaskStatus
    {
      OK,
      FAILED,
      CANCELLED
    };

    struct Event
    {
      EventType type = EventType::LOG;
      LogLevel level = LogLevel::INFO;
      TaskStatus status = TaskStatus::OK;

      Util::String id;
      Util::String label;
      Util::String message;
      Util::String hint;
      Util::List<Util::String> details;

      // -1 means unknown, which renders as a spinner instead of a bar
      int64_t done = -1;
      int64_t total = -1;
    };

    namespace Events {
      bool parse(Util::StringView p_Line, Event &p_Event);
      Util::String to_json(const Event &p_Event);
    } // namespace Events
  } // namespace Cli
} // namespace Low
