#pragma once

#include "LowGfxToken.h"

namespace Low {
  namespace Gfx {
    struct BufferTag;

    using Buffer = Token<BufferTag>;

    enum class BufferUsage : u32
    {
      None = 0,
      Vertex = 1 << 0,
      Index = 1 << 1,
      Uniform = 1 << 2,
      Storage = 1 << 3,
      TransferSrc = 1 << 4,
      TransferDst = 1 << 5,
      Indirect = 1 << 6
    };

    enum class BufferMemoryUsage : u8
    {
      GpuOnly,
      CpuToGpu,
      GpuToCpu
    };

    inline BufferUsage operator|(BufferUsage p_Left,
                                 BufferUsage p_Right)
    {
      return static_cast<BufferUsage>(static_cast<u32>(p_Left) |
                                      static_cast<u32>(p_Right));
    }

    inline BufferUsage operator&(BufferUsage p_Left,
                                 BufferUsage p_Right)
    {
      return static_cast<BufferUsage>(static_cast<u32>(p_Left) &
                                      static_cast<u32>(p_Right));
    }

    inline BufferUsage &operator|=(BufferUsage &p_Left,
                                   BufferUsage p_Right)
    {
      p_Left = p_Left | p_Right;
      return p_Left;
    }

    struct BufferDesc
    {
      u64 size = 0;
      BufferUsage usage = BufferUsage::None;
      BufferMemoryUsage memory_usage = BufferMemoryUsage::GpuOnly;
      const char *debug_name = nullptr;
    };

    struct BufferCopyRegion
    {
      u64 src_offset = 0;
      u64 dst_offset = 0;
      u64 size = 0;
    };
  } // namespace Gfx
} // namespace Low
