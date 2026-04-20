#include "LowEditorVisualScriptingBoolNodes.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace BoolNodes {
        namespace {
          static const ImU32 g_BoolColor =
              IM_COL32(220, 48, 48, 255);

          struct NotNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_bool_not);
            }

            Util::String get_title(const Graph &, NodeId) const override
            {
              return "NOT";
            }

            Util::String get_category(const Graph &, NodeId) const override
            {
              return "Bool";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_BoolColor;
            }

            void setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                                    const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_bool_pin_metadata("A"), p_Schema);
              Editor::Pin l_Output = make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_bool_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(Graph &p_Graph, NodeId p_NodeId,
                                    PinId, CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(!");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "A")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct OrNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_bool_or);
            }

            Util::String get_title(const Graph &, NodeId) const override
            {
              return "OR";
            }

            Util::String get_category(const Graph &, NodeId) const override
            {
              return "Bool";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_BoolColor;
            }

            void setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                                    const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_bool_pin_metadata("A"), p_Schema);
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_bool_pin_metadata("B"), p_Schema);
              Editor::Pin l_Output = make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_bool_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(Graph &p_Graph, NodeId p_NodeId,
                                    PinId, CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "A")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(" || ");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "B")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct AndNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_bool_and);
            }

            Util::String get_title(const Graph &, NodeId) const override
            {
              return "AND";
            }

            Util::String get_category(const Graph &, NodeId) const override
            {
              return "Bool";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_BoolColor;
            }

            void setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                                    const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_bool_pin_metadata("A"), p_Schema);
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_bool_pin_metadata("B"), p_Schema);
              Editor::Pin l_Output = make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_bool_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(Graph &p_Graph, NodeId p_NodeId,
                                    PinId, CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "A")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(" && ");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "B")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct EqualsNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_bool_equals);
            }

            Util::String get_title(const Graph &, NodeId) const override
            {
              return "Equals";
            }

            Util::String get_category(const Graph &, NodeId) const override
            {
              return "Bool";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_BoolColor;
            }

            void setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                                    const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_dynamic_pin_metadata("A"), p_Schema);
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_dynamic_pin_metadata("B"), p_Schema);
              Editor::Pin l_Output = make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_bool_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(Graph &p_Graph, NodeId p_NodeId,
                                    PinId, CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "A")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(" == ");
              compile_input_pin(p_Graph, p_NodeId,
                                p_Graph.find_input_pin_checked(p_NodeId, "B")->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          static NotNodeClass g_NotNodeClass;
          static OrNodeClass g_OrNodeClass;
          static AndNodeClass g_AndNodeClass;
          static EqualsNodeClass g_EqualsNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_NotNodeClass);
          p_Graph.register_node_class(g_OrNodeClass);
          p_Graph.register_node_class(g_AndNodeClass);
          p_Graph.register_node_class(g_EqualsNodeClass);

          NodeSpawnEntry l_NotEntry;
          l_NotEntry.id = N(vs_spawn_bool_not);
          l_NotEntry.category = "Bool";
          l_NotEntry.title = "Not";
          l_NotEntry.search_text = "bool not";
          l_NotEntry.node_class = g_NotNodeClass.get_name();
          p_Graph.register_spawn_entry(l_NotEntry);

          NodeSpawnEntry l_OrEntry;
          l_OrEntry.id = N(vs_spawn_bool_or);
          l_OrEntry.category = "Bool";
          l_OrEntry.title = "Or";
          l_OrEntry.search_text = "bool or";
          l_OrEntry.node_class = g_OrNodeClass.get_name();
          p_Graph.register_spawn_entry(l_OrEntry);

          NodeSpawnEntry l_AndEntry;
          l_AndEntry.id = N(vs_spawn_bool_and);
          l_AndEntry.category = "Bool";
          l_AndEntry.title = "And";
          l_AndEntry.search_text = "bool and";
          l_AndEntry.node_class = g_AndNodeClass.get_name();
          p_Graph.register_spawn_entry(l_AndEntry);

          NodeSpawnEntry l_EqualsEntry;
          l_EqualsEntry.id = N(vs_spawn_bool_equals);
          l_EqualsEntry.category = "Bool";
          l_EqualsEntry.title = "Equals";
          l_EqualsEntry.search_text = "bool equals compare";
          l_EqualsEntry.node_class = g_EqualsNodeClass.get_name();
          p_Graph.register_spawn_entry(l_EqualsEntry);
        }
      } // namespace BoolNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
