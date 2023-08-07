#include "LowEditorAssetWidget.h"

#include "LowRendererImGuiHelper.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowEditorGui.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"

#include "LowCoreMeshAsset.h"
#include "LowCoreMaterial.h"
#include "LowCorePrefab.h"

#include "LowUtilString.h"
#include "LowUtilFileIO.h"
#include <algorithm>
#include <cstring>
#include <string>

#define UPDATE_INTERVAL 3.0f

namespace Low {
  namespace Editor {
    const Util::String g_CategoryLabels[] = {ICON_FA_CUBES " Meshes",
                                             ICON_FA_SPRAY_CAN " Materials",
                                             ICON_FA_BOX_OPEN " Prefabs"};
    const uint32_t g_CategoryCount = 3;

    const float g_ElementSize = 128.0f;

    Util::Map<Util::Handle, Util::String> g_Paths;

    void render_meshes(AssetTypeConfig &, ImRect);
    void render_materials(AssetTypeConfig &, ImRect);
    void render_prefabs(AssetTypeConfig &, ImRect);

    static void save_mesh_asset(Util::Handle p_Handle)
    {
      Core::MeshAsset l_Asset = p_Handle.get_id();

      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path = LOW_DATA_PATH;
      l_Path += "/assets/meshes/" + LOW_TO_STRING(l_Asset.get_unique_id()) +
                ".mesh.yaml";

      if (g_Paths.find(p_Handle) != g_Paths.end()) {
        l_Path = g_Paths[p_Handle];
      }

      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved mesh asset '" << l_Asset.get_name() << "' to file."
                   << LOW_LOG_END;
    }

    void AssetWidget::save_prefab_asset(Util::Handle p_Handle)
    {
      Core::Prefab l_Asset = p_Handle.get_id();

      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path = LOW_DATA_PATH;
      l_Path += "/assets/prefabs/" + LOW_TO_STRING(l_Asset.get_unique_id()) +
                ".prefab.yaml";

      if (g_Paths.find(p_Handle) != g_Paths.end()) {
        l_Path = g_Paths[p_Handle];
      }

      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved prefab '" << l_Asset.get_name() << "' to file."
                   << LOW_LOG_END;
    }

    void AssetWidget::save_material_asset(Util::Handle p_Handle)
    {
      Core::Material l_Asset = p_Handle.get_id();
      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path = LOW_DATA_PATH;
      l_Path += "/assets/materials/" + LOW_TO_STRING(l_Asset.get_unique_id()) +
                ".material.yaml";
      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved material '" << l_Asset.get_name() << "' to file."
                   << LOW_LOG_END;
    }

    static void update_directory_list(AssetTypeConfig &p_Config)
    {
      p_Config.elements.clear();
      Util::List<Util::String> l_FilePaths;

      Util::FileIO::list_directory(p_Config.currentPath.c_str(), l_FilePaths);

      Util::String l_Ending = p_Config.suffix;

      for (Util::String &i_Path : l_FilePaths) {
        FileElement i_Element;
        i_Element.directory = Util::FileIO::is_directory(i_Path.c_str());

        Util::String i_Filename =
            i_Path.substr(p_Config.currentPath.length() + 1, i_Path.length());

        if (i_Element.directory) {
          i_Element.handle = 0;
          i_Element.name = i_Filename;
          p_Config.elements.push_back(i_Element);
        } else if (Util::StringHelper::ends_with(i_Path, p_Config.suffix)) {
          Util::String i_Name =
              i_Filename.substr(0, i_Filename.length() - l_Ending.length());

          i_Element.handle =
              Util::find_handle_by_unique_id(std::stoull(i_Name.c_str()));

          Util::RTTI::TypeInfo &i_TypeInfo =
              Util::Handle::get_type_info(i_Element.handle.get_type());
          i_Element.name = ((Util::Name *)i_TypeInfo.properties[N(name)].get(
                                i_Element.handle))
                               ->c_str();

          g_Paths[i_Element.handle] = i_Path;
          p_Config.elements.push_back(i_Element);
        }
      }

      std::sort(
          p_Config.elements.begin(), p_Config.elements.end(),
          [](const FileElement &a, const FileElement &b) {
            Util::String aName = a.name;
            Util::String bName = b.name;

            std::transform(aName.begin(), aName.end(), aName.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            std::transform(bName.begin(), bName.end(), bName.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            return aName < bName;
          });

      p_Config.update = false;
    }

    void render_material_details_footer(Util::Handle p_Handle,
                                        Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::Material l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {
        AssetWidget::save_material_asset(p_Handle);
      }
    }

    void render_mesh_asset_details_footer(Util::Handle p_Handle,
                                          Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::MeshAsset l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {
        save_mesh_asset(p_Handle);
      }
    }

    void render_prefab_details_footer(Util::Handle p_Handle,
                                      Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::Prefab l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {
        AssetWidget::save_prefab_asset(p_Handle);
      }
    }

    AssetWidget::AssetWidget()
        : m_SelectedCategory(0), m_UpdateCounter(UPDATE_INTERVAL)
    {
      Util::String l_DataPath = LOW_DATA_PATH;
      l_DataPath += "/assets/";

      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::MeshAsset::TYPE_ID;
        l_Config.subfolder = false;
        l_Config.rootPath = l_DataPath + "meshes";
        l_Config.currentPath = l_Config.rootPath;
        l_Config.render = &render_meshes;
        l_Config.suffix = ".mesh.yaml";

        m_TypeConfigs.push_back(l_Config);
      }
      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::Material::TYPE_ID;
        l_Config.subfolder = false;
        l_Config.rootPath = l_DataPath + "materials";
        l_Config.currentPath = l_Config.rootPath;
        l_Config.render = &render_materials;
        l_Config.suffix = ".material.yaml";

        m_TypeConfigs.push_back(l_Config);
      }
      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::Prefab::TYPE_ID;
        l_Config.subfolder = false;
        l_Config.rootPath = l_DataPath + "prefabs";
        l_Config.currentPath = l_Config.rootPath;
        l_Config.render = &render_prefabs;
        l_Config.suffix = ".prefab.yaml";

        m_TypeConfigs.push_back(l_Config);
      }
    }

