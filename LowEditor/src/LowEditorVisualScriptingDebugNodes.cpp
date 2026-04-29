#include "LowEditorVisualScriptNodes.h"

#include "IconsCodicons.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace DebugNodes {
        namespace {
          struct LogNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_debug_log);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "Log";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "Debug";
            }

            virtual Util::String
            get_category(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "Debug";
            }

            virtual Util::String
            get_icon(const Graph &p_Graph,
                     NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return ICON_CI_BUG;
            }

            virtual ImU32 get_color(const Graph &p_Graph,
                                    NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return IM_COL32(102, 113, 115, 255);
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ExecInMetadata =
                  make_execution_pin_metadata("Exec");
              p_Graph.add_pin(l_ExecIn, l_ExecInMetadata, p_Schema);

              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecOutMetadata =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);

              Editor::Pin l_MessageIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_MessageInMetadata =
                  make_string_pin_metadata("Message");
              l_MessageInMetadata.default_value =
                  Util::Variant(Util::String(""));
              p_Graph.add_pin(l_MessageIn, l_MessageInMetadata,
                              p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_MessagePin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Message");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");

              p_CompileContext.main_code.append("Log::info(");
              compile_input_pin(p_Graph, p_NodeId, l_MessagePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(");").endl();

              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          static LogNodeClass g_LogNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_LogNodeClass);

          NodeSpawnEntry l_LogEntry;
          l_LogEntry.id = N(vs_spawn_debug_log);
          l_LogEntry.category = "Debug";
          l_LogEntry.title = "Log";
          l_LogEntry.subtitle = "Debug";
          l_LogEntry.search_text = "log debug print message";
          l_LogEntry.node_class = g_LogNodeClass.get_name();
          p_Graph.register_spawn_entry(l_LogEntry);
        }
      } // namespace DebugNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
