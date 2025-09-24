#include "LowEditorHandlePropertiesSection.h"

#include "LowUtilGlobals.h"
#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowCoreEntity.h"
#include "LowCoreTransform.h"
#include "LowCoreRigidbody.h"
#include "LowCorePrefabInstance.h"

#include "LowEditor.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorMainWindow.h"
#include "LowEditorGui.h"
#include "LowEditorAssetWidget.h"
#include "LowEditorChangeList.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorAssetOperations.h"
#include "LowEditorTypeEditor.h"

#include "LowUtilString.h"
#include "LowUtilDiffUtil.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vcruntime_string.h>
#include <algorithm>
#include "IconsLucide.h"

namespace Low {
  namespace Editor {
    bool HandlePropertiesSection::render_entity(float p_Delta)
    {
      const ImVec2 l_CursorPos = ImGui::GetCursorScreenPos();

      Core::Entity l_Entity = m_Handle.get_id();
      ImGui::Text("Name");
      ImGui::SameLine();

      bool l_IsChildEntity =
          Core::Component::Transform(
              l_Entity.get_transform().get_parent())
              .is_alive();

      bool l_IsPrefabInstance =
          l_Entity.has_component(
              Core::Component::PrefabInstance::TYPE_ID) &&
          Core::Component::PrefabInstance(
              l_Entity.get_component(
                  Core::Component::PrefabInstance::TYPE_ID))
              .get_prefab()
              .is_alive();

      bool l_Added = false;

      static char l_NameBuffer[255];
      static Core::Entity l_LastEntity = 0;

      if (l_LastEntity != l_Entity) {
        memcpy(l_NameBuffer, l_Entity.get_name().c_str(),
               strlen(l_Entity.get_name().c_str()) + 1);
        l_LastEntity = l_Entity;
      }

      if (Gui::InputText("##name", l_NameBuffer, 255,
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        l_Entity.set_name(LOW_NAME(l_NameBuffer));
      }
      ImGui::Dummy({0.0f, 4.0f});

      if (!l_IsChildEntity) {
        ImGui::BeginGroup();
        ImGui::Text("Region: ");
        ImGui::SameLine();
        if (l_Entity.get_region().is_alive()) {
          ImGui::Text(l_Entity.get_region().get_name().c_str());
        } else {
          ImGui::Text("None");
        }

        ImGui::SameLine();
        {
          Util::String l_PopupName = "region_selector";
          if (ImGui::Button(ICON_LC_CIRCLE_DOT)) {
            ImGui::OpenPopup(l_PopupName.c_str());
          }

          if (ImGui::BeginPopup(l_PopupName.c_str())) {

            for (Core::Region i_Region :
                 Core::Region::ms_LivingInstances) {
              if (!i_Region.get_scene().is_loaded()) {
                continue;
              }
              if (ImGui::Selectable(i_Region.get_name().c_str())) {
                i_Region.add_entity(l_Entity);
              }
            }
            ImGui::EndPopup();
          }
        }
        ImGui::EndGroup();
      }

      if (l_IsPrefabInstance) {
        int l_GreyVal = 120;
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            IM_COL32(l_GreyVal, l_GreyVal, l_GreyVal, 255));
        ImGui::PushFont(Fonts::UI());
        ImGui::Text("Prefab instance");
        ImGui::PopFont();
        ImGui::PopStyleColor();
      }

      ImGui::Dummy({0.0f, 4.0f});

      if (Gui::AddButton("Add component")) {
        ImGui::OpenPopup("add_component_popup");
      }

      if (ImGui::BeginPopup("add_component_popup")) {
        for (auto it = Util::Handle::get_component_types().begin();
             it != Util::Handle::get_component_types().end(); ++it) {
          if (l_Entity.has_component(*it)) {
            continue;
          }
          Util::RTTI::TypeInfo &i_TypeInfo =
              Util::Handle::get_type_info(*it);

          if (ImGui::MenuItem(i_TypeInfo.name.c_str())) {
            i_TypeInfo.make_component(l_Entity.get_id());
            set_selected_entity(l_Entity);
            l_Added = true;
          }
        }
        ImGui::EndPopup();
      }

      ImGui::Dummy({0.0f, 7.0f});

      return l_Added;
    }

    bool HandlePropertiesSection::render_default(float p_Delta)
    {
      TypeEditor::show(m_Handle, m_Metadata);
      return false;
    }

    void HandlePropertiesSection::render(float p_Delta)
    {
      if (!m_Metadata.typeInfo.is_alive(m_Handle)) {
        return;
      }

      /*
      if (Gui::CollapsingHeaderButton(
              m_Metadata.name.c_str(),
              m_DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0,
              ICON_LC_COG)) {
              */

      Util::String title = m_Metadata.name.c_str();
      const char *t = title.c_str();
      Util::String l_HeaderTitle = "";
      if (m_Metadata.editor.hasIcon) {
        l_HeaderTitle += m_Metadata.editor.icon;
        l_HeaderTitle += " ";
      }
      l_HeaderTitle += m_Metadata.name.c_str();

      if (Gui::CollapsingHeader(
              l_HeaderTitle.c_str(),
              m_DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {

        Util::StoredHandle l_StoredHandle;
        Util::DiffUtil::store_handle(l_StoredHandle, m_Handle);

        bool l_SkipFooter = false;

        ImGui::PushID(m_Handle.get_id());
        if (m_Handle.get_type() == Renderer::Material::TYPE_ID) {
          l_SkipFooter = l_SkipFooter || render_entity(p_Delta);
        } else {
          l_SkipFooter = l_SkipFooter || render_default(p_Delta);
        }
        if (!l_SkipFooter && render_footer != nullptr) {
          render_footer(m_Handle, m_Metadata.typeInfo);
        }
        ImGui::PopID();

        if (!l_SkipFooter) {
          Util::StoredHandle l_AfterChange;
          Util::DiffUtil::store_handle(l_AfterChange, m_Handle);

          if (!Renderer::Material::is_alive(m_Handle)) {
            Transaction l_Transaction = Transaction::from_diff(
                m_Handle, l_StoredHandle, l_AfterChange);

            get_global_changelist().add_entry(l_Transaction);
          }
        }
      }
    }

    HandlePropertiesSection::HandlePropertiesSection(
        const Util::Handle p_Handle, bool p_DefaultOpen)
        : m_Handle(p_Handle), m_DefaultOpen(p_DefaultOpen)
    {
      m_Metadata = get_type_metadata(p_Handle.get_type());
      m_Open = p_DefaultOpen;
    }
  } // namespace Editor
} // namespace Low
