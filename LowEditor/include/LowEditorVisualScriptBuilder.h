#pragma once

#include "LowEditorApi.h"
#include "LowEditorVisualScripting.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      struct LOW_EDITOR_API NodeHandle
      {
        Graph *graph = nullptr;
        NodeId id;

        bool is_valid() const;

        Node *find_node();
        const Node *find_node() const;

        Pin *find_input_pin(const Util::String &p_DisplayName);
        const Pin *
        find_input_pin(const Util::String &p_DisplayName) const;

        Pin *find_output_pin(const Util::String &p_DisplayName);
        const Pin *
        find_output_pin(const Util::String &p_DisplayName) const;
      };

      struct LOW_EDITOR_API GraphBuilder
      {
        Graph *graph = nullptr;
        const NodeGraphSchema *schema = nullptr;

        GraphBuilder() = default;
        GraphBuilder(Graph &p_Graph,
                     const NodeGraphSchema *p_Schema = nullptr);

        GraphBuilder &set_graph(Graph &p_Graph);
        GraphBuilder &set_schema(const NodeGraphSchema *p_Schema);

        bool add_variable(const Variable &p_Variable);
        bool add_bool_variable(const Util::String &p_Name,
                               bool p_DefaultValue = false);
        bool add_number_variable(
            const Util::String &p_Name, float p_DefaultValue = 0.0f,
            NumberSubtype p_NumberSubtype = NumberSubtype::Float);
        bool add_string_variable(
            const Util::String &p_Name,
            const Util::String &p_DefaultValue = "",
            StringSubtype p_StringSubtype = StringSubtype::String);
        bool add_handle_variable(
            const Util::String &p_Name,
            Util::TypeIdentifier p_HandleType =
                Util::TypeIdentifier());

        NodeHandle add_node(Util::Name p_NodeClass,
                            const Math::Vector2 &p_Position);
        NodeHandle add_spawn_node(Util::Name p_SpawnEntryId,
                                  const Math::Vector2 &p_Position);

        bool connect(PinId p_StartPinId, PinId p_EndPinId);
        bool connect(const NodeHandle &p_StartNode,
                     const Util::String &p_StartPinName,
                     const NodeHandle &p_EndNode,
                     const Util::String &p_EndPinName);
        bool connect_exec(
            const NodeHandle &p_StartNode, const NodeHandle &p_EndNode,
            const Util::String &p_StartPinName = "Exec",
            const Util::String &p_EndPinName = "Exec");

        bool set_input_default(const NodeHandle &p_Node,
                               const Util::String &p_PinName,
                               const Util::Variant &p_Value);

      private:
        LinkId allocate_link_id();
      };
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
