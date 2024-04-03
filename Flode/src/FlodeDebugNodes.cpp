#include "FlodeDebugNodes.h"

namespace Flode {
  namespace DebugNodes {

    ImU32 g_DebugColor = IM_COL32(102, 113, 115, 255);

    Low::Util::String LogNode::get_name(NodeNameType p_Type) const
    {
      return "Log";
    }

    ImU32 LogNode::get_color() const
    {
      return g_DebugColor;
    }

    void LogNode::setup_default_pins()
    {
      create_pin(PinDirection::Input, "", PinType::Flow);
      create_pin(PinDirection::Output, "", PinType::Flow);

      m_MessagePin =
          create_pin(PinDirection::Input, "Message", PinType::String);
    }

    void LogNode::compile(Low::Util::StringBuilder &p_Builder) const
    {
      p_Builder.append("LOW_LOG_DEBUG << ");
      compile_input_pin(p_Builder, m_MessagePin->id);
      p_Builder.append(" << LOW_LOG_END;").endl();
    }

    Node *log_create_instance()
    {
      return new LogNode;
    }

    void register_nodes()
    {
      register_node_type("Debug", "Log", &log_create_instance,
                         N(FlodeDebugLog));
    }

  } // namespace DebugNodes
} // namespace Flode
