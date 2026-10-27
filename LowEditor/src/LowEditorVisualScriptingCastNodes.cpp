#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace CastNodes {
        namespace {
          struct CastNumberToStringNode : public Node
          {
            Util::String get_icon(const Graph &) const override
            {
              return ICON_LC_ARROW_RIGHT_LEFT;
            }

            Util::Name get_name() const override
            {
              return N(vs_cast_number_to_string);
            }

            Util::String get_title(const Graph &) const override
            {
              return "Number to string";
            }

            Util::String get_category(const Graph &) const override
            {
              return "Cast";
            }

            ImU32 get_color(const Graph &) const override
            {
              return IM_COL32(120, 120, 160, 255);
            }

            void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id,
                                 make_number_pin("Value")),
                  p_Schema);

              Pin l_OutputMetadata = make_string_pin("Result");
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
              p_CompileContext.main_code.append(
                  "Low::Util::String(std::to_string(");
              compile_input_pin(
                  p_Graph,
                  p_Graph.find_input_pin_checked(id, "Value")->id,
                  p_CompileContext);
              p_CompileContext.main_code.append(").c_str())");
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_CastNumberToStringNodeType =
              p_Graph.register_node_type<CastNumberToStringNode>();

          NodeSpawnEntry l_Entry;
          l_Entry.id = N(vs_spawn_cast_number_to_string);
          l_Entry.category = "Cast";
          l_Entry.title = "Number to string";
          l_Entry.search_text = "cast number string";
          l_Entry.node_type = l_CastNumberToStringNodeType;
          p_Graph.register_spawn_entry(l_Entry);
        }
      } // namespace CastNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
