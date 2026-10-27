#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace BoolNodes {
        namespace {
          static const ImU32 g_BoolColor = IM_COL32(220, 48, 48, 255);

          struct NotNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_BAN;
            }

            Util::Name get_name() const override
            {
              return N(vs_bool_not);
            }

            Util::String get_title(const Graph &) const override
            {
              return "NOT";
            }

            Util::String get_category(const Graph &) const override
            {
              return "Bool";
            }

            Util::String
            get_compact_title(const Graph &) const override
            {
              return "NOT";
            }

            bool is_compact(const Graph &) const override
            {
              return true;
            }

            ImU32 get_color(const Graph &) const override
            {
              return g_BoolColor;
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_bool_pin("A")),
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
              p_CompileContext.main_code.append("(!");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "A")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct OrNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_SPLIT;
            }

            Util::Name get_name() const override
            {
              return N(vs_bool_or);
            }

            Util::String get_title(const Graph &) const override
            {
              return "OR";
            }

            Util::String
            get_compact_title(const Graph &) const override
            {
              return "OR";
            }

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
              return g_BoolColor;
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_bool_pin("A")),
                  p_Schema);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_bool_pin("B")),
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
              p_CompileContext.main_code.append(" || ");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "B")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct AndNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_AMPERSANDS;
            }

            Util::Name get_name() const override
            {
              return N(vs_bool_and);
            }

            Util::String get_title(const Graph &) const override
            {
              return "AND";
            }

            Util::String
            get_compact_title(const Graph &) const override
            {
              return "AND";
            }

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
              return g_BoolColor;
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_bool_pin("A")),
                  p_Schema);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_bool_pin("B")),
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
              p_CompileContext.main_code.append(" && ");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "B")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          struct EqualsNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_EQUAL;
            }

            Util::Name get_name() const override
            {
              return N(vs_bool_equals);
            }

            Util::String get_title(const Graph &) const override
            {
              return ICON_FA_EQUALS ICON_FA_EQUALS "";
            }

            Util::String
            get_compact_title(const Graph &) const override
            {
              return "==";
            }

            Util::String get_category(const Graph &) const override
            {
              return "Bool";
            }

            ImU32 get_color(const Graph &) const override
            {
              return g_BoolColor;
            }

            bool is_compact(const Graph &) const override
            {
              return true;
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_dynamic_pin("A")),
                  p_Schema);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, make_dynamic_pin("B")),
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
              p_CompileContext.main_code.append(" == ");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "B")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_NotNodeType =
              p_Graph.register_node_type<NotNode>();
          const Util::Name l_OrNodeType =
              p_Graph.register_node_type<OrNode>();
          const Util::Name l_AndNodeType =
              p_Graph.register_node_type<AndNode>();
          const Util::Name l_EqualsNodeType =
              p_Graph.register_node_type<EqualsNode>();

          NodeSpawnEntry l_NotEntry;
          l_NotEntry.id = N(vs_spawn_bool_not);
          l_NotEntry.category = "Bool";
          l_NotEntry.title = "Not";
          l_NotEntry.search_text = "bool not";
          l_NotEntry.node_type = l_NotNodeType;
          p_Graph.register_spawn_entry(l_NotEntry);

          NodeSpawnEntry l_OrEntry;
          l_OrEntry.id = N(vs_spawn_bool_or);
          l_OrEntry.category = "Bool";
          l_OrEntry.title = "Or";
          l_OrEntry.search_text = "bool or";
          l_OrEntry.node_type = l_OrNodeType;
          p_Graph.register_spawn_entry(l_OrEntry);

          NodeSpawnEntry l_AndEntry;
          l_AndEntry.id = N(vs_spawn_bool_and);
          l_AndEntry.category = "Bool";
          l_AndEntry.title = "And";
          l_AndEntry.search_text = "bool and";
          l_AndEntry.node_type = l_AndNodeType;
          p_Graph.register_spawn_entry(l_AndEntry);

          NodeSpawnEntry l_EqualsEntry;
          l_EqualsEntry.id = N(vs_spawn_bool_equals);
          l_EqualsEntry.category = "Bool";
          l_EqualsEntry.title = "Equals";
          l_EqualsEntry.search_text = "bool equals compare";
          l_EqualsEntry.node_type = l_EqualsNodeType;
          p_Graph.register_spawn_entry(l_EqualsEntry);
        }
      } // namespace BoolNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
