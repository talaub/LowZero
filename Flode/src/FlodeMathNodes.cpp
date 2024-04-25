#include "FlodeMathNodes.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace MathNodes {

    ImU32 g_MathColor = IM_COL32(65, 145, 146, 255);

    Low::Util::String
    AddNumberNode::get_name(NodeNameType p_Type) const
    {
      if (p_Type == NodeNameType::Compact) {
        return ICON_FA_PLUS "";
      }

      return "Add";
    }

    ImU32 AddNumberNode::get_color() const
    {
      return g_MathColor;
    }

    void AddNumberNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Number);
      create_pin(PinDirection::Input, "", PinType::Number);

      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void AddNumberNode::compile_output_pin(
        Low::Util::StringBuilder &p_Builder,
        NodeEd::PinId p_PinId) const
    {
      Pin *l_Pin = find_output_pin_checked(p_PinId);

      p_Builder.append("(");
      compile_input_pin(p_Builder, pins[0]->id);
      p_Builder.append(" + ");
      compile_input_pin(p_Builder, pins[1]->id);
      p_Builder.append(")");
    }

    Node *addnumber_create_instance()
    {
      return new AddNumberNode;
    }

    Node *multiplynumber_create_instance()
    {
      return new MultiplyNumberNode;
    }

    Node *subtractnumber_create_instance()
    {
      return new SubtractNumberNode;
    }

    Node *dividenumber_create_instance()
    {
      return new DivideNumberNode;
    }

    ImU32 SubtractNumberNode::get_color() const
    {
      return g_MathColor;
    }

    void SubtractNumberNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Number);
      create_pin(PinDirection::Input, "", PinType::Number);

      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void SubtractNumberNode::compile_output_pin(
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

    ImU32 MultiplyNumberNode::get_color() const
    {
      return g_MathColor;
    }

    void MultiplyNumberNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Number);
      create_pin(PinDirection::Input, "", PinType::Number);

      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void MultiplyNumberNode::compile_output_pin(
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

    ImU32 DivideNumberNode::get_color() const
    {
      return g_MathColor;
    }

    void DivideNumberNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Number);
      create_pin(PinDirection::Input, "", PinType::Number);

      create_pin(PinDirection::Output, "", PinType::Number);
    }

    void DivideNumberNode::compile_output_pin(
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

    void register_nodes()
    {
      {
        Low::Util::Name l_TypeName = N(FlodeMathAddNumbers);
        register_node(l_TypeName, &addnumber_create_instance);
        register_spawn_node("Math", "Add", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathSubtractNumbers);
        register_node(l_TypeName, &subtractnumber_create_instance);
        register_spawn_node("Math", "Subtract", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathMultiplyNumbers);
        register_node(l_TypeName, &multiplynumber_create_instance);
        register_spawn_node("Math", "Multiply", l_TypeName);
      }
      {
        Low::Util::Name l_TypeName = N(FlodeMathDivideNumbers);
        register_node(l_TypeName, &dividenumber_create_instance);
        register_spawn_node("Math", "Divide", l_TypeName);
      }
    }
  } // namespace MathNodes
} // namespace Flode
