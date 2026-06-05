#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace FunctionNodes {
        namespace {
          static ImU32 g_FunctionColor = IM_COL32(100, 160, 220, 255);

          struct FunctionEntryNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_function_entry);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const Node *l_Meta = p_Graph.find_node(p_NodeId);
              if (l_Meta && !l_Meta->title.empty()) {
                return l_Meta->title;
              }
              return "Function Entry";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Function";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Function";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_PLAY;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_FunctionColor;
            }

            virtual bool is_deletable(const Graph &,
                                      NodeId) const override
            {
              return false;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecMeta = make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecMeta, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct CallFunctionNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_function_call);
            }

            virtual Util::SharedPtr<NodeUserData>
            create_user_data() const override
            {
              return Util::make_shared<CallFunctionNodeData>();
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const CallFunctionNodeData *l_Data =
                  p_Graph.get_node_user_data<CallFunctionNodeData>(
                      p_NodeId);
              if (l_Data && !l_Data->function_name.empty()) {
                return l_Data->function_name;
              }
              return "Call Function";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Function";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Function";
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_FunctionColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ExecInMeta =
                  make_execution_pin_metadata("");
              p_Graph.add_pin(l_ExecIn, l_ExecInMeta, p_Schema);

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
              const CallFunctionNodeData *l_Data =
                  p_Graph.get_node_user_data<CallFunctionNodeData>(
                      p_NodeId);
              if (l_Data && !l_Data->function_name.empty()) {
                const Util::String l_FuncId =
                    make_vs_identifier(l_Data->function_name,
                                       "function");
                p_CompileContext.main_code.append(l_FuncId.c_str());
                p_CompileContext.main_code.append("();").endl();
              }

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void serialize(
                Graph &p_Graph, NodeId p_NodeId,
                Util::Serial::Node &p_Data) const override
            {
              const CallFunctionNodeData *l_Data =
                  p_Graph.get_node_user_data<CallFunctionNodeData>(
                      p_NodeId);
              if (l_Data) {
                p_Data["function_name"] = l_Data->function_name;
              }
            }

            virtual void deserialize(
                Graph &p_Graph, NodeId p_NodeId,
                Util::Serial::Node &p_Data) const override
            {
              CallFunctionNodeData *l_Data =
                  p_Graph.get_node_user_data<CallFunctionNodeData>(
                      p_NodeId);
              if (l_Data && p_Data["function_name"]) {
                l_Data->function_name =
                    p_Data["function_name"].as<Util::String>();
              }
              p_Graph.refresh_node_display_metadata(p_NodeId);
            }
          };

          static FunctionEntryNodeClass g_FunctionEntryNodeClass;
          static CallFunctionNodeClass g_CallFunctionNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_FunctionEntryNodeClass);
          p_Graph.register_node_class(g_CallFunctionNodeClass);
        }
      } // namespace FunctionNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
