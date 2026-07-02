#pragma once

#include "LowGfxToken.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Gfx {
    struct CommandListTag;
    struct GpuFenceTag;
    struct GpuSemaphoreTag;

    using CommandList = Token<CommandListTag>;
    using GpuFence = Token<GpuFenceTag>;
    using GpuSemaphore = Token<GpuSemaphoreTag>;

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

    enum class PipelineStage : u32
    {
      None = 0,
      TopOfPipe = 1 << 0,
      DrawIndirect = 1 << 1,
      VertexInput = 1 << 2,
      VertexShader = 1 << 3,
      FragmentShader = 1 << 4,
      ColorAttachment = 1 << 5,
      ComputeShader = 1 << 6,
      Transfer = 1 << 7,
      BottomOfPipe = 1 << 8,
      AllGraphics = 1 << 9,
      AllCommands = 1 << 10
    };

    inline PipelineStage operator|(PipelineStage p_Left,
                                  PipelineStage p_Right)
    {
      return static_cast<PipelineStage>(static_cast<u32>(p_Left) |
                                        static_cast<u32>(p_Right));
    }

    inline PipelineStage operator&(PipelineStage p_Left,
                                  PipelineStage p_Right)
    {
      return static_cast<PipelineStage>(static_cast<u32>(p_Left) &
                                        static_cast<u32>(p_Right));
    }

    inline PipelineStage &operator|=(PipelineStage &p_Left,
                                    PipelineStage p_Right)
    {
      p_Left = p_Left | p_Right;
      return p_Left;
    }

    struct SubmitWait
    {
      GpuSemaphore semaphore;
      PipelineStage stage = PipelineStage::AllCommands;
    };

    struct SubmitDesc
    {
      QueueRole queue = QueueRole::Graphics;
      Util::Span<const CommandList> command_lists;
      Util::Span<const SubmitWait> waits;
      Util::Span<const GpuSemaphore> signals;
    };
  } // namespace Gfx
} // namespace Low
