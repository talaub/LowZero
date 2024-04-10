#include "FlodeEditor.h"

#include "utilities/drawing.h"
#include "utilities/widgets.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilString.h"

#include "FlodeMathNodes.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  Editor::Editor() : m_FirstRun(true)
  {
    NodeEd::Config l_Config;
    m_Context = NodeEd::CreateEditor(&l_Config);
  }

  void Editor::load(Low::Util::String p_Path)
  {
  }

  void Editor::render_graph(float p_Delta)
  {
    // TODO: Collect connected pins on graph
    // collect_connected_pins();

    for (const auto &i_Node : m_Graph->m_Nodes) {
      i_Node->render();
    }

    for (const Flode::Link *i_Link : m_Graph->m_Links) {
      render_link(p_Delta, i_Link);
    }
  }

  void Editor::create_node_popup()
  {
    ImGui::SetNextWindowSize(ImVec2(300, 200));
    if (ImGui::BeginPopup("Create Node")) {
      auto l_Categories = get_node_types();

      ImGui::Text("Create Node");
      ImGui::Separator();

      for (auto cit = l_Categories.begin(); cit != l_Categories.end();
           ++cit) {
        if (ImGui::TreeNode(cit->first.c_str())) {
          for (auto it = cit->second.begin(); it != cit->second.end();
               ++it) {
            if (ImGui::MenuItem(it->first.c_str())) {
              Node *i_Node = m_Graph->create_node(it->second);
              NodeEd::SetNodePosition(i_Node->id,
                                      m_StoredMousePosition);
            }
          }
          ImGui::TreePop();
        }
      }
      ImGui::EndPopup();
    }
  }

  void Editor::render_data_panel()
  {
    auto &io = ImGui::GetIO();

    float l_Width = ImGui::GetContentRegionAvail().x / 4.0f;

    l_Width = LOW_MATH_MAX(300.0f, l_Width);
    l_Width = LOW_MATH_MIN(500.0f, l_Width);

    // 300 seems to be a nice width so we hardcode it for now. Maybe
    // we can make it "docked" like we do it in the different managers
    l_Width = 300.0f;

    ImGui::BeginChild(
        "Selection",
        ImVec2(l_Width, ImGui::GetContentRegionAvail().y), true, 0);
    if (m_SelectedNodes.size() == 1) {
      m_SelectedNodes[0]->render_data();
    }
    ImGui::EndChild();
  }

  void Editor::render(float p_Delta)
  {
    bool l_Save = false;
    if (ImGui::Button(ICON_FA_SAVE " Save")) {
      l_Save = true;
    }

    if (m_Graph) {
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_COG " Compile")) {
        m_Graph->compile();
      }
    }

    render_data_panel();

    ImGui::SameLine();

    {
      ImVec2 l_Region = ImGui::GetContentRegionAvail();
      if (l_Region.x < 5 || l_Region.y < 5) {
        return;
      }
    }

    NodeEd::SetCurrentEditor(m_Context);
    NodeEd::Begin("FlodeEditor", ImVec2(0.0, 0.0f));

    m_StoredMousePosition = ImGui::GetMousePos();

    if (m_FirstRun) {
      m_FirstRun = false;
      Low::Util::String l_Path = Low::Util::get_project().dataPath;
      l_Path += "/assets/flode/" + LOW_TO_STRING(25) + ".flode.yaml";

      Low::Util::Yaml::Node l_Node =
          Low::Util::Yaml::load_file(l_Path.c_str());

      m_Graph = new Graph;
      m_Graph->deserialize(l_Node);
    }

    NodeEd::Suspend();
    /*
    if (NodeEd::ShowNodeContextMenu(&contextNodeId))
      ImGui::OpenPopup("Node Context Menu");
    else if (NodeEd::ShowPinContextMenu(&contextPinId))
      ImGui::OpenPopup("Pin Context Menu");
    else if (NodeEd::ShowLinkContextMenu(&contextLinkId))
      ImGui::OpenPopup("Link Context Menu");
    else */
    if (NodeEd::ShowBackgroundContextMenu()) {
      ImGui::OpenPopup("Create Node");
    }
    NodeEd::Resume();

    NodeEd::Suspend();
    create_node_popup();
    NodeEd::Resume();

    m_SelectedNodes.clear();

    if (m_Graph) {
      Low::Util::List<NodeEd::NodeId> l_SelectedNodeIds;
      l_SelectedNodeIds.resize(NodeEd::GetSelectedObjectCount());

      int l_SelectedNodeCount = NodeEd::GetSelectedNodes(
          l_SelectedNodeIds.data(),
          static_cast<int>(l_SelectedNodeIds.size()));

      for (int i = 0; i < l_SelectedNodeCount; ++i) {
        m_SelectedNodes.push_back(
            m_Graph->find_node(l_SelectedNodeIds[i]));
      }

      render_graph(p_Delta);
    }

    if (l_Save) {
      Low::Util::Yaml::Node l_Node;
      m_Graph->serialize(l_Node);

      Low::Util::String l_Path = Low::Util::get_project().dataPath;
      l_Path += "/assets/flode/" + LOW_TO_STRING(25) + ".flode.yaml";

      Low::Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved flode graph '"
                   << "test"
                   << "' to file." << LOW_LOG_END;
    }

    // Handle creation action, returns true if editor want to create
    // new object (node or link)
    if (NodeEd::BeginCreate(ImColor(255, 255, 255), 2.0f)) {

      auto showLabel = [](const char *label, ImColor color) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() -
                             ImGui::GetTextLineHeight());
        auto size = ImGui::CalcTextSize(label);

        auto padding = ImGui::GetStyle().FramePadding;
        auto spacing = ImGui::GetStyle().ItemSpacing;

        ImGui::SetCursorPos(ImGui::GetCursorPos() +
                            ImVec2(spacing.x, -spacing.y));

        auto rectMin = ImGui::GetCursorScreenPos() - padding;
        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

        auto drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(rectMin, rectMax, color,
                                size.y * 0.15f);
        ImGui::TextUnformatted(label);
      };

      NodeEd::PinId l_InputPinId = 0, l_OutputPinId = 0;
      if (NodeEd::QueryNewLink(&l_OutputPinId, &l_InputPinId)) {
        // QueryNewLink returns true if editor want to create new
        // link between pins.
        //
        // Link can be created only for two valid pins, it is up
        // to you to validate if connection make sense. Editor is
        // happy to make any.
        //
        // Link always goes from input to output. User may choose
        // to drag link from output pin or input pin. This
        // determine which pin ids are valid and which are not:
        //   * input valid, output invalid - user started to drag
        //   new ling from input pin
        //   * input invalid, output valid - user started to drag
        //   new ling from output pin
        //   * input valid, output valid   - user dragged link
        //   over other pin, can be validated

        if (l_InputPinId &&
            l_OutputPinId) // both are valid, let's accept link
        {
          // ed::AcceptNewItem() return true when user release
          // mouse button.
          if (m_Graph->can_create_link(l_InputPinId, l_OutputPinId)) {
            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
            if (NodeEd::AcceptNewItem()) {
              // Since we accepted new link, lets add one to our
              // list of links.
              m_Graph->create_link_castable(l_InputPinId,
                                            l_OutputPinId);
            }
          } else {
            showLabel("x Not compatible", ImColor(45, 32, 32, 180));
            NodeEd::RejectNewItem(ImColor(255, 0, 0), 2.0f);
          }

          // You may choose to reject connection between these
          // nodes by calling ed::RejectNewItem(). This will allow
          // editor to give visual feedback by changing link
          // thickness and color.
        }
      }
      NodeEd::PinId pinId = 0;
      if (NodeEd::QueryNewNode(&pinId)) {
        Flode::Pin *newLinkPin = m_Graph->find_pin(pinId);
        showLabel("+ Create Node", ImColor(32, 45, 32, 180));

        if (NodeEd::AcceptNewItem()) {
          // newLinkPin = nullptr;
          NodeEd::Suspend();
          ImGui::OpenPopup("Create Node");
          NodeEd::Resume();
        }
      }
    }
    NodeEd::EndCreate();

    // Handle deletion action
    if (NodeEd::BeginDelete()) {
      // There may be many links marked for deletion, let's loop
      // over them.
      NodeEd::LinkId l_DeleteLinkId;
      while (NodeEd::QueryDeletedLink(&l_DeleteLinkId)) {
        // If you agree that link can be deleted, accept deletion.
        if (NodeEd::AcceptDeletedItem()) {
          // Then remove link from your data.
          m_Graph->delete_link(l_DeleteLinkId);
        }

        // You may reject link deletion by calling:
        // ed::RejectDeletedItem();
      }
    }
    NodeEd::EndDelete(); // Wrap up deletion action

    NodeEd::End();
    NodeEd::SetCurrentEditor(nullptr);
  }

  void Editor::render_link(float p_Delta, const Flode::Link *p_Link)
  {
    NodeEd::Link(p_Link->id, p_Link->inputPinId, p_Link->outputPinId,
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);
  }
} // namespace Flode
