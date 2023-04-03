#include "LowEditorPropertyEditors.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include <string.h>

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void render_name_editor(Util::String &p_Label, Util::Name &p_Name,
                              bool p_RenderLabel)
      {
        if (p_RenderLabel) {
          ImGui::Text(p_Label.c_str());
          ImGui::SameLine();
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

      void render_editor(Util::RTTI::PropertyInfo &p_PropertyInfo,
                         const void *p_DataPtr)
      {
        if (p_PropertyInfo.type == Util::RTTI::PropertyType::NAME) {
          render_name_editor(Util::String(p_PropertyInfo.name.c_str()),
                             *(Util::Name *)p_DataPtr, true);
        }
      }
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
