#include "LowEditorVisualScriptNodes.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace CastNodes {
        namespace {
          struct CastNumberToStringNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_cast_number_to_string);
            }

            Util::String get_title(const Graph &, NodeId) const override
            {
              return "Number to string";
            }

            Util::String get_category(const Graph &, NodeId) const override
            {
              return "Cast";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return IM_COL32(120, 120, 160, 255);
            }

            void setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                                    const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_number_pin_metadata("Value"), p_Schema);
              Editor::Pin l_Output = make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata = make_string_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(Graph &p_Graph, NodeId p_NodeId,
                                    PinId, CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append(
                  "Low::Util::String(std::to_string(");
              compile_input_pin(
                  p_Graph, p_NodeId,
                  p_Graph.find_input_pin_checked(p_NodeId, "Value")->pin,
                  p_CompileContext);
              p_CompileContext.main_code.append(").c_str())");
            }
          };

          static CastNumberToStringNodeClass
              g_CastNumberToStringNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_CastNumberToStringNodeClass);

          NodeSpawnEntry l_Entry;
          l_Entry.id = N(vs_spawn_cast_number_to_string);
          l_Entry.category = "Cast";
          l_Entry.title = "Number to string";
          l_Entry.search_text = "cast number string";
          l_Entry.node_class = g_CastNumberToStringNodeClass.get_name();
          p_Graph.register_spawn_entry(l_Entry);
        }
      } // namespace CastNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
