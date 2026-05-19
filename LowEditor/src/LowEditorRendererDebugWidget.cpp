#include "LowEditorRendererDebugWidget.h"

#include "LowEditorGui.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorIcons.h"
#include "LowEditorThemes.h"

#include "LowRendererRenderView.h"
#include "LowRendererTexture.h"
#include "LowUtil.h"
#include "LowUtilAssetManager.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"

#include <cstdio>
#include <imgui.h>

namespace Low {
  namespace Editor {
    namespace {
      struct TextureCard
      {
        const char *title;
        const char *subtitle;
        Renderer::Texture texture;
      };

      float get_render_view_aspect(Renderer::RenderView p_RenderView)
      {
        if (!p_RenderView.is_alive() ||
            p_RenderView.get_dimensions().y == 0) {
          return 16.0f / 9.0f;
        }

        return static_cast<float>(p_RenderView.get_dimensions().x) /
               static_cast<float>(p_RenderView.get_dimensions().y);
      }

      void render_section_header(const char *p_Title)
      {
        ImGui::Spacing();
        ImGui::TextUnformatted(p_Title);
        ImGui::Separator();
      }

      void render_empty_state(const char *p_Text)
      {
        const Theme &l_Theme = theme_get_current();
        ImVec2 l_Available = ImGui::GetContentRegionAvail();
        ImVec2 l_Size(l_Available.x, 120.0f);
        ImVec2 l_Min = ImGui::GetCursorScreenPos();
        ImVec2 l_Max(l_Min.x + l_Size.x, l_Min.y + l_Size.y);
        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();

        ImGui::Dummy(l_Size);

        l_DrawList->AddRectFilled(l_Min, l_Max,
                                  color_to_imcolor(l_Theme.input), 4.0f);
        l_DrawList->AddRect(l_Min, l_Max,
                            color_to_imcolor(l_Theme.border), 4.0f);

        ImVec2 l_TextSize = ImGui::CalcTextSize(p_Text);
        ImVec2 l_TextPosition(l_Min.x + (l_Size.x - l_TextSize.x) * 0.5f,
                              l_Min.y + (l_Size.y - l_TextSize.y) * 0.5f);
        l_DrawList->AddText(l_TextPosition,
                            color_to_imcolor(l_Theme.textDisabled),
                            p_Text);
      }

      void render_texture_card(const TextureCard &p_Card,
                               const float p_Width,
                               const float p_Aspect,
                               const float p_MaxPreviewHeight,
                               const bool p_Emphasized)
      {
        const Theme &l_Theme = theme_get_current();
        const float l_Padding = 8.0f;
        const float l_HeaderHeight = p_Emphasized ? 44.0f : 40.0f;
        const float l_PreviewWidth = p_Width - (l_Padding * 2.0f);
        float l_PreviewHeight = l_PreviewWidth / p_Aspect;
        if (l_PreviewHeight > p_MaxPreviewHeight) {
          l_PreviewHeight = p_MaxPreviewHeight;
        }
        if (l_PreviewHeight < 72.0f) {
          l_PreviewHeight = 72.0f;
        }

        const float l_CardHeight =
            l_HeaderHeight + l_PreviewHeight + (l_Padding * 2.0f);
        const float l_ItemHeight = l_CardHeight + 8.0f;
        const ImVec2 l_CardSize(p_Width, l_ItemHeight);
        const ImVec2 l_Min = ImGui::GetCursorScreenPos();
        const ImVec2 l_Max(l_Min.x + p_Width, l_Min.y + l_CardHeight);

        ImGui::PushID(p_Card.title);
        ImGui::InvisibleButton("##texture-card", l_CardSize);
        const bool l_Hovered = ImGui::IsItemHovered();

        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
        const ImColor l_Background =
            l_Hovered ? color_to_imcolor(l_Theme.headerHover)
                      : color_to_imcolor(l_Theme.input);

        l_DrawList->AddRectFilled(l_Min, l_Max, l_Background, 4.0f);
        l_DrawList->AddRect(l_Min, l_Max,
                            color_to_imcolor(l_Theme.border), 4.0f);

        const ImVec2 l_TitleMin(l_Min.x + l_Padding,
                                l_Min.y + l_Padding);
        l_DrawList->AddText(l_TitleMin, color_to_imcolor(l_Theme.text),
                            p_Card.title);
        l_DrawList->AddText(
            ImVec2(l_TitleMin.x, l_TitleMin.y + ImGui::GetTextLineHeight()),
            color_to_imcolor(l_Theme.subtext), p_Card.subtitle);

        const ImVec2 l_PreviewMin(l_Min.x + l_Padding,
                                  l_Min.y + l_HeaderHeight);
        const ImVec2 l_PreviewMax(l_PreviewMin.x + l_PreviewWidth,
                                  l_PreviewMin.y + l_PreviewHeight);

        l_DrawList->AddRectFilled(l_PreviewMin, l_PreviewMax,
                                  color_to_imcolor(l_Theme.base), 3.0f);
        l_DrawList->AddRect(l_PreviewMin, l_PreviewMax,
                            color_to_imcolor(l_Theme.border), 3.0f);

        Renderer::Texture l_Texture = p_Card.texture;
        if (l_Texture.is_imgui_texture_ready()) {
          l_DrawList->AddImage(
              l_Texture.get_gpu().get_imgui_texture_id(),
              ImVec2(l_PreviewMin.x + 1.0f, l_PreviewMin.y + 1.0f),
              ImVec2(l_PreviewMax.x - 1.0f, l_PreviewMax.y - 1.0f));
        } else {
          const char *l_Text = "Texture not ready";
          const ImVec2 l_TextSize = ImGui::CalcTextSize(l_Text);
          const ImVec2 l_TextPosition(
              l_PreviewMin.x +
                  ((l_PreviewMax.x - l_PreviewMin.x) - l_TextSize.x) *
                      0.5f,
              l_PreviewMin.y +
                  ((l_PreviewMax.y - l_PreviewMin.y) - l_TextSize.y) *
                      0.5f);
          l_DrawList->AddText(l_TextPosition,
                              color_to_imcolor(l_Theme.textDisabled),
                              l_Text);
        }

        ImGui::PopID();
      }

