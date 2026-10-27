#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace FunctionNodes {
        namespace {
          static ImU32 g_FunctionColor = IM_COL32(100, 160, 220, 255);

          struct FunctionEntryNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_function_entry);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              return "Function Entry";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Function";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Function";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_LC_PLAY;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_FunctionColor;
            }

            virtual bool is_deletable(const Graph &) const override
            {
              return false;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecMeta = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ExecMeta), p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

        } // namespace

        Util::Name CallFunctionNode::get_name() const
        {
          return N(vs_function_call);
        }

        Util::String
        CallFunctionNode::get_title(const Graph &p_Graph) const
        {
          if (!function_name.empty()) {
            return function_name;
          }
          return "Call Function";
        }

        Util::String
        CallFunctionNode::get_subtitle(const Graph &) const
        {
          return "Function";
        }

        Util::String
        CallFunctionNode::get_category(const Graph &) const
        {
          return "Function";
        }

        Util::String CallFunctionNode::get_icon(const Graph &) const
        {
          return ICON_CI_SYMBOL_METHOD;
        }

        ImU32 CallFunctionNode::get_color(const Graph &) const
        {
          return g_FunctionColor;
        }

        void CallFunctionNode::setup_default_pins(
            Graph &p_Graph, const NodeGraphSchema *p_Schema) const
        {

          Pin l_ExecInMeta = make_execution_pin("");
          p_Graph.add_pin(make_input_pin(p_Graph, id, l_ExecInMeta),
                          p_Schema);

          Pin l_ExecOutMeta = make_execution_pin("Then");
          p_Graph.add_pin(make_output_pin(p_Graph, id, l_ExecOutMeta),
                          p_Schema);
        }

        void CallFunctionNode::compile(
            Graph &p_Graph, CompileContext &p_CompileContext) const
        {
          if (!function_name.empty()) {
            const Util::String l_FuncId =
                make_vs_identifier(function_name, "function");
            p_CompileContext.main_code.append(l_FuncId.c_str());
            p_CompileContext.main_code.append("();").endl();
          }

          const Pin *l_ThenPin =
              p_Graph.find_output_pin_checked(id, "Then");
          p_Graph.continue_compilation(l_ThenPin->id,
                                       p_CompileContext);
        }

        void
        CallFunctionNode::serialize(Util::Serial::Node &p_Data) const
        {
          p_Data["function_name"] = function_name;
        }

        void CallFunctionNode::deserialize(
            const Util::Serial::Node &p_Data)
        {
          if (p_Data["function_name"]) {
            function_name =
                p_Data["function_name"].as<Util::String>();
          }
        }

        namespace {

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_FunctionEntryNodeType =
              p_Graph.register_node_type<FunctionEntryNode>();
          const Util::Name l_CallFunctionNodeType =
              p_Graph.register_node_type<CallFunctionNode>();
        }
      } // namespace FunctionNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
