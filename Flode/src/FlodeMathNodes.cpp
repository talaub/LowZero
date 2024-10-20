#include "FlodeMathNodes.h"

#include "Flode.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace MathNodes {

    ImU32 g_MathColor = IM_COL32(65, 145, 146, 255);

    Low::Util::String AddNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return ICON_FA_PLUS "";
      }

      return "Add";
    }

    ImU32 AddNode::get_color() const
    {
      return g_MathColor;
    }

    void AddNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", m_PinType);
    }

    void
    AddNode::compile_output_pin(Low::Util::StringBuilder &p_Builder,
                                NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" + ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void AddNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void AddNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void AddNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;
          pins[2]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
          setup_default_value_for_pin(pins[2]);
        }
      }
    }

    bool
    AddNode::accept_dynamic_pin_connection(Pin *p_Pin,
                                           Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::String ||
             p_ConnectedPin->type == PinType::Number ||
             p_ConnectedPin->type == PinType::Vector2 ||
             p_ConnectedPin->type == PinType::Vector3;
    }

    Node *add_create_instance()
    {
      return new AddNode;
    }

    Node *multiply_create_instance()
    {
      return new MultiplyNode;
    }

    Node *subtract_create_instance()
    {
      return new SubtractNode;
    }

    Node *divide_create_instance()
    {
      return new DivideNode;
    }

    ImU32 SubtractNode::get_color() const
    {
      return g_MathColor;
    }

    void SubtractNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", m_PinType);
    }

    void SubtractNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" - ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void SubtractNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void SubtractNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void SubtractNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;
          pins[2]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
          setup_default_value_for_pin(pins[2]);
        }
      }
    }

    bool SubtractNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::String ||
             p_ConnectedPin->type == PinType::Number ||
             p_ConnectedPin->type == PinType::Vector2 ||
             p_ConnectedPin->type == PinType::Vector3;
    }

    ImU32 MultiplyNode::get_color() const
    {
      return g_MathColor;
    }

    void MultiplyNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", m_PinType);
    }

    void MultiplyNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" * ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void MultiplyNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void MultiplyNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void MultiplyNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;
          pins[2]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
          setup_default_value_for_pin(pins[2]);
        }
      }
    }

    bool MultiplyNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    ImU32 DivideNode::get_color() const
    {
      return g_MathColor;
    }

    void DivideNode::setup_default_pins()
    {
      LOW_LOG_DEBUG << (int)m_PinType << LOW_LOG_END;
      create_pin(PinDirection::Input, "", m_PinType);
      create_pin(PinDirection::Input, "", m_PinType);

      create_pin(PinDirection::Output, "", m_PinType);
    }

    void DivideNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" / ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    void DivideNode::serialize(Low::Util::Yaml::Node &p_Node) const
    {
      p_Node["pintype"] =
          Flode::pin_type_to_string(m_PinType).c_str();
    }

    void DivideNode::deserialize(Low::Util::Yaml::Node &p_Node)
    {
      if (p_Node["pintype"]) {
        m_PinType = Flode::string_to_pin_type(
            LOW_YAML_AS_STRING(p_Node["pintype"]));
      }
    }

    void DivideNode::on_pin_connected(Pin *p_Pin)
    {
      if (p_Pin->direction == PinDirection::Input) {
        Pin *l_ConnectedPin =
            graph->find_pin(graph->get_connected_pin(p_Pin->id));

        if (m_PinType != l_ConnectedPin->type) {
          m_PinType = l_ConnectedPin->type;

          pins[0]->type = m_PinType;
          pins[1]->type = m_PinType;
          pins[2]->type = m_PinType;

          setup_default_value_for_pin(pins[0]);
          setup_default_value_for_pin(pins[1]);
          setup_default_value_for_pin(pins[2]);
        }
      }
    }

    bool DivideNode::accept_dynamic_pin_connection(
        Pin *p_Pin, Pin *p_ConnectedPin) const
    {
      return p_ConnectedPin->type == PinType::Number;
    }

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeMathAdd);
        register_node(l_TypeName, &add_create_instance);
        register_spawn_node("Math", "Add", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathSubtract);
        register_node(l_TypeName, &subtract_create_instance);
        register_spawn_node("Math", "Subtract", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathMultiply);
        register_node(l_TypeName, &multiply_create_instance);
        register_spawn_node("Math", "Multiply", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathDivide);
        register_node(l_TypeName, &divide_create_instance);
        register_spawn_node("Math", "Divide", l_TypeName);
      }
    }
  } // namespace MathNodes
} // namespace Flode
