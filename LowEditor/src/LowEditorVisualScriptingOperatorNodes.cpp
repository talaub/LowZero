#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace OperatorNodes {
        namespace {
          static const ImU32 g_OperatorColor =
              IM_COL32(65, 145, 146, 255);

          struct ComparisonNodeBase : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_CI_SYMBOL_OPERATOR;
            }

            const char *const m_Operator;

            ComparisonNodeBase(const char *p_Operator)
                : m_Operator(p_Operator)
            {
            }

            Util::String get_title(const Graph &) const override = 0;

            Util::String get_category(const Graph &) const override
            {
              return "Bool";
            }

            bool is_compact(const Graph &) const override
            {
              return true;
            }

            ImU32 get_color(const Graph &) const override
            {
              return g_OperatorColor;
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_number_pin("A")),
                  p_Schema);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_number_pin("B")),
                  p_Schema);

              Pin l_OutputMetadata = make_bool_pin("Result");
              l_OutputMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_OutputMetadata),
                  p_Schema);
            }

            void compile_output_pin(
                Graph &p_Graph, PinId,
                CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append("(");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "A")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(m_Operator);
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "B")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct GreaterEqualsNode : public ComparisonNodeBase
          {
            Util::String get_title(const Graph &) const override
            {
              return "Greater equals";
            }

            GreaterEqualsNode() : ComparisonNodeBase(" >= ")
            {
            }

            Util::String
            get_compact_title(const Graph &p_Graph) const override
            {
              return ">=";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_greater_equals);
            }
          };

          struct LessEqualsNode : public ComparisonNodeBase
          {
            Util::String get_title(const Graph &) const override
            {
              return "Less equals";
            }

            LessEqualsNode() : ComparisonNodeBase(" <= ")
            {
            }

            Util::String
            get_compact_title(const Graph &p_Graph) const override
            {
              return "<=";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_less_equals);
            }
          };

          struct LessNode : public ComparisonNodeBase
          {
            Util::String get_title(const Graph &) const override
            {
              return "Less";
            }

            LessNode() : ComparisonNodeBase(" < ")
            {
            }

            Util::String
            get_compact_title(const Graph &p_Graph) const override
            {
              return "<";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_less);
            }
          };

          struct GreaterNode : public ComparisonNodeBase
          {
            Util::String get_title(const Graph &) const override
            {
              return "Greater";
            }

            GreaterNode() : ComparisonNodeBase(" > ")
            {
            }

            Util::String
            get_compact_title(const Graph &p_Graph) const override
            {
              return ">";
            }

            Util::Name get_name() const override
            {
              return N(vs_operator_greater);
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_GreaterEqualsNodeType =
              p_Graph.register_node_type<GreaterEqualsNode>();
          const Util::Name l_LessEqualsNodeType =
              p_Graph.register_node_type<LessEqualsNode>();
          const Util::Name l_LessNodeType =
              p_Graph.register_node_type<LessNode>();
          const Util::Name l_GreaterNodeType =
              p_Graph.register_node_type<GreaterNode>();

          NodeSpawnEntry l_GreaterEqualsEntry;
          l_GreaterEqualsEntry.id =
              N(vs_spawn_operator_greater_equals);
          l_GreaterEqualsEntry.category = "Bool";
          l_GreaterEqualsEntry.title = "Greater equals";
          l_GreaterEqualsEntry.search_text = "greater equals compare";
          l_GreaterEqualsEntry.node_type = l_GreaterEqualsNodeType;
          p_Graph.register_spawn_entry(l_GreaterEqualsEntry);

          NodeSpawnEntry l_LessEqualsEntry;
          l_LessEqualsEntry.id = N(vs_spawn_operator_less_equals);
          l_LessEqualsEntry.category = "Bool";
          l_LessEqualsEntry.title = "Less equals";
          l_LessEqualsEntry.search_text = "less equals compare";
          l_LessEqualsEntry.node_type = l_LessEqualsNodeType;
          p_Graph.register_spawn_entry(l_LessEqualsEntry);

          NodeSpawnEntry l_LessEntry;
          l_LessEntry.id = N(vs_spawn_operator_less);
          l_LessEntry.category = "Bool";
          l_LessEntry.title = "Less";
          l_LessEntry.search_text = "less compare";
          l_LessEntry.node_type = l_LessNodeType;
          p_Graph.register_spawn_entry(l_LessEntry);

          NodeSpawnEntry l_GreaterEntry;
          l_GreaterEntry.id = N(vs_spawn_operator_greater);
          l_GreaterEntry.category = "Bool";
          l_GreaterEntry.title = "Greater";
          l_GreaterEntry.search_text = "greater compare";
          l_GreaterEntry.node_type = l_GreaterNodeType;
          p_Graph.register_spawn_entry(l_GreaterEntry);
        }
      } // namespace OperatorNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
