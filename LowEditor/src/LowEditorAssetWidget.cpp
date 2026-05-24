#include "LowEditorAssetWidget.h"

#include "LowCoreUiWidgetAsset.h"
#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorMainWindow.h"
#include "LowEditorThemes.h"
#include "LowEditorTypeEditor.h"
#include "LowMath.h"
#include "LowRendererMaterial.h"
#include "LowRendererMaterialResource.h"
#include "LowRendererMeshResource.h"
#include "LowRendererSkeletonResource.h"
#include "LowRendererTextureState.h"
#include "LowRendererMesh.h"
#include "LowRendererResourceManager.h"
#include "LowUtilFileSystem.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsLucide.h"
#include "LowUtilAssetManager.h"

#include "LowEditor.h"
#include "LowCorePrefab.h"

#include "LowUtil.h"
#include "LowUtilString.h"

#define UPDATE_INTERVAL 3.0f

namespace Low {
  namespace Editor {

    const ImVec2 g_Margin(14.0f, 8.0f);

    void AssetWidget::save_prefab_asset(Util::Handle p_Handle)
    {
      Core::Prefab l_Asset = p_Handle.get_id();

      /*
      Util::Yaml::Node l_Node;
      l_Asset.serialize(l_Node);
      Util::String l_Path =
          Util::get_project().dataPath + "/assets/prefabs/" +
          LOW_TO_STRING(l_Asset.get_unique_id()) + ".prefab.yaml";

      if (g_Paths.find(p_Handle) != g_Paths.end()) {
        l_Path = g_Paths[p_Handle];
      }

      Util::Yaml::write_file(l_Path.c_str(), l_Node);
      */

      LOW_LOG_INFO << "Saved prefab '" << l_Asset.get_name()
                   << "' to file." << LOW_LOG_END;
    }

    AssetWidget::AssetWidget()
        : m_UpdateCounter(UPDATE_INTERVAL),
          m_ContextMenuHandle(Util::Handle::DEAD)
    {
      m_DataWatcher = Util::FileSystem::watch_directory(
          Util::get_project().dataPath,
          [](Util::FileSystem::FileWatcher &p_Watcher) {
            AssetType l_AssetType = AssetType::File;
            if (p_Watcher.subtype == "meshresource") {
              l_AssetType = AssetType::Mesh;
            } else if (p_Watcher.subtype == "materialresource") {
              l_AssetType = AssetType::Material;
            } else if (p_Watcher.subtype == "model") {
              l_AssetType = AssetType::Model;
            } else if (p_Watcher.subtype == "texresource") {
              l_AssetType = AssetType::Texture;
            } else if (p_Watcher.subtype == "skeletonresource") {
              l_AssetType = AssetType::Skeleton;
            } else if (p_Watcher.subtype == "fontresource") {
              l_AssetType = AssetType::Font;
            } else if (p_Watcher.subtype == "flode" ||
                       p_Watcher.subtype == "vs") {
              l_AssetType = AssetType::Flode;
            } else if (p_Watcher.subtype == "uiwidget") {
              l_AssetType = AssetType::UiWidget;
            } else if (p_Watcher.extension == "cpp" ||
                       p_Watcher.extension == "as") {
              l_AssetType = AssetType::Script;
            }

            p_Watcher.typeEnum = (u32)l_AssetType;

            Util::Handle l_Handle = Util::Handle::DEAD;
            switch (l_AssetType) {
            case AssetType::Mesh: {
              Renderer::MeshResource l_Resource =
                  Renderer::MeshResource::find_by_path(
                      p_Watcher.trimmedPath);

              if (l_Resource.is_alive()) {
                p_Watcher.assetId = l_Resource.get_mesh_id();
                l_Handle = Renderer::ResourceManager::find_asset<
                    Renderer::Mesh>(l_Resource.get_mesh_id());
              }

              break;
            }
            case AssetType::Texture: {
              Renderer::TextureResource l_Resource =
                  Renderer::TextureResource::find_by_path(
                      p_Watcher.path);
              if (l_Resource.is_alive()) {
                p_Watcher.assetId = l_Resource.get_texture_id();
                l_Handle = Renderer::ResourceManager::find_asset<
                    Renderer::Texture>(l_Resource.get_texture_id());
              }

              break;
            }
            case AssetType::Material: {
              Renderer::MaterialResource l_Resource =
                  Renderer::MaterialResource::find_by_path(
                      p_Watcher.path);
              if (l_Resource.is_alive()) {
                p_Watcher.assetId = l_Resource.get_material_id();
                l_Handle = Renderer::ResourceManager::find_asset<
                    Renderer::Material>(l_Resource.get_material_id());
              }

              break;
            }
            case AssetType::Skeleton: {
              Renderer::SkeletonResource l_Resource =
                  Renderer::SkeletonResource::find_by_path(
                      p_Watcher.path);
              if (l_Resource.is_alive()) {
                p_Watcher.assetId = l_Resource.get_skeleton_id();
                l_Handle = Renderer::ResourceManager::find_asset<
                    Renderer::Skeleton>(l_Resource.get_skeleton_id());
              }

              break;
            }
            case AssetType::Font: {
              Renderer::FontResource l_Resource =
                  Renderer::FontResource::find_by_path(
                      p_Watcher.path);
              if (l_Resource.is_alive()) {
                p_Watcher.assetId = l_Resource.get_font_id();
                l_Handle = Renderer::ResourceManager::find_asset<
                    Renderer::Font>(l_Resource.get_font_id());

                Renderer::Font l_Font = l_Handle.get_id();
              }

              break;
            }
            case AssetType::UiWidget: {
              l_Handle =
                  Util::AssetManager::_find_by_path(p_Watcher.path);
              break;
            }
            }

            return l_Handle;
          },
          m_UpdateCounter);

      m_SelectedDirectory = m_DataWatcher;
    }