    static bool render_directory(uint32_t p_Id, Util::String p_Icon,
                                 Util::String p_Label, bool p_Draggable,
                                 Util::String p_Path, AssetTypeConfig &p_Config)
    {
      bool l_Result = false;

      ImGuiStyle &l_Style = ImGui::GetStyle();

      float l_Padding = l_Style.WindowPadding.x;

      ImGui::PushID(p_Id);
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      l_Result =
          ImGui::Button(p_Icon.c_str(), ImVec2(g_ElementSize, g_ElementSize));
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Util::Handle l_PayloadHandle = *(uint64_t *)l_Payload->Data;

          if (g_Paths.find(l_PayloadHandle) != g_Paths.end()) {
            Util::RTTI::TypeInfo &l_TypeInfo =
                Util::Handle::get_type_info(p_Config.typeId);
            if (l_TypeInfo.is_alive(l_PayloadHandle)) {
              Util::String l_Destination =
                  p_Path + "/" +
                  LOW_TO_STRING(
                      *(uint64_t *)l_TypeInfo.properties[N(unique_id)].get(
                          l_PayloadHandle)) +
                  p_Config.suffix;
              Util::FileIO::move_sync(g_Paths[l_PayloadHandle].c_str(),
                                      l_Destination.c_str());
              p_Config.update = true;
            }
          }
        }
        ImGui::EndDragDropTarget();
      }
      ImGui::PopFont();
      ImGui::TextWrapped(p_Label.c_str());

      ImGui::PopID();

      return l_Result;
    }

    static bool render_element(uint32_t p_Id, Util::String p_Icon,
                               Util::String p_Label, bool p_Draggable,
                               Util::Handle p_Handle)
    {
      bool l_Result = false;

      ImGuiStyle &l_Style = ImGui::GetStyle();

      float l_Padding = l_Style.WindowPadding.x;

      ImGui::PushID(p_Id);
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      l_Result =
          ImGui::Button(p_Icon.c_str(), ImVec2(g_ElementSize, g_ElementSize));
      if (p_Draggable && p_Handle.get_id() > 0) {
        if (ImGui::BeginPopupContextItem()) {
          set_selected_handle(p_Handle);
          if (ImGui::MenuItem("Delete")) {
            Util::RTTI::TypeInfo &l_TypeInfo =
                Util::Handle::get_type_info(p_Handle.get_type());
            l_TypeInfo.destroy(p_Handle);

            if (g_Paths.find(p_Handle) != g_Paths.end()) {
              Util::String l_Path = g_Paths[p_Handle];
              if (Util::FileIO::file_exists_sync(l_Path.c_str())) {
                Util::FileIO::delete_sync(l_Path.c_str());
              }
            }
          }
          ImGui::EndPopup();
        }
      }
      if (p_Draggable) {
        Gui::drag_handle(p_Handle);
      }
      ImGui::PopFont();
      ImGui::TextWrapped(p_Label.c_str());

      ImGui::PopID();

      return l_Result;
    }

    void AssetWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_FILE " Assets");

      m_UpdateCounter += p_Delta;
      if (m_UpdateCounter > UPDATE_INTERVAL ||
          m_TypeConfigs[m_SelectedCategory].update) {
        m_UpdateCounter = 0.0f;
        AssetTypeConfig &l_CurrentConfig = m_TypeConfigs[m_SelectedCategory];
        update_directory_list(m_TypeConfigs[m_SelectedCategory]);
      }

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

      ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
      ImRect l_Rect(l_Cursor, {l_Cursor.x + ImGui::GetContentRegionAvail().x,
                               l_Cursor.y + ImGui::GetContentRegionAvail().y});

      ImGui::BeginChild("Content",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true);
      if (m_SelectedCategory >= 0 && m_SelectedCategory < g_CategoryCount) {
        m_TypeConfigs[m_SelectedCategory].render(
            m_TypeConfigs[m_SelectedCategory], l_Rect);
      }

      ImGui::EndChild();

      ImGui::End();

      // ImGui::ShowDemoWindow();
    }

    static void render_meshes(AssetTypeConfig &p_Config, ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      int l_Columns = LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (render_element(l_Id++, ICON_FA_PLUS, "Create mesh asset", false, 0)) {
        ImGui::OpenPopup("Create mesh asset");
      }

      if (ImGui::BeginPopupModal("Create mesh asset")) {
        ImGui::Text("You are about to create a new mesh asset. Please select "
                    "a name.");
        static char l_NameBuffer[255];
        ImGui::InputText("##name", l_NameBuffer, 255);

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        bool l_IsEnter =
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));

        if (ImGui::Button("Create") || l_IsEnter) {
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
            Core::MeshAsset l_Asset = Core::MeshAsset::make(l_Name);
            p_Config.update = true;

            g_Paths[l_Asset] = p_Config.currentPath + "/" +
                               LOW_TO_STRING(l_Asset.get_unique_id()) +
                               p_Config.suffix;
            save_mesh_asset(l_Asset);
            set_selected_handle(l_Asset);
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();

      if (p_Config.subfolder) {
        Util::List<Util::String> l_Parts;
        Util::StringHelper::split(p_Config.currentPath, '/', l_Parts);

        Util::String l_Path = "";
        for (int i = 0; i < l_Parts.size(); ++i) {
          if (i) {
            l_Path += "/";
          }
          l_Path += l_Parts[i];
        }

        if (render_directory(l_Id++, ICON_FA_FOLDER_OPEN, "back", false, l_Path,
                             p_Config)) {
          p_Config.currentPath = l_Path;
          p_Config.subfolder = p_Config.currentPath != p_Config.rootPath;
          p_Config.update = true;
        }
        ImGui::NextColumn();
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        if (it->directory) {
          if (render_directory(l_Id++, ICON_FA_FOLDER, it->name, true,
                               p_Config.currentPath + "/" + it->name,
                               p_Config)) {
            p_Config.subfolder = true;
            p_Config.currentPath += "/" + it->name;
            p_Config.update = true;
          }
          ImGui::NextColumn();
        }
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        if (!it->directory) {
          if (render_element(l_Id++, ICON_FA_CUBE, it->name, true,
                             it->handle)) {
            set_selected_handle(it->handle);
            HandlePropertiesSection i_Section(it->handle, true);
            i_Section.render_footer = &render_mesh_asset_details_footer;
            get_details_widget()->add_section(i_Section);
          }
          ImGui::NextColumn();
        }
      }

      ImGui::Columns(1);
    }

    static void render_materials(AssetTypeConfig &p_Config, ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      int l_Columns = LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (render_element(l_Id++, ICON_FA_PLUS, "Create material", false, 0)) {
        ImGui::OpenPopup("Create material");
      }

      if (ImGui::BeginPopupModal("Create material")) {
        ImGui::Text(
            "You are about to create a new material. Please select a name.");
        static char l_NameBuffer[255];
        ImGui::InputText("##name", l_NameBuffer, 255);

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }

        bool l_IsEnter =
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));
        ImGui::SameLine();
        if (ImGui::Button("Create") || l_IsEnter) {
          Util::Name l_Name = LOW_NAME(l_NameBuffer);

          bool l_Ok = true;

          for (auto it = Core::Material::ms_LivingInstances.begin();
               it != Core::Material::ms_LivingInstances.end(); ++it) {
            if (l_Name == it->get_name()) {
              LOW_LOG_ERROR << "Cannot create material. The chosen name '"
                            << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::Material l_NewMaterial = Core::Material::make(l_Name);
            AssetWidget::save_material_asset(l_NewMaterial.get_id());
            p_Config.update = true;
            set_selected_handle(l_NewMaterial);
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();

      if (p_Config.subfolder) {
        Util::List<Util::String> l_Parts;
        Util::StringHelper::split(p_Config.currentPath, '/', l_Parts);

        Util::String l_Path = "";
        for (int i = 0; i < l_Parts.size(); ++i) {
          if (i) {
            l_Path += "/";
          }
          l_Path += l_Parts[i];
        }

        if (render_directory(l_Id++, ICON_FA_FOLDER_OPEN, "back", false, l_Path,
                             p_Config)) {
          p_Config.currentPath = l_Path;
          p_Config.subfolder = p_Config.currentPath != p_Config.rootPath;
          p_Config.update = true;
        }
        ImGui::NextColumn();
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        if (it->directory) {
          if (render_directory(l_Id++, ICON_FA_FOLDER, it->name, true,
                               p_Config.currentPath + "/" + it->name,
                               p_Config)) {
            p_Config.subfolder = true;
            p_Config.currentPath += "/" + it->name;
            p_Config.update = true;
          }
          ImGui::NextColumn();
        }
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        if (!it->directory) {
          if (render_element(l_Id++, ICON_FA_SPRAY_CAN, it->name, true,
                             it->handle)) {
            set_selected_handle(it->handle);
            HandlePropertiesSection i_Section(it->handle, true);
            i_Section.render_footer = &render_mesh_asset_details_footer;
            get_details_widget()->add_section(i_Section);
          }
          ImGui::NextColumn();
        }
      }

      ImGui::Columns(1);
    }

    static void render_prefabs(AssetTypeConfig &p_Config, ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      if (ImGui::BeginDragDropTargetCustom(p_Bounds, 34839)) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {
            Core::Prefab l_Prefab = Core::Prefab::make(l_Entity);

            g_Paths[l_Prefab] = p_Config.currentPath + "/" +
                                LOW_TO_STRING(l_Prefab.get_unique_id()) +
                                p_Config.suffix;
            AssetWidget::save_prefab_asset(l_Prefab);
            p_Config.update = true;
          }
        }
        ImGui::EndDragDropTarget();
      }

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      int l_Columns = LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (p_Config.subfolder) {
        Util::List<Util::String> l_Parts;
        Util::StringHelper::split(p_Config.currentPath, '/', l_Parts);

        Util::String l_Path = "";
        for (int i = 0; i < l_Parts.size(); ++i) {
          if (i) {
            l_Path += "/";
          }
          l_Path += l_Parts[i];
        }

        if (render_directory(l_Id++, ICON_FA_FOLDER_OPEN, "back", false, l_Path,
                             p_Config)) {
          p_Config.currentPath = l_Path;
          p_Config.subfolder = p_Config.currentPath != p_Config.rootPath;
          p_Config.update = true;
        }
        ImGui::NextColumn();
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        if (it->directory) {
          if (render_directory(l_Id++, ICON_FA_FOLDER, it->name, true,
                               p_Config.currentPath + "/" + it->name,
                               p_Config)) {
            p_Config.subfolder = true;
            p_Config.currentPath += "/" + it->name;
            p_Config.update = true;
          }
          ImGui::NextColumn();
        }
      }

      for (auto it = p_Config.elements.begin(); it != p_Config.elements.end();
           ++it) {
        Core::Prefab i_Prefab = it->handle.get_id();
        if (!it->directory &&
            !Core::Prefab(i_Prefab.get_parent().get_id()).is_alive()) {
          if (render_element(l_Id++, ICON_FA_BOX_OPEN, it->name, true,
                             it->handle)) {
            set_selected_handle(it->handle);
            HandlePropertiesSection i_Section(it->handle, true);
            i_Section.render_footer = &render_prefab_details_footer;
            get_details_widget()->add_section(i_Section);
          }
          ImGui::NextColumn();
        }
      }

      ImGui::Columns(1);
    }
  } // namespace Editor
} // namespace Low

#undef UPDATE_INTERVAL
