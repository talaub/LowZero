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

        ImFont *icon_600;
        ImFont *icon_700;
        ImFont *icon_800;

        ImFont *fa_600;
        ImFont *fa_700;
        ImFont *fa_800;

        ImFont *codicon_600;
        ImFont *codicon_700;
        ImFont *codicon_800;

        ImFont *lucide_400;
        ImFont *lucide_600;
        ImFont *lucide_690;
        ImFont *lucide_700;
        ImFont *lucide_800;
      };

      void initialize();

      LOW_RENDERER_API Fonts &fonts();
    } // namespace ImGuiHelper
  } // namespace Renderer
} // namespace Low
