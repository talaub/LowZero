#include "LowEditorStateGraphWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorHandlePropertiesSection.h"

namespace Low {
  namespace Editor {
    namespace ed = ax::NodeEditor;

    StateGraphWidget::StateGraphWidget()
    {
      ax::NodeEditor::Config l_Config;
      m_Context = ax::NodeEditor::CreateEditor(&l_Config);
    }

    void StateGraphWidget::render(float p_Delta)
    {
      return;
      ImGui::Begin(ICON_FA_PROJECT_DIAGRAM " StateGraph");

      auto &io = ImGui::GetIO();

      ed::SetCurrentEditor(m_Context);
      ed::Begin("StateGraphEditor", ImVec2(0.0, 0.0f));
      int uniqueId = 1;
      // Start drawing nodes.
      ed::BeginNode(uniqueId++);
      ImGui::Text("Node A");
      ed::BeginPin(uniqueId++, ed::PinKind::Input);
      ImGui::Text(ICON_FA_CARET_RIGHT " In");
      ed::EndPin();
      ImGui::SameLine();
      ed::BeginPin(uniqueId++, ed::PinKind::Output);
      ImGui::Text("Out " ICON_FA_CARET_RIGHT);
      ed::EndPin();
      ed::EndNode();
      ed::BeginNode(uniqueId++);
      ImGui::Text("Node B");
      ed::BeginPin(uniqueId++, ed::PinKind::Input);
      ImGui::Text(ICON_FA_CARET_RIGHT " In");
      ed::EndPin();
      ImGui::SameLine();
      ed::BeginPin(uniqueId++, ed::PinKind::Output);
      ImGui::Text("Out " ICON_FA_CARET_RIGHT);
      ed::EndPin();
      ed::EndNode();
      ed::End();
      ed::SetCurrentEditor(nullptr);

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
