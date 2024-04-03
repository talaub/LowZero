#include "Flode.h"

#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"
#include "LowUtilFileIO.h"

#include "LowEditorPropertyEditors.h"
#include "LowEditorBase.h"

#include "LowRendererImGuiHelper.h"

#include "utilities/drawing.h"
#include "utilities/widgets.h"

namespace Flode {

  ImVec4 g_NodePadding(4, 2, 4, 4);
  int m_PinIconSize = 20;

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>>
      g_NodeTypes;

  Low::Util::Map<PinType, Low::Util::Map<PinType, Low::Util::Name>>
      g_CastNodes;

  Low::Util::Map<Low::Util::Name, Flode::create_node_callback>
      g_NodeTypeNames;

  Low::Util::String get_pin_default_value_as_string(Pin *p_Pin)
  {
    switch (p_Pin->type) {
    case PinType::Number:
      return (LOW_TO_STRING(p_Pin->defaultValue.m_Float) + "f");
    case PinType::String:
      return "\"\"";
    default:
      break;
    }

    return "";
  }

  void setup_variant_for_pin_type(PinType p_PinType,
                                  Low::Util::Variant &p_Variant)
  {
    switch (p_PinType) {
    case PinType::Number: {
      p_Variant.m_Type = Low::Util::VariantType::Float;
      p_Variant.m_Float = 0.0f;
      break;
    }
    default: {
      p_Variant.m_Type = Low::Util::VariantType::String;
      p_Variant.m_Int32 = 0;
      break;
    }
    }
  }

  Low::Util::String pin_direction_to_string(PinDirection p_Direction)
  {
    switch (p_Direction) {
    case PinDirection::Input:
      return "input";
    case PinDirection::Output:
      return "output";
    }

    _LOW_ASSERT(false);
  }

  Low::Util::String pin_type_to_string(PinType p_Type)
  {
    switch (p_Type) {
    case PinType::Flow:
      return "flow";
    case PinType::Number:
      return "number";
    case PinType::String:
      return "string";
    }

    _LOW_ASSERT(false);
  }

  PinType string_to_pin_type(Low::Util::String p_String)
  {
    if (p_String == "flow") {
      return PinType::Flow;
    }
    if (p_String == "number") {
      return PinType::Number;
    }
    if (p_String == "string") {
      return PinType::String;
    }

    _LOW_ASSERT(false);
  }

  void register_node(Low::Util::Name p_TypeName,
                     Flode::create_node_callback p_Callback)
  {
    LOW_ASSERT(g_NodeTypeNames.find(p_TypeName) ==
                   g_NodeTypeNames.end(),
               "A node type with this typename has already been "
               "registered.");

    g_NodeTypeNames[p_TypeName] = p_Callback;
  }

  void register_spawn_node(Low::Util::String p_Category,
                           Low::Util::String p_Name,
                           Low::Util::Name p_TypeName)
  {
    LOW_ASSERT(
        g_NodeTypeNames.find(p_TypeName) != g_NodeTypeNames.end(),
        "Node was not registered before added as a spawnable node.");
    g_NodeTypes[p_Category][p_Name] = p_TypeName;
  }

  void register_cast_node(PinType p_FromType, PinType p_ToType,
                          Low::Util::Name p_TypeName)
  {
    LOW_ASSERT(
        g_NodeTypeNames.find(p_TypeName) != g_NodeTypeNames.end(),
        "Node was not registered before added as a cast node.");
    g_CastNodes[p_FromType][p_ToType] = p_TypeName;
  }

  bool can_cast(PinType p_FromType, PinType p_ToType)
  {
    auto l_FromEntry = g_CastNodes.find(p_FromType);
    if (l_FromEntry == g_CastNodes.end()) {
      return false;
    }

    auto l_ToEntry = l_FromEntry->second.find(p_ToType);
    return l_ToEntry != l_FromEntry->second.end();
  }

