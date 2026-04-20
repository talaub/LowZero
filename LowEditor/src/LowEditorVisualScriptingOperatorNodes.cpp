#include "LowEditorVisualScriptingOperatorNodes.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace OperatorNodes {
        namespace {
          static const ImU32 g_OperatorColor =
              IM_COL32(65, 145, 146, 255);

          struct ComparisonNodeClassBase : public NodeClass
          {
            Util::String m_Title;
            const char *m_Operator;

            ComparisonNodeClassBase(Util::String p_Title,
                                    const char *p_Operator)
                : m_Title(p_Title), m_Operator(p_Operator)
            {
            }

            Util::String get_title(const Graph &,
                                   NodeId) const override
            {
              return m_Title;
            }

            Util::String get_category(const Graph &,
                                      NodeId) const override
            {
              return "Bool";
            }

            bool is_compact(const Graph &, NodeId) const override
            {
              return true;
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_OperatorColor;
            }

            void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_number_pin_metadata("A"),
                              p_Schema);
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_number_pin_metadata("B"),
                              p_Schema);
              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_bool_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId,
                CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(");
              compile_input_pin(
                  p_Graph, p_NodeId,
                  p_Graph.find_input_pin_checked(p_NodeId, "A")->pin,
                  p_CompileContext);
              p_CompileContext.main_code.append(m_Operator);
              compile_input_pin(
                  p_Graph, p_NodeId,
                  p_Graph.find_input_pin_checked(p_NodeId, "B")->pin,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct GreaterEqualsNodeClass
              : public ComparisonNodeClassBase
          {
            GreaterEqualsNodeClass()
                : ComparisonNodeClassBase("Greater equals", " >= ")
            {
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ">=";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_greater_equals);
            }
          };

          struct LessEqualsNodeClass : public ComparisonNodeClassBase
          {
            LessEqualsNodeClass()
                : ComparisonNodeClassBase("Less equals", " <= ")
            {
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return "<=";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_less_equals);
            }
          };

          struct LessNodeClass : public ComparisonNodeClassBase
          {
            LessNodeClass() : ComparisonNodeClassBase("Less", " < ")
            {
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return "<";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_less);
            }
          };

          struct GreaterNodeClass : public ComparisonNodeClassBase
          {
            GreaterNodeClass()
                : ComparisonNodeClassBase("Greater", " > ")
            {
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ">";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_greater);
            }
          };

          static GreaterEqualsNodeClass g_GreaterEqualsNodeClass;
          static LessEqualsNodeClass g_LessEqualsNodeClass;
          static LessNodeClass g_LessNodeClass;
          static GreaterNodeClass g_GreaterNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_GreaterEqualsNodeClass);
          p_Graph.register_node_class(g_LessEqualsNodeClass);
          p_Graph.register_node_class(g_LessNodeClass);
          p_Graph.register_node_class(g_GreaterNodeClass);

          NodeSpawnEntry l_GreaterEqualsEntry;
          l_GreaterEqualsEntry.id =
              N(vs_spawn_operator_greater_equals);
          l_GreaterEqualsEntry.category = "Bool";
          l_GreaterEqualsEntry.title = "Greater equals";
          l_GreaterEqualsEntry.search_text = "greater equals compare";
          l_GreaterEqualsEntry.node_class =
              g_GreaterEqualsNodeClass.get_name();
          p_Graph.register_spawn_entry(l_GreaterEqualsEntry);

          NodeSpawnEntry l_LessEqualsEntry;
          l_LessEqualsEntry.id = N(vs_spawn_operator_less_equals);
          l_LessEqualsEntry.category = "Bool";
          l_LessEqualsEntry.title = "Less equals";
          l_LessEqualsEntry.search_text = "less equals compare";
          l_LessEqualsEntry.node_class =
              g_LessEqualsNodeClass.get_name();
          p_Graph.register_spawn_entry(l_LessEqualsEntry);

          NodeSpawnEntry l_LessEntry;
          l_LessEntry.id = N(vs_spawn_operator_less);
          l_LessEntry.category = "Bool";
          l_LessEntry.title = "Less";
          l_LessEntry.search_text = "less compare";
          l_LessEntry.node_class = g_LessNodeClass.get_name();
          p_Graph.register_spawn_entry(l_LessEntry);

          NodeSpawnEntry l_GreaterEntry;
          l_GreaterEntry.id = N(vs_spawn_operator_greater);
          l_GreaterEntry.category = "Bool";
          l_GreaterEntry.title = "Greater";
          l_GreaterEntry.search_text = "greater compare";
          l_GreaterEntry.node_class = g_GreaterNodeClass.get_name();
          p_Graph.register_spawn_entry(l_GreaterEntry);
        }
      } // namespace OperatorNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
