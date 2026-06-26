#pragma once

#include "LowGfxToken.h"

namespace Low {
  namespace Gfx {
    struct CommandListTag;

    using CommandList = Token<CommandListTag>;

    enum class QueueRole : u8
    {
      Graphics,
      Compute,
      Transfer
    };

    enum class CommandListState : u8
    {
      Initial,
      Recording,
      Executable,
      Submitted
    };
  } // namespace Gfx
} // namespace Low