  Low::Util::Name get_cast_node_typename(PinType p_FromType,
                                         PinType p_ToType)
  {
    auto l_FromEntry = g_CastNodes.find(p_FromType);
    LOW_ASSERT(l_FromEntry != g_CastNodes.end(),
               "No cast node registrered");

    auto l_ToEntry = l_FromEntry->second.find(p_ToType);
    LOW_ASSERT(l_ToEntry != l_FromEntry->second.end(),
               "No cast node registered");

    return l_ToEntry->second;
  }

  Low::Util::Map<Low::Util::String,
                 Low::Util::Map<Low::Util::String, Low::Util::Name>> &
  get_node_types()
  {
    return g_NodeTypes;
  }

  Node *spawn_node_of_type(Low::Util::Name p_TypeName)
  {
    Node *l_Node = g_NodeTypeNames[p_TypeName]();
    l_Node->typeName = p_TypeName;

    return l_Node;
  }

  static ImColor get_icon_color(Flode::PinType type)
  {
    using namespace Flode;

    switch (type) {
    default:
    case PinType::Flow:
      return ImColor(255, 255, 255);
      /*
    case PinType::Bool:
      return ImColor(220, 48, 48);
    case PinType::Int:
      return ImColor(68, 201, 156);
      */
    case PinType::Number:
      return ImColor(147, 226, 74);
    case PinType::String:
      return ImColor(124, 21, 153);
      /*
    case PinType::Object:
      return ImColor(51, 150, 215);
    case PinType::Function:
      return ImColor(218, 0, 183);
    case PinType::Delegate:
      return ImColor(255, 48, 48);
      */
    }
  };

