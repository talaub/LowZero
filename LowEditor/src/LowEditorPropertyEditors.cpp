#include "LowEditorPropertyEditors.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include <string.h>

#include <algorithm>

#define DISPLAY_LABEL(s) std::replace(s.begin(), s.end(), '_', ' ')

namespace Low {
  namespace Editor {
    namespace PropertyEditors {

      static void render_label(Util::String &p_Label)
      {
        Util::String l_DisplayLabel = p_Label;
        DISPLAY_LABEL(l_DisplayLabel);
        ImGui::Text(l_DisplayLabel.c_str());
        ImGui::SameLine();
      }

      void render_name_editor(Util::String &p_Label, Util::Name &p_Name,
                              bool p_RenderLabel)
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

      void render_quaternion_editor(Util::String &p_Label,
                                    Math::Quaternion &p_Quaternion,
                                    bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        Math::Vector3 l_Vector = Math::VectorUtil::to_euler(p_Quaternion);

        if (ImGui::DragFloat3(l_Label.c_str(), (float *)&l_Vector, 0.2f)) {
          p_Quaternion = Math::VectorUtil::from_euler(l_Vector);
        }
      }

      void render_vector3_editor(Util::String &p_Label, Math::Vector3 &p_Vector,
                                 bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        Math::Vector3 l_Vector = p_Vector;

        if (ImGui::DragFloat3(l_Label.c_str(), (float *)&l_Vector, 0.2f)) {
          p_Vector = l_Vector;
        }
      }

      void render_vector2_editor(Util::String &p_Label, Math::Vector2 &p_Vector,
                                 bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          render_label(p_Label);
        }

        Util::String l_Label = "##";
        l_Label += p_Label.c_str();

        ImGui::DragFloat2(l_Label.c_str(), (float *)&p_Vector);
      }

      void render_checkbox_bool_editor(Util::String &p_Label, bool &p_Bool,
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

      bool render_handle_selector(Util::String p_Label, uint16_t p_Type,
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

        bool l_Changed = false;

        Util::Handle l_CurrentHandle = *p_HandleId;

        Util::String l_PopupName = Util::String("_choose_element_") + p_Label;

        const char *l_DisplayName = "None";

        Util::RTTI::PropertyInfo l_NameProperty;
        bool l_HasNameProperty = false;

        if (p_TypeInfo.properties.find(N(name)) !=
            p_TypeInfo.properties.end()) {

          l_NameProperty = p_TypeInfo.properties[N(name)];

          if (p_TypeInfo.is_alive(l_CurrentHandle)) {
            l_DisplayName = ((Util::Name *)p_TypeInfo.properties[N(name)].get(
                                 l_CurrentHandle))
                                ->c_str();
          }
          l_HasNameProperty = true;
        }
        ImGui::Text(l_DisplayName);
        ImGui::SameLine();
        if (ImGui::Button("Choose...")) {
          ImGui::OpenPopup(l_PopupName.c_str());
        }

        if (ImGui::BeginPopup(l_PopupName.c_str())) {
          Util::Handle *l_Handles = p_TypeInfo.get_living_instances();

          for (uint32_t i = 0u; i < p_TypeInfo.get_living_count(); ++i) {
            char *i_DisplayName = "Object";
            if (l_HasNameProperty) {
              l_DisplayName =
                  ((Util::Name *)l_NameProperty.get(l_Handles[i]))->c_str();
            }
            if (ImGui::Selectable(l_DisplayName)) {
              *p_HandleId = l_Handles[i].get_id();
              l_Changed = true;
            }
          }
          ImGui::EndPopup();
        }

        ImGui::EndGroup();
        return l_Changed;
      }

      void render_handle_selector(Util::RTTI::PropertyInfo &p_PropertyInfo,
                                  Util::Handle p_Handle)
      {
        Util::String l_Label = p_PropertyInfo.name.c_str();

        Util::RTTI::TypeInfo &l_PropTypeInfo =
            Util::Handle::get_type_info(p_PropertyInfo.handleType);

        uint64_t l_CurrentValue = *(uint64_t *)p_PropertyInfo.get(p_Handle);

        if (render_handle_selector(l_Label, l_PropTypeInfo, &l_CurrentValue)) {
          p_PropertyInfo.set(p_Handle, &l_CurrentValue);
        }
      }

      bool render_color_selector(Util::String p_Label, Math::Color *p_Color)
      {
        render_label(p_Label);

        return ImGui::ColorEdit4((Util::String("##") + p_Label).c_str(),
                                 (float *)p_Color);
      }

      void render_colorrgb_editor(Util::String &p_Label,
                                  Math::ColorRGB &p_Color, bool p_RenderLabel)
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

      void render_editor(Util::RTTI::PropertyInfo &p_PropertyInfo,
                         const void *p_DataPtr)
      {
        if (p_PropertyInfo.type == Util::RTTI::PropertyType::NAME) {
          render_name_editor(Util::String(p_PropertyInfo.name.c_str()),
                             *(Util::Name *)p_DataPtr, true);
        } else if (p_PropertyInfo.type == Util::RTTI::PropertyType::VECTOR2) {
          render_vector2_editor(Util::String(p_PropertyInfo.name.c_str()),
                                *(Math::Vector2 *)p_DataPtr, true);
        } else if (p_PropertyInfo.type == Util::RTTI::PropertyType::VECTOR3) {
          render_vector3_editor(Util::String(p_PropertyInfo.name.c_str()),
                                *(Math::Vector3 *)p_DataPtr, true);
        } else if (p_PropertyInfo.type ==
                   Util::RTTI::PropertyType::QUATERNION) {
          render_quaternion_editor(Util::String(p_PropertyInfo.name.c_str()),
                                   *(Math::Quaternion *)p_DataPtr, true);
        } else if (p_PropertyInfo.type == Util::RTTI::PropertyType::COLORRGB) {
          render_colorrgb_editor(Util::String(p_PropertyInfo.name.c_str()),
                                 *(Math::ColorRGB *)p_DataPtr, true);
        } else if (p_PropertyInfo.type == Util::RTTI::PropertyType::BOOL) {
          render_checkbox_bool_editor(Util::String(p_PropertyInfo.name.c_str()),
                                      *(bool *)p_DataPtr, true);
        } else if (p_PropertyInfo.type == Util::RTTI::PropertyType::FLOAT) {
          render_float_editor(Util::String(p_PropertyInfo.name.c_str()),
                              *(float *)p_DataPtr, true);
        }
      }
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
