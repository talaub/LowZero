#pragma once

#include "LowRenderer2Api.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    enum class AnimationClipState : u8
    {
      UNKNOWN,
      UNLOADED,
      SCHEDULEDTOLOAD,
      LOADINGTOMEMORY,
      LOADED,
    };

    namespace AnimationClipStateEnumHelper {
      void LOW_RENDERER2_API initialize();
      void LOW_RENDERER2_API cleanup();

      Low::Util::Name LOW_RENDERER2_API
      entry_name(Low::Renderer::AnimationClipState p_Value);
      Low::Util::Name LOW_RENDERER2_API _entry_name(uint8_t p_Value);

      Low::Renderer::AnimationClipState LOW_RENDERER2_API
      entry_value(Low::Util::Name p_Name);
      uint8_t LOW_RENDERER2_API _entry_value(Low::Util::Name p_Name);

      u16 LOW_RENDERER2_API get_enum_id();

      u8 LOW_RENDERER2_API get_entry_count();
    } // namespace AnimationClipStateEnumHelper
  } // namespace Renderer
} // namespace Low
