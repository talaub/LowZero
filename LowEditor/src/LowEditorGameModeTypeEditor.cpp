#include "LowEditorGameModeTypeEditor.h"

#include "LowCoreGameMode.h"
#include "LowCoreGameplaySystem.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorBase.h"

#include "LowEditorThemes.h"
#include "LowUtilHandle.h"
#include "LowUtilVariant.h"
#include "imgui.h"
#include <IconsLucide.h>

namespace Low {
  namespace Editor {
    void GameModeTypeEditor::render_storage(
        Core::GameModeSystemStorage &p_Storage)
    {
      Core::GameplaySystem l_System = p_Storage.system;

      for (const auto &[i_Name, i_PropertyType] :
           l_System.get_exposed_variables()) {
        show_line(
            i_Name.c_str(),
            [i_Name, &p_Storage, i_PropertyType]() -> bool {
              Util::Variant i_VarValue;
              i_VarValue.m_Type = Util::property_type_to_variant_type(
                  (Util::RTTI::PropertyType::Enum)i_PropertyType);
              if (i_VarValue.m_Type == Util::VariantType::String) {
                Util::String i_Str = "";
                i_VarValue = i_Str;
              }

              auto i_Pos = p_Storage.values.find(i_Name);

              if (i_Pos != p_Storage.values.end()) {
                i_VarValue = i_Pos->second;
              }

              if (Base::VariantEdit("##variantedit", i_VarValue)) {
                p_Storage.values[i_Name] = i_VarValue;
                return true;
              }

              return false;
            });
      }
    }

    void GameModeTypeEditor::render(float p_Delta)
    {
      Core::GameMode l_GameMode = m_Handle.get_id();

      if (ImGui::BeginPopupModal(
              "Add system", nullptr,
              ImGuiWindowFlags_AlwaysAutoResize |
                  ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Add system to GameMode");
        static Core::GameplaySystem l_SystemHandle = 0;
        Util::RTTI::TypeInfo &l_SystemTypeInfo =
            Util::Handle::get_type_info(
                Core::GameplaySystem::type_id());
        if (PropertyEditors::render_handle_selector(
                "handleselector", l_SystemTypeInfo,
                (u64 *)&l_SystemHandle)) {
        }
        bool l_Existing = false;
        if (l_SystemHandle.is_alive()) {
          for (const Core::GameModeSystemStorage &i_Storage :
               l_GameMode.get_systems()) {
            if (i_Storage.system == l_SystemHandle) {
              l_Existing = true;
              break;
            }
          }
          if (l_Existing) {

          } else {
            if (Gui::AddButton()) {
              Core::GameModeSystemStorage l_Storage;
              l_Storage.system = l_SystemHandle;
              l_GameMode.get_systems().push_back(l_Storage);

              l_SystemHandle = 0;
              ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
          }
        }

        if (Gui::Button("Cancel")) {
          l_SystemHandle = 0;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
      show_editor(N(name));
      show_editor(N(blocks_underlying));
      Gui::SectionHeader(ICON_LC_CHART_NETWORK, "SYSTEMS");
      if (Gui::AddButton("Add system")) {
        ImGui::OpenPopup("Add system");
      }

      ImGui::Dummy(ImVec2(0, 10.0f));

      for (auto it = l_GameMode.get_systems().begin();
           it != l_GameMode.get_systems().end();) {
        bool i_Delete = false;
        if (Gui::CollapsibleHeader(it->system.get_name().c_str(),
                                   nullptr, theme_get_current().debug,
                                   LOW_EDITOR_ICON_DELETE,
                                   &i_Delete)) {
          render_storage(*it);
        }
        if (i_Delete) {
          it = l_GameMode.get_systems().erase(it);
        } else {
          ++it;
        }
      }

      ImGui::Dummy(ImVec2(0, 15));

      ImGui::Separator();
    }
  } // namespace Editor
} // namespace Low
