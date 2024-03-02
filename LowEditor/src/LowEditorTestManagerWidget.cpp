#include "LowEditorTestManagerWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowCoreMeshAsset.h"

namespace Low {
  namespace Editor {
    TestManagerWidget::TestManagerWidget(uint16_t p_TypeId)
        : m_LayoutConstructed(false)
    {
      m_TypeInfo = Util::Handle::get_type_info(p_TypeId);

      m_ListWindowName = "List##Manager";
      m_ListWindowName += m_TypeInfo.typeId;
      m_InfoWindowName = "Info##Manager";
      m_InfoWindowName += m_TypeInfo.typeId;
    }

    void TestManagerWidget::render(float p_Delta)
    {
      Util::Name l_TypeName = m_TypeInfo.name;

      ImGui::PushID(l_TypeName.m_Index);

      ImGui::SetNextWindowSize(ImVec2(500, 400),
                               ImGuiCond_FirstUseEver);
      ImGui::Begin(m_TypeInfo.name.c_str(), nullptr,
                   ImGuiWindowFlags_NoCollapse);

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
      Util::String l_ListName = l_TypeName.c_str();
      l_ListName += " - ManagerList";
      ImGui::Text(l_ListName.c_str());
      ImGui::End();

      ImGui::Begin(m_InfoWindowName.c_str(), nullptr, l_WindowFlags);
      Util::String l_InfoName = l_TypeName.c_str();
      l_InfoName += " - ManagerList";
      ImGui::Text(l_InfoName.c_str());
      ImGui::End();

      ImGui::PopID();
    }
  } // namespace Editor
} // namespace Low
