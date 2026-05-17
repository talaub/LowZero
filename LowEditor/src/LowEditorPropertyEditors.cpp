#include "LowEditorPropertyEditors.h"

#include "LowEditor.h"
#include "LowEditorGui.h"
#include "LowEditorThemes.h"
#include "LowEditorBase.h"
#include "LowEditorFonts.h"

#include "LowCore.h"
#include "LowCoreScriptAsset.h"
#include "LowCoreUiWidgetAsset.h"

#include "LowUtilLogger.h"
#include "LowUtil.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include <string.h>

#include "LowUtilGlobals.h"

#include "LowRendererFont.h"
#include "LowRendererMaterial.h"
#include "LowRendererMesh.h"
#include "LowRendererModel.h"
#include "LowRendererResourceManager.h"
#include "LowRendererTexture.h"
#include "LowRendererTextureState.h"

#include <algorithm>

#define DISPLAY_LABEL(s) std::replace(s.begin(), s.end(), '_', ' ')

#define HANDLE_SELECTOR_NAME_OFFSET_X 4.0f
#define HANDLE_SELECTOR_NAME_OFFSET_Y 2.0f
#define HANDLE_SELECTOR_BUTTON_WIDTH 35.0f
#define ASSET_SELECTOR_PREVIEW_SIZE 23.0f
#define ASSET_SELECTOR_INLINE_HEIGHT 38.0f
#define ASSET_SELECTOR_INLINE_PREVIEW_SIZE 30.0f
#define ASSET_SELECTOR_POPUP_HEIGHT 320.0f
#define ASSET_SELECTOR_DIRECTORY_WIDTH 155.0f
#define ASSET_SELECTOR_CONTENT_WIDTH 360.0f

namespace Low {
  namespace Editor {
    namespace PropertyEditors {

      Util::Map<uint16_t, Util::FileSystem::WatchHandle>
          g_SelectedDirectoryPerType;
      Util::Map<AssetType, Util::String> g_SelectedAssetDirectory;

      static bool asset_type_from_handle_type(
          const uint16_t p_TypeId, AssetType &p_AssetType)
      {
        if (p_TypeId == Renderer::Mesh::type_id()) {
          p_AssetType = AssetType::Mesh;
          return true;
        }
        if (p_TypeId == Renderer::Material::type_id()) {
          p_AssetType = AssetType::Material;
          return true;
        }
        if (p_TypeId == Renderer::Texture::type_id()) {
          p_AssetType = AssetType::Texture;
          return true;
        }
        if (p_TypeId == Renderer::Font::type_id()) {
          p_AssetType = AssetType::Font;
          return true;
        }
        if (p_TypeId == Renderer::Model::type_id()) {
          p_AssetType = AssetType::Model;
          return true;
        }
        if (p_TypeId == Core::UI::WidgetAsset::type_id()) {
          p_AssetType = AssetType::UiWidget;
          return true;
        }
        if (p_TypeId == Core::Scripting::Asset::type_id()) {
          p_AssetType = AssetType::Script;
          return true;
        }
        return false;
      }

      static Renderer::EditorImage get_editor_image_for_asset_handle(
          const AssetType p_AssetType, const Util::Handle p_Handle,
          bool &p_IsFallback)
      {
        Renderer::EditorImage l_EditorImage = Util::Handle::DEAD;

        switch (p_AssetType) {
        case AssetType::Mesh: {
          Renderer::Mesh l_Mesh = p_Handle.get_id();
          if (l_Mesh.is_alive()) {
            l_EditorImage = l_Mesh.get_editor_image();
          }
          break;
        }
        case AssetType::Texture: {
          Renderer::Texture l_Texture = p_Handle.get_id();
          if (l_Texture.is_alive()) {
            l_EditorImage = l_Texture.get_editor_image();
          }
          break;
        }
        case AssetType::Font: {
          Renderer::Font l_Font = p_Handle.get_id();
          if (l_Font.is_alive()) {
            l_EditorImage = l_Font.get_editor_image();
          }
          break;
        }
        }

        if (l_EditorImage.is_alive() &&
            l_EditorImage.get_state() ==
                Renderer::TextureState::UNLOADED) {
          Renderer::ResourceManager::load_editor_image(l_EditorImage);
        }

        p_IsFallback =
            !l_EditorImage.is_alive() ||
            l_EditorImage.get_state() != Renderer::TextureState::LOADED;
        if (p_IsFallback) {
          l_EditorImage = get_editor_image_for_asset_type(p_AssetType);
        }

        return l_EditorImage;
      }

      static Util::String get_asset_path_for_handle(
          const AssetType p_AssetType, const Util::Handle p_Handle)
      {
        switch (p_AssetType) {
        case AssetType::Mesh: {
          Renderer::Mesh l_Mesh = p_Handle.get_id();
          if (l_Mesh.is_alive() && l_Mesh.get_resource().is_alive()) {
            return l_Mesh.get_resource().get_path();
          }
          break;
        }
        case AssetType::Texture: {
          Renderer::Texture l_Texture = p_Handle.get_id();
          if (l_Texture.is_alive() &&
              l_Texture.get_resource().is_alive()) {
            return l_Texture.get_resource().get_path();
          }
          break;
        }
        case AssetType::Material: {
          Renderer::Material l_Material = p_Handle.get_id();
          if (l_Material.is_alive() &&
              l_Material.get_resource().is_alive()) {
            return l_Material.get_resource().get_path();
          }
          break;
        }
        case AssetType::Font: {
          Renderer::Font l_Font = p_Handle.get_id();
          if (l_Font.is_alive() && l_Font.get_resource().is_alive()) {
            return l_Font.get_resource().get_path();
          }
          break;
        }
        case AssetType::Model: {
          Renderer::Model l_Model = p_Handle.get_id();
          if (l_Model.is_alive() && l_Model.get_resource().is_alive()) {
            return l_Model.get_resource().get_path();
          }
          break;
        }
        case AssetType::UiWidget: {
          Core::UI::WidgetAsset l_Widget = p_Handle.get_id();
          if (l_Widget.is_alive()) {
            return l_Widget.get_path();
          }
          break;
        }
        case AssetType::Script: {
          Core::Scripting::Asset l_Asset = p_Handle.get_id();
          if (l_Asset.is_alive()) {
            return l_Asset.get_source_path();
          }
          break;
        }
        }

        return "";
      }

      static Util::String get_asset_directory_for_handle(
          const AssetType p_AssetType, const Util::Handle p_Handle)
      {
        Util::String l_Path =
            Util::PathHelper::normalize(get_asset_path_for_handle(
                p_AssetType, p_Handle));

        if (l_Path.empty()) {
          return "";
        }

        l_Path = Util::StringHelper::replace(l_Path, '\\', '/');
        Util::String l_DataPath = Util::StringHelper::replace(
            Util::PathHelper::normalize(Util::get_project().dataPath),
            '\\', '/');

        if (!l_DataPath.empty() &&
            Util::StringHelper::begins_with(l_Path, l_DataPath)) {
          l_Path = l_Path.substr(l_DataPath.size());
          if (!l_Path.empty() && l_Path[0] == '/') {
            l_Path = l_Path.substr(1);
          }
        }

        const size_t l_Slash = l_Path.find_last_of('/');
        if (l_Slash == Util::String::npos) {
          return ".";
        }
        return l_Path.substr(0, l_Slash);
      }

      static bool asset_directory_matches_filter(
          const Util::String &p_AssetDirectory,
          const Util::String &p_FilterDirectory)
      {
        if (p_FilterDirectory.empty()) {
          return true;
        }

        if (p_AssetDirectory == p_FilterDirectory) {
          return true;
        }

        Util::String l_Prefix = p_FilterDirectory;
        l_Prefix += "/";
        return Util::StringHelper::begins_with(p_AssetDirectory,
                                               l_Prefix);
      }

      static bool list_contains_string(const Util::List<Util::String> &p_List,
                                       const Util::String &p_Value)
      {
        for (u32 i = 0; i < p_List.size(); ++i) {
          if (p_List[i] == p_Value) {
            return true;
          }
        }
        return false;
      }

      static void add_asset_directory_with_parents(
          Util::List<Util::String> &p_Directories,
          const Util::String &p_Directory)
      {
        if (p_Directory.empty()) {
          return;
        }

        if (p_Directory == ".") {
          if (!list_contains_string(p_Directories, p_Directory)) {
            p_Directories.push_back(p_Directory);
          }
          return;
        }

        Util::String l_Current;
        Util::List<Util::String> l_Parts;
        Util::StringHelper::split(p_Directory, '/', l_Parts);
        for (u32 i = 0; i < l_Parts.size(); ++i) {
          if (!l_Current.empty()) {
            l_Current += "/";
          }
          l_Current += l_Parts[i];

          if (!list_contains_string(p_Directories, l_Current)) {
            p_Directories.push_back(l_Current);
          }
        }
      }