  static void draw_pin_icon(const Flode::Pin *pin, bool connected,
                            int alpha)
  {
    using namespace ax::Drawing;

    IconType iconType;
    ImColor color = get_icon_color(pin->type);
    color.Value.w = alpha / 255.0f;
    switch (pin->type) {
    case Flode::PinType::Flow:
      iconType = IconType::Flow;
      break;
      /*
    case Flode::PinType::Bool:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Int:
      iconType = IconType::Circle;
      break;
      */
    case Flode::PinType::Number:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::String:
      iconType = IconType::Circle;
      break;
      /*
    case Flode::PinType::Object:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Function:
      iconType = IconType::Circle;
      break;
    case Flode::PinType::Delegate:
      iconType = IconType::Square;
      break;
      */
    default:
      return;
    }

    ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize),
                             static_cast<float>(m_PinIconSize)),
                      iconType, connected, color,
                      ImColor(32, 32, 32, alpha));
  };

  Low::Util::String Node::get_name(NodeNameType p_Type) const
  {
    return "FlodeNode";
  }

  void Node::compile_input_pin(Low::Util::StringBuilder &p_Builder,
                               NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin_checked(p_PinId);

    LOW_ASSERT(l_Pin->direction == PinDirection::Input,
               "Pin is not an input pin");

    if (graph->is_pin_connected(l_Pin->id)) {

      Pin *l_ConnectedPin =
          graph->find_pin(graph->get_connected_pin(l_Pin->id));
      Node *l_ConnectedNode =
          graph->find_node(l_ConnectedPin->nodeId);

      l_ConnectedNode->compile_output_pin(p_Builder,
                                          l_ConnectedPin->id);

    } else {
      p_Builder.append(get_pin_default_value_as_string(l_Pin));
    }
  }

  Pin *Node::find_pin_checked(NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin(p_PinId);

    LOW_ASSERT(l_Pin, "Could not find pin by id");

    return l_Pin;
  }

  Pin *Node::find_output_pin_checked(NodeEd::PinId p_PinId) const
  {
    Pin *l_Pin = find_pin_checked(p_PinId);

    LOW_ASSERT(l_Pin->direction == PinDirection::Output,
               "Expected pin to be an output pin.");

    return l_Pin;
  }

  Pin *Node::find_pin(NodeEd::PinId p_PinId) const
  {
    for (Pin *i_Pin : pins) {
      if (i_Pin->id == p_PinId) {
        return i_Pin;
      }
    }

    return nullptr;
  }

  Pin *Node::create_pin(PinDirection p_Direction,
                        Low::Util::String p_Title, PinType p_Type,
                        u64 p_PinId)
  {
    Pin *l_Pin = new Pin;

    l_Pin->title = p_Title;
    if (p_PinId == 0) {
      l_Pin->id = graph->m_IdCounter++;
    } else {
      l_Pin->id = p_PinId;
    }
    l_Pin->type = p_Type;
    l_Pin->direction = p_Direction;
    l_Pin->nodeId = id;
    setup_variant_for_pin_type(p_Type, l_Pin->defaultValue);

    pins.push_back(l_Pin);

    return l_Pin;
  }

  void Node::render_header()
  {
    ImGui::BeginHorizontal("header");

    ImGui::Spring(0);
    ImGui::TextUnformatted(get_name(NodeNameType::Full).c_str());
    ImGui::Spring(1);
    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Spring(0);

    ImGui::EndHorizontal();

    m_HeaderMin = ImGui::GetItemRectMin();
    m_HeaderMax = ImGui::GetItemRectMax();
  }

  void Node::render_header_cosmetics()
  {
    ImVec2 l_Max = ImGui::GetItemRectMax();

    const auto l_HalfBorderWidth =
        NodeEd::GetStyle().NodeBorderWidth * 0.5f;

    auto l_DrawList = NodeEd::GetNodeBackgroundDrawList(id);
    l_DrawList->AddRectFilled(
        m_HeaderMin - ImVec2(g_NodePadding.z - l_HalfBorderWidth,
                             g_NodePadding.y - l_HalfBorderWidth),
        ImVec2(l_Max.x - 4.0f, m_HeaderMax.y) +
            ImVec2(g_NodePadding.x - l_HalfBorderWidth, 0),
        get_color(), NodeEd::GetStyle().NodeRounding,
        ImDrawFlags_RoundCornersTop);
  }

  void Node::render_pin(Flode::Pin *p_Pin)
  {
    bool l_Connected = graph->is_pin_connected(p_Pin->id);

    if (p_Pin->direction == Flode::PinDirection::Input) {
      NodeEd::BeginPin(p_Pin->id, NodeEd::PinKind::Input);
    } else {
      ImGui::Spring(0);
      NodeEd::BeginPin(p_Pin->id, NodeEd::PinKind::Output);
    }

    auto alpha = ImGui::GetStyle().Alpha;

    ImGui::BeginHorizontal(p_Pin->id.AsPointer());
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (p_Pin->direction == Flode::PinDirection::Input) {
      draw_pin_icon(p_Pin, l_Connected, (int)(alpha * 255));
      ImGui::Spring(0);
      if (!p_Pin->title.empty()) {
        ImGui::TextUnformatted(p_Pin->title.c_str());
        ImGui::Spring(0);
      }
      if (!l_Connected) {
        if (p_Pin->type == PinType::Number) {
          ImGui::PushItemWidth(50.0f);
          Low::Editor::Base::VariantEdit("##editdefaultvalue",
                                         p_Pin->defaultValue);
          ImGui::PopItemWidth();
        }
      }
    } else {
      if (!p_Pin->title.empty()) {
        ImGui::Spring(0);
        ImGui::TextUnformatted(p_Pin->title.c_str());
      }
      ImGui::Spring(0);
      draw_pin_icon(p_Pin, l_Connected, (int)(alpha * 255));
    }
    ImGui::PopStyleVar();
    ImGui::EndHorizontal();
    NodeEd::EndPin();
  }

  void Node::render_input_pins()
  {
    // ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);
    ImGui::BeginVertical("inputs");

    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                         ImVec2(0, 0.5f));
    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

    for (Flode::Pin *i_Pin : pins) {
      if (i_Pin->direction == Flode::PinDirection::Input) {
        render_pin(i_Pin);
      }
    }

    NodeEd::PopStyleVar(2);
    ImGui::Spring(1, 0);
    ImGui::EndVertical();
  }

  void Node::render_output_pins()
  {
    ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);

    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotAlignment,
                         ImVec2(1.0f, 0.5f));
    NodeEd::PushStyleVar(NodeEd::StyleVar_PivotSize, ImVec2(0, 0));

    for (Flode::Pin *i_Pin : pins) {
      if (i_Pin->direction == Flode::PinDirection::Output) {
        render_pin(i_Pin);
      }
    }

    NodeEd::PopStyleVar(2);

    ImGui::Spring(1, 0);
    ImGui::EndVertical();
  }

  void Node::default_render_compact()
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

    NodeEd::PushStyleVar(NodeEd::StyleVar_NodePadding, g_NodePadding);

    NodeEd::BeginNode(id);
    ImGui::BeginVertical("node");

    ImGui::PushID(id.AsPointer());

    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

    {
      ImGui::BeginHorizontal("content");
      ImGui::Spring(0, 0);
    }

    render_input_pins();

    ImGui::Spring(0);
    ImGui::PushFont(Low::Renderer::ImGuiHelper::fonts().common_800);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImVec4(0.9f, 0.9f, 0.9f, 0.5f));
    ImGui::TextUnformatted(get_name(NodeNameType::Compact).c_str());
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Spring(0);

    render_output_pins();

    ImGui::EndHorizontal();
    ImGui::EndVertical();

    NodeEd::EndNode();

    NodeEd::PopStyleVar();

    ImGui::PopID();

    // NodeEd::PopStyleColor(l_ColorCount);
  }

  void Node::default_render()
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

    NodeEd::PushStyleVar(NodeEd::StyleVar_NodePadding, g_NodePadding);

    NodeEd::BeginNode(id);
    ImGui::BeginVertical("node");

    ImGui::PushID(id.AsPointer());

    render_header();

    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);

    ImGui::BeginHorizontal("content");
    // ImGui::Spring(0, 0);

    render_input_pins();

    ImGui::Spring(1);

    render_output_pins();

    ImGui::EndHorizontal(); // End content

    ImGui::PopID();

    ImGui::EndVertical(); // End node

    NodeEd::EndNode();

    NodeEd::PopStyleVar();

    render_header_cosmetics();

    // ImGui::PopID();

    // NodeEd::PopStyleColor(l_ColorCount);
  }

  void Node::render()
  {
    if (is_compact()) {
      default_render_compact();
    } else {
      default_render();
    }
  }

  Node *Graph::find_node(NodeEd::NodeId p_NodeId) const
  {
    for (Flode::Node *i_Node : m_Nodes) {
      if (i_Node->id == p_NodeId) {
        return i_Node;
      }
    }

    return nullptr;
  }

  Pin *Graph::find_pin(NodeEd::PinId p_PinId)
  {
    for (Flode::Node *i_Node : m_Nodes) {
      Flode::Pin *i_Pin = i_Node->find_pin(p_PinId);

      if (i_Pin) {
        return i_Pin;
      }
    }

    return nullptr;
  }

  void Graph::delete_link(NodeEd::LinkId p_LinkId)
  {
    for (auto it = m_Links.begin(); it != m_Links.end();) {
      Link *i_Link = *it;
      if (i_Link->id == p_LinkId) {
        it = m_Links.erase(it);

        delete i_Link;
      } else {
        ++it;
      }
    }
  }

  Link *Graph::create_link_castable(NodeEd::PinId p_InputPin,
                                    NodeEd::PinId p_OutputPin)
  {
    Pin *l_InputPin = find_pin(p_InputPin);
    Pin *l_OutputPin = find_pin(p_OutputPin);

    _LOW_ASSERT(l_InputPin->direction == PinDirection::Input);
    _LOW_ASSERT(l_OutputPin->direction == PinDirection::Output);

    if (l_InputPin->type == l_OutputPin->type) {
      return create_link(p_InputPin, p_OutputPin);
    }

    LOW_ASSERT(can_cast(l_OutputPin->type, l_InputPin->type),
               "Cannot cast between these two pin types.");

    Low::Util::Name l_CastTypeName =
        get_cast_node_typename(l_OutputPin->type, l_InputPin->type);

    Node *l_CastNode = create_node(l_CastTypeName);

    create_link(p_InputPin, l_CastNode->pins[1]->id);

    return create_link(l_CastNode->pins[0]->id, p_OutputPin);
  }

  Link *Graph::create_link(NodeEd::PinId p_InputPin,
                           NodeEd::PinId p_OutputPin)
  {
    Link *l_Link = new Link(NodeEd::LinkId(m_IdCounter++), p_InputPin,
                            p_OutputPin);
    m_Links.push_back(l_Link);

    // Draw new link
    NodeEd::Link(m_Links.back()->id, m_Links.back()->inputPinId,
                 m_Links.back()->outputPinId);

    return m_Links.back();
  }

  bool Graph::can_create_link(NodeEd::PinId p_InputPinId,
                              NodeEd::PinId p_OutputPinId)
  {
    if (p_InputPinId == p_OutputPinId) {
      return false;
    }

    Flode::Pin *l_InputPin = find_pin(p_InputPinId);
    Flode::Pin *l_OutputPin = find_pin(p_OutputPinId);

    if (l_InputPin->direction == l_OutputPin->direction) {
      return false;
    }

    if (l_InputPin->nodeId == l_OutputPin->nodeId) {
      return false;
    }

    if (l_InputPin->type != l_OutputPin->type) {
      if (!can_cast(l_OutputPin->type, l_InputPin->type)) {
        return false;
      }
    }

    return true;
  }

  Node *Graph::create_node(Low::Util::Name p_TypeName)
  {
    Node *l_Node = spawn_node_of_type(p_TypeName);

    l_Node->graph = this;

    l_Node->id = m_IdCounter++;
    l_Node->setup_default_pins();

    m_Nodes.push_back(l_Node);

    return l_Node;
  }

  NodeEd::PinId Graph::get_connected_pin(NodeEd::PinId p_PinId) const
  {
    for (Link *i_Link : m_Links) {
      if (i_Link->inputPinId == p_PinId) {
        return i_Link->outputPinId;
      }
      if (i_Link->outputPinId == p_PinId) {
        return i_Link->inputPinId;
      }
    }

    _LOW_ASSERT(false);

    return 0;
  }

  bool Graph::is_pin_connected(NodeEd::PinId p_PinId) const
  {
    // TODO: Make more efficient by caching info

    for (Link *i_Link : m_Links) {
      if (i_Link->inputPinId == p_PinId) {
        return true;
      }
      if (i_Link->outputPinId == p_PinId) {
        return true;
      }
    }

    return false;
  }

  void Graph::clean_unconnected_links()
  {
    for (auto it = m_Links.begin(); it != m_Links.end();) {
      Link *i_Link = *it;

      Pin *i_InputPin = find_pin(i_Link->inputPinId);
      Pin *i_OutputPin = find_pin(i_Link->outputPinId);

      if (i_InputPin && i_OutputPin) {
        ++it;
      } else {
        it = m_Links.erase(it);
        delete i_Link;
      }
    }
  }

  void Graph::serialize(Low::Util::Yaml::Node &p_Node) const
  {
    p_Node["idcounter"] = m_IdCounter;
    for (const Node *i_Node : m_Nodes) {
      Low::Util::Yaml::Node i_Yaml;

      i_Yaml["type"] = i_Node->typeName.c_str();
      i_Yaml["id"] = i_Node->id.Get();

      for (Pin *i_Pin : i_Node->pins) {
        Low::Util::Yaml::Node i_PinYaml;
        i_PinYaml["id"] = i_Pin->id.Get();
        i_PinYaml["direction"] =
            pin_direction_to_string(i_Pin->direction).c_str();
        i_PinYaml["type"] = pin_type_to_string(i_Pin->type).c_str();

        if (i_Pin->defaultValue.m_Type !=
            Low::Util::VariantType::String) {
          Low::Util::Serialization::serialize_variant(
              i_PinYaml["default_value"], i_Pin->defaultValue);
        }

        i_Yaml["pins"].push_back(i_PinYaml);
      }

      Low::Math::Vector2 i_Position;
      ImVec2 i_ImPos = NodeEd::GetNodePosition(i_Node->id);
      i_Position.x = i_ImPos.x;
      i_Position.y = i_ImPos.y;

      Low::Util::Serialization::serialize(i_Yaml["position"],
                                          i_Position);

      i_Node->serialize(i_Yaml["node_data"]);

      p_Node["nodes"].push_back(i_Yaml);
    }

    for (const Link *i_Link : m_Links) {
      Low::Util::Yaml::Node i_LinkYaml;
      i_LinkYaml["id"] = i_Link->id.Get();
      i_LinkYaml["input"] = i_Link->inputPinId.Get();
      i_LinkYaml["output"] = i_Link->outputPinId.Get();

      p_Node["links"].push_back(i_LinkYaml);
    }
  }

  void Graph::deserialize(Low::Util::Yaml::Node &p_Node)
  {
    if (p_Node["nodes"]) {
      for (auto it = p_Node["nodes"].begin();
           it != p_Node["nodes"].end(); ++it) {
        Low::Util::Yaml::Node &i_NodeNode = *it;

        Low::Util::Name i_NodeTypeName =
            LOW_YAML_AS_NAME(i_NodeNode["type"]);

        Node *i_Node = spawn_node_of_type(i_NodeTypeName);
        i_Node->id = i_NodeNode["id"].as<u64>();
        i_Node->graph = this;

        if (i_NodeNode["node_data"]) {
          i_Node->deserialize(i_NodeNode["node_data"]);
        }

        i_Node->setup_default_pins();

        for (u32 i = 0; i < i_Node->pins.size(); ++i) {
          i_Node->pins[i]->id = i_NodeNode["pins"][i]["id"].as<u64>();
          if (i_NodeNode["pins"][i]["default_value"]) {
            i_Node->pins[i]->defaultValue =
                Low::Util::Serialization::deserialize_variant(
                    i_NodeNode["pins"][i]["default_value"]);
          }
        }

        Low::Math::Vector2 i_Position =
            Low::Util::Serialization::deserialize_vector2(
                i_NodeNode["position"]);
        NodeEd::SetNodePosition(i_Node->id,
                                ImVec2(i_Position.x, i_Position.y));
        m_Nodes.push_back(i_Node);
      }
    }

    if (p_Node["links"]) {
      for (auto it = p_Node["links"].begin();
           it != p_Node["links"].end(); ++it) {
        Low::Util::Yaml::Node &i_LinkNode = *it;

        Link *i_Link = create_link(i_LinkNode["input"].as<u64>(),
                                   i_LinkNode["output"].as<u64>());

        i_Link->id = i_LinkNode["id"].as<u64>();
      }
    }

    if (p_Node["idcounter"]) {
      m_IdCounter = p_Node["idcounter"].as<u64>();
    }
  }

  void Graph::compile() const
  {
    Low::Util::StringBuilder l_Builder;

    l_Builder.append("#pragma once").endl().endl();
    l_Builder.append("#include \"LowUtil.h\"").endl();
    l_Builder.append("#include \"LowUtilString.h\"").endl();
    l_Builder.append("#include \"LowUtilContainers.h\"").endl();
    l_Builder.append("#include \"LowUtilLogger.h\"").endl();
    l_Builder.endl();
    l_Builder.append("namespace MtdScripts {").endl();
    l_Builder.append("namespace Test {").endl();

    for (Node *i_Node : m_Nodes) {
      if (i_Node->typeName == N(FlodeSyntaxFunction)) {
        i_Node->compile(l_Builder);
      }
    }

    l_Builder.append("}").endl();
    l_Builder.append("}").endl();

    Low::Util::String l_Path = LOW_DATA_PATH;
    l_Path += "/scripts/" + LOW_TO_STRING(25) + "script.cpp";

    Low::Util::FileIO::File l_File = Low::Util::FileIO::open(
        l_Path.c_str(), Low::Util::FileIO::FileMode::WRITE);

    Low::Util::FileIO::write_sync(l_File, l_Builder.get().c_str());

    Low::Util::FileIO::close(l_File);

    LOW_LOG_INFO << "Compiled flode graph '"
                 << "test"
                 << "' to file." << LOW_LOG_END;
  }
} // namespace Flode