    void AssetWidget::render_directory_list(
        const Util::FileSystem::DirectoryWatcher &p_DirectoryWatcher,
        const Util::String p_DisplayName)
    {
      using namespace Low::Util::FileSystem;

      if (p_DirectoryWatcher.hidden) {
        return;
      }

      ImGuiTreeNodeFlags l_BaseFlags =
          ImGuiTreeNodeFlags_OpenOnArrow |
          ImGuiTreeNodeFlags_OpenOnDoubleClick |
          ImGuiTreeNodeFlags_SpanAvailWidth;

      const bool l_Leaf = p_DirectoryWatcher.subdirectories.empty();

      if (l_Leaf) {
        l_BaseFlags |= ImGuiTreeNodeFlags_Leaf |
                       ImGuiTreeNodeFlags_NoTreePushOnOpen;
      }

      Util::String l_DisplayName = p_DirectoryWatcher.name;
      if (!p_DisplayName.empty()) {
        l_DisplayName = p_DisplayName;
      }

      if (m_SelectedDirectory == p_DirectoryWatcher.watchHandle) {
        l_BaseFlags |= ImGuiTreeNodeFlags_Selected;
      }

      ImGui::PushID(p_DirectoryWatcher.watchHandle);

      const bool l_Open = ImGui::TreeNodeEx(
          "##row", l_BaseFlags, "%s", l_DisplayName.c_str());

      if (ImGui::IsItemClicked()) {
        m_SelectedDirectory = p_DirectoryWatcher.watchHandle;
      }

      if (!l_Leaf && l_Open) {
        for (auto it = p_DirectoryWatcher.subdirectories.begin();
             it != p_DirectoryWatcher.subdirectories.end(); ++it) {
          render_directory_list(get_directory_watcher(*it));
        }
        ImGui::TreePop();
      }

      ImGui::PopID();
    }

