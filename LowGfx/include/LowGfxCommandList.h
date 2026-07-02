#pragma once

#include "LowGfxToken.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Gfx {
    struct CommandListTag;
    struct GpuFenceTag;

    using CommandList = Token<CommandListTag>;
    using GpuFence = Token<GpuFenceTag>;

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

    struct SubmitDesc
    {
      QueueRole queue = QueueRole::Graphics;
      Util::Span<const CommandList> command_lists;
    };
  } // namespace Gfx
} // namespace Low