      static Util::String get_asset_directory_display_name(
          const Util::String &p_Directory)
      {
        if (p_Directory == ".") {
          return "Root";
        }

        const size_t l_Slash = p_Directory.find_last_of('/');
        if (l_Slash == Util::String::npos) {
          return p_Directory;
        }
        return p_Directory.substr(l_Slash + 1);
      }

      static void draw_asset_handle_thumbnail(
          ImDrawList *p_DrawList, ImVec2 p_Min, ImVec2 p_Max,
          const AssetType p_AssetType, const Util::Handle p_Handle,
          const float p_Rounding)
      {
        bool l_IsFallback = false;
        Renderer::EditorImage l_EditorImage =
            get_editor_image_for_asset_handle(p_AssetType, p_Handle,
                                              l_IsFallback);

        p_DrawList->AddRectFilled(
            p_Min, p_Max, ImGui::GetColorU32(ImGuiCol_WindowBg),
            p_Rounding);

        if (!l_EditorImage.is_alive() ||
            l_EditorImage.get_state() != Renderer::TextureState::LOADED) {
          return;
        }

        if (l_IsFallback) {
          const ImVec2 l_Size = p_Max - p_Min;
          const float l_IconSize =
              LOW_MATH_MIN(l_Size.x, l_Size.y) * 0.66f;
          const ImVec2 l_IconMin =
              p_Min + (l_Size - ImVec2(l_IconSize, l_IconSize)) * 0.5f;
          p_DrawList->AddImage(
              l_EditorImage.get_gpu().get_imgui_texture_id(),
              l_IconMin,
              l_IconMin + ImVec2(l_IconSize, l_IconSize));
        } else {
          p_DrawList->AddImageRounded(
              l_EditorImage.get_gpu().get_imgui_texture_id(), p_Min,
              p_Max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE,
              p_Rounding);
        }
      }

      static bool render_asset_selector_row(
          const Util::FileSystem::FileWatcher &p_FileWatcher,
          const AssetType p_AssetType, const bool p_Selected)
      {
        Util::String l_DisplayName = p_FileWatcher.nameCleanPrettified;
        if (l_DisplayName.empty()) {
          l_DisplayName = "Object";
        }

        ImGui::PushID(p_FileWatcher.watchHandle);

        const float l_RowHeight = 54.0f;
        const float l_PreviewSize = 44.0f;
        const float l_Rounding = 5.0f;
        const ImVec2 l_RowSize(ImGui::GetContentRegionAvail().x,
                               l_RowHeight);
        const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        const ImVec2 l_Max = l_Pos + l_RowSize;

        ImGui::InvisibleButton("asset_selector_row", l_RowSize);
        const bool l_Hovered = ImGui::IsItemHovered();
        const bool l_Clicked = ImGui::IsItemClicked();

        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
        const Theme &l_Theme = theme_get_current();

        const ImU32 l_BackgroundColor =
            p_Selected ? color_to_imcolor(l_Theme.headerActive)
            : l_Hovered ? color_to_imcolor(l_Theme.headerHover)
                        : color_to_imcolor(l_Theme.input);

        l_DrawList->AddRectFilled(l_Pos, l_Max, l_BackgroundColor,
                                  l_Rounding);
        l_DrawList->AddRect(l_Pos, l_Max,
                            color_to_imcolor(l_Theme.border),
                            l_Rounding);
        l_DrawList->AddRectFilled(
            l_Pos, ImVec2(l_Pos.x + 3.0f, l_Max.y),
            color_to_imcolor(get_color_for_asset_type(p_AssetType)),
            l_Rounding, ImDrawFlags_RoundCornersLeft);

        const ImVec2 l_ThumbMin = l_Pos + ImVec2(10.0f, 5.0f);
        const ImVec2 l_ThumbMax =
            l_ThumbMin + ImVec2(l_PreviewSize, l_PreviewSize);
        draw_asset_handle_thumbnail(l_DrawList, l_ThumbMin,
                                    l_ThumbMax, p_AssetType,
                                    p_FileWatcher.handle, 4.0f);

        const float l_TextX = l_ThumbMax.x + 10.0f;
        const ImVec2 l_NamePos(l_TextX, l_Pos.y + 8.0f);
        const ImVec2 l_NameClipMax(l_Max.x - 10.0f,
                                   l_NamePos.y +
                                       ImGui::GetTextLineHeight());
        ImGui::RenderTextEllipsis(
            l_DrawList, l_NamePos, l_NameClipMax, l_NameClipMax.x,
            l_DisplayName.c_str(), nullptr, nullptr);

        const Util::String l_TypeName = Util::StringHelper::to_upper(
            get_asset_type_name(p_AssetType));
        ImVec4 l_TypeColor =
            ImGui::GetStyleColorVec4(ImGuiCol_Text);
        l_TypeColor.w = 0.3f;
        ImGui::PushFont(Fonts::UI(14, Fonts::Weight::Light));
        l_DrawList->AddText(ImVec2(l_TextX, l_Pos.y + 31.0f),
                            ImColor(l_TypeColor),
                            l_TypeName.c_str());
        ImGui::PopFont();

        ImGui::PopID();
        return l_Clicked;
      }

      static bool render_asset_selector_row(
          const Util::Handle p_Handle, const Util::String &p_DisplayName,
          const AssetType p_AssetType, const bool p_Selected)
      {
        ImGui::PushID((void *)p_Handle.get_id());

        const float l_RowHeight = 54.0f;
        const float l_PreviewSize = 44.0f;
        const float l_Rounding = 5.0f;
        const ImVec2 l_RowSize(ImGui::GetContentRegionAvail().x,
                               l_RowHeight);
        const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        const ImVec2 l_Max = l_Pos + l_RowSize;

        ImGui::InvisibleButton("asset_selector_row", l_RowSize);
        const bool l_Hovered = ImGui::IsItemHovered();
        const bool l_Clicked = ImGui::IsItemClicked();

        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
        const Theme &l_Theme = theme_get_current();

        const ImU32 l_BackgroundColor =
            p_Selected ? color_to_imcolor(l_Theme.headerActive)
            : l_Hovered ? color_to_imcolor(l_Theme.headerHover)
                        : color_to_imcolor(l_Theme.input);

        l_DrawList->AddRectFilled(l_Pos, l_Max, l_BackgroundColor,
                                  l_Rounding);
        l_DrawList->AddRect(l_Pos, l_Max,
                            color_to_imcolor(l_Theme.border),
                            l_Rounding);
        l_DrawList->AddRectFilled(
            l_Pos, ImVec2(l_Pos.x + 3.0f, l_Max.y),
            color_to_imcolor(get_color_for_asset_type(p_AssetType)),
            l_Rounding, ImDrawFlags_RoundCornersLeft);

        const ImVec2 l_ThumbMin = l_Pos + ImVec2(10.0f, 5.0f);
        const ImVec2 l_ThumbMax =
            l_ThumbMin + ImVec2(l_PreviewSize, l_PreviewSize);
        draw_asset_handle_thumbnail(l_DrawList, l_ThumbMin,
                                    l_ThumbMax, p_AssetType,
                                    p_Handle, 4.0f);

        const float l_TextX = l_ThumbMax.x + 10.0f;
        const ImVec2 l_NamePos(l_TextX, l_Pos.y + 8.0f);
        const ImVec2 l_NameClipMax(l_Max.x - 10.0f,
                                   l_NamePos.y +
                                       ImGui::GetTextLineHeight());
        ImGui::RenderTextEllipsis(
            l_DrawList, l_NamePos, l_NameClipMax, l_NameClipMax.x,
            p_DisplayName.c_str(), nullptr, nullptr);

        const Util::String l_TypeName = Util::StringHelper::to_upper(
            get_asset_type_name(p_AssetType));
        ImVec4 l_TypeColor =
            ImGui::GetStyleColorVec4(ImGuiCol_Text);
        l_TypeColor.w = 0.3f;
        ImGui::PushFont(Fonts::UI(14, Fonts::Weight::Light));
        l_DrawList->AddText(ImVec2(l_TextX, l_Pos.y + 31.0f),
                            ImColor(l_TypeColor),
                            l_TypeName.c_str());
        ImGui::PopFont();

        ImGui::PopID();
        return l_Clicked;
      }

