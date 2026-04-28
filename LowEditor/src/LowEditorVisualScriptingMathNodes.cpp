#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace MathNodes {
        namespace {
          static const ImU32 g_MathColor =
              IM_COL32(65, 145, 146, 255);
          static const ImU32 g_RandomColor =
              IM_COL32(200, 100, 100, 255);

          struct BinaryNumberNodeClassBase : public NodeClass
          {
            Util::String m_Title;
            const char *m_Operator;
            bool m_StartDynamic = false;

            BinaryNumberNodeClassBase(Util::String p_Title,
                                      const char *p_Operator,
                                      bool p_StartDynamic = false)
                : m_Title(p_Title), m_Operator(p_Operator),
                  m_StartDynamic(p_StartDynamic)
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
              return "Math";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_MathColor;
            }

            void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              const Pin l_InputMetadata =
                  m_StartDynamic ? make_dynamic_pin_metadata("A")
                                 : make_number_pin_metadata("A");
              Pin l_InputBMetadata =
                  m_StartDynamic ? make_dynamic_pin_metadata("B")
                                 : make_number_pin_metadata("B");
              Pin l_OutputMetadata =
                  m_StartDynamic ? make_dynamic_pin_metadata("Result")
                                 : make_number_pin_metadata("Result");
              l_OutputMetadata.show_default_value_when_unlinked =
                  false;

              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              l_InputMetadata, p_Schema);
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              l_InputBMetadata, p_Schema);
              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            bool can_connect_pin(
                Graph &, NodeId, PinId, const Pin &p_PinMetadata,
                const Pin &p_OtherPinMetadata) const override
            {
              if (m_StartDynamic &&
                  p_PinMetadata.type == PinType::Dynamic) {
                return p_OtherPinMetadata.type == PinType::Number;
              }

              return true;
            }

            void on_pin_connected(Graph &p_Graph, NodeId p_NodeId,
                                  PinId,
                                  PinId p_OtherPinId) const override
            {
              if (!m_StartDynamic) {
                return;
              }

              const Pin *l_OtherPin = p_Graph.find_pin(p_OtherPinId);
              if (!l_OtherPin ||
                  l_OtherPin->type != PinType::Number) {
                return;
              }

              for (Editor::Pin *i_Pin :
                   p_Graph.graph.get_node_pins(p_NodeId)) {
                Pin *l_PinMetadata = p_Graph.find_pin(i_Pin->id);
                if (!l_PinMetadata) {
                  continue;
                }

                const bool l_WasDynamic =
                    l_PinMetadata->type == PinType::Dynamic;
                l_PinMetadata->type = PinType::Number;
                l_PinMetadata->number_subtype =
                    l_OtherPin->number_subtype;
                if (l_WasDynamic &&
                    i_Pin->direction == PinDirection::Input) {
                  l_PinMetadata->default_value =
                      default_value_for_pin(*l_PinMetadata);
                }
              }
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

          struct AddNodeClass : public BinaryNumberNodeClassBase
          {
            AddNodeClass()
                : BinaryNumberNodeClassBase("Add", " + ", true)
            {
            }

            Util::Name get_name() const override
            {
              return N(vs_math_add);
            }

            bool is_compact(const Graph &, NodeId) const override
            {
              return true;
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ICON_LC_PLUS;
            }
          };

          struct SubtractNodeClass : public BinaryNumberNodeClassBase
          {
            SubtractNodeClass()
                : BinaryNumberNodeClassBase("Subtract", " - ", true)
            {
            }

            Util::Name get_name() const override
            {
              return N(vs_math_subtract);
            }

            bool is_compact(const Graph &, NodeId) const override
            {
              return true;
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ICON_LC_MINUS;
            }
          };

          struct MultiplyNodeClass : public BinaryNumberNodeClassBase
          {
            MultiplyNodeClass()
                : BinaryNumberNodeClassBase("Multiply", " * ")
            {
            }

            Util::Name get_name() const override
            {
              return N(vs_math_multiply);
            }

            bool is_compact(const Graph &, NodeId) const override
            {
              return true;
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ICON_LC_X;
            }
          };

          struct DivideNodeClass : public BinaryNumberNodeClassBase
          {
            DivideNodeClass()
                : BinaryNumberNodeClassBase("Divide", " / ")
            {
            }

            Util::Name get_name() const override
            {
              return N(vs_math_divide);
            }

            bool is_compact(const Graph &, NodeId) const override
            {
              return true;
            }

            Util::String get_compact_title(const Graph &,
                                           NodeId) const override
            {
              return ICON_LC_DIVIDE;
            }
          };

          struct PercentChanceNodeClass : public NodeClass
          {
            Util::Name get_name() const override
            {
              return N(vs_random_percent_chance);
            }

            Util::String get_title(const Graph &,
                                   NodeId) const override
            {
              return "Percent chance";
            }

            Util::String get_category(const Graph &,
                                      NodeId) const override
            {
              return "Random";
            }

            ImU32 get_color(const Graph &, NodeId) const override
            {
              return g_RandomColor;
            }

            void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              p_Graph.add_pin(make_input_pin(p_Graph, p_NodeId),
                              make_number_pin_metadata("Percent"),
                              p_Schema);
              Editor::Pin l_Output =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_OutputMetadata =
                  make_bool_pin_metadata("Success");
              l_OutputMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_Output, l_OutputMetadata, p_Schema);
            }

            void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId,
                CompileContext &p_CompileContext) const override
            {
              p_CompileContext.main_code.append(
                  "Low::Math::Util::random_percent((uint8_t)");
              compile_input_pin(
                  p_Graph, p_NodeId,
                  p_Graph.find_input_pin_checked(p_NodeId, "Percent")
                      ->pin,
                  p_CompileContext);
              p_CompileContext.main_code.append(")");
            }
          };

          static AddNodeClass g_AddNodeClass;
          static SubtractNodeClass g_SubtractNodeClass;
          static MultiplyNodeClass g_MultiplyNodeClass;
          static DivideNodeClass g_DivideNodeClass;
          static PercentChanceNodeClass g_PercentChanceNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_AddNodeClass);
          p_Graph.register_node_class(g_SubtractNodeClass);
          p_Graph.register_node_class(g_MultiplyNodeClass);
          p_Graph.register_node_class(g_DivideNodeClass);
          p_Graph.register_node_class(g_PercentChanceNodeClass);

          NodeSpawnEntry l_AddEntry;
          l_AddEntry.id = N(vs_spawn_math_add);
          l_AddEntry.category = "Math";
          l_AddEntry.title = "Add";
          l_AddEntry.search_text = "math add plus";
          l_AddEntry.node_class = g_AddNodeClass.get_name();
          p_Graph.register_spawn_entry(l_AddEntry);

          NodeSpawnEntry l_SubtractEntry;
          l_SubtractEntry.id = N(vs_spawn_math_subtract);
          l_SubtractEntry.category = "Math";
          l_SubtractEntry.title = "Subtract";
          l_SubtractEntry.search_text = "math subtract minus";
          l_SubtractEntry.node_class = g_SubtractNodeClass.get_name();
          p_Graph.register_spawn_entry(l_SubtractEntry);

          NodeSpawnEntry l_MultiplyEntry;
          l_MultiplyEntry.id = N(vs_spawn_math_multiply);
          l_MultiplyEntry.category = "Math";
          l_MultiplyEntry.title = "Multiply";
          l_MultiplyEntry.search_text = "math multiply times";
          l_MultiplyEntry.node_class = g_MultiplyNodeClass.get_name();
          p_Graph.register_spawn_entry(l_MultiplyEntry);

          NodeSpawnEntry l_DivideEntry;
          l_DivideEntry.id = N(vs_spawn_math_divide);
          l_DivideEntry.category = "Math";
          l_DivideEntry.title = "Divide";
          l_DivideEntry.search_text = "math divide";
          l_DivideEntry.node_class = g_DivideNodeClass.get_name();
          p_Graph.register_spawn_entry(l_DivideEntry);

          NodeSpawnEntry l_PercentChanceEntry;
          l_PercentChanceEntry.id = N(vs_spawn_random_percent_chance);
          l_PercentChanceEntry.category = "Random";
          l_PercentChanceEntry.title = "Percent chance";
          l_PercentChanceEntry.search_text = "random percent chance";
          l_PercentChanceEntry.node_class =
              g_PercentChanceNodeClass.get_name();
          p_Graph.register_spawn_entry(l_PercentChanceEntry);
        }
      } // namespace MathNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
