#pragma once

#include "LowMath.h"

namespace Low {
  namespace Gfx {
    struct Adapter
    {
      static constexpr u32 INVALID_INDEX = LOW_UINT32_MAX;
      static constexpr u32 INVALID_OWNER = 0;

      u32 index = INVALID_INDEX;
      u32 generation = 0;
      u32 owner_id = INVALID_OWNER;

      explicit operator bool() const
      {
        return index != INVALID_INDEX;
      }

      auto operator<=>(const Adapter &) const = default;
    };

    enum class PowerPreference : u8
    {
      Default,
      LowPower,
      HighPerformance
    };
  } // namespace Gfx
} // namespace Low
