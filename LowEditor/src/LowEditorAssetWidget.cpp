#include "LowEditorAssetWidget.h"

#include "LowRendererImGuiHelper.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include "IconsCodicons.h"

#include "LowEditor.h"
#include "LowEditorGui.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorFlodeWidget.h"
#include "LowEditorBase.h"

#include "LowCore.h"
#include "LowCoreMeshAsset.h"
#include "LowCoreMaterial.h"
#include "LowCorePrefab.h"

#include "LowUtil.h"
#include "LowUtilString.h"
#include "LowUtilFileIO.h"

#include <algorithm>
#include <cstring>
#include <string>

#define UPDATE_INTERVAL 3.0f

namespace Low {
  namespace Editor {
    const Util::String g_CategoryLabels[] = {
        ICON_LC_BOX " Meshes", ICON_LC_SPRAY_CAN " Materials",
        ICON_LC_PACKAGE_OPEN " Prefabs", ICON_LC_WORKFLOW " Flode"};
    const uint32_t g_CategoryCount = 4;

    const float g_ElementSize = 128.0f;

    Util::Map<Util::Handle, Util::String> g_Paths;

    void render_meshes(AssetTypeConfig &, ImRect);
    void render_materials(AssetTypeConfig &, ImRect);
    void render_prefabs(AssetTypeConfig &, ImRect);
    void render_flodes(AssetTypeConfig &, ImRect);

    static void save_mesh_asset(Util::Handle p_Handle)
    {
      Core::MeshAsset l_Asset = p_Handle.get_id();

      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path =
          Util::get_project().dataPath + "/assets/meshes/" +
          LOW_TO_STRING(l_Asset.get_unique_id()) + ".mesh.yaml";

      if (g_Paths.find(p_Handle) != g_Paths.end()) {
        l_Path = g_Paths[p_Handle];
      }

      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved mesh asset '" << l_Asset.get_name()
                   << "' to file." << LOW_LOG_END;
    }

    void AssetWidget::save_prefab_asset(Util::Handle p_Handle)
    {
      Core::Prefab l_Asset = p_Handle.get_id();

      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path =
          Util::get_project().dataPath + "/assets/prefabs/" +
          LOW_TO_STRING(l_Asset.get_unique_id()) + ".prefab.yaml";

      if (g_Paths.find(p_Handle) != g_Paths.end()) {
        l_Path = g_Paths[p_Handle];
      }

      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved prefab '" << l_Asset.get_name()
                   << "' to file." << LOW_LOG_END;
    }

    void AssetWidget::save_material_asset(Util::Handle p_Handle)
    {
      Core::Material l_Asset = p_Handle.get_id();
      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path =
          Util::get_project().dataPath + "/assets/materials/" +
          LOW_TO_STRING(l_Asset.get_unique_id()) + ".material.yaml";
      Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved material '" << l_Asset.get_name()
                   << "' to file." << LOW_LOG_END;
    }

    void
    render_material_details_footer(Util::Handle p_Handle,
                                   Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::Material l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {
        AssetWidget::save_material_asset(p_Handle);
      }
    }

    void
    render_mesh_asset_details_footer(Util::Handle p_Handle,
                                     Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Core::MeshAsset l_Asset = p_Handle.get_id();

      if (ImGui::Button("Save")) {
        save_mesh_asset(p_Handle);
      }
    }

    void
    render_prefab_details_footer(Util::Handle p_Handle,
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

      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::MeshAsset::TYPE_ID;
        l_Config.render = &render_meshes;
        l_Config.rootDirectoryWatchHandle =
            Core::get_filesystem_watchers().meshAssetDirectory;
        l_Config.currentDirectoryWatchHandle =
            l_Config.rootDirectoryWatchHandle;

        m_TypeConfigs.push_back(l_Config);
      }
      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::Material::TYPE_ID;
        l_Config.rootDirectoryWatchHandle =
            Core::get_filesystem_watchers().materialAssetDirectory;
        l_Config.currentDirectoryWatchHandle =
            l_Config.rootDirectoryWatchHandle;
        l_Config.render = &render_materials;

