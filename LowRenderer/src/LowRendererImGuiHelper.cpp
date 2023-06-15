#include "LowRendererImGuiHelper.h"

#include "IconsFontAwesome5.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

namespace Low {
  namespace Renderer {
    namespace ImGuiHelper {
      Fonts g_Fonts;

      void initialize()
      {
        Util::String l_BaseInternalFontPath =
            Util::String(LOW_DATA_PATH) + "/_internal/assets/fonts/";

        Util::String l_IconFontPath =
            l_BaseInternalFontPath + FONT_ICON_FILE_NAME_FAS;
        Util::String l_CommonFontPath =
            l_BaseInternalFontPath + "Roboto-Regular.ttf";

        static const ImWchar l_IconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};

        ImGuiIO &io = ImGui::GetIO();

        {
          float l_FontSize = 12.0f;

          ImFontConfig l_Config;
          l_Config.OversampleH = 2;
          l_Config.OversampleV = 1;
          l_Config.GlyphExtraSpacing.x = 0.7f;

          g_Fonts.common_300 = io.Fonts->AddFontFromFileTTF(
              l_CommonFontPath.c_str(), l_FontSize, &l_Config);
        }
        {
          float l_FontSize = 19.0f;

          ImFontConfig l_Config;
          l_Config.OversampleH = 2;
          l_Config.OversampleV = 1;
          l_Config.GlyphExtraSpacing.x = 0.7f;

          g_Fonts.common_500 = io.Fonts->AddFontFromFileTTF(
              l_CommonFontPath.c_str(), l_FontSize, &l_Config);
        }
        {
          float l_FontSize = 40.0f;

          ImFontConfig l_Config;
          l_Config.OversampleH = 2;
          l_Config.OversampleV = 1;
          l_Config.GlyphExtraSpacing.x = 0.7f;

          g_Fonts.common_800 = io.Fonts->AddFontFromFileTTF(
              l_CommonFontPath.c_str(), l_FontSize, &l_Config);
        }

        {
          float l_FontSize = 40.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_800;

          g_Fonts.icon_800 = io.Fonts->AddFontFromFileTTF(
              l_IconFontPath.c_str(), l_FontSize, &l_Config, l_IconRanges);
        }
      }

      Fonts &fonts()
      {
        return g_Fonts;
      }
    } // namespace ImGuiHelper
  }   // namespace Renderer
} // namespace Low
