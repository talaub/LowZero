#include "LowEditorPropertyEditors.h"

#include "LowEditor.h"
#include "LowEditorGui.h"

#include "LowCore.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include <string.h>

#include <algorithm>
#include <vcruntime_string.h>

#define DISPLAY_LABEL(s) std::replace(s.begin(), s.end(), '_', ' ')

namespace Low {
  namespace Editor {
    namespace PropertyEditors {

      Util::Map<uint16_t, Util::FileSystem::WatchHandle>
          g_SelectedDirectoryPerType;

      static void render_label(Util::String &p_Label)
      {
        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_LabelWidth = l_FullWidth * LOW_EDITOR_LABEL_WIDTH_REL;
        float l_LabelHeight = 20.0f;

        Util::String l_DisplayLabel = p_Label;
        DISPLAY_LABEL(l_DisplayLabel);
        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(), l_Pos,
            l_Pos + ImVec2(l_LabelWidth, l_LabelHeight),
            l_Pos.x + l_LabelWidth - LOW_EDITOR_SPACING,
            l_Pos.x + l_LabelWidth, l_DisplayLabel.c_str(),
            l_DisplayLabel.c_str() + l_DisplayLabel.size(), nullptr);
        ImGui::SetCursorScreenPos(l_Pos);
        Util::String l_InvisLabel = "##";
        l_InvisLabel += "INVIS" + p_Label;
        ImGui::InvisibleButton(l_InvisLabel.c_str(),
                               ImVec2(l_LabelWidth, l_LabelHeight));
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(l_DisplayLabel.c_str());
        }
        ImGui::SetCursorScreenPos(
            {l_Pos.x + l_LabelWidth + LOW_EDITOR_SPACING, l_Pos.y});
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
        if (p_RenderLabel) {
          render_label(p_Label);
        }

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

            if (ImGui::Selectable(l_EnumInfo.entries[i].name.c_str(),
                                  l_CurrentValue ==
                                      l_EnumInfo.entries[i].value)) {
              if (!i_Disabled) // Only allow selection if the item is
                               // not disabled
              {
                l_CurrentValue = l_EnumInfo.entries[i].value;
                l_Result = true;
              }
            }

            if (i_Disabled) {
              ImGui::PopItemFlag();
              ImGui::PopStyleVar();
            }

            // Set the initial focus when opening the combo (scrolling
            // + keyboard navigation focus)
            if (l_CurrentValue == l_EnumInfo.entries[i].value) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }

        *p_Value = l_CurrentValue;

