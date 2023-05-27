#pragma once

#include "LowUtilEnums.h"

#define SYSTEM_ON_START(name)                                                  \
  {                                                                            \
    static bool _has_started = false;                                          \
    if (!_has_started) {                                                       \
      name();                                                                  \
      _has_started = true;                                                     \
    }                                                                          \
  }

namespace Low {
  namespace Core {
    namespace System {
      typedef void (*TickCallback)(float, Util::EngineState);
    } // namespace System
  }   // namespace Core
} // namespace Low
