#include "LowEditorVisualScriptNodes.h"

#include "IconsCodicons.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace DebugNodes {
        namespace {
          struct LogNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_debug_log);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "Log";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "Debug";
            }

            virtual Util::String
            get_category(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "Debug";
            }

            virtual Util::String
            get_icon(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return ICON_CI_BUG;
            }

            virtual ImU32
            get_color(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return IM_COL32(102, 113, 115, 255);
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMetadata = make_execution_pin("Exec");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMetadata),
                  p_Schema);

              Pin l_ExecOutMetadata = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ExecOutMetadata),
                  p_Schema);

              Pin l_MessageInMetadata = make_string_pin("Message");
              l_MessageInMetadata.default_value =
                  Util::Variant(Util::String(""));
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_MessageInMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_MessagePin =
                  p_Graph.find_input_pin_checked(id, "Message");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");

              p_CompileContext.main_code.append("Log::info(");
              compile_input_pin(p_Graph, l_MessagePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(");").endl();

              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_LogNodeType =
              p_Graph.register_node_type<LogNode>();

          NodeSpawnEntry l_LogEntry;
          l_LogEntry.id = N(vs_spawn_debug_log);
          l_LogEntry.category = "Debug";
          l_LogEntry.title = "Log";
          l_LogEntry.subtitle = "Debug";
          l_LogEntry.search_text = "log debug print message";
          l_LogEntry.node_type = l_LogNodeType;
          p_Graph.register_spawn_entry(l_LogEntry);
        }
      } // namespace DebugNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
