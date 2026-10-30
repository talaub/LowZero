#include "LowCoreScripting.h"
#include "LowEditor.h"
#include "LowEditorVisualScriptNodes.h"

#include "IconsCodicons.h"
#include "LowEditorIcons.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace SyntaxNodes {
        namespace {
          static ImU32 g_SyntaxColor = IM_COL32(189, 128, 53, 255);

          static const Variable *
          get_selected_variable(const Graph &p_Graph, NodeId p_NodeId)
          {
            const auto *l_Node =
                p_Graph.find_node_as<VariableNode>(p_NodeId);
            if (!l_Node || l_Node->variable_name.empty()) {
              return nullptr;
            }

            return p_Graph.find_variable(l_Node->variable_name);
          }

          static const Core::Scripting::FunctionInfo &
          get_selected_global_cpp_function(const Graph &p_Graph,
                                           NodeId p_NodeId)
          {
            const auto *l_Node =
                p_Graph.find_node_as<GlobalFunctionCallNode>(
                    p_NodeId);
            if (!l_Node || l_Node->function_name.empty()) {
              _LOW_ASSERT(false);
            }

            return Core::Scripting::
                find_registered_global_function_checked(
                    l_Node->function_name);
          }

          static Util::String global_function_call_temp_var_name(
              NodeId p_NodeId, const Util::String &p_PinDisplayName)
          {
            Util::StringBuilder l_Name;
            l_Name.append("__fn_call_")
                .append((u64)p_NodeId.value)
                .append("_");
            if (p_PinDisplayName == "Return value") {
              l_Name.append("ret");
            } else {
              l_Name.append(p_PinDisplayName);
            }
            return l_Name.get();
          }

          static Util::String
          global_function_call_pin_type_string(const Pin &p_Pin)
          {
            Util::String l_Type = pin_type_to_script_type_string(
                p_Pin.type, p_Pin.number_subtype,
                p_Pin.string_subtype, p_Pin.handle_type);
            if (l_Type.empty()) {
              return l_Type;
            }
            if (p_Pin.container_type == PinContainerType::List) {
              l_Type = "array<" + l_Type + ">";
            }
            return l_Type;
          }

          static PinType scripting_type_to_pin_type(
              const Core::Scripting::TypeInfo &p_Type)
          {
            using TypeKind = Core::Scripting::TypeKind;

            if (p_Type.as_type == "Vector2")
              return PinType::Vector2;
            if (p_Type.as_type == "Vector3")
              return PinType::Vector3;
            if (p_Type.as_type == "Vector4")
              return PinType::Vector4;
            if (p_Type.as_type == "Quaternion")
              return PinType::Quaternion;
            if (p_Type.as_type == "Signal")
              return PinType::Signal;

            switch (p_Type.kind) {
            case TypeKind::Bool:
              return PinType::Bool;
            case TypeKind::Float:
            case TypeKind::Int32:
            case TypeKind::UInt32:
            case TypeKind::UInt64:
              return PinType::Number;
            case TypeKind::String:
            case TypeKind::Name:
              return PinType::String;
            case TypeKind::Handle:
              return PinType::Handle;
            case TypeKind::Struct:
              return PinType::Struct;
            case TypeKind::Enum:
              return PinType::Enum;
            default:
              return PinType::Dynamic;
            }
          }

          static NumberSubtype scripting_type_to_number_subtype(
              const Core::Scripting::TypeInfo &p_Type)
          {
            using TypeKind = Core::Scripting::TypeKind;
            switch (p_Type.kind) {
            case TypeKind::Int32:
              return NumberSubtype::Int32;
            case TypeKind::UInt32:
              return NumberSubtype::UInt32;
            case TypeKind::UInt64:
              return NumberSubtype::UInt64;
            default:
              return NumberSubtype::Float;
            }
          }

          static PinContainerType
          scripting_container_to_pin_container(
              Core::Scripting::TypeContainer p_Container)
          {
            if (p_Container == Core::Scripting::TypeContainer::List) {
              return PinContainerType::List;
            }
            return PinContainerType::None;
          }

          static Pin make_pin_metadata_from_function_parameter_info(
              const Core::Scripting::FunctionParameterInfo
                  &p_ParamInfo,
              const Util::String &p_DisplayName)
          {
            Pin l_Pin;
            l_Pin.display_name = p_DisplayName;
            l_Pin.type = scripting_type_to_pin_type(p_ParamInfo.type);
            l_Pin.number_subtype =
                scripting_type_to_number_subtype(p_ParamInfo.type);
            l_Pin.string_subtype =
                p_ParamInfo.type.kind ==
                        Core::Scripting::TypeKind::Name
                    ? StringSubtype::Name
                    : StringSubtype::String;
            l_Pin.handle_type = p_ParamInfo.type.referenced_type;
            l_Pin.container_type =
                scripting_container_to_pin_container(
                    p_ParamInfo.type.container);
            l_Pin.widget = PinWidget::DefaultValue;
            l_Pin.show_default_value_when_unlinked = true;
            return l_Pin;
          }

          static Pin make_pin_metadata_from_variable(
              const Variable &p_Variable,
              const Util::String &p_DisplayName)
          {
            Pin l_Pin;
            l_Pin.display_name = p_DisplayName;
            l_Pin.type = p_Variable.type;
            l_Pin.string_subtype = p_Variable.string_subtype;
            l_Pin.number_subtype = p_Variable.number_subtype;
            l_Pin.handle_type = p_Variable.handle_type;
            l_Pin.container_type = p_Variable.container_type;
            l_Pin.default_value = p_Variable.default_value;
            l_Pin.widget = PinWidget::DefaultValue;
            l_Pin.show_default_value_when_unlinked = true;
            return l_Pin;
          }

          struct GetVariableNode : public VariableNode
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_get_variable);
            }

            virtual bool is_compact(const Graph &p_Graph) const
            {
              return true;
            }

            virtual Util::String
            get_compact_title(const Graph &p_Graph) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              return l_Variable ? l_Variable->name
                                : Util::String("Get variable");
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              return l_Variable ? l_Variable->name
                                : Util::String("Get variable");
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Variable";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              if (!l_Variable) {
                return;
              }

              Pin l_ValueMetadata =
                  make_pin_metadata_from_variable(*l_Variable, "");
              l_ValueMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              LOW_ASSERT(
                  l_Variable,
                  "No variable selected for get variable node");
              if (!l_Variable) {
                return;
              }

              p_CompileContext.main_code.append(
                  l_Variable->name.c_str());
            }
          };

          struct SetVariableNode : public VariableNode
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_set_variable);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              if (!l_Variable) {
                return "Set variable";
              }

              Util::String l_Title = "Set ";
              l_Title += l_Variable->name;
              return l_Title;
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Variable";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SyntaxColor;
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

              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              if (!l_Variable) {
                return;
              }

              Pin l_ValueMetadata = make_pin_metadata_from_variable(
                  *l_Variable, l_Variable->name);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, id);
              LOW_ASSERT(
                  l_Variable,
                  "No variable selected for set variable node");
              if (!l_Variable) {
                return;
              }

              const Pin *l_ValuePin = p_Graph.find_input_pin_checked(
                  id, l_Variable->name);

              p_CompileContext.main_code.append(
                  l_Variable->name.c_str());
              p_CompileContext.main_code.append(" = ");
              compile_input_pin(p_Graph, l_ValuePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }
          };

          struct ReturnNumberNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_return_number);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Return";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Number";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_CI_ARROW_RIGHT;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMetadata = make_execution_pin("Exec");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMetadata),
                  p_Schema);

              Pin l_ValueMetadata = make_number_pin("");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ValuePin =
                  p_Graph.find_input_pin_checked(id, "");

              p_CompileContext.main_code.append("return ");
              compile_input_pin(p_Graph, l_ValuePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();
            }
          };

          struct ReturnBoolNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_return_bool);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "Return";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Bool";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_CI_ARROW_RIGHT;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMetadata = make_execution_pin("Exec");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMetadata),
                  p_Schema);

              Pin l_ValueMetadata = make_bool_pin("", false);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ValueMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ValuePin =
                  p_Graph.find_input_pin_checked(id, "");

              p_CompileContext.main_code.append("return ");
              compile_input_pin(p_Graph, l_ValuePin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();
            }
          };

          struct ForNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_for);
            }

            virtual Util::String
            get_title(const Graph &) const override
            {
              return "for";
            }

            virtual Util::String
            get_subtitle(const Graph &) const override
            {
              return "Loop";
            }

            virtual Util::String
            get_category(const Graph &) const override
            {
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMetadata = make_execution_pin("Exec");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMetadata),
                  p_Schema);

              Pin l_StartMetadata = make_number_pin("Start");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_StartMetadata),
                  p_Schema);

              Pin l_EndMetadata = make_number_pin("End");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_EndMetadata),
                  p_Schema);

              Pin l_StepMetadata = make_number_pin("Step");
              l_StepMetadata.default_value = Util::Variant(1.0f);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_StepMetadata),
                  p_Schema);

              Pin l_ThenMetadata = make_execution_pin("Then");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_ThenMetadata),
                  p_Schema);

              Pin l_LoopMetadata = make_execution_pin("Loop");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_LoopMetadata),
                  p_Schema);

              Pin l_IndexMetadata = make_number_pin("Index");
              l_IndexMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_IndexMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_StartPin =
                  p_Graph.find_input_pin_checked(id, "Start");
              const Pin *l_EndPin =
                  p_Graph.find_input_pin_checked(id, "End");
              const Pin *l_StepPin =
                  p_Graph.find_input_pin_checked(id, "Step");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(id, "Then");
              const Pin *l_LoopPin =
                  p_Graph.find_output_pin_checked(id, "Loop");

              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__for_index")
                  .append((u64)id.value);

              p_CompileContext.main_code.append("for (int ")
                  .append(l_IndexVariableName.get())
                  .append(" = ");
              compile_input_pin(p_Graph, l_StartPin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append("; ")
                  .append(l_IndexVariableName.get())
                  .append(" < ");
              compile_input_pin(p_Graph, l_EndPin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append("; ")
                  .append(l_IndexVariableName.get())
                  .append(" += ");
              compile_input_pin(p_Graph, l_StepPin->id,
                                p_CompileContext);
              p_CompileContext.main_code.append(") {").endl();
              p_Graph.continue_compilation(l_LoopPin->id,
                                           p_CompileContext);
              p_CompileContext.main_code.append("}").endl();

              p_Graph.continue_compilation(l_ThenPin->id,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)p_PinId;
              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__for_index")
                  .append((u64)id.value);
              p_CompileContext.main_code.append(
                  l_IndexVariableName.get());
            }
          };

          struct IfNode : public Node
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_if);
            }

            virtual Util::String
            get_title(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "if";
            }

            virtual Util::String
            get_subtitle(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "Branch";
            }

            virtual Util::String
            get_category(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return "Syntax";
            }

            virtual Util::String
            get_icon(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32
            get_color(const Graph &p_Graph) const override
            {
              (void)p_Graph;
              (void)id;
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph,
                const NodeGraphSchema *p_Schema) const override
            {

              Pin l_ExecInMetadata = make_execution_pin("Exec");
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ExecInMetadata),
                  p_Schema);

              Pin l_ConditionMetadata =
                  make_bool_pin("Condition", false);
              p_Graph.add_pin(
                  make_input_pin(p_Graph, id, l_ConditionMetadata),
                  p_Schema);

              Pin l_TrueOutMetadata = make_execution_pin("True");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_TrueOutMetadata),
                  p_Schema);

              Pin l_FalseOutMetadata = make_execution_pin("False");
              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, l_FalseOutMetadata),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ConditionPin =
                  p_Graph.find_input_pin_checked(id, "Condition");
              const Pin *l_TruePin =
                  p_Graph.find_output_pin_checked(id, "True");
              const Pin *l_FalsePin =
                  p_Graph.find_output_pin_checked(id, "False");

              const bool l_HasTrueBranch =
                  l_TruePin->id.is_valid() &&
                  p_Graph.is_pin_connected(l_TruePin->id);
              const bool l_HasFalseBranch =
                  l_FalsePin->id.is_valid() &&
                  p_Graph.is_pin_connected(l_FalsePin->id);

              if (l_HasTrueBranch) {
                p_CompileContext.main_code.append("if (");
                compile_input_pin(p_Graph, l_ConditionPin->id,
                                  p_CompileContext);
                p_CompileContext.main_code.append(") {").endl();
                p_Graph.continue_compilation(l_TruePin->id,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();

                if (l_HasFalseBranch) {
                  p_CompileContext.main_code.append("else {").endl();
                  p_Graph.continue_compilation(l_FalsePin->id,
                                               p_CompileContext);
                  p_CompileContext.main_code.append("}").endl();
                }
              } else if (l_HasFalseBranch) {
                p_CompileContext.main_code.append("if (!(");
                compile_input_pin(p_Graph, l_ConditionPin->id,
                                  p_CompileContext);
                p_CompileContext.main_code.append(")) {").endl();
                p_Graph.continue_compilation(l_FalsePin->id,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();
              }
            }
          };

        } // namespace

        Util::Name GlobalFunctionCallNode::get_name() const
        {
          return N(vs_syntax_global_function_call);
        }

        Util::String
        GlobalFunctionCallNode::get_title(const Graph &p_Graph) const
        {
          if (function_name.empty()) {
            return "Call global C++ function";
          }
          return function_info.visual_script_info.title.empty()
                     ? prettify_name(function_info.name)
                     : function_info.visual_script_info.title;
        }

        Util::String GlobalFunctionCallNode::get_subtitle(
            const Graph &p_Graph) const
        {
          if (function_name.empty()) {
            return "Low Function";
          }
          return function_info.visual_script_info.category.c_str();
        }

        Util::String GlobalFunctionCallNode::get_category(
            const Graph &p_Graph) const
        {
          if (function_name.empty()) {
            return "Syntax";
          }
          return function_info.visual_script_info.category.c_str();
        }

        Util::String
        GlobalFunctionCallNode::get_icon(const Graph &p_Graph) const
        {
          if (function_name.empty() ||
              !function_info.visual_script_info.icon_name.is_valid()) {
            return ICON_CI_SYMBOL_METHOD;
          }
          return get_icon_by_name(
              function_info.visual_script_info.icon_name);
        }

        ImU32 GlobalFunctionCallNode::get_color(const Graph &) const
        {
          return g_SyntaxColor;
        }

        void GlobalFunctionCallNode::setup_default_pins(
            Graph &p_Graph, const NodeGraphSchema *p_Schema) const
        {
          const Core::Scripting::FunctionInfo &l_Function =
              get_selected_global_cpp_function(p_Graph, id);

          Pin l_ExecInMetadata = make_execution_pin("Exec");
          p_Graph.add_pin(
              make_input_pin(p_Graph, id, l_ExecInMetadata),
              p_Schema);

          Pin l_ExecOutMetadata = make_execution_pin("Then");
          p_Graph.add_pin(
              make_output_pin(p_Graph, id, l_ExecOutMetadata),
              p_Schema);

          if (l_Function.return_type.kind !=
              Core::Scripting::TypeKind::Void) {
            Pin l_Metadata;
            l_Metadata.display_name = "Return value";
            l_Metadata.type =
                scripting_type_to_pin_type(l_Function.return_type);
            l_Metadata.number_subtype =
                scripting_type_to_number_subtype(
                    l_Function.return_type);
            l_Metadata.string_subtype =
                l_Function.return_type.kind ==
                        Core::Scripting::TypeKind::Name
                    ? StringSubtype::Name
                    : StringSubtype::String;
            l_Metadata.handle_type =
                l_Function.return_type.referenced_type;
            l_Metadata.container_type =
                scripting_container_to_pin_container(
                    l_Function.return_type.container);
            l_Metadata.widget = PinWidget::DefaultValue;
            l_Metadata.show_default_value_when_unlinked = true;

            p_Graph.add_pin(make_output_pin(p_Graph, id, l_Metadata),
                            p_Schema);
          }

          for (const Core::Scripting::FunctionParameterInfo &i_Param :
               l_Function.parameters) {
            if (i_Param.type.direction ==
                Core::Scripting::ParameterDirection::None) {
              continue;
            } else if (i_Param.type.direction ==
                       Core::Scripting::ParameterDirection::InOut) {
              continue;
            } else if (i_Param.type.direction ==
                       Core::Scripting::ParameterDirection::Value) {
              continue;
            } else if (i_Param.type.direction ==
                       Core::Scripting::ParameterDirection::In) {
              Pin i_Metadata =
                  make_pin_metadata_from_function_parameter_info(
                      i_Param, i_Param.name.c_str());

              p_Graph.add_pin(make_input_pin(p_Graph, id, i_Metadata),
                              p_Schema);
            } else if (i_Param.type.direction ==
                       Core::Scripting::ParameterDirection::Out) {
              Pin i_Metadata =
                  make_pin_metadata_from_function_parameter_info(
                      i_Param, i_Param.name.c_str());

              p_Graph.add_pin(
                  make_output_pin(p_Graph, id, i_Metadata), p_Schema);
            }
          }
        }

        void GlobalFunctionCallNode::compile(
            Graph &p_Graph, CompileContext &p_CompileContext) const
        {
        }

        void GlobalFunctionCallNode::serialize(
            Util::Serial::Node &p_Data) const
        {
          p_Data["function_name"] = function_name;
        }

        void GlobalFunctionCallNode::deserialize(
            const Util::Serial::Node &p_Data)
        {
          if (p_Data["function_name"]) {
            function_name =
                p_Data["function_name"].as<Util::String>();
          }
          if (!function_name.empty()) {
            function_info = Core::Scripting::
                find_registered_global_function_checked(
                    function_name);
          }
        }

        namespace {

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_GetVariableNodeType =
              p_Graph.register_node_type<GetVariableNode>();
          const Util::Name l_SetVariableNodeType =
              p_Graph.register_node_type<SetVariableNode>();
          const Util::Name l_ReturnNumberNodeType =
              p_Graph.register_node_type<ReturnNumberNode>();
          const Util::Name l_ReturnBoolNodeType =
              p_Graph.register_node_type<ReturnBoolNode>();
          const Util::Name l_ForNodeType =
              p_Graph.register_node_type<ForNode>();
          const Util::Name l_IfNodeType =
              p_Graph.register_node_type<IfNode>();
          const Util::Name l_GlobalFunctionCallNodeType =
              p_Graph.register_node_type<GlobalFunctionCallNode>();

          NodeSpawnEntry l_ReturnNumberEntry;
          l_ReturnNumberEntry.id = N(vs_spawn_syntax_return_number);
          l_ReturnNumberEntry.category = "Syntax";
          l_ReturnNumberEntry.title = "Return number";
          l_ReturnNumberEntry.subtitle = "Return";
          l_ReturnNumberEntry.search_text = "return number";
          l_ReturnNumberEntry.node_type = l_ReturnNumberNodeType;
          p_Graph.register_spawn_entry(l_ReturnNumberEntry);

          NodeSpawnEntry l_ReturnBoolEntry;
          l_ReturnBoolEntry.id = N(vs_spawn_syntax_return_bool);
          l_ReturnBoolEntry.category = "Syntax";
          l_ReturnBoolEntry.title = "Return bool";
          l_ReturnBoolEntry.subtitle = "Return";
          l_ReturnBoolEntry.search_text = "return bool";
          l_ReturnBoolEntry.node_type = l_ReturnBoolNodeType;
          p_Graph.register_spawn_entry(l_ReturnBoolEntry);

          NodeSpawnEntry l_ForEntry;
          l_ForEntry.id = N(vs_spawn_syntax_for);
          l_ForEntry.category = "Syntax";
          l_ForEntry.title = "For";
          l_ForEntry.subtitle = "Loop";
          l_ForEntry.search_text = "for loop index start end step";
          l_ForEntry.node_type = l_ForNodeType;
          p_Graph.register_spawn_entry(l_ForEntry);

          NodeSpawnEntry l_IfEntry;
          l_IfEntry.id = N(vs_spawn_syntax_if);
          l_IfEntry.category = "Syntax";
          l_IfEntry.title = "If";
          l_IfEntry.subtitle = "Branch";
          l_IfEntry.search_text = "if branch condition true false";
          l_IfEntry.node_type = l_IfNodeType;
          p_Graph.register_spawn_entry(l_IfEntry);

          // TODO: register global C++ functions using
          // register_spawn_entry

          const Util::List<Core::Scripting::FunctionInfo>
              &l_RegisteredCppGlobalFunctions =
                  Core::Scripting::get_registered_global_functions();

          for (const Core::Scripting::FunctionInfo &i_Function :
               l_RegisteredCppGlobalFunctions) {
            if (!i_Function.visual_script_info.exposed) {
              continue;
            }
            NodeSpawnEntry i_Entry;

            Util::String i_EntryId = "vs_spawn_global_cpp_function_";
            i_EntryId += i_Function.bind_namespace +
                         "::" + i_Function.bind_name.c_str();

            i_Entry.id = LOW_NAME(i_EntryId.c_str());
            i_Entry.category =
                i_Function.visual_script_info.category.c_str();
            i_Entry.title = i_Function.visual_script_info.title.empty()
                                ? prettify_name(i_Function.name)
                                : i_Function.visual_script_info.title;
            i_Entry.subtitle =
                i_Function.visual_script_info.category.c_str();
            i_Entry.node_type = l_GlobalFunctionCallNodeType;
            i_Entry.search_text =
                Util::String(i_Function.name.c_str()) + " " + i_Entry.title;

            i_Entry.initialize_node = [i_Function](Graph &,
                                                   Node &p_Node) {
              Util::String l_FnName = i_Function.bind_namespace;
              if (!l_FnName.empty()) {
                l_FnName += "::";
              }
              l_FnName += i_Function.bind_name.c_str();
              auto &l_Node =
                  static_cast<GlobalFunctionCallNode &>(p_Node);
              l_Node.function_name = l_FnName;

              l_Node.function_info = i_Function;
            };

            p_Graph.register_spawn_entry(i_Entry);
          }
        }
      } // namespace SyntaxNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
