#pragma once

#include "LowRenderer2Api.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    enum class ImageResourceState : u8
    {
      UNKNOWN,
      UNLOADED,
      SCHEDULEDTOLOAD,
      LOADINGTOMEMORY,
      MEMORYLOADED,
      UPLOADINGTOGPU,
      LOADED,
    };

    namespace ImageResourceStateEnumHelper {
      void LOW_RENDERER2_API initialize();
      void LOW_RENDERER2_API cleanup();

      Low::Util::Name LOW_RENDERER2_API
      entry_name(Low::Renderer::ImageResourceState p_Value);
      Low::Util::Name LOW_RENDERER2_API _entry_name(uint8_t p_Value);

      Low::Renderer::ImageResourceState LOW_RENDERER2_API
      entry_value(Low::Util::Name p_Name);
      uint8_t LOW_RENDERER2_API _entry_value(Low::Util::Name p_Name);

      u16 LOW_RENDERER2_API get_enum_id();

      u8 LOW_RENDERER2_API get_entry_count();
    } // namespace ImageResourceStateEnumHelper
  }   // namespace Renderer
} // namespace Low
