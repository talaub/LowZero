#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace SignalNodes {
        namespace {
          static ImU32 g_SignalColor = IM_COL32(240, 180, 40, 255);

        } // namespace

        Util::Name MakeSignalNode::get_name() const
        {
          return N(vs_signal_make);
        }

        Util::String
        MakeSignalNode::get_title(const Graph &p_Graph) const
        {
          if (!function_name.empty()) {
            return Util::String("Make Signal: ") + function_name;
          }
          return "Make Signal";
        }

        Util::String MakeSignalNode::get_subtitle(const Graph &) const
        {
          return "Signal";
        }

        Util::String MakeSignalNode::get_category(const Graph &) const
        {
          return "Signal";
        }

        Util::String MakeSignalNode::get_icon(const Graph &) const
        {
          return ICON_LC_ZAP;
        }

        ImU32 MakeSignalNode::get_color(const Graph &) const
        {
          return g_SignalColor;
        }

        void MakeSignalNode::setup_default_pins(
            Graph &p_Graph, const NodeGraphSchema *p_Schema) const
        {

          Pin l_SignalMetadata;
          l_SignalMetadata.display_name = "Signal";
          l_SignalMetadata.type = PinType::Signal;
          l_SignalMetadata.show_default_value_when_unlinked = false;
          p_Graph.add_pin(
              make_output_pin(p_Graph, id, l_SignalMetadata),
              p_Schema);
        }

        void MakeSignalNode::compile_output_pin(
            Graph &p_Graph, PinId p_PinId,
            CompileContext &p_CompileContext) const
        {
          (void)p_PinId;
          if (function_name.empty()) {
            p_CompileContext.main_code.append("Signal()");
            return;
          }

          const Util::String l_FuncId =
              make_vs_identifier(function_name, "function");

          p_CompileContext.main_code.append("make_signal(this, \"");
          p_CompileContext.main_code.append(l_FuncId);
          p_CompileContext.main_code.append("\")");
        }

        void
        MakeSignalNode::serialize(Util::Serial::Node &p_Data) const
        {
          {
            p_Data["function_name"] = function_name;
          }
        }

        void
        MakeSignalNode::deserialize(const Util::Serial::Node &p_Data)
        {
          if (p_Data["function_name"]) {
            function_name =
                p_Data["function_name"].as<Util::String>();
          }
        }

        namespace {

          struct InvokeSignalNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_signal_invoke);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Invoke Signal";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Signal";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Signal";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_LC_ZAP;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SignalColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMeta = make_execution_pin("");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMeta),
                  p_Schema);

              Pin l_SignalMeta;
              l_SignalMeta.display_name = "Signal";
              l_SignalMeta.type = PinType::Signal;
              l_SignalMeta.show_default_value_when_unlinked = false;
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_SignalMeta),
                  p_Schema);

              Pin l_ExecOutMeta = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ExecOutMeta),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_SignalPin =
                  p_Graph.find_input_pin_checked(id, "Signal");

              if (l_SignalPin->id.is_valid() &&
                  p_Graph.is_pin_connected(l_SignalPin->id)) {
                compile_input_pin(p_Graph, l_SignalPin->id,
                                  p_CompileContext);
                p_CompileContext.main_code.append(".invoke();")
                    .endl();
              }

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_MakeSignalNodeType =
              p_Graph.register_node_type<MakeSignalNode>();
          const Util::Name l_InvokeSignalNodeType =
              p_Graph.register_node_type<InvokeSignalNode>();

          NodeSpawnEntry l_MakeSignalEntry;
          l_MakeSignalEntry.id = N(vs_spawn_signal_make);
          l_MakeSignalEntry.category = "Signal";
          l_MakeSignalEntry.title = "Make Signal";
          l_MakeSignalEntry.search_text = "make signal bind";
          l_MakeSignalEntry.node_type = l_MakeSignalNodeType;
          p_Graph.register_spawn_entry(l_MakeSignalEntry);

          NodeSpawnEntry l_InvokeSignalEntry;
          l_InvokeSignalEntry.id = N(vs_spawn_signal_invoke);
          l_InvokeSignalEntry.category = "Signal";
          l_InvokeSignalEntry.title = "Invoke Signal";
          l_InvokeSignalEntry.search_text = "invoke signal call fire";
          l_InvokeSignalEntry.node_type = l_InvokeSignalNodeType;
          p_Graph.register_spawn_entry(l_InvokeSignalEntry);
        }
      } // namespace SignalNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
