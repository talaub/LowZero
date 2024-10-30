#include "LowRendererImGuiHelper.h"

#include "IconsFontAwesome5.h"
#include "IconsCodicons.h"
#include "IconsLucide.h"

#include "LowUtil.h"
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
            Util::get_project().dataPath + "/_internal/assets/fonts/";

        Util::String l_IconFontPath =
            l_BaseInternalFontPath + FONT_ICON_FILE_NAME_FAS;
        Util::String l_CommonFontPath =
            l_BaseInternalFontPath + "Roboto-Regular.ttf";

        Util::String l_CodiconFontPath =
            l_BaseInternalFontPath + FONT_ICON_FILE_NAME_CI;

        static const ImWchar l_IconRanges[] = {ICON_MIN_FA,
                                               ICON_MAX_16_FA, 0};

        static const ImWchar l_CodiconRanges[] = {ICON_MIN_CI,
                                                  ICON_MAX_16_CI, 0};

        ImGuiIO &io = ImGui::GetIO();

        ImVector<ImWchar> l_TextRanges;
        ImFontGlyphRangesBuilder l_TextRangeBuilder;
        // l_TextRangeBuilder.AddText("‡");
        l_TextRangeBuilder.AddRanges(
            io.Fonts->GetGlyphRangesDefault());
        l_TextRangeBuilder.BuildRanges(&l_TextRanges);

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
          float l_FontSize = 14.0f;

          ImFontConfig l_Config;
          l_Config.OversampleH = 2;
          l_Config.OversampleV = 1;
          l_Config.GlyphExtraSpacing.x = 0.7f;

          g_Fonts.common_350 = io.Fonts->AddFontFromFileTTF(
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
          Util::String l_FontPathLucide =
              l_BaseInternalFontPath + FONT_ICON_FILE_NAME_LC;

          static const ImWchar l_IconRangeLucide[] = {
              ICON_MIN_LC, ICON_MAX_16_LC, 0};

          {
            float l_FontSize = 40.0f;

            ImFontConfig l_Config;
            l_Config.PixelSnapH = true;
            l_Config.GlyphMinAdvanceX = l_FontSize;
            l_Config.MergeMode = true;
            l_Config.DstFont = g_Fonts.common_800;
            l_Config.GlyphOffset.y = 7.0f;

            g_Fonts.lucide_800 = io.Fonts->AddFontFromFileTTF(
                l_FontPathLucide.c_str(), l_FontSize, &l_Config,
                l_IconRangeLucide);
          }

          {
            float l_FontSize = 33.0f;

            ImFontConfig l_Config;
            l_Config.PixelSnapH = true;
            l_Config.GlyphMinAdvanceX = l_FontSize;
            l_Config.MergeMode = true;
            l_Config.DstFont = g_Fonts.common_500;
            l_Config.GlyphOffset.y = 4.0f;

            g_Fonts.lucide_700 = io.Fonts->AddFontFromFileTTF(
                l_FontPathLucide.c_str(), l_FontSize, &l_Config,
                l_IconRangeLucide);
          }

          {
            float l_FontSize = 28.0f;

            ImFontConfig l_Config;
            l_Config.PixelSnapH = true;
            l_Config.GlyphMinAdvanceX = l_FontSize;
            l_Config.MergeMode = true;
            // l_Config.DstFont = g_Fonts.common_500;
            l_Config.GlyphOffset.y = 10.0f;

            g_Fonts.lucide_690 = io.Fonts->AddFontFromFileTTF(
                l_FontPathLucide.c_str(), l_FontSize, &l_Config,
                l_IconRangeLucide);
          }

          {
            float l_FontSize = 25.0f;

            ImFontConfig l_Config;
            l_Config.PixelSnapH = true;
            l_Config.GlyphMinAdvanceX = l_FontSize;
            l_Config.MergeMode = true;
            l_Config.DstFont = g_Fonts.common_300;
            l_Config.GlyphOffset.y = 3.0f;

            g_Fonts.lucide_600 = io.Fonts->AddFontFromFileTTF(
                l_FontPathLucide.c_str(), l_FontSize, &l_Config,
                l_IconRangeLucide);
          }
        }

        {
          float l_FontSize = 40.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_800;
          l_Config.GlyphOffset.y = 7.0f;

          g_Fonts.codicon_800 = io.Fonts->AddFontFromFileTTF(
              l_CodiconFontPath.c_str(), l_FontSize, &l_Config,
              l_CodiconRanges);
        }

        {
          float l_FontSize = 33.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_500;
          l_Config.GlyphOffset.y = 3.0f;

          g_Fonts.codicon_700 = io.Fonts->AddFontFromFileTTF(
              l_CodiconFontPath.c_str(), l_FontSize, &l_Config,
              l_CodiconRanges);
        }

        {
          float l_FontSize = 25.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_300;
          l_Config.GlyphOffset.y = 3.0f;

          g_Fonts.codicon_600 = io.Fonts->AddFontFromFileTTF(
              l_CodiconFontPath.c_str(), l_FontSize, &l_Config,
              l_CodiconRanges);
        }

        {
          float l_FontSize = 40.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_800;

          g_Fonts.fa_800 = io.Fonts->AddFontFromFileTTF(
              l_IconFontPath.c_str(), l_FontSize, &l_Config,
              l_IconRanges);
        }

        {
          float l_FontSize = 35.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_300;

          g_Fonts.fa_700 = io.Fonts->AddFontFromFileTTF(
              l_IconFontPath.c_str(), l_FontSize, &l_Config,
              l_IconRanges);
        }

        {
          float l_FontSize = 25.0f;

          ImFontConfig l_Config;
          l_Config.PixelSnapH = true;
          l_Config.GlyphMinAdvanceX = l_FontSize;
          l_Config.MergeMode = true;
          l_Config.DstFont = g_Fonts.common_500;

          g_Fonts.fa_600 = io.Fonts->AddFontFromFileTTF(
              l_IconFontPath.c_str(), l_FontSize, &l_Config,
              l_IconRanges);
        }

        g_Fonts.icon_600 = g_Fonts.lucide_600;
        g_Fonts.icon_700 = g_Fonts.lucide_700;
        g_Fonts.icon_800 = g_Fonts.lucide_800;
      }

      Fonts &fonts()
      {
        return g_Fonts;
      }
    } // namespace ImGuiHelper
  }   // namespace Renderer
} // namespace Low
