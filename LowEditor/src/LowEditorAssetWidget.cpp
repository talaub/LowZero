#include "LowEditorAssetWidget.h"

#include "LowRendererImGuiHelper.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorGui.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"

#include "LowCoreMeshAsset.h"

namespace Low {
  namespace Editor {
    const Util::String g_CategoryLabels[] = {ICON_FA_CUBES " Meshes"};
    const uint32_t g_CategoryCount = 1;

    const float g_ElementSize = 128.0f;

    void render_mesh_asset_details_footer(Util::Handle p_Handle,
                                          Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::MeshAsset l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {

        Util::Yaml::Node l_Node;
        l_Asset.serialize(l_Node);
        Util::String l_Path = LOW_DATA_PATH;
        l_Path += "/assets/meshes/" + Util::String(l_Asset.get_name().c_str()) +
                  ".mesh.yaml";
        Util::Yaml::write_file(l_Path.c_str(), l_Node);

        LOW_LOG_INFO << "Saved mesh asset '" << l_Asset.get_name()
                     << "' to file." << LOW_LOG_END;
      }
    }

    AssetWidget::AssetWidget() : m_SelectedCategory(0)
    {
    }

    static bool render_element(uint32_t p_Id, Util::String p_Icon,
                               Util::String p_Label)
    {
      bool l_Result = false;

      ImGuiStyle &l_Style = ImGui::GetStyle();

      float l_Padding = l_Style.WindowPadding.x;

      ImGui::PushID(p_Id);
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      l_Result =
          ImGui::Button(p_Icon.c_str(), ImVec2(g_ElementSize, g_ElementSize));
      ImGui::PopFont();
      ImGui::TextWrapped(p_Label.c_str());

      ImGui::PopID();

      return l_Result;
    }

    void AssetWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_FILE " Assets");

      ImGui::BeginChild("Categories",
                        ImVec2(200, ImGui::GetContentRegionAvail().y), true, 0);
      for (uint32_t i = 0; i < g_CategoryCount; ++i) {
        if (ImGui::Selectable(g_CategoryLabels[i].c_str(),
                              i == m_SelectedCategory)) {
          m_SelectedCategory = i;
        }
      }

      ImGui::EndChild();

      ImGui::SameLine();

      ImGui::BeginChild("Content",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true);
      if (m_SelectedCategory >= 0 && m_SelectedCategory < g_CategoryCount) {
        switch (m_SelectedCategory) {
        case 0:
          render_meshes();
          break;
        }
      }

      ImGui::EndChild();

      ImGui::End();

      // ImGui::ShowDemoWindow();
    }

    void AssetWidget::render_meshes()
    {
      uint32_t l_Id = 0;

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      int l_Columns = LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (render_element(l_Id++, ICON_FA_PLUS, "Create mesh asset")) {
        ImGui::OpenPopup("Create mesh asset");
      }

      if (ImGui::BeginPopupModal("Create mesh asset")) {
        ImGui::Text(
            "You are about to create a new mesh asset. Please select a name.");
        static char l_NameBuffer[255];
        ImGui::InputText("##name", l_NameBuffer, 255);

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Create")) {
          Util::Name l_Name = LOW_NAME(l_NameBuffer);

          bool l_Ok = true;

          for (auto it = Core::MeshAsset::ms_LivingInstances.begin();
               it != Core::MeshAsset::ms_LivingInstances.end(); ++it) {
            if (l_Name == it->get_name()) {
              LOW_LOG_ERROR << "Cannot create mesh asset. The chosen name '"
                            << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::MeshAsset::make(l_Name);
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();

      for (auto it = Core::MeshAsset::ms_LivingInstances.begin();
           it != Core::MeshAsset::ms_LivingInstances.end(); ++it) {
        if (render_element(l_Id++, ICON_FA_CUBE, it->get_name().c_str())) {
          get_details_widget()->clear();
          HandlePropertiesSection i_Section(*it, true);
          i_Section.render_footer = &render_mesh_asset_details_footer;
          get_details_widget()->add_section(i_Section);
        }
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
    }
  } // namespace Editor
} // namespace Low
