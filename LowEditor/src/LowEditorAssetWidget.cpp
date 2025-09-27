#include "LowEditorAssetWidget.h"

#include "LowEditorFonts.h"
#include "LowEditorThemes.h"
#include "LowMath.h"
#include "LowRendererMaterialResource.h"
#include "LowRendererMeshResource.h"
#include "LowRendererTextureState.h"
#include "LowRendererMesh.h"
#include "LowRendererResourceManager.h"
#include "LowUtilFileSystem.h"
#include "LowUtilHandle.h"
#include "LowUtilHashing.h"
#include "LowUtilLogger.h"
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
    struct AssetCardResult
    {
      bool clicked = false;
      bool doubleClicked = false;
      bool rightClicked = false;
      bool hovered = false;
    };

    const ImVec2 g_CardSize(140, 180);
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

    AssetWidget::AssetWidget() : m_UpdateCounter(UPDATE_INTERVAL)
    {
      m_DataWatcher = Util::FileSystem::watch_directory(
          Util::get_project().dataPath,
          [](Util::FileSystem::FileWatcher &p_Watcher) {
            AssetType l_AssetType = AssetType::File;
            if (p_Watcher.subtype == "meshresource") {
              l_AssetType = AssetType::Mesh;
            } else if (p_Watcher.subtype == "material") {
              l_AssetType = AssetType::Material;
            } else if (p_Watcher.subtype == "model") {
              l_AssetType = AssetType::Model;
            } else if (p_Watcher.subtype == "texresource") {
              l_AssetType = AssetType::Texture;
            } else if (p_Watcher.subtype == "fontresource") {
              l_AssetType = AssetType::Font;
            } else if (p_Watcher.subtype == "flode") {
              l_AssetType = AssetType::Flode;
            } else if (p_Watcher.extension == "cpp") {
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

    AssetCardResult draw_asset_card(
        const Util::FileSystem::FileWatcher &p_FileWatcher)
    {
      ImGui::PushID(p_FileWatcher.watchHandle);

      const float l_Rounding = 6.0f;

      const ImVec2 l_Padding(8.0f, 8.0f);

      AssetCardResult result{};
      ImGuiStyle &style = ImGui::GetStyle();

      const AssetType l_AssetType = (AssetType)p_FileWatcher.typeEnum;

      // Sizes
      float thumb_h = g_CardSize.y * 0.65f;
      float text_h = g_CardSize.y - thumb_h;

      ImVec2 pos = ImGui::GetCursorScreenPos();
      ImVec2 card_max = pos + g_CardSize;

      // Whole card is one button region
      ImGui::InvisibleButton("asset_card_btn", g_CardSize);
      result.hovered = ImGui::IsItemHovered();
      result.clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
      result.doubleClicked =
          ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
          result.hovered;
      result.rightClicked =
          ImGui::IsItemClicked(ImGuiMouseButton_Right);

      ImDrawList *dl = ImGui::GetWindowDrawList();

      {
        // Parameters (tweak as you like)
        const float stripe_h = 8.0f; // thickness of the stripe
        const ImU32 stripe_col =
            color_to_imcolor(get_color_for_asset_type(l_AssetType));

        // Stripe rect (bottom of the card)
        ImVec2 stripe_min(pos.x, card_max.y - stripe_h);
        ImVec2 stripe_max(card_max.x, card_max.y);

        // Draw AFTER card background, BEFORE the border so the border
        // outlines it
        dl->AddRectFilled(stripe_min, stripe_max, stripe_col,
                          l_Rounding, ImDrawFlags_RoundCornersBottom);
      }

      // Background
      ImU32 col_bg = result.hovered
                         ? ImGui::GetColorU32(ImGuiCol_HeaderHovered)
                         : ImGui::GetColorU32(ImGuiCol_FrameBg);
      dl->AddRectFilled(pos, card_max - ImVec2(0.0f, 3.0f), col_bg,
                        l_Rounding);
      dl->AddRect(pos, card_max, ImGui::GetColorU32(ImGuiCol_Border),
                  l_Rounding);

      // Thumbnail
      ImVec2 thumb_min = pos;
      ImVec2 thumb_max =
          ImVec2(pos.x + g_CardSize.x, pos.y + thumb_h);
      ImVec2 img_max = ImVec2(pos.x + thumb_h, pos.y + thumb_h);

      Renderer::EditorImage l_EditorImage = Util::Handle::DEAD;

      const float l_FallbackSize = thumb_h * 0.7f;

      switch (l_AssetType) {
      case AssetType::Mesh: {
        Renderer::Mesh l_Mesh = p_FileWatcher.handle.get_id();

        if (l_Mesh.is_alive()) {
          l_EditorImage = l_Mesh.get_editor_image();
        }
        break;
      }
      }

      Util::HandleLock l_EditorImageLock(l_EditorImage, false);

      if (l_EditorImageLock.owns_lock() && l_EditorImage.is_alive() &&
          l_EditorImage.get_state() ==
              Renderer::TextureState::UNLOADED) {
        Renderer::ResourceManager::load_editor_image(l_EditorImage);
      }

      const bool l_IsFallbackEditorImage =
          l_EditorImageLock.owns_lock() &&
              (
              !l_EditorImage.is_alive() ||
          l_EditorImage.get_state() != Renderer::TextureState::LOADED);
      if (l_IsFallbackEditorImage) {
        l_EditorImage = get_editor_image_for_asset_type(l_AssetType);
      }

      const ImVec2 l_FallbackMin =
          pos +
          ImVec2(
              ((thumb_max.x - thumb_min.x) - l_FallbackSize) / 2.0f,
              ((thumb_max.y - thumb_min.y) - l_FallbackSize) / 2.0f);

      if (l_EditorImageLock.owns_lock() && l_EditorImage.is_alive() &&
          l_EditorImage.get_state() ==
              Renderer::TextureState::LOADED) {

        if (l_IsFallbackEditorImage) {
          dl->AddRectFilled(thumb_min, thumb_max,
                            make_imcolor(0.66f, 0.66, 0.66),
                            // ImGui::GetColorU32(ImGuiCol_WindowBg),
                            l_Rounding, ImDrawFlags_RoundCornersTop);

          if (l_EditorImageLock.owns_lock()) {
            dl->AddImage(
                l_EditorImage.get_gpu().get_imgui_texture_id(),
                l_FallbackMin,
                l_FallbackMin +
                    ImVec2(l_FallbackSize, l_FallbackSize));
          }
        } else if (l_EditorImageLock.owns_lock()) {
          dl->AddImageRounded(
              l_EditorImage.get_gpu().get_imgui_texture_id(),
              thumb_min, thumb_max, ImVec2(0, 0), ImVec2(1, 1),
              IM_COL32_WHITE, l_Rounding,
              ImDrawFlags_RoundCornersTop);
        }
      }

      // Filename
      float text_y = thumb_max.y + l_Padding.y;
      {
        const char *name_str =
            p_FileWatcher.nameCleanPrettified.c_str();
        ImVec2 name_pos =
            ImVec2(pos.x + l_Padding.x, thumb_max.y + l_Padding.y);

        // Available width for text inside the card
        float name_w = g_CardSize.x - l_Padding.x * 2.0f;

        // Clipping rect (only text inside this box will be visible)
        ImVec2 clip_min = name_pos;
        ImVec2 clip_max =
            ImVec2(pos.x + g_CardSize.x - l_Padding.x,
                   name_pos.y + ImGui::GetTextLineHeight());

        // Draw with ellipsis
        ImGui::RenderTextEllipsis(
            dl,                // draw list
            name_pos,          // top-left pos
            clip_max,          // max pos (text won't go beyond)
            clip_max.x,        // ellipsis max x
            name_str, nullptr, // text
            nullptr            // wrap width
        );
      }

      {
        const char *type_str = Util::StringHelper::to_upper(
                                   get_asset_type_name(l_AssetType))
                                   .c_str();
        ImVec2 type_size = ImGui::CalcTextSize(type_str);
        ImVec2 type_pos(pos.x + l_Padding.x,
                        card_max.y - style.FramePadding.y -
                            type_size.y - 6.0f);
        ImVec4 l_Color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        l_Color.w = 0.3f;
        ImGui::PushFont(Fonts::UI(14, Fonts::Weight::Light));
        dl->AddText(type_pos, ImColor(l_Color), type_str);
        ImGui::PopFont();
      }

      ImGui::PopID();

      return result;
    }

    void AssetWidget::render_directory_content(
        const Util::FileSystem::DirectoryWatcher &p_DirectoryWatcher)
    {
      float l_AvailableWidth = ImGui::GetContentRegionAvail().x;
      if (l_AvailableWidth <= 0.0f) {
        return;
      }

      float l_TotalWidth = g_CardSize.x + g_Margin.x;
      const int l_Cols = LOW_MATH_MAX(
          1, (int)Math::Util::floor((l_AvailableWidth + g_Margin.x) /
                                    l_TotalWidth));

      int l_Column = 0;

      for (auto it = p_DirectoryWatcher.files.begin();
           it != p_DirectoryWatcher.files.end(); ++it) {
        const Util::FileSystem::FileWatcher &i_FileWatcher =
            Util::FileSystem::get_file_watcher(*it);
        if (i_FileWatcher.hidden) {
          continue;
        }
        AssetCardResult i_Result = draw_asset_card(i_FileWatcher);

        AssetType i_AssetType = (AssetType)i_FileWatcher.typeEnum;

        if (i_Result.clicked) {
          if (i_AssetType == AssetType::Script) {
            open_file_in_code_editor(i_FileWatcher.path);
          } else if (i_AssetType == AssetType::File &&
                     i_FileWatcher.extension == "yaml") {
            open_file_in_code_editor(i_FileWatcher.path);
          }
        }

        l_Column++;
        if (l_Column < l_Cols) {
          ImGui::SameLine(0.0f, g_Margin.x);
        } else {
          l_Column = 0;
          ImGui::Dummy(ImVec2(0, g_Margin.y)); // vertical margin
        }
      }
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
      ImGui::EndChild();

      ImGui::End();
    }

  } // namespace Editor
} // namespace Low

#undef UPDATE_INTERVAL
