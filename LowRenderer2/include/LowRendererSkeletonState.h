#pragma once

#include "LowRenderer2Api.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    enum class SkeletonState : u8
    {
      UNKNOWN,
      UNLOADED,
      SCHEDULEDTOLOAD,
      LOADINGTOMEMORY,
      LOADED,
    };

    namespace SkeletonStateEnumHelper {
      void LOW_RENDERER2_API initialize();
      void LOW_RENDERER2_API cleanup();

      Low::Util::Name LOW_RENDERER2_API
      entry_name(Low::Renderer::SkeletonState p_Value);
      Low::Util::Name LOW_RENDERER2_API _entry_name(uint8_t p_Value);

      Low::Renderer::SkeletonState LOW_RENDERER2_API
      entry_value(Low::Util::Name p_Name);
      uint8_t LOW_RENDERER2_API _entry_value(Low::Util::Name p_Name);

      u16 LOW_RENDERER2_API get_enum_id();

      u8 LOW_RENDERER2_API get_entry_count();
    } // namespace SkeletonStateEnumHelper
  } // namespace Renderer
} // namespace Low
