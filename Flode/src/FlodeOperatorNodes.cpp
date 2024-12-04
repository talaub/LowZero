#include "FlodeOperatorNodes.h"

#include "Flode.h"

#include "IconsFontAwesome5.h"
#include "IconsCodicons.h"

namespace Flode {
  namespace OperatorNodes {
    ImU32 g_OperatorColor = IM_COL32(65, 145, 146, 255);

    Low::Util::String
    GreaterEqualsNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return ">=";
      }

      return "Greater equals";
    }

    ImU32 GreaterEqualsNode::get_color() const
    {
      return g_OperatorColor;
    }

    void GreaterEqualsNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void GreaterEqualsNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" >= ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void
    GreaterEqualsNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void GreaterEqualsNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void GreaterEqualsNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
        }
      }
    }

    bool GreaterEqualsNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    Node *greateequals_create_instance()
    {
      return new GreaterEqualsNode;
    }

    Low::Util::String
    LessEqualsNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return "<=";
      }

      return "Less equals";
    }

    ImU32 LessEqualsNode::get_color() const
    {
      return g_OperatorColor;
    }

    void LessEqualsNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void LessEqualsNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" <= ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void
    LessEqualsNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void LessEqualsNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void LessEqualsNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
        }
      }
    }

    bool LessEqualsNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    Node *lessequals_create_instance()
    {
      return new LessEqualsNode;
    }

    Low::Util::String LessNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return "<";
      }

      return "Less";
    }

    ImU32 LessNode::get_color() const
    {
      return g_OperatorColor;
    }

    void LessNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void
    LessNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                 NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" < ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void LessNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void LessNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void LessNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
        }
      }
    }

    bool
    LessNode::accept_dynamic_pin_connection(Pin *p_Pin,
                                            Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    Node *less_create_instance()
    {
      return new LessNode;
    }

    Low::Util::String GreaterNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return ">";
      }

      return "Greater";
    }

    ImU32 GreaterNode::get_color() const
    {
      return g_OperatorColor;
    }

    void GreaterNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", PinType::Bool);
    }

    void GreaterNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" > ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void GreaterNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void GreaterNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void GreaterNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
        }
      }
    }

    bool GreaterNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    Node *greater_create_instance()
    {
      return new LessNode;
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeOperatorGreaterEquals);
        register_node(l_TypeName, &greateequals_create_instance);
        register_spawn_node("Math", "Greater equals", l_TypeName);
        register_spawn_node("Bool", "Greater equals", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeOperatorLessEquals);
        register_node(l_TypeName, &lessequals_create_instance);
        register_spawn_node("Math", "Less equals", l_TypeName);
        register_spawn_node("Bool", "Less equals", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeOperatorLess);
        register_node(l_TypeName, &less_create_instance);
        register_spawn_node("Math", "Less", l_TypeName);
        register_spawn_node("Bool", "Less", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeOperatorGreater);
        register_node(l_TypeName, &greater_create_instance);
        register_spawn_node("Math", "Greater", l_TypeName);
        register_spawn_node("Bool", "Greater", l_TypeName);
      }
    }
  } // namespace OperatorNodes
} // namespace Flode
