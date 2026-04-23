#include "LowEditorVisualScriptBuilder.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      bool NodeHandle::is_valid() const
      {
        return graph && id.is_valid() && graph->find_node(id);
      }

      Node *NodeHandle::find_node()
      {
        return graph ? graph->find_node(id) : nullptr;
      }

      const Node *NodeHandle::find_node() const
      {
        return graph ? graph->find_node(id) : nullptr;
      }

      Pin *NodeHandle::find_input_pin(const Util::String &p_DisplayName)
      {
        return graph ? graph->find_input_pin(id, p_DisplayName)
                     : nullptr;
      }

      const Pin *NodeHandle::find_input_pin(
          const Util::String &p_DisplayName) const
      {
        return graph ? graph->find_input_pin(id, p_DisplayName)
                     : nullptr;
      }

      Pin *
      NodeHandle::find_output_pin(const Util::String &p_DisplayName)
      {
        return graph ? graph->find_output_pin(id, p_DisplayName)
                     : nullptr;
      }

      const Pin *NodeHandle::find_output_pin(
          const Util::String &p_DisplayName) const
      {
        return graph ? graph->find_output_pin(id, p_DisplayName)
                     : nullptr;
      }

      GraphBuilder::GraphBuilder(Graph &p_Graph,
                                 const NodeGraphSchema *p_Schema)
          : graph(&p_Graph), schema(p_Schema)
      {
      }

      GraphBuilder &GraphBuilder::set_graph(Graph &p_Graph)
      {
        graph = &p_Graph;
        return *this;
      }

      GraphBuilder &
      GraphBuilder::set_schema(const NodeGraphSchema *p_Schema)
      {
        schema = p_Schema;
        return *this;
      }

      bool GraphBuilder::add_variable(const Variable &p_Variable)
      {
        return graph && graph->add_variable(p_Variable);
      }

      bool GraphBuilder::add_bool_variable(const Util::String &p_Name,
                                           bool p_DefaultValue)
      {
        Variable l_Variable;
        l_Variable.name = p_Name;
        l_Variable.type = PinType::Bool;
        l_Variable.default_value = Util::Variant(p_DefaultValue);
        return add_variable(l_Variable);
      }

      bool GraphBuilder::add_number_variable(
          const Util::String &p_Name, float p_DefaultValue,
          NumberSubtype p_NumberSubtype)
      {
        Variable l_Variable;
        l_Variable.name = p_Name;
        l_Variable.type = PinType::Number;
        l_Variable.number_subtype = p_NumberSubtype;

        switch (p_NumberSubtype) {
        case NumberSubtype::Int32:
          l_Variable.default_value =
              Util::Variant((i32)p_DefaultValue);
          break;
        case NumberSubtype::UInt32:
          l_Variable.default_value =
              Util::Variant((u32)LOW_MATH_MAX(0.0f, p_DefaultValue));
          break;
        case NumberSubtype::UInt64:
          l_Variable.default_value = Util::Variant(
              (u64)LOW_MATH_MAX(0.0f, p_DefaultValue));
          break;
        case NumberSubtype::Float:
        default:
          l_Variable.default_value = Util::Variant(p_DefaultValue);
          break;
        }

        return add_variable(l_Variable);
      }

      bool GraphBuilder::add_string_variable(
          const Util::String &p_Name,
          const Util::String &p_DefaultValue,
          StringSubtype p_StringSubtype)
      {
        Variable l_Variable;
        l_Variable.name = p_Name;
        l_Variable.type = PinType::String;
        l_Variable.string_subtype = p_StringSubtype;
        if (p_StringSubtype == StringSubtype::Name) {
          l_Variable.default_value =
              Util::Variant(LOW_NAME(p_DefaultValue.c_str()));
        } else {
          l_Variable.default_value = Util::Variant(p_DefaultValue);
        }
        return add_variable(l_Variable);
      }

      bool GraphBuilder::add_handle_variable(
          const Util::String &p_Name,
          Util::TypeIdentifier p_HandleType)
      {
        Variable l_Variable;
        l_Variable.name = p_Name;
        l_Variable.type = PinType::Handle;
        l_Variable.handle_type = p_HandleType;
        l_Variable.default_value =
            Util::Variant::from_handle(Util::Handle());
        return add_variable(l_Variable);
      }

      NodeHandle GraphBuilder::add_node(
          Util::Name p_NodeClass, const Math::Vector2 &p_Position)
      {
        NodeHandle l_Handle;
        l_Handle.graph = graph;

        if (!graph) {
          return l_Handle;
        }

        NodeGraphMutationResult<Editor::Node> l_Result =
            graph->create_node(p_NodeClass, p_Position, schema);
        if (l_Result.succeeded()) {
          l_Handle.id = l_Result.value->id;
        }

        return l_Handle;
      }

      NodeHandle GraphBuilder::add_spawn_node(
          Util::Name p_SpawnEntryId, const Math::Vector2 &p_Position)
      {
        NodeHandle l_Handle;
        l_Handle.graph = graph;

        if (!graph) {
          return l_Handle;
        }

        NodeGraphMutationResult<Editor::Node> l_Result =
            graph->create_node_from_spawn_entry(p_SpawnEntryId,
                                                p_Position, schema);
        if (l_Result.succeeded()) {
          l_Handle.id = l_Result.value->id;
        }

        return l_Handle;
      }

      bool GraphBuilder::connect(PinId p_StartPinId, PinId p_EndPinId)
      {
        if (!graph || !p_StartPinId.is_valid() || !p_EndPinId.is_valid()) {
          return false;
        }

        Editor::Link l_Link;
        l_Link.id = allocate_link_id();
        l_Link.start_pin = p_StartPinId;
        l_Link.end_pin = p_EndPinId;

        return graph->add_link(l_Link, schema).succeeded();
      }

      bool GraphBuilder::connect(const NodeHandle &p_StartNode,
                                 const Util::String &p_StartPinName,
                                 const NodeHandle &p_EndNode,
                                 const Util::String &p_EndPinName)
      {
        if (!p_StartNode.is_valid() || !p_EndNode.is_valid()) {
          return false;
        }

        const Pin *l_StartPin =
            p_StartNode.find_output_pin(p_StartPinName);
        const Pin *l_EndPin = p_EndNode.find_input_pin(p_EndPinName);

        if (!l_StartPin || !l_EndPin) {
          return false;
        }

        return connect(l_StartPin->pin, l_EndPin->pin);
      }

      bool GraphBuilder::connect_exec(
          const NodeHandle &p_StartNode, const NodeHandle &p_EndNode,
          const Util::String &p_StartPinName,
          const Util::String &p_EndPinName)
      {
        return connect(p_StartNode, p_StartPinName, p_EndNode,
                       p_EndPinName);
      }

      bool GraphBuilder::set_input_default(
          const NodeHandle &p_NodeHandle, const Util::String &p_PinName,
          const Util::Variant &p_Value)
      {
        if (!p_NodeHandle.graph || !p_NodeHandle.id.is_valid()) {
          return false;
        }

        NodeHandle l_Node = p_NodeHandle;
        Pin *l_Pin = l_Node.find_input_pin(p_PinName);
        if (!l_Pin) {
          return false;
        }

        l_Pin->default_value = p_Value;
        return true;
      }

      LinkId GraphBuilder::allocate_link_id()
      {
        return graph ? LinkId{graph->id_counter++} : LinkId{};
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
