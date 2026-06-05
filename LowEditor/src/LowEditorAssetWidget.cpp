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
#include "LowRendererEditorImage.h"
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

    static bool render_asset_creation_menu_entry(
        AssetCreation::Action &p_Action,
        const AssetCreation::Context &p_Context)
    {
      bool l_Enabled = true;
      if (p_Action.is_enabled) {
        l_Enabled = p_Action.is_enabled(p_Context);
      }

      const ImVec2 l_Start = ImGui::GetCursorScreenPos();
      const float l_Width = ImGui::GetContentRegionAvail().x;
      const float l_Height = 54.0f;
      const float l_PreviewSize = 44.0f;
      const float l_Rounding = 5.0f;

      ImGui::PushID((int)p_Action.id.m_Index);
      const bool l_Clicked =
          ImGui::InvisibleButton("creation_entry",
                                 ImVec2(l_Width, l_Height)) &&
          l_Enabled;
      const bool l_Hovered = ImGui::IsItemHovered() && l_Enabled;
      ImGui::PopID();

      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
      const Theme &l_Theme = theme_get_current();
      const ImVec2 l_End(l_Start.x + l_Width,
                         l_Start.y + l_Height);

      const ImU32 l_Background =
          l_Hovered ? color_to_imcolor(l_Theme.headerHover)
                    : color_to_imcolor(l_Theme.input);
      const ImU32 l_TextColor =
          ImGui::GetColorU32(l_Enabled ? ImGuiCol_Text
                                       : ImGuiCol_TextDisabled);

      l_DrawList->AddRectFilled(l_Start, l_End, l_Background,
                                l_Rounding);
      l_DrawList->AddRect(l_Start, l_End,
                          color_to_imcolor(l_Theme.border),
                          l_Rounding);
      l_DrawList->AddRectFilled(
          l_Start, ImVec2(l_Start.x + 3.0f, l_End.y),
          color_to_imcolor(get_color_for_asset_type(
              p_Action.assetType)),
          l_Rounding, ImDrawFlags_RoundCornersLeft);

      const ImVec2 l_IconMin(l_Start.x + 10.0f,
                             l_Start.y + 5.0f);
      const ImVec2 l_IconMax(l_IconMin.x + l_PreviewSize,
                             l_IconMin.y + l_PreviewSize);

      Renderer::EditorImage l_Image =
          get_editor_image_for_asset_type(p_Action.assetType);
      if (l_Image.is_alive() &&
          l_Image.get_state() == Renderer::TextureState::LOADED &&
          l_Image.get_gpu().is_imgui_texture_initialized()) {
        l_DrawList->AddImage(l_Image.get_gpu().get_imgui_texture_id(),
                             l_IconMin, l_IconMax);
      } else {
        Util::String l_Fallback =
            p_Action.icon.empty() ? "+" : p_Action.icon;
        const ImVec2 l_TextSize =
            ImGui::CalcTextSize(l_Fallback.c_str());
        l_DrawList->AddText(
            ImVec2(l_IconMin.x +
                       ((l_PreviewSize - l_TextSize.x) * 0.5f),
                   l_IconMin.y +
                       ((l_PreviewSize - l_TextSize.y) * 0.5f)),
            l_TextColor, l_Fallback.c_str());
      }

      const float l_TextX = l_IconMax.x + 10.0f;
      const ImVec2 l_NamePos(l_TextX, l_Start.y + 8.0f);
      const ImVec2 l_NameClipMax(l_End.x - 10.0f,
                                 l_NamePos.y +
                                     ImGui::GetTextLineHeight());
      ImGui::RenderTextEllipsis(
          l_DrawList, l_NamePos, l_NameClipMax, l_NameClipMax.x,
          p_Action.label.c_str(), nullptr, nullptr);

      const Util::String l_TypeName = Util::StringHelper::to_upper(
          get_asset_type_name(p_Action.assetType));
      ImVec4 l_TypeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      l_TypeColor.w = 0.3f;
      ImGui::PushFont(Fonts::UI(14, Fonts::Weight::Light));
      l_DrawList->AddText(ImVec2(l_TextX, l_Start.y + 31.0f),
                          ImColor(l_TypeColor), l_TypeName.c_str());
      ImGui::PopFont();

      if (!l_Enabled) {
        l_DrawList->AddRectFilled(l_Start, l_End,
                                  ImGui::GetColorU32(
                                      ImGuiCol_ModalWindowDimBg),
                                  l_Rounding);
      }

      return l_Clicked;
    }

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
          m_ContextMenuHandle(Util::Handle::DEAD),
          m_PendingCreationAction(nullptr)
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

      bool l_OpenNameDialog = false;
      bool l_OpenCustomDialog = false;

      ImGui::SetNextWindowSize(ImVec2(280.0f, 0.0f),
                               ImGuiCond_Appearing);
      if (ImGui::BeginPopup("WINDOWCONTEXT")) {
        using namespace Low::Util::FileSystem;
        DirectoryWatcher &l_Watcher =
            get_directory_watcher(m_SelectedDirectory);

        AssetCreation::Context l_Context;
        l_Context.directoryPath = l_Watcher.path;

        Util::List<AssetCreation::Action *> l_Actions;
        AssetCreation::collect_actions(l_Context, l_Actions);

        if (l_Actions.empty()) {
          ImGui::MenuItem("No creation actions", nullptr, false,
                          false);
        }

        for (AssetCreation::Action *i_Action : l_Actions) {
          if (render_asset_creation_menu_entry(*i_Action,
                                               l_Context)) {
            m_PendingCreationAction = i_Action;
            m_CreationDirectoryPath = l_Context.directoryPath;

            const char *l_DefaultName =
                i_Action->defaultName.is_valid()
                    ? i_Action->defaultName.c_str()
                    : "Asset";
            m_CreationName = LOW_NAME(l_DefaultName);

            if (i_Action->render_dialog) {
              l_OpenCustomDialog = true;
            } else {
              l_OpenNameDialog = true;
            }
          }
          ImGui::Dummy(ImVec2(0.0f, 4.0f));
        }
        ImGui::EndPopup();
      }

      if (l_OpenNameDialog) {
        ImGui::OpenPopup("Create Asset");
      }
      if (l_OpenCustomDialog) {
        ImGui::OpenPopup("AssetCreationCustomDialog");
      }

      if (m_PendingCreationAction) {
        ImGuiViewport *l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(l_Viewport->GetCenter(),
                                ImGuiCond_Appearing,
                                ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f),
                                 ImGuiCond_Appearing);
      }

      if (m_PendingCreationAction &&
          ImGui::BeginPopupModal(
              "Create Asset", nullptr,
              ImGuiWindowFlags_AlwaysAutoResize |
                  ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextUnformatted(m_PendingCreationAction->label.c_str());
        ImGui::Spacing();

        Gui::NameInput("Name", m_CreationName, 128);
        const bool l_SubmitName =
            ImGui::IsItemFocused() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
             ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));

        ImGui::Spacing();

        const bool l_CanCreate =
            m_CreationName.is_valid() &&
            m_CreationName.c_str()[0] != '\0';
        if (ImGui::Button("Create") && l_CanCreate) {
          AssetCreation::Context l_Context;
          l_Context.directoryPath = m_CreationDirectoryPath;
          if (m_PendingCreationAction->create) {
            m_PendingCreationAction->create(
                l_Context, m_CreationName);
          }
          m_PendingCreationAction = nullptr;
          ImGui::CloseCurrentPopup();
        }
        if (l_SubmitName && l_CanCreate &&
            m_PendingCreationAction) {
          AssetCreation::Context l_Context;
          l_Context.directoryPath = m_CreationDirectoryPath;
          if (m_PendingCreationAction->create) {
            m_PendingCreationAction->create(l_Context,
                                            m_CreationName);
          }
          m_PendingCreationAction = nullptr;
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          m_PendingCreationAction = nullptr;
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }

      if (m_PendingCreationAction &&
          ImGui::BeginPopup("AssetCreationCustomDialog")) {
        if (m_PendingCreationAction->render_dialog) {
          AssetCreation::DialogContext l_Context;
          l_Context.context.directoryPath = m_CreationDirectoryPath;
          l_Context.popupId = "AssetCreationCustomDialog";
          m_PendingCreationAction->render_dialog(l_Context);
        }
        ImGui::EndPopup();
      }

      ImGui::EndChild();

      ImGui::End();
    }

  } // namespace Editor
} // namespace Low

#undef UPDATE_INTERVAL
