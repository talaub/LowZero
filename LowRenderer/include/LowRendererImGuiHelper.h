#pragma once

#include "LowRendererApi.h"

#include "imgui.h"

namespace Low {
  namespace Renderer {
    namespace ImGuiHelper {
      struct Fonts
      {
        ImFont *common_300;
        ImFont *common_350;
        ImFont *common_500;
        ImFont *common_800;
        ImFont *icon_800;
      };

      void initialize();

      LOW_RENDERER_API Fonts &fonts();
    } // namespace ImGuiHelper
  }   // namespace Renderer
} // namespace Low
