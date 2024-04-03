#include "LowEditorBase.h"

#include "imgui.h"

#include "LowEditorGui.h"

namespace Low {
  namespace Editor {
    namespace Base {
      bool Vector3Edit(const char *p_Label, Math::Vector3 *p_Vector3)
      {
        Math::Vector3 l_Vec = *p_Vector3;
        if (Gui::Vector3Edit(l_Vec)) {
          *p_Vector3 = l_Vec;
          return true;
        }
        return false;
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

      bool NameEdit(const char *p_Label, Util::Name *p_Name)
      {
        Util::Name l_Name = *p_Name;

        char l_Buffer[255];
        uint32_t l_NameLength = strlen(l_Name.c_str());
        memcpy(l_Buffer, l_Name.c_str(), l_NameLength);
        l_Buffer[l_NameLength] = '\0';

        if (ImGui::InputText(p_Label, l_Buffer, 255,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          p_Name->m_Index = LOW_NAME(l_Buffer).m_Index;

          return true;
        }

        return false;
      }

      bool VariantEdit(const char *p_Label, Util::Variant &p_Variant)
      {
        switch (p_Variant.m_Type) {
        case (Util::VariantType::Float):
          return FloatEdit(p_Label, &p_Variant.m_Float);
        case (Util::VariantType::Int32):
          return IntEdit(p_Label, &p_Variant.m_Int32);
        case (Util::VariantType::Vector3):
          return Vector3Edit(p_Label, &p_Variant.m_Vector3);
        case (Util::VariantType::Name):
          return NameEdit(p_Label, (Util::Name *)&p_Variant.m_Uint32);
        }
      }

    } // namespace Base
  }   // namespace Editor
} // namespace Low
