#include "LowEditorVisualScriptingSyntaxNodes.h"

#include "IconsCodicons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace SyntaxNodes {
        namespace {
          struct IfNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_if);
            }

            virtual Util::String
            get_title(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "if";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "Branch";
            }

            virtual Util::String
            get_category(const Graph &p_Graph,
                         NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32
            get_color(const Graph &p_Graph, NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return IM_COL32(189, 128, 53, 255);
            }

            virtual void
            setup_default_pins(Graph &p_Graph, NodeId p_NodeId,
                               const NodeGraphSchema *p_Schema) const
                override
            {
              Editor::Pin l_ExecIn = make_input_pin(p_Graph, p_NodeId);
              Pin l_ExecInMetadata =
                  make_execution_pin_metadata("Exec");
              p_Graph.add_pin(l_ExecIn, l_ExecInMetadata, p_Schema);

              Editor::Pin l_ConditionIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ConditionMetadata =
                  make_bool_pin_metadata("Condition", false);
              p_Graph.add_pin(l_ConditionIn, l_ConditionMetadata, p_Schema);

              Editor::Pin l_TrueOut = make_output_pin(p_Graph, p_NodeId);
              Pin l_TrueOutMetadata =
                  make_execution_pin_metadata("True");
              p_Graph.add_pin(l_TrueOut, l_TrueOutMetadata, p_Schema);

              Editor::Pin l_FalseOut = make_output_pin(p_Graph, p_NodeId);
              Pin l_FalseOutMetadata =
                  make_execution_pin_metadata("False");
              p_Graph.add_pin(l_FalseOut, l_FalseOutMetadata, p_Schema);
            }

            virtual void compile(Graph &p_Graph, NodeId p_NodeId,
                                 CompileContext &p_CompileContext) const
                override
            {
              const Pin *l_ConditionPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Condition");
              const Pin *l_TruePin =
                  p_Graph.find_output_pin_checked(p_NodeId, "True");
              const Pin *l_FalsePin =
                  p_Graph.find_output_pin_checked(p_NodeId, "False");

              const bool l_HasTrueBranch =
                  l_TruePin->pin.is_valid() &&
                  p_Graph.is_pin_connected(l_TruePin->pin);
              const bool l_HasFalseBranch =
                  l_FalsePin->pin.is_valid() &&
                  p_Graph.is_pin_connected(l_FalsePin->pin);

              if (l_HasTrueBranch) {
                p_CompileContext.main_code.append("if (");
                compile_input_pin(p_Graph, p_NodeId, l_ConditionPin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(") {").endl();
                p_Graph.continue_compilation(l_TruePin->pin,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();

                if (l_HasFalseBranch) {
                  p_CompileContext.main_code.append("else {")
                      .endl();
                  p_Graph.continue_compilation(l_FalsePin->pin,
                                               p_CompileContext);
                  p_CompileContext.main_code.append("}").endl();
                }
              } else if (l_HasFalseBranch) {
                p_CompileContext.main_code.append("if (!(");
                compile_input_pin(p_Graph, p_NodeId, l_ConditionPin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(")) {").endl();
                p_Graph.continue_compilation(l_FalsePin->pin,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();
              }
            }
          };

          static IfNodeClass g_IfNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_IfNodeClass);

          NodeSpawnEntry l_IfEntry;
          l_IfEntry.id = N(vs_spawn_syntax_if);
          l_IfEntry.category = "Syntax";
          l_IfEntry.title = "If";
          l_IfEntry.subtitle = "Branch";
          l_IfEntry.search_text = "if branch condition true false";
          l_IfEntry.node_class = g_IfNodeClass.get_name();
          p_Graph.register_spawn_entry(l_IfEntry);
        }
      } // namespace SyntaxNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