      void render_texture_grid(const TextureCard *p_Cards,
                               const uint32_t p_Count,
                               const float p_Aspect,
                               const bool p_Emphasized)
      {
        const float l_Gap = 8.0f;
        const float l_AvailableWidth = ImGui::GetContentRegionAvail().x;
        uint32_t l_Columns = 1;
        if (l_AvailableWidth > 900.0f) {
          l_Columns = 3;
        } else if (l_AvailableWidth > 560.0f) {
          l_Columns = 2;
        }

        const float l_Width =
            (l_AvailableWidth - (l_Gap * (l_Columns - 1))) /
            static_cast<float>(l_Columns);
        const float l_MaxPreviewHeight = p_Emphasized ? 260.0f : 165.0f;

        for (uint32_t i = 0; i < p_Count; ++i) {
          if ((i % l_Columns) != 0) {
            ImGui::SameLine(0.0f, l_Gap);
          }

          render_texture_card(p_Cards[i], l_Width, p_Aspect,
                              l_MaxPreviewHeight, p_Emphasized);
        }
      }
    } // namespace

    void RendererDebugWidget::render(float p_Delta)
    {
      ImGui::Begin("Renderer Debug", &m_Open);

      PropertyEditors::render_handle_selector(
          "Render View", Renderer::RenderView::type_id(),
          (u64 *)&m_RenderView);

      ImGui::Spacing();

      if (!m_RenderView.is_alive()) {
        render_empty_state("Select a RenderView to inspect its buffers.");
        ImGui::End();
        return;
      }

      const Math::UVector2 l_Dimensions = m_RenderView.get_dimensions();
      char l_DimensionText[64];
      snprintf(l_DimensionText, sizeof(l_DimensionText), "%ux%u",
               l_Dimensions.x, l_Dimensions.y);

      const TextureCard l_FinalImages[] = {
          {"Tonemapped", l_DimensionText,
           m_RenderView.get_tonemapped_image()},
          {"Lit HDR", l_DimensionText, m_RenderView.get_lit_image()}};

      const TextureCard l_GBufferImages[] = {
          {"Albedo", l_DimensionText, m_RenderView.get_gbuffer_albedo()},
          {"Normals", l_DimensionText, m_RenderView.get_gbuffer_normals()},
          {"View Position", l_DimensionText,
           m_RenderView.get_gbuffer_viewposition()},
          {"Depth", l_DimensionText, m_RenderView.get_gbuffer_depth()}};

      render_section_header(m_RenderView.get_name().c_str());
      render_texture_grid(l_FinalImages, 2, get_render_view_aspect(m_RenderView),
                          true);

      render_section_header("GBuffer");
      render_texture_grid(l_GBufferImages, 4,
                          get_render_view_aspect(m_RenderView), false);

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
