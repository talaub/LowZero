#include "LowEditorBase.h"

#include "imgui.h"

#include "LowEditorGui.h"

namespace Low {
  namespace Editor {
    namespace Base {
      bool Vector3Edit(const char *p_Label, Math::Vector3 *p_Vector3,
                       float p_MaxWidth)
      {
        Math::Vector3 l_Vec = *p_Vector3;
        bool l_Edited = false;
        if (Gui::Vector3Edit(l_Vec, p_MaxWidth)) {
          *p_Vector3 = l_Vec;
          l_Edited = true;
        }
        return l_Edited;
      }

      bool FloatEdit(const char *p_Label, float *p_Val, float p_Min,
                     float p_Max, float p_Step)
      {
        return ImGui::DragFloat(p_Label, p_Val, p_Step, p_Min, p_Max,
                                "%.1f");
      }

      bool IntEdit(const char *p_Label, int *p_Val, int p_Min,
                   int p_Max, int p_Step)
      {
        return ImGui::DragInt(p_Label, p_Val, p_Step, p_Min, p_Max);
      }

      bool StringEdit(const char *p_Label, Util::String *p_String)
      {
        Util::String l_String = *p_String;

        char l_Buffer[255];
        uint32_t l_StringLength = strlen(l_String.c_str());
        memcpy(l_Buffer, l_String.c_str(), l_StringLength);
        l_Buffer[l_StringLength] = '\0';

        if (ImGui::InputText(p_Label, l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          *p_String = l_Buffer;

          return true;
        }

        return false;
      }

      bool NameEdit(const char *p_Label, Util::Name *p_Name)
      {
        Util::Name l_Name = *p_Name;

        char l_Buffer[255];
        if (l_Name.is_valid()) {
          uint32_t l_NameLength = strlen(l_Name.c_str());
          memcpy(l_Buffer, l_Name.c_str(), l_NameLength);
          l_Buffer[l_NameLength] = '\0';
        } else {
          const char *l_Content = "None";
          uint32_t l_NameLength = 4;
          memcpy(l_Buffer, l_Content, l_NameLength);
          l_Buffer[l_NameLength] = '\0';
        }

        if (ImGui::InputText(p_Label, l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          p_Name->m_Index = LOW_NAME(l_Buffer).m_Index;

          return true;
        }

        return false;
      }

      bool BoolEdit(const char *p_Label, bool *p_Bool)
      {
        return ImGui::Checkbox(p_Label, p_Bool);
      }

      bool VariantEdit(const char *p_Label, Util::Variant &p_Variant)
      {
        switch (p_Variant.m_Type) {
        case (Util::VariantType::Float):
          return FloatEdit(p_Label, &p_Variant.m_Float);
        case (Util::VariantType::Int32):
          return IntEdit(p_Label, &p_Variant.m_Int32);
        case (Util::VariantType::Bool):
          return BoolEdit(p_Label, &p_Variant.m_Bool);
        case (Util::VariantType::Vector3):
          return Vector3Edit(p_Label, &p_Variant.m_Vector3);
        case (Util::VariantType::Name):
          return NameEdit(p_Label, (Util::Name *)&p_Variant.m_Uint32);
        }
      }

    } // namespace Base
  }   // namespace Editor
} // namespace Low