        m_TypeConfigs.push_back(l_Config);
      }
      {
        AssetTypeConfig l_Config;
        l_Config.typeId = Core::Prefab::TYPE_ID;
        l_Config.rootDirectoryWatchHandle =
            Core::get_filesystem_watchers().prefabAssetDirectory;
        l_Config.currentDirectoryWatchHandle =
            l_Config.rootDirectoryWatchHandle;
        l_Config.render = &render_prefabs;

        m_TypeConfigs.push_back(l_Config);
      }
      {
        AssetTypeConfig l_Config;
        l_Config.typeId = 0;
        l_Config.rootDirectoryWatchHandle =
            get_directory_watchers().flodeDirectory;
        l_Config.currentDirectoryWatchHandle =
            l_Config.rootDirectoryWatchHandle;
        l_Config.render = &render_flodes;

        m_TypeConfigs.push_back(l_Config);
      }
    }

    static bool
    render_directory(uint32_t p_Id, Util::String p_Icon,
                     Util::String p_Label, bool p_Draggable,
                     Util::FileSystem::WatchHandle p_WatchHandle,
                     AssetTypeConfig &p_Config)
    {
      bool l_Result = false;

      ImGuiStyle &l_Style = ImGui::GetStyle();

      float l_Padding = l_Style.WindowPadding.x;

      ImGui::PushID(p_Id);
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      l_Result = ImGui::Button(p_Icon.c_str(),
                               ImVec2(g_ElementSize, g_ElementSize));
      if (l_Result) {
        p_Config.currentDirectoryWatchHandle = p_WatchHandle;
      }
      ImGui::PopFont();
      ImGui::TextWrapped(p_Label.c_str());

      ImGui::PopID();

      return l_Result;
    }

    static bool render_element(uint32_t p_Id, Util::String p_Icon,
                               Util::String p_Label, bool p_Draggable,
                               Util::Handle p_Handle,
                               bool *p_RequireUpdate)
    {
      bool l_Result = false;

      ImGuiStyle &l_Style = ImGui::GetStyle();

      float l_Padding = l_Style.WindowPadding.x;

      ImGui::PushID(p_Id);
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().lucide_800);
      l_Result = ImGui::Button(p_Icon.c_str(),
                               ImVec2(g_ElementSize, g_ElementSize));
      if (p_Draggable && p_Handle.get_id() > 0) {
        if (ImGui::BeginPopupContextItem()) {
          ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_500);

          set_selected_handle(p_Handle);
          if (ImGui::MenuItem("Delete")) {
            Util::RTTI::TypeInfo &l_TypeInfo =
                Util::Handle::get_type_info(p_Handle.get_type());

            if (p_RequireUpdate) {
              *p_RequireUpdate = true;
            }

            LOW_LOG_WARN << "Deleting..." << LOW_LOG_END;

            if (g_Paths.find(p_Handle) != g_Paths.end()) {
              Util::String l_Path = g_Paths[p_Handle];
              LOW_LOG_WARN << "CHECKING file" << LOW_LOG_END;
              if (Util::FileIO::file_exists_sync(l_Path.c_str())) {
                LOW_LOG_WARN << "Delete file" << LOW_LOG_END;
                Util::FileIO::delete_sync(l_Path.c_str());
              }
            }

            l_TypeInfo.destroy(p_Handle);
          }

          ImGui::PopFont();
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
      ImGui::Begin(ICON_LC_FILE " Assets");

      ImGui::BeginChild("Categories",
                        ImVec2(200, ImGui::GetContentRegionAvail().y),
                        true, 0);
      for (uint32_t i = 0; i < g_CategoryCount; ++i) {
        if (ImGui::Selectable(g_CategoryLabels[i].c_str(),
                              i == m_SelectedCategory)) {
          m_SelectedCategory = i;
        }
      }

      ImGui::EndChild();

      ImGui::SameLine();

      ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
      ImRect l_Rect(l_Cursor,
                    {l_Cursor.x + ImGui::GetContentRegionAvail().x,
                     l_Cursor.y + ImGui::GetContentRegionAvail().y});

      ImGui::BeginChild("Content",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true);
      if (m_SelectedCategory >= 0 &&
          m_SelectedCategory < g_CategoryCount) {
        m_TypeConfigs[m_SelectedCategory].render(
            m_TypeConfigs[m_SelectedCategory], l_Rect);
      }

      ImGui::EndChild();

      ImGui::End();

      // ImGui::ShowDemoWindow();
    }

    static void render_meshes(AssetTypeConfig &p_Config,
                              ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
          Util::FileSystem::get_directory_watcher(
              p_Config.currentDirectoryWatchHandle);

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 3.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      int l_Columns =
          LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (render_element(l_Id++, ICON_LC_PLUS, "Create mesh asset",
                         false, 0, 0)) {
        ImGui::OpenPopup("Create mesh asset");
      }

      if (ImGui::BeginPopupModal("Create mesh asset")) {
        ImGui::Text(
            "You are about to create a new mesh asset. Please select "
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
               it != Core::MeshAsset::ms_LivingInstances.end();
               ++it) {
            if (l_Name == it->get_name()) {
              LOW_LOG_ERROR
                  << "Cannot create mesh asset. The chosen name '"
                  << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::MeshAsset l_Asset = Core::MeshAsset::make(l_Name);
            l_DirectoryWatcher.update = true;

            save_mesh_asset(l_Asset);
            set_selected_handle(l_Asset);
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();

      if (p_Config.currentDirectoryWatchHandle !=
          p_Config.rootDirectoryWatchHandle) {

        if (render_directory(
                l_Id++, ICON_LC_FOLDER_OPEN, "back", false,
                l_DirectoryWatcher.parentWatchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              l_DirectoryWatcher.parentWatchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.subdirectories.begin();
           it != l_DirectoryWatcher.subdirectories.end(); ++it) {
        Util::FileSystem::DirectoryWatcher &i_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(*it);

        if (render_directory(
                l_Id++, ICON_LC_FOLDER, i_DirectoryWatcher.name, true,
                i_DirectoryWatcher.watchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              i_DirectoryWatcher.watchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.files.begin();
           it != l_DirectoryWatcher.files.end(); ++it) {
        Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        Core::MeshAsset i_MeshAsset = i_FileWatcher.handle.get_id();

        if (!i_MeshAsset.is_alive()) {
          continue;
        }

        g_Paths[i_MeshAsset.get_id()] = i_FileWatcher.path;

        if (!l_SearchString.empty()) {
          Util::String i_LowName = i_MeshAsset.get_name().c_str();
          i_LowName.make_lower();
          if (!Util::StringHelper::contains(i_LowName,
                                            l_SearchString)) {
            continue;
          }
        }

        if (render_element(l_Id++, ICON_LC_BOX,
                           i_MeshAsset.get_name().c_str(), true,
                           i_MeshAsset, &l_DirectoryWatcher.update)) {
          set_selected_handle(i_MeshAsset);
          HandlePropertiesSection i_Section(i_MeshAsset, true);
          i_Section.render_footer = &render_mesh_asset_details_footer;
          get_details_widget()->add_section(i_Section);
        }
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
    }

    static void render_materials(AssetTypeConfig &p_Config,
                                 ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
          Util::FileSystem::get_directory_watcher(
              p_Config.currentDirectoryWatchHandle);

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 3.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      int l_Columns =
          LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (render_element(l_Id++, ICON_LC_PLUS, "Create material",
                         false, 0, 0)) {
        ImGui::OpenPopup("Create material");
      }

      if (ImGui::BeginPopupModal("Create material")) {
        ImGui::Text("You are about to create a new material. Please "
                    "select a name.");
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
              LOW_LOG_ERROR
                  << "Cannot create material. The chosen name '"
                  << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::Material l_NewMaterial =
                Core::Material::make(l_Name);
            AssetWidget::save_material_asset(l_NewMaterial.get_id());
            l_DirectoryWatcher.update = true;
            set_selected_handle(l_NewMaterial);
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      ImGui::NextColumn();

      if (p_Config.rootDirectoryWatchHandle !=
          p_Config.currentDirectoryWatchHandle) {

        if (render_directory(
                l_Id++, ICON_LC_FOLDER_OPEN, "back", false,
                l_DirectoryWatcher.parentWatchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              l_DirectoryWatcher.parentWatchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.subdirectories.begin();
           it != l_DirectoryWatcher.subdirectories.end(); ++it) {
        Util::FileSystem::DirectoryWatcher &i_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(*it);
        if (render_directory(
                l_Id++, ICON_LC_FOLDER, i_DirectoryWatcher.name, true,
                i_DirectoryWatcher.watchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              i_DirectoryWatcher.watchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.files.begin();
           it != l_DirectoryWatcher.files.end(); ++it) {
        Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        Core::Material i_Material = i_FileWatcher.handle.get_id();
        bool i_ShouldUpdate = l_DirectoryWatcher.update;

        if (!i_Material.is_alive()) {
          continue;
        }

        g_Paths[i_Material.get_id()] = i_FileWatcher.path;

        if (!l_SearchString.empty()) {
          Util::String i_LowName = i_Material.get_name().c_str();
          i_LowName.make_lower();
          if (!Util::StringHelper::contains(i_LowName,
                                            l_SearchString)) {
            continue;
          }
        }

        if (render_element(l_Id++, ICON_LC_SPRAY_CAN,
                           i_Material.get_name().c_str(), true,
                           i_FileWatcher.handle,
                           &l_DirectoryWatcher.update)) {
          if (!i_ShouldUpdate && l_DirectoryWatcher.update) {
            break;
          }
          set_selected_handle(i_FileWatcher.handle);
          HandlePropertiesSection i_Section(i_FileWatcher.handle,
                                            true);
          i_Section.render_footer = &render_material_details_footer;
          get_details_widget()->add_section(i_Section);
        }
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
    }

    static void render_flodes(AssetTypeConfig &p_Config,
                              ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
          Util::FileSystem::get_directory_watcher(
              p_Config.currentDirectoryWatchHandle);

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      static bool l_Hidden = false;

      Base::BoolEdit("Hidden", &l_Hidden);

      ImGui::SameLine();

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search));

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      int l_Columns =
          LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (p_Config.rootDirectoryWatchHandle !=
          p_Config.currentDirectoryWatchHandle) {
        if (render_directory(
                l_Id++, ICON_LC_FOLDER_OPEN, "back", false,
                l_DirectoryWatcher.parentWatchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              l_DirectoryWatcher.parentWatchHandle;
        }
      } else {
        if (render_element(l_Id++, ICON_LC_PLUS, "Create flode graph",
                           false, 0, 0)) {
          ImGui::OpenPopup("Create flode");
        }

        if (ImGui::BeginPopupModal("Create flode")) {
          ImGui::Text(
              "You are about to create a new flode graph. Please "
              "select a name.");
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

            if (l_Ok) {
              Flode::Graph *l_Graph = new Flode::Graph;
              l_Graph->m_Name = l_Name;
              l_Graph->m_Internal = false;
              if (get_flode_widget()->m_Editor->m_Graph) {
                delete get_flode_widget()->m_Editor->m_Graph;
              }
              get_flode_widget()->m_Editor->m_Graph = l_Graph;
            }
            ImGui::CloseCurrentPopup();
          }

          ImGui::EndPopup();
        }
      }

      ImGui::NextColumn();

      for (auto it = l_DirectoryWatcher.subdirectories.begin();
           it != l_DirectoryWatcher.subdirectories.end(); ++it) {
        Util::FileSystem::DirectoryWatcher &i_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(*it);
        if (render_directory(
                l_Id++, ICON_LC_FOLDER, i_DirectoryWatcher.name, true,
                i_DirectoryWatcher.watchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              i_DirectoryWatcher.watchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.files.begin();
           it != l_DirectoryWatcher.files.end(); ++it) {
        Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        if (!Util::StringHelper::ends_with(i_FileWatcher.path,
                                           ".flode.yaml")) {
          continue;
        }
        Util::String i_Name = i_FileWatcher.name.substr(
            0, i_FileWatcher.name.size() - 11);

        if (!l_SearchString.empty()) {
          Util::String i_LowName = i_Name;
          i_LowName.make_lower();
          if (!Util::StringHelper::contains(i_LowName,
                                            l_SearchString)) {
            continue;
          }
        }
        if (!l_Hidden &&
            Util::StringHelper::begins_with(i_Name, "__")) {
          continue;
        }
        if (render_element(l_Id++, ICON_LC_WORKFLOW, i_Name, true,
                           i_FileWatcher.handle,
                           &l_DirectoryWatcher.update)) {
          open_flode_graph(i_FileWatcher.path);
          // get_flode_widget()->m_Editor->load(i_FileWatcher.path);
        }
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
    }

    static void render_prefabs(AssetTypeConfig &p_Config,
                               ImRect p_Bounds)
    {
      uint32_t l_Id = 0;

      Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
          Util::FileSystem::get_directory_watcher(
              p_Config.currentDirectoryWatchHandle);

      if (ImGui::BeginDragDropTargetCustom(p_Bounds, 34839)) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {
            Core::Prefab l_Prefab = Core::Prefab::make(l_Entity);

            AssetWidget::save_prefab_asset(l_Prefab);
            l_DirectoryWatcher.update = true;
          }
        }
        ImGui::EndDragDropTarget();
      }

      float l_ContentWidth = ImGui::GetContentRegionAvail().x;

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 3.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      int l_Columns =
          LOW_MATH_MAX(1, (int)(l_ContentWidth / (g_ElementSize)));
      ImGui::Columns(l_Columns, NULL, false);

      if (p_Config.rootDirectoryWatchHandle !=
          p_Config.currentDirectoryWatchHandle) {
        if (render_directory(
                l_Id++, ICON_LC_FOLDER_OPEN, "back", false,
                l_DirectoryWatcher.parentWatchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              l_DirectoryWatcher.parentWatchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.subdirectories.begin();
           it != l_DirectoryWatcher.subdirectories.end(); ++it) {
        Util::FileSystem::DirectoryWatcher &i_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(*it);
        if (render_directory(
                l_Id++, ICON_LC_FOLDER, i_DirectoryWatcher.name, true,
                i_DirectoryWatcher.watchHandle, p_Config)) {
          p_Config.currentDirectoryWatchHandle =
              i_DirectoryWatcher.watchHandle;
        }
        ImGui::NextColumn();
      }

      for (auto it = l_DirectoryWatcher.files.begin();
           it != l_DirectoryWatcher.files.end(); ++it) {
        Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        Core::Prefab i_Prefab = i_FileWatcher.handle.get_id();
        if (!i_Prefab.is_alive()) {
          continue;
        }

        if (!Core::Prefab(i_Prefab.get_parent().get_id())
                 .is_alive()) {

          if (!l_SearchString.empty()) {
            Util::String i_LowName = i_Prefab.get_name().c_str();
            i_LowName.make_lower();
            if (!Util::StringHelper::contains(i_LowName,
                                              l_SearchString)) {
              continue;
            }
          }

          g_Paths[i_Prefab.get_id()] = i_FileWatcher.path;
          if (render_element(l_Id++, ICON_LC_PACKAGE_OPEN,
                             i_Prefab.get_name().c_str(), true,
                             i_FileWatcher.handle,
                             &l_DirectoryWatcher.update)) {
            set_selected_handle(i_FileWatcher.handle);
            HandlePropertiesSection i_Section(i_FileWatcher.handle,
                                              true);
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
