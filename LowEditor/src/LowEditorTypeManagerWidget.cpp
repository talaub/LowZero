#include "LowEditorTypeManagerWidget.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorTypeEditor.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowCoreMeshAsset.h"

#include "LowUtil.h"
#include "LowUtilString.h"
#include "LowUtilFileIO.h"

namespace Low {
  namespace Editor {
    static Util::String
    get_asset_path(const Util::Handle p_Handle,
                   Util::RTTI::TypeInfo &p_TypeInfo)
    {
      Util::String l_LowerCase = p_TypeInfo.name.c_str();
      l_LowerCase.make_lower();

      Util::RTTI::PropertyInfo l_NamePropertyInfo =
          p_TypeInfo.properties[N(name)];

      // Read name from handle and convert to string
      Util::String l_NameString =
          ((Util::Name *)l_NamePropertyInfo.get(p_Handle))->c_str();

      Util::String l_SavePath =
          Util::get_project().dataPath + "/assets/" + l_LowerCase +
          "/" + l_NameString + "." + l_LowerCase + ".yaml";

      return l_SavePath;
    }

    static void save(const Util::Handle p_Handle,
                     Util::RTTI::TypeInfo &p_TypeInfo)
    {
      // Contains the name of the type in lowercase
      Util::String l_SavePath = get_asset_path(p_Handle, p_TypeInfo);

      Util::Yaml::Node l_Node;
      p_TypeInfo.serialize(p_Handle, l_Node);

      Util::Yaml::write_file(l_SavePath.c_str(), l_Node);
      LOW_LOG_INFO << "Saved " << p_TypeInfo.name << LOW_LOG_END;
    }

    static void delete_asset(const Util::Handle p_Handle,
                             Util::RTTI::TypeInfo &p_TypeInfo)
    {
      // Contains the name of the type in lowercase
      Util::String l_SavePath = get_asset_path(p_Handle, p_TypeInfo);

      delete_file_if_exists(l_SavePath);
    }

    static void render_footer(const Util::Handle p_Handle,
                              Util::RTTI::TypeInfo &p_TypeInfo)
    {
      TypeMetadata &l_TypeMetadata =
          get_type_metadata(p_TypeInfo.typeId);

      if (l_TypeMetadata.editor.saveable) {
        if (ImGui::Button("Save")) {
          save(p_Handle, p_TypeInfo);
        }
      }
    }

    TypeManagerWidget::TypeManagerWidget(uint16_t p_TypeId)
        : m_LayoutConstructed(false)
    {
      m_TypeInfo = Util::Handle::get_type_info(p_TypeId);

      m_ListWindowName = "List##TypeManager";
      m_ListWindowName += m_TypeInfo.typeId;
      m_InfoWindowName = "Info##TypeManager";
      m_InfoWindowName += m_TypeInfo.typeId;

      m_NamePropertyInfo = m_TypeInfo.properties[N(name)];
    }

    void TypeManagerWidget::select(Util::Handle p_Handle)
    {
      m_Selected = p_Handle;
      m_Sections.clear();
      HandlePropertiesSection i_Section(p_Handle, true);
      i_Section.render_footer = &render_footer;
      m_Sections.push_back(i_Section);
    }

    void TypeManagerWidget::render_info(float p_Delta)
    {
      for (auto it = m_Sections.begin(); it != m_Sections.end();
           ++it) {
        it->render(p_Delta);
      }
    }