        return l_Result;
      }

      bool render_enum_selector(PropertyMetadata &p_Metadata,
                                Util::Handle p_Handle)
      {
        u8 l_CurrentValue = *(u8 *)p_Metadata.propInfo.get(p_Handle);

        bool l_Result = render_enum_selector(
            p_Metadata.propInfo.handleType, &l_CurrentValue,
            p_Metadata.name.c_str(), true);

        p_Metadata.propInfo.set(p_Handle, &l_CurrentValue);

        return l_Result;
      }

      void render_name_editor(Util::String &p_Label,
                              Util::Name &p_Name, bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        char l_Buffer[255];
        uint32_t l_NameLength = strlen(p_Name.c_str());
        memcpy(l_Buffer, p_Name.c_str(), l_NameLength);
        l_Buffer[l_NameLength] = '\0';

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        if (ImGui::InputText(l_Label.c_str(), l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          p_Name.m_Index = LOW_NAME(l_Buffer).m_Index;
        }
      }

      void render_string_editor(Util::String &p_Label,
                                Util::String &p_String,
                                bool p_Multiline, bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

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
          }
        } else {
          if (ImGui::InputText(
                  l_Label.c_str(), l_Buffer, 1024,
                  ImGuiInputTextFlags_EnterReturnsTrue)) {
            p_String = l_Buffer;
          }
        }
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
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        Math::Vector3 l_Vector =
            Math::VectorUtil::to_euler(p_Quaternion);

        if (Gui::Vector3Edit(l_Vector)) {
          p_Quaternion = Math::VectorUtil::from_euler(l_Vector);
          return true;
        }

        return false;
      }

      bool render_vector3_editor(Util::String p_Label,
                                 Math::Vector3 &p_Vector,
                                 bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        Math::Vector3 l_Vector = p_Vector;

        if (Gui::Vector3Edit(l_Vector)) {
          p_Vector = l_Vector;
          return true;
        }

        /*
              if (ImGui::DragFloat3(l_Label.c_str(), (float
           *)&l_Vector, 0.2f)) { p_Vector = l_Vector; return true;
              }
        */
        return false;
      }

      void render_vector2_editor(Util::String &p_Label,
                                 Math::Vector2 &p_Vector,
                                 bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        ImGui::DragFloat2(l_Label.c_str(), (float *)&p_Vector);
      }

      void render_checkbox_bool_editor(Util::String &p_Label,
                                       bool &p_Bool,
                                       bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        ImGui::Checkbox(l_Label.c_str(), &p_Bool);
      }

      void render_float_editor(Util::String &p_Label, float &p_Float,
                               bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        ImGui::DragFloat(l_Label.c_str(), &p_Float);
      }

      void render_uint32_editor(Util::String &p_Label, u32 &p_Value,
                                bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        int l_LocalValue = p_Value;

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        if (ImGui::DragInt(l_Label.c_str(), &l_LocalValue, 1.0f, 0.0f,
                           500000.0f)) {
          p_Value = l_LocalValue;
        }
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
        ImGui::BeginGroup();
        render_label(p_Label);

        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_ButtonWidth = 80.0f;
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
            l_DisplayName =
                ((Util::Name *)p_TypeInfo.properties[N(name)].get(
                     l_CurrentHandle))
                    ->c_str();
          }
          l_HasNameProperty = true;
        }
        int l_NameLength = strlen(l_DisplayName);

        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(), l_Pos,
            l_Pos + ImVec2(l_NameWidth, LOW_EDITOR_LABEL_HEIGHT_ABS),
            l_Pos.x + l_NameWidth - LOW_EDITOR_SPACING,
            l_Pos.x + l_NameWidth, l_DisplayName,
            l_DisplayName + l_NameLength, nullptr);

        ImGui::SetCursorScreenPos(
            l_Pos + ImVec2(l_NameWidth + LOW_EDITOR_SPACING, 0.0f));

        if (ImGui::Button("Choose...")) {
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
          ImGui::InputText("##search", l_SearchBuffer,
                           SEARCH_BUFFER_SIZE);

#undef SEARCH_BUFFER_SIZE;

          Util::Handle *l_Handles = p_TypeInfo.get_living_instances();

          for (uint32_t i = 0u; i < p_TypeInfo.get_living_count();
               ++i) {
            char *i_DisplayName = "Object";
            if (l_HasNameProperty) {
              i_DisplayName =
                  ((Util::Name *)l_NameProperty.get(l_Handles[i]))
                      ->c_str();
            }

            if (strlen(l_SearchBuffer) > 0 &&
                !strstr(i_DisplayName, l_SearchBuffer)) {
              continue;
            }
            if (ImGui::Selectable(i_DisplayName)) {
              l_SearchBuffer[0] = '\0';
              *p_HandleId = l_Handles[i].get_id();
              l_Changed = true;
            }
          }
          ImGui::EndPopup();
        }

        ImGui::EndGroup();
        return l_Changed;
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
        render_label(p_Label);

        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_ButtonWidth = 80.0f;
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
            l_DisplayName =
                ((Util::Name *)p_TypeInfo.properties[N(name)].get(
                     l_CurrentHandle))
                    ->c_str();
          }
          l_HasNameProperty = true;
        }
        int l_NameLength = strlen(l_DisplayName);

        ImGui::RenderTextEllipsis(
            ImGui::GetWindowDrawList(), l_Pos,
            l_Pos + ImVec2(l_NameWidth, LOW_EDITOR_LABEL_HEIGHT_ABS),
            l_Pos.x + l_NameWidth - LOW_EDITOR_SPACING,
            l_Pos.x + l_NameWidth, l_DisplayName,
            l_DisplayName + l_NameLength, nullptr);

        ImGui::SetCursorScreenPos(
            l_Pos + ImVec2(l_NameWidth + LOW_EDITOR_SPACING, 0.0f));

        if (ImGui::Button("Choose...")) {
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

        int l_PopupHeight = 200;
        int l_PopupSelectorWidth = 300;

        if (ImGui::BeginPopup(l_PopupName.c_str())) {
          ImGui::BeginChild("Categories", ImVec2(150, l_PopupHeight),
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
              l_DisplayName =
                  ((Util::Name *)p_TypeInfo.properties[N(name)].get(
                       l_CurrentHandle))
                      ->c_str();
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

            Util::Name i_Name = *(Util::Name *)l_NameProperty.get(
                i_FileWatcher.handle);

            if (ImGui::Selectable(i_Name.c_str(), false)) {
              *p_HandleId = i_FileWatcher.handle.get_id();
              l_Changed = true;
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

      void
      render_handle_selector(Util::RTTI::PropertyInfo &p_PropertyInfo,
                             Util::Handle p_Handle)
      {
        Util::String l_Label = p_PropertyInfo.name.c_str();

        Util::RTTI::TypeInfo &l_PropTypeInfo =
            Util::Handle::get_type_info(p_PropertyInfo.handleType);

        uint64_t l_CurrentValue =
            *(uint64_t *)p_PropertyInfo.get(p_Handle);

        Util::FileSystem::WatchHandle l_WatchHandle =
            Core::get_filesystem_watcher(p_PropertyInfo.handleType);

        if (l_WatchHandle) {
          if (render_fs_handle_selector(l_Label, l_PropTypeInfo,
                                        &l_CurrentValue)) {
            p_PropertyInfo.set(p_Handle, &l_CurrentValue);
          }
        } else {
          if (render_handle_selector(l_Label, l_PropTypeInfo,
                                     &l_CurrentValue)) {
            p_PropertyInfo.set(p_Handle, &l_CurrentValue);
          }
        }
      }

      bool render_color_selector(Util::String p_Label,
                                 Math::Color *p_Color)
      {
        render_label(p_Label);

        return ImGui::ColorEdit4(
            (Util::String("##") + p_Label).c_str(), (float *)p_Color);
        /*
              return ImGui::ColorPicker4((Util::String("##") +
           p_Label).c_str(), (float *)p_Color);
        */
      }

      void render_colorrgb_editor(Util::String &p_Label,
                                  Math::ColorRGB &p_Color,
                                  bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        Math::ColorRGB l_Color = p_Color;

        if (ImGui::ColorEdit3(l_Label.c_str(), (float *)&l_Color)) {
          p_Color = l_Color;
        }
      }

      void
      render_shape_editor(Util::String &p_Label,
                          Util::RTTI::PropertyInfo &p_PropertyInfo,
                          Util::Handle p_Handle,
                          Math::Shape *p_DataPtr, bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

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
          if (render_quaternion_editor("BoxRotation",
                                       l_Shape.box.rotation, true)) {
            l_Changed = true;
          }
          ImGui::PopID();
          ImGui::PushID("BOXSCL");
          if (render_vector3_editor("BoxHalfExtents",
                                    l_Shape.box.halfExtents, true)) {
            l_Changed = true;
          }
          ImGui::PopID();
        } else {
          ImGui::Text("Shape type not editable");
        }

        if (l_Changed) {
          p_PropertyInfo.set(p_Handle, &l_Shape);
        }
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
            render_editor(
                p_Metadata.properties[i], p_Handle,
                p_Metadata.properties[i].propInfo.get(p_Handle));
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
                p_Metadata.properties[i].propInfo.get(p_Handle),
                false);
            return;
          }
        }
      }

      void render_editor(PropertyMetadata &p_PropertyMetadata,
                         Util::Handle p_Handle, const void *p_DataPtr,
                         bool p_RenderLabel)
      {
        ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        if (p_PropertyMetadata.propInfo.type ==
            Util::RTTI::PropertyType::ENUM) {
          render_enum_selector(p_PropertyMetadata, p_Handle);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::NAME) {
          render_name_editor(
              Util::String(p_PropertyMetadata.name.c_str()),
              *(Util::Name *)p_DataPtr, p_RenderLabel);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::STRING) {
          render_string_editor(
              p_PropertyMetadata,
              Util::String(p_PropertyMetadata.propInfo.name.c_str()),
              *(Util::String *)p_DataPtr, p_RenderLabel);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::VECTOR2) {
          Math::Vector2 l_Vec = *(Math::Vector2 *)p_DataPtr;
          render_vector2_editor(
              Util::String(p_PropertyMetadata.name.c_str()), l_Vec,
              p_RenderLabel);

          p_PropertyMetadata.propInfo.set(p_Handle, &l_Vec);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::VECTOR3) {
          Math::Vector3 l_Vec = *(Math::Vector3 *)p_DataPtr;
          render_vector3_editor(
              Util::String(p_PropertyMetadata.name.c_str()), l_Vec,
              p_RenderLabel);
          p_PropertyMetadata.propInfo.set(p_Handle, &l_Vec);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::QUATERNION) {
          Math::Quaternion l_Quat = *(Math::Quaternion *)p_DataPtr;
          render_quaternion_editor(
              Util::String(p_PropertyMetadata.name.c_str()), l_Quat,
              p_RenderLabel);
          p_PropertyMetadata.propInfo.set(p_Handle, &l_Quat);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::COLORRGB) {
          render_colorrgb_editor(
              Util::String(p_PropertyMetadata.name.c_str()),
              *(Math::ColorRGB *)p_DataPtr, p_RenderLabel);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::BOOL) {
          render_checkbox_bool_editor(
              Util::String(p_PropertyMetadata.name.c_str()),
              *(bool *)p_DataPtr, p_RenderLabel);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::FLOAT) {
          float l_Float = *(float *)p_DataPtr;
          render_float_editor(
              Util::String(p_PropertyMetadata.name.c_str()), l_Float,
              p_RenderLabel);
          p_PropertyMetadata.propInfo.set(p_Handle, &l_Float);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::UINT32) {
          render_uint32_editor(
              Util::String(p_PropertyMetadata.name.c_str()),
              *(u32 *)p_DataPtr, p_RenderLabel);
        } else if (p_PropertyMetadata.propInfo.type ==
                   Util::RTTI::PropertyType::SHAPE) {
          render_shape_editor(
              Util::String(p_PropertyMetadata.name.c_str()),
              p_PropertyMetadata.propInfo, p_Handle,
              (Math::Shape *)p_DataPtr, p_RenderLabel);
        }

        ImGui::SetCursorScreenPos(
            l_Pos +
            ImVec2(0.0f, LOW_EDITOR_LABEL_HEIGHT_ABS + 12.0f));
      }

      void render_editor(Util::String p_Label,
                         Util::Function<void()> p_Function)
      {
        render_label(p_Label);

        p_Function();
      }
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
