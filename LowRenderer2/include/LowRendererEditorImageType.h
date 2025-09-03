#pragma once

#include "LowRenderer2Api.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    enum class EditorImageType : u8
    {
      MESH,
      ICON,
      TEXTURE,
    };

    namespace EditorImageTypeEnumHelper {
      void LOW_RENDERER2_API initialize();
      void LOW_RENDERER2_API cleanup();

      Low::Util::Name LOW_RENDERER2_API
      entry_name(Low::Renderer::EditorImageType p_Value);
      Low::Util::Name LOW_RENDERER2_API _entry_name(uint8_t p_Value);

      Low::Renderer::EditorImageType LOW_RENDERER2_API
      entry_value(Low::Util::Name p_Name);
      uint8_t LOW_RENDERER2_API _entry_value(Low::Util::Name p_Name);

      u16 LOW_RENDERER2_API get_enum_id();

      u8 LOW_RENDERER2_API get_entry_count();
    } // namespace EditorImageTypeEnumHelper
  } // namespace Renderer
} // namespace Low
