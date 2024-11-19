#pragma once

#include "LowUtilEnums.h"

#define SYSTEM_ON_START(name)                                        \
  {                                                                  \
    static bool _has_started = false;                                \
    if (!_has_started) {                                             \
      name();                                                        \
      _has_started = true;                                           \
    }                                                                \
  }

#define SYSTEM_ON_START_PLAYING(name, state)                         \
  {                                                                  \
    static bool _has_started = false;                                \
    if (!_has_started && state == Low::Util::EngineState::PLAYING) { \
      name();                                                        \
      _has_started = true;                                           \
    }                                                                \
    if (state != Low::Util::EngineState::PLAYING) {                  \
      _has_started = false;                                          \
    }                                                                \
  }

namespace Low {
  namespace Core {
    namespace System {
      typedef void (*TickCallback)(float, Util::EngineState);
    } // namespace System
  }   // namespace Core
} // namespace Low
