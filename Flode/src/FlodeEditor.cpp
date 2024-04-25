#include "FlodeEditor.h"

#include "utilities/drawing.h"
#include "utilities/widgets.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilString.h"

#include "LowEditorBase.h"
#include "LowEditorMainWindow.h"

#include "LowRendererImGuiHelper.h"

#include "FlodeMathNodes.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  Editor::Editor() : m_LoadPath(""), m_Graph(nullptr)
  {
    NodeEd::Config l_Config;
    m_Context = NodeEd::CreateEditor(&l_Config);
  }

  void Editor::load(Low::Util::String p_Path)
  {
    m_LoadPath = p_Path;
  }

  void Editor::render_graph(float p_Delta)
  {
    if (!m_Graph) {
      return;
    }

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
      if (!m_Graph) {
        ImGui::Text("No graph loaded");
      } else {
        auto l_Categories = get_node_types();

        ImGui::Text("Create Node");
        ImGui::Separator();

        for (auto cit = l_Categories.begin();
             cit != l_Categories.end(); ++cit) {
          if (ImGui::TreeNode(cit->first.c_str())) {
            for (auto it = cit->second.begin();
                 it != cit->second.end(); ++it) {
              if (ImGui::MenuItem(it->first.c_str())) {
                Node *i_Node = m_Graph->create_node(it->second);
                NodeEd::SetNodePosition(i_Node->id,
                                        m_StoredMousePosition);
              }
            }
            ImGui::TreePop();
          }
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

    Low::Util::List<PinType> l_Types = {
        PinType::String,  PinType::Number,  PinType::Bool,
        PinType::Vector2, PinType::Vector3, PinType::Quaternion,
        PinType::Handle};

    struct Funcs
    {
      static bool PinTypeGetter(void *data, int n,
                                const char **out_str)
      {
        PinType l_Type = ((PinType *)data)[n];
        Low::Util::String l_Name =
            Low::Editor::prettify_name(pin_type_to_string(l_Type));
        *out_str = (char *)malloc(l_Name.size() + 1);
        memcpy((void *)*out_str, l_Name.c_str(), l_Name.size());
        (*(char **)out_str)[l_Name.size()] = '\0';
        return true;
      }
    };

    ImGui::BeginChild(
        "Selection",
        ImVec2(l_Width, ImGui::GetContentRegionAvail().y), true, 0);
    if (m_SelectedNodes.size() == 1) {
      m_SelectedNodes[0]->render_data();
    } else if (m_Graph) {
      ImGui::PushID(238392);
      ImGui::PushFont(Low::Renderer::ImGuiHelper::fonts().common_500);
      ImGui::Text("Graph");
      ImGui::PopFont();
      {
        ImGui::PushItemWidth(100.0f);
        ImGui::Text("Name");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::Text(m_Graph->m_Name.c_str());
        ImGui::PopItemWidth();
      }
      {
        ImGui::PushID(985927);
        ImGui::PushItemWidth(100.0f);
        ImGui::Text("Namespace");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        if (ImGui::Button(ICON_FA_PLUS "")) {
          m_Graph->m_Namespace.push_back(Low::Util::Name("ns"));
        }
        ImGui::PopItemWidth();
        int i = 0;
        for (auto it = m_Graph->m_Namespace.begin();
             it != m_Graph->m_Namespace.end();) {
          ImGui::PushID(i);
          ImGui::PushItemWidth(120.0f);
          Low::Editor::Base::NameEdit("##namespaceedit", &*it);
          ImGui::PopItemWidth();
          ImGui::SameLine();
          if (ImGui::Button(ICON_FA_TRASH "")) {
            it = m_Graph->m_Namespace.erase(it);
          } else {
            it++;
          }
          ImGui::PopID();
          i++;
        }
        ImGui::PopID();
      }
      ImGui::Separator();
      ImGui::PopID();
      ImGui::PushID(638745);
      ImGui::PushFont(Low::Renderer::ImGuiHelper::fonts().common_500);
      ImGui::Text("Variables");
      ImGui::PopFont();
      if (ImGui::Button(ICON_FA_PLUS "")) {
        Variable *l_Variable = new Variable();
        l_Variable->name = "NewVar";
        l_Variable->type = PinType::Number;

        m_Graph->m_Variables.push_back(l_Variable);
      }

      int i = 0;
      for (auto it = m_Graph->m_Variables.begin();
           it != m_Graph->m_Variables.end();) {
        Variable *i_Variable = *it;
        ImGui::PushID(i);
        i++;
        ImGui::PushItemWidth(120.0f);
        Low::Editor::Base::StringEdit("##variablename",
                                      &i_Variable->name);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(100.0f);

        int l_CurrentValue = 0;
        for (int i = 0; i < l_Types.size(); ++i) {
          if (l_Types[i] == i_Variable->type) {
            l_CurrentValue = i;
            break;
          }
        }
        bool l_Result = ImGui::Combo("##paramtype", &l_CurrentValue,
                                     &Funcs::PinTypeGetter,
                                     l_Types.data(), l_Types.size());

        if (l_Result) {
          // l_Changed = true;

          i_Variable->type = l_Types[l_CurrentValue];
        }

        ImGui::SameLine();

        bool i_Deleted = false;
        if (ImGui::Button(ICON_FA_TRASH "")) {
          it = m_Graph->m_Variables.erase(it);
          i_Deleted = true;
        } else {
          it++;
        }

        if (!i_Deleted) {
          if (i_Variable->type == PinType::Handle) {
            ImGui::Dummy(ImVec2(120.0f, 0.0f));
            ImGui::SameLine();

            int i_CurrentTypeValue = 0;

            for (; i_CurrentTypeValue < get_exposed_types().size();
                 ++i_CurrentTypeValue) {
              if (get_exposed_types()[i_CurrentTypeValue].typeId ==
                  i_Variable->typeId) {
                break;
              }
            }

            bool i_TypeChanged = ImGui::Combo(
                "##variableselector", &i_CurrentTypeValue,
                [](void *data, int n, const char **out_str) {
                  Low::Editor::TypeMetadata &l_Metadata =
                      ((Low::Editor::TypeMetadata *)data)[n];
                  Low::Util::String l_Name = l_Metadata.name.c_str();
                  *out_str = (char *)malloc(l_Name.size() + 1);
                  memcpy((void *)*out_str, l_Name.c_str(),
                         l_Name.size());
                  (*(char **)out_str)[l_Name.size()] = '\0';
                  return true;
                },
                get_exposed_types().data(),
                get_exposed_types().size());

            if (i_TypeChanged) {
              for (u32 i = 0; i < get_exposed_types().size(); ++i) {
                if (i == i_CurrentTypeValue) {
                  i_Variable->typeId = get_exposed_types()[i].typeId;
                  break;
                }
              }
            }
          }
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
      }

      ImGui::PopID();
    }
    ImGui::EndChild();
  }

  void Editor::render(float p_Delta)
  {
    bool l_Save = false;

    if (m_Graph) {
      if (ImGui::Button(ICON_FA_SAVE " Save")) {
        l_Save = true;
      }
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

    if (!m_LoadPath.empty()) {
      if (m_Graph) {
        delete m_Graph;
      }

      Low::Util::Yaml::Node l_Node =
          Low::Util::Yaml::load_file(m_LoadPath.c_str());

      m_Graph = new Graph;
      m_Graph->deserialize(l_Node);

      m_LoadPath = "";
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
      m_Graph->clean_unconnected_links();
      m_Graph->serialize(l_Node);

      Low::Util::String l_Path = Low::Util::get_project().dataPath;
      l_Path += "/assets/flode/" +
                Low::Util::String(m_Graph->m_Name.c_str()) +
                ".flode.yaml";

      Low::Util::Yaml::write_file(l_Path.c_str(), l_Node);

      LOW_LOG_INFO << "Saved flode graph '" << m_Graph->m_Name
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

      NodeEd::NodeId l_DeleteNodeId;
      bool l_HasDeletedNode = false;
      while (NodeEd::QueryDeletedNode(&l_DeleteNodeId)) {
        // If you agree that node can be deleted, accept deletion.
        if (NodeEd::AcceptDeletedItem()) {
          l_HasDeletedNode = true;
          // Then remove node from your data.
          m_Graph->delete_node(l_DeleteNodeId);
        }

        // You may reject node deletion by calling:
        // ed::RejectDeletedItem();
      }

      if (l_HasDeletedNode) {
        // Clear the selected nodes in case they were deleted to not
        // run into dangling pointers
        m_SelectedNodes.clear();
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