    void TypeManagerWidget::render_list(float p_Delta)
    {
      Util::List<Util::Handle> l_HandlesToDelete;

      for (u32 i = 0; i < m_TypeInfo.get_living_count(); ++i) {
        Util::Handle i_Handle = m_TypeInfo.get_living_instances()[i];
        Util::Name i_Name =
            *(Util::Name *)m_NamePropertyInfo.get(i_Handle);

        if (ImGui::Selectable(i_Name.c_str(),
                              i_Handle.get_id() ==
                                  m_Selected.get_id())) {
          select(i_Handle);
        }

        if (ImGui::BeginPopupContextItem(
                LOW_TO_STRING(i_Handle.get_id()).c_str())) {
          select(i_Handle);
          if (ImGui::MenuItem("Delete")) {
            l_HandlesToDelete.push_back(i_Handle);
          }
          ImGui::EndPopup();
        }
      }

      for (auto it = l_HandlesToDelete.begin();
           it != l_HandlesToDelete.end();) {
        Util::Handle i_Handle = *it;
        if (!i_Handle.is_registered_type()) {
          it = l_HandlesToDelete.erase(it);
        }
        Util::RTTI::TypeInfo &i_TypeInfo =
            Util::Handle::get_type_info(i_Handle.get_type());

        get_global_changelist().add_entry(
            History::destroy_handle_transaction(i_Handle));

        TypeEditor::handle_before_delete(i_Handle);

        delete_asset(i_Handle, i_TypeInfo);

        i_TypeInfo.destroy(i_Handle);

        it = l_HandlesToDelete.erase(it);
      }
    }

    void TypeManagerWidget::render(float p_Delta)
    {
      Util::Name l_TypeName = m_TypeInfo.name;

      ImGui::PushID(l_TypeName.m_Index);

      ImGui::SetNextWindowSize(ImVec2(500, 400),
                               ImGuiCond_FirstUseEver);
      ImGui::Begin(m_TypeInfo.name.c_str(), &m_Open,
                   ImGuiWindowFlags_NoCollapse);

      Util::String l_CreateString = "Create ";
      l_CreateString += l_TypeName.c_str();

      if (ImGui::Button("Add")) {
        ImGui::OpenPopup(l_CreateString.c_str());
      }

      if (ImGui::BeginPopupModal(l_CreateString.c_str())) {
        ImGui::Text(
            "You are about to create a new asset. Please select "
            "a name.");
        static char l_NameBuffer[255];
        ImGui::InputText("##name", l_NameBuffer, 255);

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }

        bool l_IsEnter =
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter));

        if (ImGui::Button("Create") || l_IsEnter) {
          Util::Name l_Name = LOW_NAME(l_NameBuffer);

          bool l_Ok = true;

          /*
          for (auto it = Core::MeshAsset::ms_LivingInstances.begin();
               it != Core::MeshAsset::ms_LivingInstances.end();
               ++it) {
            if (l_Name == it->get_name()) {
              LOW_LOG_ERROR
                  << "Cannot create mesh asset. The chosen name '"
                  << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }
          */

          if (l_Ok) {
            Util::Handle l_NewHandle =
                m_TypeInfo.make_default(l_Name);

            save(l_NewHandle, m_TypeInfo);

            TypeEditor::handle_after_add(l_NewHandle);

            select(l_NewHandle);
          }
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      bool l_DockSpaceCreated =
          ImGui::DockBuilderGetNode(m_DockSpaceId) != nullptr;
      if (!l_DockSpaceCreated) {
        m_DockSpaceId = ImGui::GetID(l_TypeName.c_str());

        ImGuiID l_MainDockId = m_DockSpaceId;

        ImGui::DockBuilderRemoveNode(l_MainDockId);
        ImGui::DockBuilderAddNode(l_MainDockId);

        ImGuiID l_DockMain = l_MainDockId;
        ImGuiID l_DockLeft = ImGui::DockBuilderSplitNode(
            l_DockMain, ImGuiDir_Left, 0.40f, NULL, &l_DockMain);

        ImGui::DockBuilderDockWindow(m_ListWindowName.c_str(),
                                     l_DockLeft);
        ImGui::DockBuilderDockWindow(m_InfoWindowName.c_str(),
                                     l_DockMain);

        m_LayoutConstructed = true;
      }

      ImGui::DockSpace(m_DockSpaceId, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_None);
      ImGui::End();

      ImGuiWindowFlags l_WindowFlags =
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

      ImGui::Begin(m_ListWindowName.c_str(), nullptr, l_WindowFlags);
      render_list(p_Delta);
      ImGui::End();

      ImGui::Begin(m_InfoWindowName.c_str(), nullptr, l_WindowFlags);
      render_info(p_Delta);
      ImGui::End();

      ImGui::PopID();
    }
  } // namespace Editor
} // namespace Low
