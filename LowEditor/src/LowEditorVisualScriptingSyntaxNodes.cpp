#include "LowCoreScripting.h"
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
            const Node *l_Node = p_Graph.find_node(p_NodeId);
            if (!l_Node || l_Node->variable_name.empty()) {
              return nullptr;
            }

            return p_Graph.find_variable(l_Node->variable_name);
          }

          static const Core::Scripting::FunctionInfo &
          get_selected_global_cpp_function(const Graph &p_Graph,
                                           NodeId p_NodeId)
          {
            const Node *l_Node = p_Graph.find_node(p_NodeId);
            if (!l_Node || l_Node->function_name.empty()) {
              _LOW_ASSERT(false);
            }

            return Core::Scripting::
                find_registered_global_function_checked(
                    l_Node->function_name);
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

          struct GetVariableNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_get_variable);
            }

            virtual bool is_compact(const Graph &p_Graph,
                                    NodeId p_NodeId) const
            {
              return true;
            }

            virtual Util::String
            get_compact_title(const Graph &p_Graph,
                              NodeId p_NodeId) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              return l_Variable ? l_Variable->name
                                : Util::String("Get variable");
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              return l_Variable ? l_Variable->name
                                : Util::String("Get variable");
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Variable";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              if (!l_Variable) {
                return;
              }

              Editor::Pin l_ValueOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata =
                  make_pin_metadata_from_variable(*l_Variable, "");
              l_ValueMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_ValueOut, l_ValueMetadata, p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
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

          struct SetVariableNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_set_variable);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              if (!l_Variable) {
                return "Set variable";
              }

              Util::String l_Title = "Set ";
              l_Title += l_Variable->name;
              return l_Title;
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Variable";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
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

              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              if (!l_Variable) {
                return;
              }

              Editor::Pin l_ValueIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata = make_pin_metadata_from_variable(
                  *l_Variable, l_Variable->name);
              p_Graph.add_pin(l_ValueIn, l_ValueMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Variable *l_Variable =
                  get_selected_variable(p_Graph, p_NodeId);
              LOW_ASSERT(
                  l_Variable,
                  "No variable selected for set variable node");
              if (!l_Variable) {
                return;
              }

              const Pin *l_ValuePin = p_Graph.find_input_pin_checked(
                  p_NodeId, l_Variable->name);

              p_CompileContext.main_code.append(
                  l_Variable->name.c_str());
              p_CompileContext.main_code.append(" = ");
              compile_input_pin(p_Graph, p_NodeId, l_ValuePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct ReturnNumberNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_return_number);
            }

            virtual Util::String get_title(const Graph &,
                                           NodeId) const override
            {
              return "Return";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Number";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_CI_ARROW_RIGHT;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
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

              Editor::Pin l_ValueIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata = make_number_pin_metadata("");
              p_Graph.add_pin(l_ValueIn, l_ValueMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ValuePin =
                  p_Graph.find_input_pin_checked(p_NodeId, "");

              p_CompileContext.main_code.append("return ");
              compile_input_pin(p_Graph, p_NodeId, l_ValuePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();
            }
          };

          struct ReturnBoolNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_return_bool);
            }

            virtual Util::String get_title(const Graph &,
                                           NodeId) const override
            {
              return "Return";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Bool";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_CI_ARROW_RIGHT;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
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

              Editor::Pin l_ValueIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata = make_bool_pin_metadata("", false);
              p_Graph.add_pin(l_ValueIn, l_ValueMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ValuePin =
                  p_Graph.find_input_pin_checked(p_NodeId, "");

              p_CompileContext.main_code.append("return ");
              compile_input_pin(p_Graph, p_NodeId, l_ValuePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();
            }
          };

          struct ForNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_for);
            }

            virtual Util::String get_title(const Graph &,
                                           NodeId) const override
            {
              return "for";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Loop";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
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

              Editor::Pin l_StartIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_StartMetadata = make_number_pin_metadata("Start");
              p_Graph.add_pin(l_StartIn, l_StartMetadata, p_Schema);

              Editor::Pin l_EndIn = make_input_pin(p_Graph, p_NodeId);
              Pin l_EndMetadata = make_number_pin_metadata("End");
              p_Graph.add_pin(l_EndIn, l_EndMetadata, p_Schema);

              Editor::Pin l_StepIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_StepMetadata = make_number_pin_metadata("Step");
              l_StepMetadata.default_value = Util::Variant(1.0f);
              p_Graph.add_pin(l_StepIn, l_StepMetadata, p_Schema);

              Editor::Pin l_ThenOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ThenMetadata =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ThenOut, l_ThenMetadata, p_Schema);

              Editor::Pin l_LoopOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_LoopMetadata =
                  make_execution_pin_metadata("Loop");
              p_Graph.add_pin(l_LoopOut, l_LoopMetadata, p_Schema);

              Editor::Pin l_IndexOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_IndexMetadata = make_number_pin_metadata("Index");
              l_IndexMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_IndexOut, l_IndexMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_StartPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Start");
              const Pin *l_EndPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "End");
              const Pin *l_StepPin =
                  p_Graph.find_input_pin_checked(p_NodeId, "Step");
              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              const Pin *l_LoopPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Loop");

              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__for_index")
                  .append((u64)p_NodeId.value);

              p_CompileContext.main_code.append("for (int ")
                  .append(l_IndexVariableName.get())
                  .append(" = ");
              compile_input_pin(p_Graph, p_NodeId, l_StartPin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append("; ")
                  .append(l_IndexVariableName.get())
                  .append(" < ");
              compile_input_pin(p_Graph, p_NodeId, l_EndPin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append("; ")
                  .append(l_IndexVariableName.get())
                  .append(" += ");
              compile_input_pin(p_Graph, p_NodeId, l_StepPin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(") {").endl();
              p_Graph.continue_compilation(l_LoopPin->pin,
                                           p_CompileContext);
              p_CompileContext.main_code.append("}").endl();

              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_Graph;
              (void)p_PinId;
              Util::StringBuilder l_IndexVariableName;
              l_IndexVariableName.append("__for_index")
                  .append((u64)p_NodeId.value);
              p_CompileContext.main_code.append(
                  l_IndexVariableName.get());
            }
          };

          struct IfNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_if);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
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
            get_icon(const Graph &p_Graph,
                     NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return ICON_CI_GIT_BRANCH;
            }

            virtual ImU32 get_color(const Graph &p_Graph,
                                    NodeId p_NodeId) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              return g_SyntaxColor;
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

              Editor::Pin l_ConditionIn =
                  make_input_pin(p_Graph, p_NodeId);
              Pin l_ConditionMetadata =
                  make_bool_pin_metadata("Condition", false);
              p_Graph.add_pin(l_ConditionIn, l_ConditionMetadata,
                              p_Schema);

              Editor::Pin l_TrueOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_TrueOutMetadata =
                  make_execution_pin_metadata("True");
              p_Graph.add_pin(l_TrueOut, l_TrueOutMetadata, p_Schema);

              Editor::Pin l_FalseOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_FalseOutMetadata =
                  make_execution_pin_metadata("False");
              p_Graph.add_pin(l_FalseOut, l_FalseOutMetadata,
                              p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ConditionPin =
                  p_Graph.find_input_pin_checked(p_NodeId,
                                                 "Condition");
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
                compile_input_pin(p_Graph, p_NodeId,
                                  l_ConditionPin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(") {").endl();
                p_Graph.continue_compilation(l_TruePin->pin,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();

                if (l_HasFalseBranch) {
                  p_CompileContext.main_code.append("else {").endl();
                  p_Graph.continue_compilation(l_FalsePin->pin,
                                               p_CompileContext);
                  p_CompileContext.main_code.append("}").endl();
                }
              } else if (l_HasFalseBranch) {
                p_CompileContext.main_code.append("if (!(");
                compile_input_pin(p_Graph, p_NodeId,
                                  l_ConditionPin->pin,
                                  p_CompileContext);
                p_CompileContext.main_code.append(")) {").endl();
                p_Graph.continue_compilation(l_FalsePin->pin,
                                             p_CompileContext);
                p_CompileContext.main_code.append("}").endl();
              }
            }
          };

          struct GlobalFunctionCallNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_global_function_call);
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const Node *l_Node = p_Graph.find_node(p_NodeId);
              if (!l_Node || l_Node->variable_name.empty()) {
                return "Call global C++ function";
              }
              return l_Node->function_name;
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "Call C++ function";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "Syntax";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_CIRCLE;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_SyntaxColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              const Core::Scripting::FunctionInfo &l_Function =
                  get_selected_global_cpp_function(p_Graph, p_NodeId);

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

              for (const Core::Scripting::FunctionParameterInfo
                       &i_Param : l_Function.parameters) {
                if (i_Param.type.direction ==
                    Core::Scripting::ParameterDirection::None) {
                  continue;
                } else if (i_Param.type.direction ==
                           Core::Scripting::ParameterDirection::
                               InOut) {
                  continue;
                } else if (i_Param.type.direction ==
                           Core::Scripting::ParameterDirection::
                               Value) {
                  continue;
                } else if (i_Param.type.direction ==
                           Core::Scripting::ParameterDirection::In) {
                  Editor::Pin i_ParamPin =
                      make_input_pin(p_Graph, p_NodeId);

                  Pin i_Metadata =
                      make_pin_metadata_from_function_parameter_info(
                          i_Param, i_Param.name.c_str());

                  p_Graph.add_pin(i_ParamPin, i_Metadata);
                } else if (i_Param.type.direction ==
                           Core::Scripting::ParameterDirection::Out) {
                  Editor::Pin i_ParamPin =
                      make_output_pin(p_Graph, p_NodeId);

                  Pin i_Metadata =
                      make_pin_metadata_from_function_parameter_info(
                          i_Param, i_Param.name.c_str());

                  p_Graph.add_pin(i_ParamPin, i_Metadata);
                }
              }
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
            }
          };

          static GetVariableNodeClass g_GetVariableNodeClass;
          static SetVariableNodeClass g_SetVariableNodeClass;
          static ReturnNumberNodeClass g_ReturnNumberNodeClass;
          static ReturnBoolNodeClass g_ReturnBoolNodeClass;
          static ForNodeClass g_ForNodeClass;
          static IfNodeClass g_IfNodeClass;
          static GlobalFunctionCallNodeClass
              g_GlobalFunctionCallNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_GetVariableNodeClass);
          p_Graph.register_node_class(g_SetVariableNodeClass);
          p_Graph.register_node_class(g_ReturnNumberNodeClass);
          p_Graph.register_node_class(g_ReturnBoolNodeClass);
          p_Graph.register_node_class(g_ForNodeClass);
          p_Graph.register_node_class(g_IfNodeClass);
          p_Graph.register_node_class(g_GlobalFunctionCallNodeClass);

          NodeSpawnEntry l_ReturnNumberEntry;
          l_ReturnNumberEntry.id = N(vs_spawn_syntax_return_number);
          l_ReturnNumberEntry.category = "Syntax";
          l_ReturnNumberEntry.title = "Return number";
          l_ReturnNumberEntry.subtitle = "Return";
          l_ReturnNumberEntry.search_text = "return number";
          l_ReturnNumberEntry.node_class =
              g_ReturnNumberNodeClass.get_name();
          p_Graph.register_spawn_entry(l_ReturnNumberEntry);

          NodeSpawnEntry l_ReturnBoolEntry;
          l_ReturnBoolEntry.id = N(vs_spawn_syntax_return_bool);
          l_ReturnBoolEntry.category = "Syntax";
          l_ReturnBoolEntry.title = "Return bool";
          l_ReturnBoolEntry.subtitle = "Return";
          l_ReturnBoolEntry.search_text = "return bool";
          l_ReturnBoolEntry.node_class =
              g_ReturnBoolNodeClass.get_name();
          p_Graph.register_spawn_entry(l_ReturnBoolEntry);

          NodeSpawnEntry l_ForEntry;
          l_ForEntry.id = N(vs_spawn_syntax_for);
          l_ForEntry.category = "Syntax";
          l_ForEntry.title = "For";
          l_ForEntry.subtitle = "Loop";
          l_ForEntry.search_text = "for loop index start end step";
          l_ForEntry.node_class = g_ForNodeClass.get_name();
          p_Graph.register_spawn_entry(l_ForEntry);

          NodeSpawnEntry l_IfEntry;
          l_IfEntry.id = N(vs_spawn_syntax_if);
          l_IfEntry.category = "Syntax";
          l_IfEntry.title = "If";
          l_IfEntry.subtitle = "Branch";
          l_IfEntry.search_text = "if branch condition true false";
          l_IfEntry.node_class = g_IfNodeClass.get_name();
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
            i_Entry.title = i_Function.name.c_str();
            i_Entry.subtitle = "C++ Function";
            i_Entry.node_class =
                g_GlobalFunctionCallNodeClass.get_name();
            i_Entry.search_text = i_Function.name.c_str();

            i_Entry.initialize_node = [&i_Function](Graph &,
                                                    Node &p_Node) {
              Util::String l_FnName = i_Function.bind_namespace;
              if (!l_FnName.empty()) {
                l_FnName += "::";
              }
              l_FnName += i_Function.bind_name.c_str();
              p_Node.function_name = l_FnName;
            };

            p_Graph.register_spawn_entry(i_Entry);
          }
        }
      } // namespace SyntaxNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
