#pragma once

#include "LowEditorApi.h"

#include "LowEditorGui.h"
#include "LowEditorWidget.h"
#include "LowEditorMetadata.h"

#include "LowUtilHandle.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void LOW_EDITOR_API render_editor(
          PropertyMetadata &p_PropertyMetadata, Util::Handle p_Handle,
          const void *p_DataPtr, bool p_RenderLabel = true);

      bool LOW_EDITOR_API render_color_selector(Util::String p_Label,
                                                Math::Color *p_Color);

      bool LOW_EDITOR_API render_enum_selector(
          u16 p_EnumId, int *p_Value, Util::String p_Label,
          bool p_RenderLabel, Util::List<int> p_FilterList);

      bool LOW_EDITOR_API render_handle_selector(
          Util::String p_Label, Util::RTTI::TypeInfo &p_TypeInfo,
          uint64_t *p_HandleId);
      bool LOW_EDITOR_API
      render_handle_selector(Util::String p_Label, uint16_t p_Type,
                             uint64_t *p_HandleId);

      bool LOW_EDITOR_API render_enum_selector(u16 p_EnumId,
                                               int *p_Value,
                                               Util::String p_Label,
                                               bool p_RenderLabel);

      bool LOW_EDITOR_API render_enum_selector(
          PropertyMetadata &p_Metadata, Util::Handle p_Handle);

      void LOW_EDITOR_API render_handle_selector(
          Util::RTTI::PropertyInfoBase &p_PropertyInfoBase,
          Util::Handle p_Handle);

      void LOW_EDITOR_API render_editor(Util::Handle p_Handle,
                                        TypeMetadata &p_Metadata,
                                        Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor(Util::Handle p_Handle,
                                        Util::Name p_PropertyName);

      void LOW_EDITOR_API
      render_handle_selector(PropertyMetadata &p_PropertyMetadata,
                             Util::Handle p_Handle);

      void LOW_EDITOR_API render_editor_no_label(
          Util::Handle p_Handle, TypeMetadata &p_Metadata,
          Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor_no_label(
          Util::Handle p_Handle, Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor(
          Util::String p_Label, Util::Function<void()> p_Function);

      bool LOW_EDITOR_API render_string_editor(Util::String &p_Label,
                                               Util::String &p_String,
                                               bool p_Multiline,
                                               bool p_RenderLabel);

      void
      render_editor(Util::String p_Label, Util::Handle p_Handle,
                    Util::RTTI::PropertyInfoBase p_PropertyInfoBase,
                    bool p_RenderLabel = true);

      bool render_line(Util::String p_Label,
                       const Util::Function<bool()> &p_DrawEditor);

      bool LOW_EDITOR_API render_struct_editor(
          Util::String p_Label, Util::TypeIdentifier p_StructType,
          void *p_StructPtr);

      template <typename T>
      bool render_list_editor(
          Util::String p_Label, Util::List<T> &p_List,
          const Util::Function<bool(Util::String, T &)>
              &p_ElementEditor,
          T p_DefaultValue = T())
      {
        bool l_Changed = false;

        ImGui::PushID(p_Label.c_str());

        if (Gui::CollapsibleHeader(p_Label.c_str())) {
          int l_RemoveIndex = -1;

          for (u32 i = 0; i < p_List.size(); ++i) {
            ImGui::PushID((int)i);

            Util::String l_IndexLabel = LOW_TO_STRING(i);

            render_line(l_IndexLabel, [&]() {
              const bool l_ElementChanged =
                  p_ElementEditor(l_IndexLabel, p_List[i]);
              ImGui::SameLine();
              if (Gui::DeleteButton()) {
                l_RemoveIndex = (int)i;
              }
              return l_ElementChanged;
            });

            ImGui::PopID();
          }

          if (l_RemoveIndex >= 0) {
            p_List.erase(p_List.begin() + l_RemoveIndex);
            l_Changed = true;
          }

          if (Gui::AddButton("Add Element")) {
            p_List.push_back(p_DefaultValue);
            l_Changed = true;
          }
        }

        ImGui::PopID();

        return l_Changed;
      }
    } // namespace PropertyEditors
  } // namespace Editor
} // namespace Low
