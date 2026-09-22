#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace SignalNodes {
        namespace {
          static ImU32 g_SignalColor = IM_COL32(240, 180, 40, 255);

          struct MakeSignalNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_signal_make);
            }

            virtual Util::SharedPtr<NodeUserData>
            create_user_data() const override
            {
              return Util::make_shared<MakeSignalNodeData>();
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const MakeSignalNodeData *l_Data =
                  p_Graph.get_node_user_data<MakeSignalNodeData>(
                      p_NodeId);
              if (l_Data && !l_Data->function_name.empty()) {
                return Util::String("Make Signal: ") +
                       l_Data->function_name;
              }
              return "Make Signal";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Signal";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Signal";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_ZAP;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SignalColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_SignalOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_SignalMetadata;
              l_SignalMetadata.display_name = "Signal";
              l_SignalMetadata.type = PinType::Signal;
              l_SignalMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_SignalOut, l_SignalMetadata,
                              p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const MakeSignalNodeData *l_Data =
                  p_Graph.get_node_user_data<MakeSignalNodeData>(
                      p_NodeId);
              if (!l_Data || l_Data->function_name.empty()) {
                p_CompileContext.main_code.append("Signal()");
                return;
              }

              const Util::String l_FuncId =
                  make_vs_identifier(l_Data->function_name,
                                     "function");

              p_CompileContext.main_code.append(
                  "make_signal(this, \"");
              p_CompileContext.main_code.append(l_FuncId);
              p_CompileContext.main_code.append("\")");
            }

            virtual void serialize(
                Graph &p_Graph, NodeId p_NodeId,
                Util::Serial::Node &p_Data) const override
            {
              const MakeSignalNodeData *l_Data =
                  p_Graph.get_node_user_data<MakeSignalNodeData>(
                      p_NodeId);
              if (l_Data) {
                p_Data["function_name"] = l_Data->function_name;
              }
            }

            virtual void deserialize(
                Graph &p_Graph, NodeId p_NodeId,
                Util::Serial::Node &p_Data) const override
            {
              MakeSignalNodeData *l_Data =
                  p_Graph.get_node_user_data<MakeSignalNodeData>(
                      p_NodeId);
              if (l_Data && p_Data["function_name"]) {
                l_Data->function_name =
                    p_Data["function_name"].as<Util::String>();
              }
              p_Graph.refresh_node_display_metadata(p_NodeId);
            }
          };

          struct InvokeSignalNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_signal_invoke);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Invoke Signal";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Signal";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Signal";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_ZAP;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SignalColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ExecInMeta = make_execution_pin_metadata("");
              p_Graph.add_pin(l_ExecIn, l_ExecInMeta, p_Schema);

              Editor::Pin l_SignalIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_SignalMeta;
              l_SignalMeta.display_name = "Signal";
              l_SignalMeta.type = PinType::Signal;
              l_SignalMeta.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_SignalIn, l_SignalMeta, p_Schema);

              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecOutMeta =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecOutMeta, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_SignalPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Signal");

              if (l_SignalPin->pin.is_valid() &&
                  p_Graph.is_pin_connected(l_SignalPin->pin)) {
                compile_input_pin(p_Graph, p_NodeId,
                                  l_SignalPin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(".invoke();")
                    .endl();
              }

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          static MakeSignalNodeClass g_MakeSignalNodeClass;
          static InvokeSignalNodeClass g_InvokeSignalNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_MakeSignalNodeClass);
          p_Graph.register_node_class(g_InvokeSignalNodeClass);

          NodeSpawnEntry l_MakeSignalEntry;
          l_MakeSignalEntry.id = N(vs_spawn_signal_make);
          l_MakeSignalEntry.category = "Signal";
          l_MakeSignalEntry.title = "Make Signal";
          l_MakeSignalEntry.search_text = "make signal bind";
          l_MakeSignalEntry.node_class =
              g_MakeSignalNodeClass.get_name();
          p_Graph.register_spawn_entry(l_MakeSignalEntry);

          NodeSpawnEntry l_InvokeSignalEntry;
          l_InvokeSignalEntry.id = N(vs_spawn_signal_invoke);
          l_InvokeSignalEntry.category = "Signal";
          l_InvokeSignalEntry.title = "Invoke Signal";
          l_InvokeSignalEntry.search_text = "invoke signal call fire";
          l_InvokeSignalEntry.node_class =
              g_InvokeSignalNodeClass.get_name();
          p_Graph.register_spawn_entry(l_InvokeSignalEntry);
        }
      } // namespace SignalNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
