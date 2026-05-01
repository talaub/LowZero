#include "LowEditorSimpleAssetEditors.h"

#include "LowCore.h"
#include "LowEditor.h"
#include "LowEditorFonts.h"

#include "LowEditorThemes.h"
#include "LowEditorViewport.h"
#include "LowMath.h"
#include "LowRendererMesh.h"
#include "LowRendererTexture.h"
#include "LowRendererFont.h"
#include "LowRendererTextureState.h"
#include "LowRendererResourceManager.h"
#include "LowUtilHashing.h"
#include "LowUtilLogger.h"
#include <imgui.h>

namespace Low {
  namespace Editor {
    static void draw_header(const Util::String &p_Name,
                            const Util::String &p_Type,
                            const AssetType p_AssetType,
                            float p_Height = 72.0f)
    {
      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
      ImGuiStyle &l_Style = ImGui::GetStyle();

      const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
      const float l_Width = ImGui::GetContentRegionAvail().x;

      const float l_Rounding = 10.0f;
      const float l_AccentHeight = 4.0f;
      const float l_PaddingX = 14.0f;
      const float l_PaddingTop = 2.0f;
      const float l_NameOffsetY = 10.0f;
      const float l_TypeOffsetY = 42.0f;

      const ImVec2 l_Min = l_Pos;
      const ImVec2 l_Max =
          ImVec2(l_Pos.x + l_Width, l_Pos.y + p_Height);

      const ImU32 l_BgColor =
          ImGui::GetColorU32(ImVec4(0.11f, 0.12f, 0.13f, 1.0f));
      const ImU32 l_BorderColor =
          ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.06f));
      const ImU32 l_AccentU32 =
          color_to_imcolor(get_color_for_asset_type(p_AssetType));
      const ImU32 l_TypeColor =
          ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.58f));

      // Background panel
      l_DrawList->AddRectFilled(l_Min, l_Max, l_BgColor, l_Rounding);
      l_DrawList->AddRect(l_Min, l_Max, l_BorderColor, l_Rounding);

      // Horizontal accent strip at the top
      l_DrawList->AddRectFilled(
          l_Min, ImVec2(l_Max.x, l_Min.y + l_AccentHeight),
          l_AccentU32, l_Rounding, ImDrawFlags_RoundCornersTop);

      // Optional subtle separator near bottom
      l_DrawList->AddLine(
          ImVec2(l_Min.x + 1.0f, l_Max.y - 1.0f),
          ImVec2(l_Max.x - 1.0f, l_Max.y - 1.0f),
          ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.04f)));

      const ImVec2 l_IconPos = ImVec2(l_Min.x + l_PaddingX,
                                      l_Min.y + l_PaddingTop + 10.0f);

      const ImVec2 l_TextPos = ImVec2(l_Min.x + l_PaddingX + 60.0f,
                                      l_Min.y + l_PaddingTop);

      const ImVec2 l_IconDim = ImVec2(50, 50);

      l_DrawList->AddImage(
          get_editor_image_for_asset_type(p_AssetType)
              .get_gpu()
              .get_imgui_texture_id(),
          l_IconPos, l_IconPos + l_IconDim);

      // Name
      ImGui::SetCursorScreenPos(
          ImVec2(l_TextPos.x, l_TextPos.y + l_NameOffsetY));
      ImGui::PushFont(Low::Editor::Fonts::UI(23.0f));
      ImGui::TextUnformatted(p_Name.c_str());
      ImGui::PopFont();

      // Type
      ImGui::SetCursorScreenPos(
          ImVec2(l_TextPos.x, l_TextPos.y + l_TypeOffsetY));
      ImGui::PushStyleColor(ImGuiCol_Text, l_TypeColor);
      ImGui::TextUnformatted(p_Type.c_str());
      ImGui::PopStyleColor();
      ImGui::Dummy(ImVec2(0, 10));
    }

    MeshAssetEditor::MeshAssetEditor(Util::Handle p_Handle)
        : TypeEditor(p_Handle)
    {
      m_MeshViewer =
          new MeshViewer(p_Handle.get_id(), Math::UVector2(500, 500));
    }

    MeshAssetEditor::~MeshAssetEditor()
    {
      delete m_MeshViewer;
    }

    void MeshAssetEditor::render(const float p_Delta)
    {
      Renderer::Mesh l_Mesh = m_Handle.get_id();

      draw_header(l_Mesh.get_name().c_str(), "Mesh", AssetType::Mesh);

      show_line("ID", Util::hash_to_string(
                          l_Mesh.get_resource().get_mesh_id()));

      {
        Util::StringBuilder l_Line;
        l_Line.append(l_Mesh.get_submesh_count());
        show_line("Submeshes", l_Line.get());
      }

      show_line("Source file",
                l_Mesh.get_resource().get_source_file());

      const ImVec2 l_Avail = ImGui::GetContentRegionAvail();

      m_MeshViewer->tick(LOW_DELTA_TIME);

      m_MeshViewer->set_dimensions(l_Avail.x, l_Avail.y);
    }

    void TextureAssetEditor::render(const float p_Delta)
    {
      Renderer::Texture l_Texture = m_Handle.get_id();

      draw_header(l_Texture.get_name().c_str(), "Texture",
                  AssetType::Texture);

      show_line("ID", Util::hash_to_string(
                          l_Texture.get_resource().get_texture_id()));

      {
        Util::StringBuilder l_Line;
        if (l_Texture.get_state() == Renderer::TextureState::LOADED) {
          l_Line.append(l_Texture.get_gpu().get_full_mip_count());
        } else {
          l_Line.append("Unknown (unloaded)");
        }
        show_line("Mipcount", l_Line.get());
      }

      show_line("Source file",
                l_Texture.get_resource().get_source_file());

      Renderer::EditorImage l_EditorImage =
          l_Texture.get_editor_image();
      const ImVec2 l_Avail = ImGui::GetContentRegionAvail();
      if (l_EditorImage.is_alive()) {
        if (l_EditorImage.get_state() ==
            Renderer::TextureState::UNLOADED) {
          Renderer::ResourceManager::load_editor_image(l_EditorImage);
        }
        if (l_EditorImage.get_state() ==
            Renderer::TextureState::LOADED) {
          ImGui::Image(l_EditorImage.get_gpu().get_imgui_texture_id(),
                       ImVec2(l_Avail.x, l_Avail.y));
        }
      }
    }

    void FontAssetEditor::render(const float p_Delta)
    {
      Renderer::Font l_Font = m_Handle.get_id();

      draw_header(l_Font.get_name().c_str(), "Font", AssetType::Font);

      show_line("ID", Util::hash_to_string(
                          l_Font.get_resource().get_font_id()));

      show_line("Source file",
                l_Font.get_resource().get_source_file());
    }
  } // namespace Editor
} // namespace Low