      static bool render_asset_selector_line(
          Util::String p_Label,
          const Util::Function<bool(float)> &p_DrawEditor)
      {
        static Util::Name l_SplitterName =
            N(LOW_EDITOR_DETAILS_SPLITTER);

        float l_Splitter = Util::Globals::get(l_SplitterName);

        const float l_Min = 90.0f;
        const float l_Max = 420.0f;
        const float l_GrabWidth = 6.0f;
        const float l_Gap = 8.0f;
        const float l_LineThickness = 1.0f;
        const float l_RowHeight = ASSET_SELECTOR_INLINE_HEIGHT + 10.0f;

        ImGui::PushID(p_Label.c_str());

        const float l_AvailW = ImGui::GetContentRegionAvail().x;
        const float l_TextH = ImGui::GetTextLineHeight();

        float l_LabelW =
            Low::Math::Util::clamp(l_Splitter, l_Min, l_Max);
        const float l_EditorW =
            ImMax(10.0f, l_AvailW - (l_LabelW + l_Gap));

        const float l_PrevBottomY = ImGui::GetItemRectMax().y;
        const ImVec2 l_RowTop = ImGui::GetCursorScreenPos();
        const bool l_IsContiguous =
            ImFabs(l_PrevBottomY - l_RowTop.y) <= 1.0f;

        const float l_LabelY =
            l_RowTop.y + (l_RowHeight - l_TextH) * 0.5f;
        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(), ImVec2(l_RowTop.x, l_LabelY),
            ImVec2(l_RowTop.x + l_LabelW,
                   l_RowTop.y + l_RowHeight),
            l_LabelW, p_Label.c_str(), nullptr, nullptr);

        ImGui::SetCursorScreenPos(
            ImVec2(l_RowTop.x + l_LabelW + l_Gap,
                   l_RowTop.y +
                       (l_RowHeight - ASSET_SELECTOR_INLINE_HEIGHT) *
                           0.5f));
        ImGui::SetNextItemWidth(l_EditorW);
        const bool l_Result = p_DrawEditor(l_EditorW);

        {
          const float l_SplitOffset = l_LabelW - l_GrabWidth * 0.5f;
          ImGui::SetCursorScreenPos(
              ImVec2(l_RowTop.x + l_SplitOffset, l_RowTop.y));
          ImGui::InvisibleButton("##RowSplitter",
                                 ImVec2(l_GrabWidth, l_RowHeight));
          const bool l_Active = ImGui::IsItemActive();
          if (ImGui::IsItemHovered() || l_Active) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
          }
          if (l_Active) {
            l_Splitter += ImGui::GetIO().MouseDelta.x;
            l_Splitter =
                Low::Math::Util::clamp(l_Splitter, l_Min, l_Max);
            Util::Globals::set(l_SplitterName, l_Splitter);
            l_LabelW = l_Splitter;
          }
        }

        auto snap = [](float v) { return IM_FLOOR(v) + 0.5f; };
        const float yTopSep =
            snap(l_IsContiguous ? l_PrevBottomY : l_RowTop.y);
        const float yBottomSep = snap(l_RowTop.y + l_RowHeight);
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(l_RowTop.x, yBottomSep),
            ImVec2(l_RowTop.x + l_AvailW, yBottomSep),
            ImGui::GetColorU32(ImGuiCol_TableBorderLight));

