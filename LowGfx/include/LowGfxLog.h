#pragma once

#include "LowMath.h"

namespace Low {
  namespace Gfx {
    enum class LogLevel : u8
    {
      Trace,
      Debug,
      Info,
      Warning,
      Error,
      Fatal
    };

    using LogCallback = void (*)(LogLevel p_Level,
                                 const char *p_Message,
                                 void *p_UserData);
  } // namespace Gfx
} // namespace Low
