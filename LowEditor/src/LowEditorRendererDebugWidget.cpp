#include "LowEditorRendererDebugWidget.h"

#include "LowEditorGui.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorIcons.h"
#include "LowEditorThemes.h"

#include "LowRendererDrawCommand.h"
#include "LowRendererPointLight.h"
#include "LowRendererRenderScene.h"
#include "LowRendererRenderView.h"
#include "LowRendererTextureState.h"
#include "LowRendererTexture.h"
#include "LowRendererUiCanvas.h"
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

      const char *bool_text(bool p_Value)
      {
        return p_Value ? "Yes" : "No";
      }

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

      void render_kv_row(const char *p_Label, const char *p_Value)
      {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("%s", p_Label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(p_Value);
      }

      void render_kv_row_u32(const char *p_Label, u32 p_Value)
      {
        char l_Buffer[32];
        snprintf(l_Buffer, sizeof(l_Buffer), "%u", p_Value);
        render_kv_row(p_Label, l_Buffer);
      }

      void render_kv_row_u64(const char *p_Label, u64 p_Value)
      {
        char l_Buffer[32];
        snprintf(l_Buffer, sizeof(l_Buffer), "%llu",
                 static_cast<unsigned long long>(p_Value));
        render_kv_row(p_Label, l_Buffer);
      }

      void render_kv_row_f32(const char *p_Label, float p_Value)
      {
        char l_Buffer[32];
        snprintf(l_Buffer, sizeof(l_Buffer), "%.3f", p_Value);
        render_kv_row(p_Label, l_Buffer);
      }

      void render_kv_row_vec3(const char *p_Label,
                              Math::Vector3 p_Value)
      {
        char l_Buffer[96];
        snprintf(l_Buffer, sizeof(l_Buffer), "%.3f, %.3f, %.3f",
                 p_Value.x, p_Value.y, p_Value.z);
        render_kv_row(p_Label, l_Buffer);
      }

      void render_kv_row_color(const char *p_Label,
                               Math::ColorRGB p_Value)
      {
        char l_Buffer[96];
        snprintf(l_Buffer, sizeof(l_Buffer), "%.3f, %.3f, %.3f",
                 p_Value.r, p_Value.g, p_Value.b);
        render_kv_row(p_Label, l_Buffer);
      }

      void render_kv_row_dims(const char *p_Label,
                              Math::UVector2 p_Value)
      {
        char l_Buffer[64];
        snprintf(l_Buffer, sizeof(l_Buffer), "%ux%u", p_Value.x,
                 p_Value.y);
        render_kv_row(p_Label, l_Buffer);
      }

      bool begin_kv_table(const char *p_Id)
      {
        if (ImGui::BeginTable(
                p_Id, 2,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_SizingStretchProp)) {
          ImGui::TableSetupColumn(
              "Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
          ImGui::TableSetupColumn("Value");
          return true;
        }

        return false;
      }

      void end_kv_table()
      {
        ImGui::EndTable();
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

        l_DrawList->AddRectFilled(
            l_Min, l_Max, color_to_imcolor(l_Theme.input), 4.0f);
        l_DrawList->AddRect(l_Min, l_Max,
                            color_to_imcolor(l_Theme.border), 4.0f);

        ImVec2 l_TextSize = ImGui::CalcTextSize(p_Text);
        ImVec2 l_TextPosition(
            l_Min.x + (l_Size.x - l_TextSize.x) * 0.5f,
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
        l_DrawList->AddText(
            l_TitleMin, color_to_imcolor(l_Theme.text), p_Card.title);
        l_DrawList->AddText(
            ImVec2(l_TitleMin.x,
                   l_TitleMin.y + ImGui::GetTextLineHeight()),
            color_to_imcolor(l_Theme.subtext), p_Card.subtitle);

        const ImVec2 l_PreviewMin(l_Min.x + l_Padding,
                                  l_Min.y + l_HeaderHeight);
        const ImVec2 l_PreviewMax(l_PreviewMin.x + l_PreviewWidth,
                                  l_PreviewMin.y + l_PreviewHeight);
        l_DrawList->AddRectFilled(l_PreviewMin, l_PreviewMax,
                                  color_to_imcolor(l_Theme.base),
                                  3.0f);
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
        const float l_AvailableWidth =
            ImGui::GetContentRegionAvail().x;
        uint32_t l_Columns = 1;
        if (l_AvailableWidth > 900.0f) {
          l_Columns = 3;
        } else if (l_AvailableWidth > 560.0f) {
          l_Columns = 2;
        }

        const float l_Width =
            (l_AvailableWidth - (l_Gap * (l_Columns - 1))) /
            static_cast<float>(l_Columns);
        const float l_MaxPreviewHeight =
            p_Emphasized ? 260.0f : 165.0f;

        for (uint32_t i = 0; i < p_Count; ++i) {
          if ((i % l_Columns) != 0) {
            ImGui::SameLine(0.0f, l_Gap);
          }

          render_texture_card(p_Cards[i], l_Width, p_Aspect,
                              l_MaxPreviewHeight, p_Emphasized);
        }
      }

      void collect_texture_cards(Renderer::RenderView p_RenderView,
                                 TextureCard *p_FinalImages,
                                 TextureCard *p_GBufferImages,
                                 const char *p_DimensionText)
      {
        p_FinalImages[0] = {"Tonemapped", p_DimensionText,
                            p_RenderView.get_tonemapped_image()};
        p_FinalImages[1] = {"Lit HDR", p_DimensionText,
                            p_RenderView.get_lit_image()};

        p_GBufferImages[0] = {"Albedo", p_DimensionText,
                              p_RenderView.get_gbuffer_albedo()};
        p_GBufferImages[1] = {"Normals", p_DimensionText,
                              p_RenderView.get_gbuffer_normals()};
        p_GBufferImages[2] = {
            "View Position", p_DimensionText,
            p_RenderView.get_gbuffer_viewposition()};
        p_GBufferImages[3] = {"Depth", p_DimensionText,
                              p_RenderView.get_gbuffer_depth()};
      }

      void render_images_tab(Renderer::RenderView p_RenderView)
      {
        const Math::UVector2 l_Dimensions =
            p_RenderView.get_dimensions();
        char l_DimensionText[64];
        snprintf(l_DimensionText, sizeof(l_DimensionText), "%ux%u",
                 l_Dimensions.x, l_Dimensions.y);

        TextureCard l_FinalImages[2];
        TextureCard l_GBufferImages[4];
        collect_texture_cards(p_RenderView, l_FinalImages,
                              l_GBufferImages, l_DimensionText);

        render_section_header(p_RenderView.get_name().c_str());
        render_texture_grid(l_FinalImages, 2,
                            get_render_view_aspect(p_RenderView),
                            true);

        render_section_header("GBuffer");
        render_texture_grid(l_GBufferImages, 4,
                            get_render_view_aspect(p_RenderView),
                            false);
      }

      void render_texture_row(const char *p_Label,
                              Renderer::Texture p_Texture)
      {
        Renderer::Texture l_Texture = p_Texture;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(p_Label);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(l_Texture.is_alive()
                                   ? l_Texture.get_name().c_str()
                                   : "-");
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(bool_text(l_Texture.is_alive()));
        ImGui::TableSetColumnIndex(3);
        ImGui::TextUnformatted(
            bool_text(l_Texture.is_imgui_texture_ready()));
        ImGui::TableSetColumnIndex(4);
        ImGui::TextUnformatted(
            l_Texture.is_alive()
                ? Renderer::TextureStateEnumHelper::entry_name(
                      l_Texture.get_state())
                      .c_str()
                : "-");
      }

      void render_inspector_tab(Renderer::RenderView p_RenderView)
      {
        Renderer::RenderScene l_RenderScene =
            p_RenderView.get_render_scene();

        render_section_header("RenderView");
        if (begin_kv_table("RenderViewInspector")) {
          render_kv_row("Name", p_RenderView.get_name().c_str());
          render_kv_row_u64("Id", p_RenderView.get_id());
          render_kv_row_dims("Dimensions",
                             p_RenderView.get_dimensions());
          render_kv_row_vec3("Camera position",
                             p_RenderView.get_camera_position());
          render_kv_row_vec3("Camera direction",
                             p_RenderView.get_camera_direction());
          render_kv_row_f32("Camera FOV",
                            p_RenderView.get_camera_fov());
          render_kv_row("Render scene",
                        l_RenderScene.is_alive()
                            ? l_RenderScene.get_name().c_str()
                            : "-");
          end_kv_table();
        }

        render_section_header("Textures");
        if (ImGui::BeginTable("RenderViewTextures", 5,
                              ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable)) {
          ImGui::TableSetupColumn("Target");
          ImGui::TableSetupColumn("Texture");
          ImGui::TableSetupColumn("Alive");
          ImGui::TableSetupColumn("Imgui");
          ImGui::TableSetupColumn("State");
          ImGui::TableHeadersRow();

          render_texture_row("Albedo",
                             p_RenderView.get_gbuffer_albedo());
          render_texture_row("Normals",
                             p_RenderView.get_gbuffer_normals());
          render_texture_row("Depth",
                             p_RenderView.get_gbuffer_depth());
          render_texture_row("View position",
                             p_RenderView.get_gbuffer_viewposition());
          render_texture_row("Object map",
                             p_RenderView.get_object_map());
          render_texture_row("Highlight map",
                             p_RenderView.get_highlight_map());
          render_texture_row("Lit HDR", p_RenderView.get_lit_image());
          render_texture_row("Tonemapped",
                             p_RenderView.get_tonemapped_image());
          render_texture_row("SSGI", p_RenderView.get_ssgi_image());
          render_texture_row("Blurred",
                             p_RenderView.get_blurred_image());
          render_texture_row("SSAO", p_RenderView.get_ssao_image());
          render_texture_row("Cavities",
                             p_RenderView.get_cavities_image());
          render_texture_row("Shadow atlas",
                             p_RenderView.get_shadow_atlas());

          ImGui::EndTable();
        }

        render_section_header("Render Steps");
        if (ImGui::BeginTable("RenderViewSteps", 2,
                              ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable)) {
          ImGui::TableSetupColumn("Step");
          ImGui::TableSetupColumn("Data");
          ImGui::TableHeadersRow();

          Util::List<Renderer::RenderStep> &l_Steps =
              p_RenderView.get_steps();
          Util::List<Renderer::RenderStepDataPtr> &l_StepData =
              p_RenderView.get_step_data();
          for (u32 i = 0u; i < l_Steps.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(l_Steps[i].is_alive()
                                       ? l_Steps[i].get_name().c_str()
                                       : "-");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(
                i < l_StepData.size() && l_StepData[i] ? "Ready"
                                                       : "-");
          }

          ImGui::EndTable();
        }
      }

      void render_lights_tab(Renderer::RenderView p_RenderView)
      {
        Renderer::RenderScene l_RenderScene =
            p_RenderView.get_render_scene();
        if (!l_RenderScene.is_alive()) {
          render_empty_state(
              "Selected RenderView has no RenderScene.");
          return;
        }

        render_section_header("Directional Light");
        if (begin_kv_table("DirectionalLightInspector")) {
          render_kv_row_vec3(
              "Direction",
              l_RenderScene.get_directional_light_direction());
          render_kv_row_color(
              "Color", l_RenderScene.get_directional_light_color());
          render_kv_row_f32(
              "Intensity",
              l_RenderScene.get_directional_light_intensity());
          end_kv_table();
        }

        render_section_header("Point Lights");
        if (ImGui::BeginTable("PointLights", 6,
                              ImGuiTableFlags_BordersInnerV |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable)) {
          ImGui::TableSetupColumn("Name");
          ImGui::TableSetupColumn("Slot");
          ImGui::TableSetupColumn("Position");
          ImGui::TableSetupColumn("Color");
          ImGui::TableSetupColumn("Intensity");
          ImGui::TableSetupColumn("Range");
          ImGui::TableHeadersRow();

          u32 l_Count = 0u;
          for (u32 i = 0u; i < Renderer::PointLight::living_count();
               ++i) {
            Renderer::PointLight l_PointLight =
                Renderer::PointLight::living_instances()[i];
            if (!l_PointLight.is_alive() ||
                l_PointLight.get_render_scene_handle() !=
                    l_RenderScene.get_id()) {
              continue;
            }

            char l_PositionText[96];
            char l_ColorText[96];
            Math::Vector3 l_Position =
                l_PointLight.get_world_position();
            Math::ColorRGB l_Color = l_PointLight.get_color();
            snprintf(l_PositionText, sizeof(l_PositionText),
                     "%.2f, %.2f, %.2f", l_Position.x, l_Position.y,
                     l_Position.z);
            snprintf(l_ColorText, sizeof(l_ColorText),
                     "%.2f, %.2f, %.2f", l_Color.r, l_Color.g,
                     l_Color.b);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(l_PointLight.get_name().c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", l_PointLight.get_slot());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(l_PositionText);
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(l_ColorText);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.3f", l_PointLight.get_intensity());
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.3f", l_PointLight.get_range());
            ++l_Count;
          }

          if (l_Count == 0u) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled(
                "No point lights in this RenderScene");
          }

          ImGui::EndTable();
        }
      }

      void render_stats_tab(Renderer::RenderView p_RenderView)
      {
        Renderer::RenderScene l_RenderScene =
            p_RenderView.get_render_scene();

        render_section_header("Frame Contents");
        if (begin_kv_table("RendererStats")) {
          render_kv_row_u32(
              "Render steps",
              static_cast<u32>(p_RenderView.get_steps().size()));
          render_kv_row_u32(
              "Draw commands",
              l_RenderScene.is_alive()
                  ? static_cast<u32>(
                        l_RenderScene.get_draw_commands().size())
                  : 0u);
          render_kv_row_u32(
              "UI canvases",
              static_cast<u32>(
                  p_RenderView.get_ui_canvases().size()));

          u32 l_PointLightCount = 0u;
          if (l_RenderScene.is_alive()) {
            for (u32 i = 0u; i < Renderer::PointLight::living_count();
                 ++i) {
              Renderer::PointLight l_PointLight =
                  Renderer::PointLight::living_instances()[i];
              if (l_PointLight.is_alive() &&
                  l_PointLight.get_render_scene_handle() ==
                      l_RenderScene.get_id()) {
                ++l_PointLightCount;
              }
            }
          }
          render_kv_row_u32("Point lights", l_PointLightCount);
          end_kv_table();
        }

        render_lights_tab(p_RenderView);
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
        render_empty_state(
            "Select a RenderView to inspect its buffers.");
        ImGui::End();
        return;
      }

      if (ImGui::BeginTabBar("RendererDebugTabs")) {
        if (ImGui::BeginTabItem("Images")) {
          render_images_tab(m_RenderView);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Inspector")) {
          render_inspector_tab(m_RenderView);
          ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stats")) {
          render_stats_tab(m_RenderView);
          ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
