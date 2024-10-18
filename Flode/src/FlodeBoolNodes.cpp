#include "FlodeBoolNodes.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace BoolNodes {

    ImU32 g_BoolColor = IM_COL32(220, 48, 48, 255);

    Low::Util::String NotNode::get_name(NodeNameType p_Type) const
    {
      return "NOT";
    }

    ImU32 NotNode::get_color() const
    {
      return g_BoolColor;
    }

    void NotNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void
    NotNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(!");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(")");
    }

    Low::Util::String OrNode::get_name(NodeNameType p_Type) const
    {
      return "OR";
    }

    ImU32 OrNode::get_color() const
    {
      return g_BoolColor;
    }

    void OrNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void
    OrNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                               NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" || ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    Low::Util::String AndNode::get_name(NodeNameType p_Type) const
    {
      return "AND";
    }

    ImU32 AndNode::get_color() const
    {
      return g_BoolColor;
    }

    void AndNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Input, "", PinType::Bool);
      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void
    AndNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" && ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    Low::Util::String EqualsNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return ICON_FA_EQUALS ICON_FA_EQUALS "";
      }

      return "Equals";
    }

    ImU32 EqualsNode::get_color() const
    {
      return g_BoolColor;
    }

    void EqualsNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType, m_PinTypeId);
      create_pin(PinDirection::Input, "", m_PinType, m_PinTypeId);

      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void EqualsNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" == ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void EqualsNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
      p_Node["pintypeid"] = m_PinTypeId;
    }

    void EqualsNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
      if (p_Node["pintypeid"]) {
        m_PinTypeId = p_Node["pintypeid"].as<u16>();
      }
    }

    void EqualsNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        m_PinType = l_ConnectedPin->type;
        m_PinTypeId = l_ConnectedPin->typeId;

        pins[0]->type = m_PinType;
        pins[0]->typeId = m_PinTypeId;
        pins[1]->type = m_PinType;
        pins[1]->typeId = m_PinTypeId;
      }
    }

    bool EqualsNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::String ||
             p_ConnectedPin->type == PinType::Number ||
             p_ConnectedPin->type == PinType::Vector2 ||
             p_ConnectedPin->type == PinType::Vector3 ||
             p_ConnectedPin->type == PinType::Handle ||
             p_ConnectedPin->type == PinType::Enum;
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeBoolNot);
        register_node(l_TypeName, []() { return new NotNode; });
        register_spawn_node("Bool", "Not", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeBoolOr);
        register_node(l_TypeName, []() { return new OrNode; });
        register_spawn_node("Bool", "Or", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeBoolAnd);
        register_node(l_TypeName, []() { return new AndNode; });
        register_spawn_node("Bool", "And", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeBoolEquals);
        register_node(l_TypeName, []() { return new EqualsNode; });
        register_spawn_node("Bool", "Equals", l_TypeName);
      }
    }

  } // namespace BoolNodes
} // namespace Flode
