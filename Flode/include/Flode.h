#pragma once

#include "FlodeApi.h"

#include <imgui_node_editor.h>

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"

namespace NodeEd = ax::NodeEditor;

namespace Flode {
  enum class NodeNameType
  {
    Full
  };

  enum class PinType
  {
    Flow,
    Bool,
    Int,
    Float,
    String,
    Object,
    Function,
    Delegate
  };

  enum class PinDirection
  {
    Input,
    Output
  };

  struct FLODE_API Pin
  {
    NodeEd::PinId id;
    Low::Util::String title;
    PinType type;
    PinDirection direction;
    NodeEd::NodeId nodeId;

    Low::Util::Variant defaultValue;
  };

  struct FLODE_API Node
  {
    NodeEd::NodeId id;
    Low::Util::List<Pin> pins;

    NodeEd::PinId create_pin(PinDirection p_Directions,
                             Low::Util::String p_Title,
                             PinType p_Type);

    virtual Low::Util::String get_name(NodeNameType p_Type) const
    {
      return "Node";
    }
    virtual ImU32 get_color() const
    {
      return IM_COL32_BLACK;
    }
    virtual void setup_default_pins()
    {
    }
  };

  struct FLODE_API Link
  {
    NodeEd::LinkId id;
    NodeEd::PinId inputPinId;
    NodeEd::PinId outputPinId;
  };

  struct FLODE_API Graph
  {
    Low::Util::List<Node> m_Nodes;
    Low::Util::List<Link> m_Links;
  };

  typedef Node (*create_node_callback)();
} // namespace Flode
