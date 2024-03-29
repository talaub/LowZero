#include "FlodeEditor.h"

#include "utilities/drawing.h"
#include "utilities/widgets.h"

#include "LowUtilAssert.h"

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Flode {
  Low::Util::Map<
      Low::Util::String,
      Low::Util::Map<Low::Util::String, Flode::create_node_callback>>
      Editor::ms_NodeTypes;

  const int m_PinIconSize = 20;

  ImVec4 g_NodePadding(4, 2, 4, 4);

  void
  Editor::register_node_type(Low::Util::String p_Category,
                             Low::Util::String p_Title,
                             Flode::create_node_callback p_Callback)
  {
    ms_NodeTypes[p_Category][p_Title] = p_Callback;
  }

  ImColor get_icon_color(Flode::PinType type)
  {
    using namespace Flode;

    switch (type) {
    default:
    case PinType::Flow:
      return ImColor(255, 255, 255);
    case PinType::Bool:
      return ImColor(220, 48, 48);
    case PinType::Int:
      return ImColor(68, 201, 156);
    case PinType::Float:
      return ImColor(147, 226, 74);
    case PinType::String:
      return ImColor(124, 21, 153);
    case PinType::Object:
      return ImColor(51, 150, 215);
    case PinType::Function:
      return ImColor(218, 0, 183);
    case PinType::Delegate:
      return ImColor(255, 48, 48);
    }
  };

  void draw_pin_icon(const Flode::Pin &pin, bool connected, int alpha)
  {
    using namespace ax::Drawing;

    IconType iconType;
    ImColor color = get_icon_color(pin.type);
    color.Value.w = alpha / 255.0f;
    switch (pin.type) {
    case Flode::PinType::Flow:
      iconType = IconType::Flow;
      break;
    case Flode::PinType::Bool:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Int:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Float:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::String:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Object:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Function:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Delegate:
      iconType = IconType::Square;
      break;
    default:
      return;
    }

    ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize),
                             static_cast<float>(m_PinIconSize)),
                      iconType, connected, color,
                      ImColor(32, 32, 32, alpha));
  };

  Editor::Editor()
  {
    NodeEd::Config l_Config;
    m_Context = NodeEd::CreateEditor(&l_Config);
  }

  void Editor::render_graph(float p_Delta)
  {
    collect_connected_pins();

    for (const Flode::Node &i_Node : m_Nodes) {
      render_node(p_Delta, i_Node);
    }

    for (const Flode::Link &i_Link : m_Links) {
      render_link(p_Delta, i_Link);
    }
  }

  Flode::Pin Editor::find_pin(NodeEd::PinId p_PinId)
  {
    for (Flode::Node &i_Node : m_Nodes) {
      for (Flode::Pin &i_Pin : i_Node.pins) {
        if (i_Pin.id == p_PinId) {
          return i_Pin;
        }
      }
    }

    _LOW_ASSERT(false);

    return Flode::Pin();
  }

  bool Editor::can_create_link(NodeEd::PinId p_InputPinId,
                               NodeEd::PinId p_OutputPinId)
  {
    if (p_InputPinId == p_OutputPinId) {
      return false;
    }

    Flode::Pin l_InputPin = find_pin(p_InputPinId);
    Flode::Pin l_OutputPin = find_pin(p_OutputPinId);

    if (l_InputPin.direction == l_OutputPin.direction) {
      return false;
    }

    if (l_InputPin.nodeId == l_OutputPin.nodeId) {
      return false;
    }

    return true;
  }

  void Editor::create_link(NodeEd::PinId p_InputPin,
                           NodeEd::PinId p_OutputPin)
  {
    m_Links.push_back({NodeEd::LinkId(500), p_InputPin, p_OutputPin});

    // Draw new link.
    NodeEd::Link(m_Links.back().id, m_Links.back().inputPinId,
                 m_Links.back().outputPinId);
  }

  void Editor::delete_link(NodeEd::LinkId p_LinkId)
  {
    for (auto &i_Link : m_Links) {
      if (i_Link.id == p_LinkId) {
        m_Links.erase(&i_Link);
        break;
      }
    }
  }

  void Editor::render(float p_Delta)
  {
    NodeEd::SetCurrentEditor(m_Context);
    NodeEd::Begin("FlodeEditor", ImVec2(0.0, 0.0f));

    render_graph(p_Delta);

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
      if (NodeEd::QueryNewLink(&l_InputPinId, &l_OutputPinId)) {
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
          if (can_create_link(l_InputPinId, l_OutputPinId)) {
            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
            if (NodeEd::AcceptNewItem()) {
              // Since we accepted new link, lets add one to our
              // list of links.
              create_link(l_InputPinId, l_OutputPinId);
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
        Flode::Pin newLinkPin = find_pin(pinId);
        showLabel("+ Create Node", ImColor(32, 45, 32, 180));

        if (NodeEd::AcceptNewItem()) {
          // newLinkPin = nullptr;
          NodeEd::Suspend();
          // ImGui::OpenPopup("Create New Node");
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
          delete_link(l_DeleteLinkId);
        }

        // You may reject link deletion by calling:
        // ed::RejectDeletedItem();
      }
    }
    NodeEd::EndDelete(); // Wrap up deletion action

    NodeEd::End();
    NodeEd::SetCurrentEditor(nullptr);
  }

  void Editor::collect_connected_pins()
  {
    m_ConnectedPins.clear();

    for (Flode::Link &i_Link : m_Links) {
      m_ConnectedPins.insert(i_Link.inputPinId.Get());
      m_ConnectedPins.insert(i_Link.outputPinId.Get());
    }
  }

  bool Editor::is_pin_connected(const Flode::Pin &p_Pin)
  {
    return false;
  }

  void Editor::render_pin(float p_Delta, const Flode::Pin &p_Pin)
  {
    if (p_Pin.direction == Flode::PinDirection::Input) {
      NodeEd::BeginPin(p_Pin.id, NodeEd::PinKind::Input);
    } else {
      ImGui::Spring(0);
      NodeEd::BeginPin(p_Pin.id, NodeEd::PinKind::Output);
    }

    auto alpha = ImGui::GetStyle().Alpha;

    // ImGui::Spring(0);

    ImGui::BeginHorizontal(p_Pin.id.AsPointer());
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (p_Pin.direction == Flode::PinDirection::Input) {
      draw_pin_icon(p_Pin, is_pin_connected(p_Pin),
                    (int)(alpha * 255));
      ImGui::Spring(0);
      if (!p_Pin.title.empty()) {
        ImGui::TextUnformatted(p_Pin.title.c_str());
        ImGui::Spring(0);
      }
      {
        if (p_Pin.type == Flode::PinType::Bool) {
          bool t;
          ImGui::Checkbox("##edit", &t);
        } else if (p_Pin.type == Flode::PinType::Int) {
          int t;
          ImGui::DragInt("##edit", &t);
        }
      }
    } else {
      if (!p_Pin.title.empty()) {
        ImGui::Spring(0);
        ImGui::TextUnformatted(p_Pin.title.c_str());
      }
      ImGui::Spring(0);
      draw_pin_icon(p_Pin, is_pin_connected(p_Pin),
                    (int)(alpha * 255));
    }
    ImGui::PopStyleVar();
    ImGui::EndHorizontal();
    NodeEd::EndPin();
  }

  void Editor::render_node(float p_Delta, const Flode::Node &p_Node)
  {
    NodeEd::GetStyle().NodeBorderWidth = 0.0f;
    NodeEd::GetStyle().NodePadding = g_NodePadding;

    /*
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_NodeBg,
        color_to_imvec4(theme_get_current().header));
    NodeEd::PushStyleColor(NodeEd::StyleColor_Bg,
                           color_to_imvec4(theme_get_current().base));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_SelNodeBorder,
        color_to_imvec4(theme_get_current().button));
    NodeEd::PushStyleColor(
        NodeEd::StyleColor_PinRect,
        color_to_imvec4(theme_get_current().button));
                           */

    u32 l_ColorCount = 4;

    ImVec2 l_HeaderMax;
    ImVec2 l_HeaderMin = l_HeaderMax = ImVec2();

    NodeEd::PushStyleVar(NodeEd::StyleVar_NodePadding, g_NodePadding);

    NodeEd::BeginNode(p_Node.id);
    ImGui::BeginVertical("node");

    ImGui::PushID(p_Node.id.AsPointer());

    {
      ImGui::BeginHorizontal("header");

      ImGui::Spring(0);
      // ImGui::TextUnformatted(p_Node.title.c_str());
      ImGui::TextUnformatted("TEST");
      ImGui::Spring(1);
      ImGui::Dummy(ImVec2(0, 20));
      ImGui::Spring(0);

      ImGui::EndHorizontal();
      l_HeaderMin = ImGui::GetItemRectMin();
      l_HeaderMax = ImGui::GetItemRectMax();
    }

    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

    {
      ImGui::BeginHorizontal("content");
      ImGui::Spring(0, 0);
    }

    {
      ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);

      NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                           ImVec2(0, 0.5f));
      NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

      for (auto &i_Pin : p_Node.pins) {
        if (i_Pin.direction == Flode::PinDirection::Input) {
          render_pin(p_Delta, i_Pin);
        }
      }

      NodeEd::PopStyleVar(2);
      ImGui::Spring(1, 0);
      ImGui::EndVertical();
    }

    {
      ImGui::Spring(1);

      ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);

      NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                           ImVec2(1.0f, 0.5f));
      NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

      for (auto &i_Pin : p_Node.pins) {
        if (i_Pin.direction == Flode::PinDirection::Output) {
          render_pin(p_Delta, i_Pin);
        }
      }

      NodeEd::PopStyleVar(2);

      ImGui::Spring(1, 0);
      ImGui::EndVertical();
    }

    ImGui::EndHorizontal();
    ImGui::EndVertical();

    NodeEd::EndNode();

    const auto l_HalfBorderWidth =
        NodeEd::GetStyle().NodeBorderWidth * 0.5f;

    auto l_DrawList = NodeEd::GetNodeBackgroundDrawList(p_Node.id);
    l_DrawList->AddRectFilled(
        l_HeaderMin - ImVec2(g_NodePadding.z - l_HalfBorderWidth,
                             g_NodePadding.y - l_HalfBorderWidth),
        l_HeaderMax + ImVec2(g_NodePadding.x - l_HalfBorderWidth, 0),
        IM_COL32_BLACK, NodeEd::GetStyle().NodeRounding,
        ImDrawFlags_RoundCornersTop);

    NodeEd::PopStyleVar();

    ImGui::PopID();

    // NodeEd::PopStyleColor(l_ColorCount);
  }

  void Editor::render_link(float p_Delta, const Flode::Link &p_Link)
  {
    NodeEd::Link(p_Link.id, p_Link.inputPinId, p_Link.outputPinId);
  }
} // namespace Flode