        const float l_SplitX = snap(l_RowTop.x + l_LabelW);
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(l_SplitX, yTopSep), ImVec2(l_SplitX, yBottomSep),
            ImGui::GetColorU32(ImGuiCol_Separator), l_LineThickness);

        ImGui::SetCursorScreenPos(
            ImVec2(l_RowTop.x, l_RowTop.y + l_RowHeight));

        ImGui::PopID();
        return l_Result;
      }

      static bool render_asset_handle_selector(
          Util::String p_Label, Util::RTTI::TypeInfo &p_TypeInfo,
          uint64_t *p_HandleId, const AssetType p_AssetType)
      {
        return render_asset_selector_line(
            p_Label, [&p_Label, &p_TypeInfo, p_HandleId,
                      p_AssetType](float p_EditorWidth) {
          ImVec2 l_Pos = ImGui::GetCursorScreenPos();
          float l_ButtonWidth = 34.0f;
          float l_FieldWidth = p_EditorWidth;

          bool l_Changed = false;
          Util::Handle l_CurrentHandle = *p_HandleId;
          Util::String l_PopupName =
              Util::String("_choose_asset_element_") + p_Label;

          const char *l_DisplayName = "None";
          Util::RTTI::PropertyInfo l_NameProperty;
          bool l_HasNameProperty = false;

          if (p_TypeInfo.properties.find(N(name)) !=
              p_TypeInfo.properties.end()) {
            l_NameProperty = p_TypeInfo.properties[N(name)];

            if (p_TypeInfo.is_alive(l_CurrentHandle)) {
              Util::Name l_Name;
              l_NameProperty.get(l_CurrentHandle, &l_Name);
              l_DisplayName = l_Name.c_str();
            }
            l_HasNameProperty = true;
          }

          const int l_NameLength = strlen(l_DisplayName);
          ImVec2 l_CursorPos = ImGui::GetCursorScreenPos();
          const float l_FieldHeight = ASSET_SELECTOR_INLINE_HEIGHT;
          ImVec2 l_WidgetSize =
              ImVec2(l_FieldWidth, l_FieldHeight);

          ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
          const Theme &l_Theme = theme_get_current();
          const float l_Rounding = 3.0f;
          const ImVec2 l_FieldMax = l_CursorPos + l_WidgetSize;
          const ImVec2 l_ButtonMin(l_FieldMax.x - l_ButtonWidth,
                                   l_CursorPos.y);

          l_DrawList->AddRectFilled(
              l_CursorPos, l_FieldMax, color_to_imcolor(l_Theme.input),
              l_Rounding);
          l_DrawList->AddRect(l_CursorPos, l_FieldMax,
                              color_to_imcolor(l_Theme.border),
                              l_Rounding);
          l_DrawList->AddRectFilled(
              l_CursorPos, ImVec2(l_CursorPos.x + 3.0f, l_FieldMax.y),
              color_to_imcolor(get_color_for_asset_type(p_AssetType)),
              l_Rounding, ImDrawFlags_RoundCornersLeft);
          l_DrawList->AddLine(
              ImVec2(l_ButtonMin.x, l_CursorPos.y + 6.0f),
              ImVec2(l_ButtonMin.x, l_FieldMax.y - 6.0f),
              color_to_imcolor(l_Theme.button));

          ImGui::InvisibleButton("##asset_handle_value",
                                 ImVec2(l_FieldWidth, l_FieldHeight));
          if (ImGui::IsItemClicked()) {
            ImGui::OpenPopup(l_PopupName.c_str());
          }

          if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *l_Payload =
                    ImGui::AcceptDragDropPayload("DG_HANDLE")) {
              Util::Handle l_PayloadHandle =
                  *(uint64_t *)l_Payload->Data;

              if (p_TypeInfo.is_alive(l_PayloadHandle)) {
                l_Changed = true;
                *p_HandleId = l_PayloadHandle.get_id();
              }
            }
            ImGui::EndDragDropTarget();
          }

          const ImVec2 l_ThumbnailMin =
              l_Pos +
              ImVec2(5.0f,
                     (l_FieldHeight -
                      ASSET_SELECTOR_INLINE_PREVIEW_SIZE) *
                             0.5f +
                         1.0f);
          const ImVec2 l_ThumbnailMax =
              l_ThumbnailMin +
              ImVec2(ASSET_SELECTOR_INLINE_PREVIEW_SIZE,
                     ASSET_SELECTOR_INLINE_PREVIEW_SIZE);
          draw_asset_handle_thumbnail(l_DrawList, l_ThumbnailMin,
                                      l_ThumbnailMax, p_AssetType,
                                      l_CurrentHandle, 4.0f);

          const float l_TextY =
              l_Pos.y +
              (l_FieldHeight - ImGui::GetTextLineHeight()) * 0.5f +
              1.0f;
          const float l_TextX =
              l_Pos.x + ASSET_SELECTOR_INLINE_PREVIEW_SIZE + 14.0f;
          ImGui::RenderTextEllipsis(
              l_DrawList, ImVec2(l_TextX, l_TextY),
              ImVec2(l_ButtonMin.x - 8.0f, l_Pos.y + l_FieldHeight),
              l_ButtonMin.x - 10.0f,
              l_DisplayName, l_DisplayName + l_NameLength, nullptr);

          const ImVec2 l_IconSize =
              ImGui::CalcTextSize(ICON_LC_CIRCLE_DOT);
          l_DrawList->AddText(
              ImVec2(l_ButtonMin.x +
                         (l_ButtonWidth - l_IconSize.x) * 0.5f,
                     l_Pos.y +
                         (l_FieldHeight - l_IconSize.y) * 0.5f +
                         1.0f),
              color_to_imcolor(l_Theme.subtext), ICON_LC_CIRCLE_DOT);

          if (ImGui::BeginPopup(l_PopupName.c_str())) {
#define SEARCH_BUFFER_SIZE 255
            static char l_SearchBuffer[SEARCH_BUFFER_SIZE] = {'\0'};
            Gui::SearchField("##asset_search", l_SearchBuffer,
                             SEARCH_BUFFER_SIZE, {0.0f, 4.0f});
#undef SEARCH_BUFFER_SIZE
            ImGui::Spacing();

            Util::Handle *l_Handles =
                p_TypeInfo.get_living_instances();

            Util::List<Util::String> l_Directories;
            for (uint32_t i = 0u; i < p_TypeInfo.get_living_count();
                 ++i) {
              add_asset_directory_with_parents(
                  l_Directories,
                  get_asset_directory_for_handle(p_AssetType,
                                                 l_Handles[i]));
            }

            Util::String &l_SelectedDirectory =
                g_SelectedAssetDirectory[p_AssetType];
            if (!l_SelectedDirectory.empty() &&
                !list_contains_string(l_Directories,
                                      l_SelectedDirectory)) {
              l_SelectedDirectory = "";
            }

            const bool l_ShowDirectories = !l_Directories.empty();
            if (l_ShowDirectories) {
              ImGui::BeginChild(
                  "Directories",
                  ImVec2(ASSET_SELECTOR_DIRECTORY_WIDTH,
                         ASSET_SELECTOR_POPUP_HEIGHT),
                  true);

              if (ImGui::Selectable("All",
                                    l_SelectedDirectory.empty())) {
                l_SelectedDirectory = "";
              }

              for (u32 i = 0; i < l_Directories.size(); ++i) {
                const Util::String &i_Directory = l_Directories[i];
                Util::String i_Display =
                    get_asset_directory_display_name(i_Directory);

                if (ImGui::Selectable(
                        i_Display.c_str(),
                        l_SelectedDirectory == i_Directory)) {
                  l_SelectedDirectory = i_Directory;
                }
              }

              ImGui::EndChild();
              ImGui::SameLine();
            }

            ImGui::BeginChild(
                "Assets",
                ImVec2(ASSET_SELECTOR_CONTENT_WIDTH,
                       ASSET_SELECTOR_POPUP_HEIGHT),
                true);

            for (uint32_t i = 0u; i < p_TypeInfo.get_living_count();
                 ++i) {
              Util::String i_DisplayName = "Object";
              if (l_HasNameProperty) {
                Util::Name i_Name;
                l_NameProperty.get(l_Handles[i], &i_Name);
                i_DisplayName = i_Name.c_str();
              }

              if (strlen(l_SearchBuffer) > 0 &&
                  !strstr(i_DisplayName.c_str(), l_SearchBuffer)) {
                continue;
              }

              if (l_ShowDirectories &&
                  !asset_directory_matches_filter(
                      get_asset_directory_for_handle(p_AssetType,
                                                     l_Handles[i]),
                      l_SelectedDirectory)) {
                continue;
              }

              if (render_asset_selector_row(
                      l_Handles[i], i_DisplayName, p_AssetType,
                      l_Handles[i].get_id() == *p_HandleId)) {
                l_SearchBuffer[0] = '\0';
                *p_HandleId = l_Handles[i].get_id();
                l_Changed = true;
                ImGui::CloseCurrentPopup();
              }
            }

            ImGui::EndChild();
            ImGui::EndPopup();
          }

          return l_Changed;
        });
      }

      bool render_enum_selector(u16 p_EnumId, u8 *p_Value,
                                Util::String p_Label,
                                bool p_RenderLabel)
      {
        return render_enum_selector(p_EnumId, p_Value, p_Label,
                                    p_RenderLabel, Util::List<u8>());
      }

      bool render_enum_selector(u16 p_EnumId, u8 *p_Value,
                                Util::String p_Label,
                                bool p_RenderLabel,
                                Util::List<u8> p_FilterList)
      {
        return render_line(p_Label, [&p_Label, &p_EnumId, p_Value,
                                     &p_FilterList]() {
          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          Util::RTTI::EnumInfo &l_EnumInfo =
              Util::get_enum_info(p_EnumId);

          Util::List<Util::RTTI::EnumEntryInfo> l_FilteredEntries;

          for (u32 i = 0; i < l_EnumInfo.entries.size(); ++i) {
            bool i_Filtered = false;
            for (u32 j = 0; j < p_FilterList.size(); ++j) {
              if (l_EnumInfo.entries[i].value == p_FilterList[j]) {
                i_Filtered = true;
                break;
              }
            }

            if (!i_Filtered) {
              l_FilteredEntries.push_back(l_EnumInfo.entries[i]);
            }
          }

          u8 l_CurrentValue = *p_Value;

          bool l_Result = false;

          if (ImGui::BeginCombo(
                  l_Label.c_str(),
                  l_EnumInfo.entry_name(l_CurrentValue).c_str())) {
            for (int i = 0; i < l_EnumInfo.entries.size(); i++) {
              bool i_Disabled = false;
              for (u32 j = 0; j < p_FilterList.size(); ++j) {
                if (l_EnumInfo.entries[i].value == p_FilterList[j]) {
                  i_Disabled = true;
                  break;
                }
              }

              if (i_Disabled) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                    ImGui::GetStyle().Alpha * 0.5f);
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
              }

              if (ImGui::Selectable(
                      l_EnumInfo.entries[i].name.c_str(),
                      l_CurrentValue ==
                          l_EnumInfo.entries[i].value)) {
                if (!i_Disabled) // Only allow selection if the item
                                 // is not disabled
                {
                  l_CurrentValue = l_EnumInfo.entries[i].value;
                  l_Result = true;
                }
              }

              if (i_Disabled) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
              }

              // Set the initial focus when opening the combo
              // (scrolling
              // + keyboard navigation focus)
              if (l_CurrentValue == l_EnumInfo.entries[i].value) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }

          *p_Value = l_CurrentValue;

          return l_Result;
        });
      }

      bool render_enum_selector(PropertyMetadata &p_Metadata,
                                Util::Handle p_Handle)
      {
        u8 l_CurrentValue;
        p_Metadata.propInfo.get(p_Handle, &l_CurrentValue);

        bool l_Result = render_enum_selector(
            p_Metadata.propInfo.handleType, &l_CurrentValue,
            p_Metadata.friendlyName.c_str(), true);

        p_Metadata.propInfo.set(p_Handle, &l_CurrentValue);

        return l_Result;
      }

      bool render_name_editor(Util::String &p_Label,
                              Util::Name &p_Name, bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Name, &p_Label]() {
          char l_Buffer[255];
          uint32_t l_NameLength = strlen(p_Name.c_str());
          memcpy(l_Buffer, p_Name.c_str(), l_NameLength);
          l_Buffer[l_NameLength] = '\0';

          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          if (Gui::InputText(l_Label.c_str(), l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            p_Name.m_Index = LOW_NAME(l_Buffer).m_Index;
            return true;
          }
          return false;
        });
      }

      bool render_string_editor(Util::String &p_Label,
                                Util::String &p_String,
                                bool p_Multiline, bool p_RenderLabel)
      {

        return render_line(p_Label, [&p_String, &p_Label,
                                     p_Multiline]() {
          char l_Buffer[1024];
          uint32_t l_NameLength = strlen(p_String.c_str());
          memcpy(l_Buffer, p_String.c_str(), l_NameLength);
          l_Buffer[l_NameLength] = '\0';

          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          if (p_Multiline) {
            if (ImGui::InputTextMultiline(
                    l_Label.c_str(), l_Buffer, 1024,
                    ImVec2(-FLT_MIN, 90),
                    ImGuiInputTextFlags_EnterReturnsTrue |
                        ImGuiInputTextFlags_CtrlEnterForNewLine)) {
              p_String = l_Buffer;
              return true;
            }
          } else {
            if (Gui::InputText(
                    l_Label.c_str(), l_Buffer, 1024,
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
              p_String = l_Buffer;
              return true;
            }
          }
          return false;
        });
      }

      void render_string_editor(PropertyMetadata &p_PropertyMetadata,
                                Util::String &p_Label,
                                Util::String &p_String,
                                bool p_RenderLabel)
      {
        render_string_editor(p_Label, p_String,
                             p_PropertyMetadata.multiline,
                             p_RenderLabel);
      }

      bool render_quaternion_editor(Util::String p_Label,
                                    Math::Quaternion &p_Quaternion,
                                    bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Quaternion]() {
          Math::Vector3 l_Vector =
              Math::VectorUtil::to_euler(p_Quaternion);

          if (Gui::Vector3Edit(l_Vector)) {
            p_Quaternion = Math::VectorUtil::from_euler(l_Vector);
            return true;
          }

          return false;
        });
      }

      bool render_vector3_editor(Util::String p_Label,
                                 Math::Vector3 &p_Vector,
                                 bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Vector]() {
          Math::Vector3 l_Vector = p_Vector;

          if (Gui::Vector3Edit(l_Vector)) {
            p_Vector = l_Vector;
            return true;
          }

          return false;
        });
      }

      bool render_vector2_editor(Util::String &p_Label,
                                 Math::Vector2 &p_Vector,
                                 bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Label, &p_Vector]() {
          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          return ImGui::DragFloat2(l_Label.c_str(),
                                   (float *)&p_Vector);
        });
      }

      bool render_checkbox_bool_editor(Util::String &p_Label,
                                       bool &p_Bool,
                                       bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Label, &p_Bool]() {
          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          return Base::BoolEdit(l_Label.c_str(), &p_Bool);
        });
      }

      bool render_float_editor(Util::String &p_Label, float &p_Float,
                               bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Label, &p_Float]() {
          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          // ImGui::DragFloat(l_Label.c_str(), &p_Float);
          return Gui::DragFloatWithButtons(l_Label.c_str(), &p_Float);
        });
      }

      bool render_uint32_editor(Util::String &p_Label, u32 &p_Value,
                                bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Label, &p_Value]() {
          int l_LocalValue = p_Value;

          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          if (Gui::DragIntWithButtons(l_Label.c_str(), &l_LocalValue,
                                      1, 0, 50000)) {
            p_Value = l_LocalValue;
            return true;
          }
          return false;
        });
      }

      // bool render_enum_selector(Util::String p_Label, )

      bool render_handle_selector(Util::String p_Label,
                                  uint16_t p_Type,
                                  uint64_t *p_HandleId)
      {
        return render_handle_selector(
            p_Label, Util::Handle::get_type_info(p_Type), p_HandleId);
      }

      bool render_handle_selector(Util::String p_Label,
                                  Util::RTTI::TypeInfo &p_TypeInfo,
                                  uint64_t *p_HandleId)
      {
        AssetType l_AssetType = AssetType::File;
        if (asset_type_from_handle_type(p_TypeInfo.typeId,
                                        l_AssetType)) {
          return render_asset_handle_selector(
              p_Label, p_TypeInfo, p_HandleId, l_AssetType);
        }

        return render_line(p_Label, [&p_Label, &p_TypeInfo,
                                     p_HandleId]() {
          ImGui::BeginGroup();

          ImVec2 l_Pos = ImGui::GetCursorScreenPos();
          float l_FullWidth = ImGui::GetContentRegionAvail().x;
          float l_ButtonWidth = HANDLE_SELECTOR_BUTTON_WIDTH;
          float l_NameWidth =
              l_FullWidth - l_ButtonWidth - LOW_EDITOR_SPACING;

          bool l_Changed = false;

          Util::Handle l_CurrentHandle = *p_HandleId;

          Util::String l_PopupName =
              Util::String("_choose_element_") + p_Label;

          const char *l_DisplayName = "None";

          Util::RTTI::PropertyInfo l_NameProperty;
          bool l_HasNameProperty = false;

          if (p_TypeInfo.properties.find(N(name)) !=
              p_TypeInfo.properties.end()) {

            l_NameProperty = p_TypeInfo.properties[N(name)];

            if (p_TypeInfo.is_alive(l_CurrentHandle)) {
              Util::Name l_Name;
              p_TypeInfo.properties[N(name)].get(l_CurrentHandle,
                                                 &l_Name);
              l_DisplayName = l_Name.c_str();
            }
            l_HasNameProperty = true;
          }
          int l_NameLength = strlen(l_DisplayName);

          ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
          ImVec2 widget_size = ImVec2(
              l_NameWidth + 2.0f,
              ImGui::GetFrameHeight()); // Customize width as needed

          // Draw background with custom corner rounding
          ImDrawList *draw_list = ImGui::GetWindowDrawList();
          ImVec4 bg_color = color_to_imvec4(
              theme_get_current().input); // Customize color here
          draw_list->AddRectFilled(
              cursor_pos,
              ImVec2(cursor_pos.x + widget_size.x,
                     cursor_pos.y + widget_size.y),
              ImGui::GetColorU32(bg_color),
              2.0f, // Rounding amount
              ImDrawFlags_RoundCornersLeft
              // corners
          );

          ImGui::SetCursorScreenPos(cursor_pos); // Reset cursor
                                                 // position
          ImGui::RenderTextEllipsis(
              ImGui::GetWindowDrawList(),
              l_Pos + ImVec2(HANDLE_SELECTOR_NAME_OFFSET_X,
                             HANDLE_SELECTOR_NAME_OFFSET_Y),
              l_Pos +
                  ImVec2(l_NameWidth, LOW_EDITOR_LABEL_HEIGHT_ABS),
              l_Pos.x + l_NameWidth - LOW_EDITOR_SPACING,
              l_DisplayName, l_DisplayName + l_NameLength, nullptr);

          ImGui::SetCursorScreenPos(l_Pos +
                                    ImVec2(l_NameWidth, 0.0f));

          if (ImGui::Button(ICON_LC_CIRCLE_DOT)) {
            ImGui::OpenPopup(l_PopupName.c_str());
          }

          if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *l_Payload =
                    ImGui::AcceptDragDropPayload("DG_HANDLE")) {
              Util::Handle l_PayloadHandle =
                  *(uint64_t *)l_Payload->Data;

              if (p_TypeInfo.is_alive(l_PayloadHandle)) {
                l_Changed = true;
                *p_HandleId = l_PayloadHandle.get_id();
              }
            }
            ImGui::EndDragDropTarget();
          }

          if (ImGui::BeginPopup(l_PopupName.c_str())) {
#define SEARCH_BUFFER_SIZE 255
            static char l_SearchBuffer[SEARCH_BUFFER_SIZE] = {'\0'};
            Gui::SearchField("##search", l_SearchBuffer,
                             SEARCH_BUFFER_SIZE, {0.0f, 3.0f});

#undef SEARCH_BUFFER_SIZE

            Util::Handle *l_Handles =
                p_TypeInfo.get_living_instances();

            for (uint32_t i = 0u; i < p_TypeInfo.get_living_count();
                 ++i) {
              Util::String i_DisplayName = "Object";
              if (l_HasNameProperty) {
                Util::Name i_Name;
                l_NameProperty.get(l_Handles[i], &i_Name);
                i_DisplayName = i_Name.c_str();
              }

              if (strlen(l_SearchBuffer) > 0 &&
                  !strstr(i_DisplayName.c_str(), l_SearchBuffer)) {
                continue;
              }
              if (ImGui::Selectable(i_DisplayName.c_str())) {
                l_SearchBuffer[0] = '\0';
                *p_HandleId = l_Handles[i].get_id();
                l_Changed = true;
              }
            }
            ImGui::EndPopup();
          }

          ImGui::EndGroup();
          return l_Changed;
        });
      }

      bool render_fs_handle_selector_directory_entry(
          Util::FileSystem::DirectoryWatcher &p_DirectoryWatcher,
          Util::FileSystem::WatchHandle p_WatchHandle,
          uint16_t p_TypeId)
      {
        auto l_Pos = g_SelectedDirectoryPerType.find(p_TypeId);

        if (ImGui::Selectable(p_DirectoryWatcher.name.c_str(),
                              p_WatchHandle == l_Pos->second)) {
          l_Pos->second = p_WatchHandle;
          return true;
        }
        return false;
      }

      bool render_fs_handle_selector_directory_structure(
          Util::FileSystem::WatchHandle p_WatchHandle,
          uint16_t p_TypeId)
      {
        Util::FileSystem::DirectoryWatcher &l_DirectoryWatcher =
            Util::FileSystem::get_directory_watcher(p_WatchHandle);

        bool l_Break = false;

        if (l_DirectoryWatcher.subdirectories.empty()) {
          l_Break = render_fs_handle_selector_directory_entry(
              l_DirectoryWatcher, p_WatchHandle, p_TypeId);
        } else {
          Util::String l_IdString = "##";
          l_IdString += p_WatchHandle;
          bool l_Open = ImGui::TreeNode(l_IdString.c_str());
          ImGui::SameLine();
          l_Break = render_fs_handle_selector_directory_entry(
              l_DirectoryWatcher, p_WatchHandle, p_TypeId);

          if (l_Open) {
            for (auto it = l_DirectoryWatcher.subdirectories.begin();
                 it != l_DirectoryWatcher.subdirectories.end();
                 ++it) {
              l_Break = render_fs_handle_selector_directory_structure(
                  *it, p_TypeId);
            }
            ImGui::TreePop();
          }
        }

        return l_Break;
      }

      bool render_fs_handle_selector(Util::String p_Label,
                                     Util::RTTI::TypeInfo &p_TypeInfo,
                                     uint64_t *p_HandleId)
      {
        AssetType l_AssetType = AssetType::File;
        const bool l_IsAssetSelector = asset_type_from_handle_type(
            p_TypeInfo.typeId, l_AssetType);

        Util::FileSystem::WatchHandle l_RootDirectoryWatchHandle =
            Core::get_filesystem_watcher(p_TypeInfo.typeId);

        {
          auto l_Pos =
              g_SelectedDirectoryPerType.find(p_TypeInfo.typeId);

          if (l_Pos == g_SelectedDirectoryPerType.end()) {
            g_SelectedDirectoryPerType[p_TypeInfo.typeId] =
                l_RootDirectoryWatchHandle;
          }
        }

        ImGui::BeginGroup();

        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_ButtonWidth = HANDLE_SELECTOR_BUTTON_WIDTH;
        float l_NameWidth =
            l_FullWidth - l_ButtonWidth - LOW_EDITOR_SPACING;

        bool l_Changed = false;

        Util::Handle l_CurrentHandle = *p_HandleId;

        Util::String l_PopupName =
            Util::String("_choose_element_") + p_Label;

        const char *l_DisplayName = "None";

        Util::RTTI::PropertyInfo l_NameProperty;
        bool l_HasNameProperty = false;

        if (p_TypeInfo.properties.find(N(name)) !=
            p_TypeInfo.properties.end()) {

          l_NameProperty = p_TypeInfo.properties[N(name)];

          if (p_TypeInfo.is_alive(l_CurrentHandle)) {
            Util::Name l_Name;
            p_TypeInfo.properties[N(name)].get(l_CurrentHandle,
                                               &l_Name);
            l_DisplayName = l_Name.c_str();
          }
          l_HasNameProperty = true;
        }
        int l_NameLength = strlen(l_DisplayName);

        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 widget_size = ImVec2(
            l_NameWidth + 3.0f,
            ImGui::GetFrameHeight()); // Customize width as needed

        // Draw background with custom corner rounding
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec4 bg_color = color_to_imvec4(
            theme_get_current().input); // Customize color here
        draw_list->AddRectFilled(cursor_pos,
                                 ImVec2(cursor_pos.x + widget_size.x,
                                        cursor_pos.y + widget_size.y),
                                 ImGui::GetColorU32(bg_color),
                                 2.0f, // Rounding amount
                                 ImDrawFlags_RoundCornersLeft
                                 // corners
        );

        ImGui::SetCursorScreenPos(cursor_pos); // Reset cursor
                                               // position

        float l_TextOffsetX = HANDLE_SELECTOR_NAME_OFFSET_X;
        if (l_IsAssetSelector) {
          const ImVec2 l_ThumbnailMin =
              l_Pos + ImVec2(3.0f, 2.0f);
          const ImVec2 l_ThumbnailMax =
              l_ThumbnailMin + ImVec2(ASSET_SELECTOR_PREVIEW_SIZE,
                                      ASSET_SELECTOR_PREVIEW_SIZE);
          draw_asset_handle_thumbnail(draw_list, l_ThumbnailMin,
                                      l_ThumbnailMax, l_AssetType,
                                      l_CurrentHandle, 2.0f);
          draw_list->AddRectFilled(
              l_Pos, ImVec2(l_Pos.x + 3.0f,
                            l_Pos.y + ImGui::GetFrameHeight()),
              color_to_imcolor(get_color_for_asset_type(l_AssetType)),
              2.0f, ImDrawFlags_RoundCornersLeft);
          l_TextOffsetX = ASSET_SELECTOR_PREVIEW_SIZE + 9.0f;
        }

        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(),
            l_Pos + ImVec2(l_TextOffsetX,
                           HANDLE_SELECTOR_NAME_OFFSET_Y),
            l_Pos + ImVec2(l_NameWidth, LOW_EDITOR_LABEL_HEIGHT_ABS),
            l_Pos.x + l_NameWidth - LOW_EDITOR_SPACING, l_DisplayName,
            l_DisplayName + l_NameLength, nullptr);

        ImGui::SetCursorScreenPos(l_Pos + ImVec2(l_NameWidth, 0.0f));

        if (ImGui::Button(ICON_LC_CIRCLE_DOT)) {
          ImGui::OpenPopup(l_PopupName.c_str());
        }

        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *l_Payload =
                  ImGui::AcceptDragDropPayload("DG_HANDLE")) {
            Util::Handle l_PayloadHandle =
                *(uint64_t *)l_Payload->Data;

            if (p_TypeInfo.is_alive(l_PayloadHandle)) {
              l_Changed = true;
              *p_HandleId = l_PayloadHandle.get_id();
            }
          }
          ImGui::EndDragDropTarget();
        }

        float l_PopupHeight =
            l_IsAssetSelector ? ASSET_SELECTOR_POPUP_HEIGHT : 200.0f;
        float l_PopupSelectorWidth =
            l_IsAssetSelector ? ASSET_SELECTOR_CONTENT_WIDTH : 300.0f;

        if (ImGui::BeginPopup(l_PopupName.c_str())) {
#define SEARCH_BUFFER_SIZE 255
          static char l_SearchBuffer[SEARCH_BUFFER_SIZE] = {'\0'};
          if (l_IsAssetSelector) {
            Gui::SearchField("##asset_search", l_SearchBuffer,
                             SEARCH_BUFFER_SIZE, {0.0f, 4.0f});
            ImGui::Spacing();
          }
#undef SEARCH_BUFFER_SIZE

          ImGui::BeginChild(
              "Categories",
              ImVec2(l_IsAssetSelector ? ASSET_SELECTOR_DIRECTORY_WIDTH
                                        : 150.0f,
                     l_PopupHeight),
              true, 0);
          render_fs_handle_selector_directory_structure(
              l_RootDirectoryWatchHandle, p_TypeInfo.typeId);

          ImGui::EndChild();

          ImGui::SameLine();

          ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
          ImRect l_Rect(l_Cursor, {l_Cursor.x + l_PopupSelectorWidth,
                                   l_Cursor.y + l_PopupHeight});

          Util::RTTI::PropertyInfo l_NameProperty;
          bool l_HasNameProperty = false;

          if (p_TypeInfo.properties.find(N(name)) !=
              p_TypeInfo.properties.end()) {

            l_NameProperty = p_TypeInfo.properties[N(name)];

            if (p_TypeInfo.is_alive(l_CurrentHandle)) {
              Util::Name l_Name;
              p_TypeInfo.properties[N(name)].get(l_CurrentHandle,
                                                 &l_Name);
              l_DisplayName = l_Name.c_str();
            }
            l_HasNameProperty = true;
          }

          Util::FileSystem::DirectoryWatcher
              &l_CurrentDirectoryWatcher =
                  Util::FileSystem::get_directory_watcher(
                      g_SelectedDirectoryPerType[p_TypeInfo.typeId]);

          ImGui::BeginChild(
              "Content", ImVec2(l_PopupSelectorWidth, l_PopupHeight),
              true);

          for (auto it = l_CurrentDirectoryWatcher.files.begin();
               it != l_CurrentDirectoryWatcher.files.end(); ++it) {
            Util::FileSystem::FileWatcher &i_FileWatcher =
                Util::FileSystem::get_file_watcher(*it);
            if (i_FileWatcher.hidden) {
              continue;
            }

            Util::Name i_Name;
            l_NameProperty.get(i_FileWatcher.handle, &i_Name);

            if (l_IsAssetSelector) {
              Util::String i_DisplayName =
                  i_FileWatcher.nameCleanPrettified;
              if (i_DisplayName.empty()) {
                i_DisplayName = i_Name.c_str();
              }

              if (strlen(l_SearchBuffer) > 0 &&
                  !strstr(i_DisplayName.c_str(), l_SearchBuffer) &&
                  !strstr(i_Name.c_str(), l_SearchBuffer)) {
                continue;
              }

              if (render_asset_selector_row(
                      i_FileWatcher, l_AssetType,
                      i_FileWatcher.handle.get_id() == *p_HandleId)) {
                l_SearchBuffer[0] = '\0';
                *p_HandleId = i_FileWatcher.handle.get_id();
                l_Changed = true;
                ImGui::CloseCurrentPopup();
              }
            } else {
              if (ImGui::Selectable(i_Name.c_str(), false)) {
                *p_HandleId = i_FileWatcher.handle.get_id();
                l_Changed = true;
              }
            }
          }
          ImGui::EndChild();

          ImGui::EndPopup();
        }

        ImGui::EndGroup();
        return l_Changed;
      }

      bool render_fs_handle_selector(Util::String p_Label,
                                     uint16_t p_TypeId,
                                     uint64_t *p_HandleId)
      {
        return render_fs_handle_selector(
            p_Label, Util::Handle::get_type_info(p_TypeId),
            p_HandleId);
      }

      void render_handle_selector(
          Util::RTTI::PropertyInfoBase &p_PropertyInfoBase,
          Util::Handle p_Handle)
      {
        Util::String l_Label = p_PropertyInfoBase.name.c_str();

        Util::RTTI::TypeInfo &l_PropTypeInfo =
            Util::Handle::get_type_info(
                p_PropertyInfoBase.handleType);

        uint64_t l_CurrentValue;
        p_PropertyInfoBase.get(p_Handle, &l_CurrentValue);

        Util::FileSystem::WatchHandle l_WatchHandle =
            Core::get_filesystem_watcher(
                p_PropertyInfoBase.handleType);

        if (l_WatchHandle) {
          if (render_fs_handle_selector(l_Label, l_PropTypeInfo,
                                        &l_CurrentValue)) {
            p_PropertyInfoBase.set(p_Handle, &l_CurrentValue);
          }
        } else {
          if (render_handle_selector(l_Label, l_PropTypeInfo,
                                     &l_CurrentValue)) {
            p_PropertyInfoBase.set(p_Handle, &l_CurrentValue);
          }
        }
      }

      void
      render_handle_selector(PropertyMetadata &p_PropertyMetadata,
                             Util::Handle p_Handle)
      {
        Util::RTTI::PropertyInfo &l_PropertyInfo =
            p_PropertyMetadata.propInfo;

        Util::String l_Label =
            p_PropertyMetadata.friendlyName.c_str();

        Util::RTTI::TypeInfo &l_PropTypeInfo =
            Util::Handle::get_type_info(l_PropertyInfo.handleType);

        uint64_t l_CurrentValue;
        l_PropertyInfo.get(p_Handle, &l_CurrentValue);

        Util::FileSystem::WatchHandle l_WatchHandle =
            Core::get_filesystem_watcher(l_PropertyInfo.handleType);

        if (l_WatchHandle) {
          if (render_fs_handle_selector(l_Label, l_PropTypeInfo,
                                        &l_CurrentValue)) {
            l_PropertyInfo.set(p_Handle, &l_CurrentValue);
          }
        } else {
          if (render_handle_selector(l_Label, l_PropTypeInfo,
                                     &l_CurrentValue)) {
            l_PropertyInfo.set(p_Handle, &l_CurrentValue);
          }
        }
      }

      bool render_color_selector(Util::String p_Label,
                                 Math::Color *p_Color)
      {
        return render_line(p_Label, [&p_Label, p_Color]() {
          return ImGui::ColorEdit4(
              (Util::String("##") + p_Label).c_str(),
              (float *)p_Color,
              ImGuiColorEditFlags_NoInputs |
                  ImGuiColorEditFlags_NoLabel);
        });
      }

      bool render_colorrgb_editor(Util::String &p_Label,
                                  Math::ColorRGB &p_Color,
                                  bool p_RenderLabel)
      {

        return render_line(p_Label, [&p_Label, &p_Color]() {
          Math::ColorRGB l_Color = p_Color;

          if (ImGui::ColorEdit3(
                  (Util::String("##") + p_Label).c_str(),
                  (float *)&l_Color,
                  ImGuiColorEditFlags_NoInputs |
                      ImGuiColorEditFlags_NoLabel)) {
            p_Color = l_Color;
            return true;
          }
          return false;
        });
      }

      bool render_shape_editor(Util::String &p_Label,
                               Math::Shape *p_DataPtr,
                               bool p_RenderLabel)
      {
        return render_line(p_Label, [&p_Label, p_DataPtr]() {
          Util::String l_Label = "##";
          l_Label += p_Label.c_str();

          int l_TypeCount = 4;
          int l_CurrentType = 0;

          bool l_Changed = false;

          Util::String l_Names[] = {"Box", "Sphere", "Cone",
                                    "Cylinder"};
          Math::ShapeType l_Types[] = {
              Math::ShapeType::BOX, Math::ShapeType::SPHERE,
              Math::ShapeType::CONE, Math::ShapeType::CYLINDER};

          Math::Shape l_Shape = *p_DataPtr;

          for (; l_CurrentType < l_TypeCount; ++l_CurrentType) {
            if (l_Shape.type == l_Types[l_CurrentType]) {
              break;
            }
          }

          if (ImGui::BeginCombo("##shapetypeselector",
                                l_Names[l_CurrentType].c_str(), 0)) {
            for (int i = 0; i < l_TypeCount; ++i) {
              if (ImGui::Selectable(l_Names[i].c_str(),
                                    i == l_CurrentType)) {
                memset(&l_Shape, 0, sizeof(l_Shape));
                l_Shape.type = l_Types[i];
                l_Changed = true;
              }
            }
            ImGui::EndCombo();
          }

          if (l_Shape.type == Math::ShapeType::BOX) {
            ImGui::PushID("BOXPOS");
            if (render_vector3_editor("BoxPosition",
                                      l_Shape.box.position, true)) {
              l_Changed = true;
            }
            ImGui::PopID();
            ImGui::PushID("BOXROT");
            if (render_quaternion_editor(
                    "BoxRotation", l_Shape.box.rotation, true)) {
              l_Changed = true;
            }
            ImGui::PopID();
            ImGui::PushID("BOXSCL");
            if (render_vector3_editor("BoxHalfExtents",
                                      l_Shape.box.halfExtents,
                                      true)) {
              l_Changed = true;
            }
            ImGui::PopID();
          } else {
            ImGui::Text("Shape type not editable");
          }

          if (l_Changed) {
            *p_DataPtr = l_Shape;
          }
          return l_Changed;
        });
      }

      void render_editor(Util::Handle p_Handle,
                         Util::Name p_PropertyName)
      {
        render_editor(p_Handle,
                      get_type_metadata(p_Handle.get_type()),
                      p_PropertyName);
      }

      void LOW_EDITOR_API render_editor(Util::Handle p_Handle,
                                        TypeMetadata &p_Metadata,
                                        Util::Name p_PropertyName)
      {
        for (int i = 0; i < p_Metadata.properties.size(); ++i) {
          if (p_Metadata.properties[i].name == p_PropertyName) {
            render_editor(p_Metadata.properties[i].friendlyName,
                          p_Handle, p_Metadata.properties[i].propInfo,
                          true);
            return;
          }
        }
        for (int i = 0; i < p_Metadata.virtualProperties.size();
             ++i) {
          if (p_Metadata.virtualProperties[i].name ==
              p_PropertyName) {
            render_editor(
                p_Metadata.virtualProperties[i].friendlyName,
                p_Handle,
                p_Metadata.virtualProperties[i].virtPropInfo, true);
            return;
          }
        }
      }

      void render_editor_no_label(Util::Handle p_Handle,
                                  Util::Name p_PropertyName)
      {
        render_editor_no_label(p_Handle,
                               get_type_metadata(p_Handle.get_type()),
                               p_PropertyName);
      }

      void render_editor_no_label(Util::Handle p_Handle,
                                  TypeMetadata &p_Metadata,
                                  Util::Name p_PropertyName)
      {
        for (int i = 0; i < p_Metadata.properties.size(); ++i) {
          if (p_Metadata.properties[i].name == p_PropertyName) {
            render_editor(
                p_Metadata.properties[i], p_Handle,
                p_Metadata.properties[i].propInfo.get_return(
                    p_Handle),
                false);
            return;
          }
        }
      }

      void render_editor(PropertyMetadata &p_PropertyMetadata,
                         Util::Handle p_Handle, const void *p_DataPtr,
                         bool p_RenderLabel)
      {
        render_editor(p_PropertyMetadata.friendlyName, p_Handle,
                      p_PropertyMetadata.propInfo, p_RenderLabel);
      }

      void render_editor(Util::String p_Label,
                         Util::Function<void()> p_Function)
      {

        p_Function();
      }

      void
      render_editor(Util::String p_Label, Util::Handle p_Handle,
                    Util::RTTI::PropertyInfoBase p_PropertyInfoBase,
                    bool p_RenderLabel)
      {
        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        if (p_PropertyInfoBase.type ==
            Util::RTTI::PropertyType::ENUM) {
          u8 l_EnumValue;
          p_PropertyInfoBase.get(p_Handle, &l_EnumValue);

          if (render_enum_selector(p_PropertyInfoBase.handleType,
                                   &l_EnumValue, p_Label,
                                   p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_EnumValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::NAME) {
          Util::Name l_NameValue;
          p_PropertyInfoBase.get(p_Handle, &l_NameValue);
          if (render_name_editor(p_Label, l_NameValue,
                                 p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_NameValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::STRING) {
          Util::String l_StringValue;
          p_PropertyInfoBase.get(p_Handle, &l_StringValue);

          if (render_string_editor(p_Label, l_StringValue, false,
                                   p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_StringValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::VECTOR2) {
          Math::Vector2 l_Vec;
          p_PropertyInfoBase.get(p_Handle, &l_Vec);
          if (render_vector2_editor(p_Label, l_Vec, p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_Vec);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::VECTOR3) {
          Math::Vector3 l_Vec;
          p_PropertyInfoBase.get(p_Handle, &l_Vec);

          if (render_vector3_editor(p_Label, l_Vec, p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_Vec);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::QUATERNION) {
          Math::Quaternion l_Quat;
          p_PropertyInfoBase.get(p_Handle, &l_Quat);
          if (render_quaternion_editor(p_Label, l_Quat,
                                       p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_Quat);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::COLORRGB) {
          Math::ColorRGB l_ColorValue;
          p_PropertyInfoBase.get(p_Handle, &l_ColorValue);

          if (render_colorrgb_editor(p_Label, l_ColorValue,
                                     p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_ColorValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::COLOR) {
          Math::Color l_ColorValue;
          p_PropertyInfoBase.get(p_Handle, &l_ColorValue);

          if (render_color_selector(p_Label, &l_ColorValue)) {
            p_PropertyInfoBase.set(p_Handle, &l_ColorValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::BOOL) {
          bool l_BoolValue;
          p_PropertyInfoBase.get(p_Handle, &l_BoolValue);
          if (render_checkbox_bool_editor(p_Label, l_BoolValue,
                                          p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_BoolValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::FLOAT) {
          float l_Float;
          p_PropertyInfoBase.get(p_Handle, &l_Float);
          if (render_float_editor(p_Label, l_Float, p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_Float);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::UINT32) {
          u32 l_IntValue;
          p_PropertyInfoBase.get(p_Handle, &l_IntValue);

          if (render_uint32_editor(p_Label, l_IntValue,
                                   p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_IntValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::INT) {
          u32 l_IntValue;
          p_PropertyInfoBase.get(p_Handle, &l_IntValue);

          if (render_uint32_editor(p_Label, l_IntValue,
                                   p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_IntValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::SHAPE) {
          Math::Shape l_ShapeValue;
          p_PropertyInfoBase.get(p_Handle, &l_ShapeValue);
          if (render_shape_editor(p_Label, &l_ShapeValue,
                                  p_RenderLabel)) {
            p_PropertyInfoBase.set(p_Handle, &l_ShapeValue);
          }
        } else if (p_PropertyInfoBase.type ==
                   Util::RTTI::PropertyType::HANDLE) {
          PropertyEditors::render_handle_selector(p_PropertyInfoBase,
                                                  p_Handle);
        }

        /*
        ImGui::SetCursorScreenPos(
            l_Pos +
            ImVec2(0.0f, LOW_EDITOR_LABEL_HEIGHT_ABS + 12.0f));
            */
      }

      bool render_line(Util::String p_Label,
                       const Util::Function<bool()> &p_DrawEditor)
      {
        static Util::Name l_SplitterName =
            N(LOW_EDITOR_DETAILS_SPLITTER);

        float l_Splitter = Util::Globals::get(l_SplitterName);

        // Tunables
        const float l_Min = 90.0f;
        const float l_Max = 420.0f;
        const float l_GrabWidth = 6.0f;
        const float l_Gap = 8.0f;
        const float l_LineThickness = 1.0f;
        const float l_PadY = 0.0f;

        ImGui::PushID(p_Label.c_str());

        ImGuiStyle &l_Style = ImGui::GetStyle();
        const float l_AvailW = ImGui::GetContentRegionAvail().x;
        const float l_FrameH = ImGui::GetFrameHeight();
        const float l_TextH = ImGui::GetTextLineHeight();

        float l_LabelW =
            Low::Math::Util::clamp(l_Splitter, l_Min, l_Max);
        const float l_EditorW =
            ImMax(10.0f, l_AvailW - (l_LabelW + l_Gap));

        // Compact mode for this row: kill vertical item spacing so
        // rows sit flush
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(l_Style.ItemSpacing.x, 0.0f));

        // Infer continuity from previous item bottom (stateless)
        const float l_PrevBottomY = ImGui::GetItemRectMax().y;

        // Top pad (0 here but keep the call—makes the math stable if
        // you tweak later)
        ImGui::Dummy(ImVec2(0.0f, l_PadY));
        const ImVec2 l_RowTop = ImGui::GetCursorScreenPos();

        const float l_ExpectedPrevBottom = l_RowTop.y - l_PadY;
        const float l_Epsilon = 1.0f;
        const bool l_IsContiguous =
            (ImFabs(l_PrevBottomY - l_ExpectedPrevBottom) <=
             l_Epsilon);

        const float l_BaselineNudge = l_Style.FramePadding.y + 5;

        // Row line: [label cell] [editor] [splitter]
        ImGui::BeginGroup();

        // Label baseline cell (use text height baseline, not frame
        // height, for tighter rows)
        ImGui::Dummy(ImVec2(l_LabelW, l_TextH));

        // Editor
        ImGui::SameLine(l_LabelW + l_Gap, 0.0f);
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0.0f, l_BaselineNudge));
        ImGui::SetNextItemWidth(l_EditorW);

        // Slightly denser framed controls (optional). Comment out if
        // you don't want this.
        ImGui::PushStyleVar(
            ImGuiStyleVar_FramePadding,
            ImVec2(l_Style.FramePadding.x,
                   ImMax(0.0f, l_Style.FramePadding.y - 1.0f)));
        const bool l_Result = p_DrawEditor();
        ImGui::PopStyleVar();

        const float l_EditorH =
            ImGui::GetItemRectSize().y + l_BaselineNudge / 2.0f;
        ImGui::EndGroup();

        // Splitter grab sized to max(label baseline, editor)
        {
          const float l_SplitOffset = l_LabelW - l_GrabWidth * 0.5f;
          ImGui::SameLine(l_SplitOffset, 0.0f);
          ImGui::InvisibleButton(
              "##RowSplitter",
              ImVec2(l_GrabWidth, ImMax(l_TextH, l_EditorH)));
          const bool l_Active = ImGui::IsItemActive();
          if (ImGui::IsItemHovered() || l_Active)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
          if (l_Active) {
            l_Splitter += ImGui::GetIO().MouseDelta.x;
            l_Splitter =
                Low::Math::Util::clamp(l_Splitter, l_Min, l_Max);
            Util::Globals::set(l_SplitterName, l_Splitter);
            l_LabelW = l_Splitter;
          }
        }

        ImGui::EndGroup();

        // If editor is taller than text baseline, add the missing
        // height so we still get a bottom separator flush
        if (l_EditorH > l_TextH)
          ImGui::Dummy(ImVec2(0.0f, l_EditorH - l_TextH));

        // Bottom pad (0 here)
        ImGui::Dummy(ImVec2(0.0f, l_PadY));
        const ImVec2 l_RowBottom = ImGui::GetCursorScreenPos();

        // Label text (ellipsis) vertically centered in final row rect
        {
          const float l_RowH = l_RowBottom.y - l_RowTop.y;
          const float l_TextY =
              l_RowTop.y + (l_RowH - l_TextH) * 0.5f;

          ImGui::RenderTextEllipsis(
              ImGui::GetWindowDrawList(), ImVec2(l_RowTop.x, l_TextY),
              ImVec2(l_RowTop.x + l_LabelW, l_RowBottom.y), l_LabelW,
              p_Label.c_str(), nullptr, nullptr);
        }

        // Pixel-snapped grid lines (clipped to window)
        auto snap = [](float v) { return IM_FLOOR(v) + 0.5f; };
        const float yTopSep = snap(
            l_IsContiguous ? l_PrevBottomY : (l_RowTop.y - l_PadY));
        const float yBottomSep = snap(l_RowBottom.y);

        // Horizontal at bottom
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(l_RowTop.x, yBottomSep),
            ImVec2(l_RowTop.x + l_AvailW, yBottomSep),
            ImGui::GetColorU32(ImGuiCol_TableBorderLight));

        // Vertical split from previous (or our top) to current bottom
        const float l_SplitX = snap(l_RowTop.x + l_LabelW);
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(l_SplitX, yTopSep), ImVec2(l_SplitX, yBottomSep),
            ImGui::GetColorU32(ImGuiCol_Separator), l_LineThickness);

        ImGui::PopStyleVar(); // ItemSpacing.y
        ImGui::PopID();
        return l_Result;
      }
    } // namespace PropertyEditors
  } // namespace Editor
} // namespace Low