    void AssetWidget::render_directory_content(
        const Util::FileSystem::DirectoryWatcher &p_DirectoryWatcher)
    {
      float l_AvailableWidth = ImGui::GetContentRegionAvail().x;
      if (l_AvailableWidth <= 0.0f) {
        return;
      }

      float l_TotalWidth = Gui::g_AssetCardSize.x + g_Margin.x;
      const int l_Cols = LOW_MATH_MAX(
          1, (int)Math::Util::floor((l_AvailableWidth + g_Margin.x) /
                                    l_TotalWidth));

      int l_Column = 0;

      for (auto it = p_DirectoryWatcher.files.begin();
           it != p_DirectoryWatcher.files.end(); ++it) {
        if (!Util::FileSystem::file_watcher_exists(*it)) {
          // TODO: New files never get watched
          continue;
        }
        const Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        if (i_FileWatcher.hidden) {
          continue;
        }
        AssetType i_AssetType = (AssetType)i_FileWatcher.typeEnum;
        if (i_AssetType == AssetType::File &&
            i_FileWatcher.extension != "yaml") {
          continue;
        }

        const AssetCardResult i_Result =
            Gui::asset_card(i_FileWatcher);

        if (i_Result.clicked) {
        }
        if (i_Result.doubleClicked) {
          if (i_AssetType == AssetType::Script) {
            open_file_in_code_editor(i_FileWatcher.path);
          } else if (i_AssetType == AssetType::File &&
                     i_FileWatcher.extension == "yaml") {
            open_file_in_code_editor(i_FileWatcher.path);
          } else {
            if (Util::Handle::is_registered_type(
                    i_FileWatcher.handle.get_type())) {
              Util::RTTI::TypeInfo &i_TypeInfo =
                  Util::Handle::get_type_info(
                      i_FileWatcher.handle.get_type());
              if (i_TypeInfo.is_alive(i_FileWatcher.handle)) {
                _open_widget_for_handle(i_FileWatcher.handle);
              }
            }
          }
        }
        if (i_Result.rightClicked) {
          m_ContextMenuHandle = i_FileWatcher.handle;
          ImGui::OpenPopup("AssetTypeActionContextMenu");
        }

        l_Column++;
        if (l_Column < l_Cols) {
          ImGui::SameLine(0.0f, g_Margin.x);
        } else {
          l_Column = 0;
          ImGui::Dummy(ImVec2(0, g_Margin.y)); // vertical margin
        }
      }

      TypeEditor::render_context_menu(
          "AssetTypeActionContextMenu", m_ContextMenuHandle,
          TypeActionSurface::AssetWidgetContextMenu);
    }

    void AssetWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_FILE " Assets");

#if 0
      Renderer::EditorImage l_EditorImage =
          Renderer::EditorImage::find_by_name(N(filetype_file));
      if (l_EditorImage.is_alive() &&
          l_EditorImage.get_state() ==
              Renderer::TextureState::LOADED) {
        ImGui::Image(l_EditorImage.get_gpu().get_imgui_texture_id(),
                     ImVec2(512, 512));
      }

      ImGui::End();
      return;
#endif

      ImGui::BeginChild("Categories",
                        ImVec2(200, ImGui::GetContentRegionAvail().y),
                        true, 0);

      {
        using namespace Low::Util::FileSystem;
        DirectoryWatcher &l_DataWatcher =
            get_directory_watcher(m_DataWatcher);

        render_directory_list(l_DataWatcher, "Game");
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

      render_directory_content(
          Util::FileSystem::get_directory_watcher(
              m_SelectedDirectory));

      if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
          ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("WINDOWCONTEXT");
      }

      if (ImGui::BeginPopup("WINDOWCONTEXT")) {
        // TODO: Change this. Asset creation options should be
        // registered from outside and not harcoded here.
        if (ImGui::MenuItem("New UI-Widget")) {
          using namespace Low::Util::FileSystem;
          DirectoryWatcher &l_Watcher =
              get_directory_watcher(m_SelectedDirectory);
          Util::AssetManager::create<Core::UI::WidgetAsset>(
              N(Testwidget), l_Watcher.path);
        }
        if (ImGui::MenuItem("New Material")) {
          using namespace Low::Util::FileSystem;
          DirectoryWatcher &l_Watcher =
              get_directory_watcher(m_SelectedDirectory);
          Util::AssetManager::create<Renderer::Material>(
              N(Material), l_Watcher.path);
        }
        ImGui::EndPopup();
      }
      ImGui::EndChild();

      ImGui::End();
    }

  } // namespace Editor
} // namespace Low

#undef UPDATE_INTERVAL
