#include "LowEditorVisualScriptEditor.h"

#include "LowCoreUiElement.h"
#include "LowEditor.h"
#include "LowEditorBase.h"
#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorMetadata.h"
#include "IconsLucide.h"
#include "LowEditorThemes.h"
#include "LowEditorVisualScriptBuilder.h"
#include "LowEditorVisualScriptNodes.h"
#include "LowMath.h"
#include "LowUtil.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"

#include <imgui.h>
#include "imgui_internal.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace {
        static const char *g_VariableDragDropPayloadType =
            "VS_VARIABLE";
        static const char *g_CanvasDropPopupId =
            "##visual_script_canvas_drop_popup";

        static void render_canvas_watermark(Document &p_Document)
        {
          ImDrawList *l_DrawList = p_Document.canvas.get_draw_list();
          if (!l_DrawList) {
            return;
          }

          ImFont *l_Font = Fonts::UI(48.0f, Fonts::Weight::Bold);
          if (!l_Font) {
            l_Font = ImGui::GetFont();
          }

          const ImVec2 l_Pos =
              ImVec2(p_Document.canvas.get_canvas_min().x + 22.0f,
                     p_Document.canvas.get_canvas_min().y + 18.0f);

          l_DrawList->AddText(l_Font, 48.0f, l_Pos,
                              IM_COL32(180, 185, 200, 32), "FLODE");
        }

        static void register_common_node_libraries(Graph &p_Graph)
        {
          BoolNodes::register_nodes(p_Graph);
          CastNodes::register_nodes(p_Graph);
          DebugNodes::register_nodes(p_Graph);
          HandleNodes::register_nodes(p_Graph);
          MathNodes::register_nodes(p_Graph);
          OperatorNodes::register_nodes(p_Graph);
          SyntaxNodes::register_nodes(p_Graph);
        }

        static ContextRegistry *g_ContextRegistry = nullptr;
      } // namespace

      void Document::apply_context(const ContextDefinition &p_Context)
      {
        initialize();
        context = p_Context.get_name();
        compile_profile = p_Context.get_default_compile_profile();
        p_Context.register_node_libraries(graph);
      }

      void Document::apply_context(
          const ContextDefinition &p_Context,
          const CompileProfileRegistry &p_ProfileRegistry)
      {
        apply_context(p_Context);
        initialize_compile_settings(p_ProfileRegistry);
      }

      bool Document::initialize_compile_settings(
          const CompileProfileRegistry &p_ProfileRegistry)
      {
        if (!compile_profile.is_valid()) {
          return false;
        }

        const CompileProfile *l_Profile =
            p_ProfileRegistry.find_profile(compile_profile);
        if (!l_Profile) {
          LOW_LOG_WARN
              << "Could not initialize compile settings. Compile "
                 "profile was not registered: "
              << compile_profile << LOW_LOG_END;
          return false;
        }

        compile_settings = l_Profile->create_settings();
        return compile_settings != nullptr;
      }

      bool Document::load_from_path(const Util::String &p_Path)
      {
        return load_from_path(p_Path, get_context_registry(),
                              get_compile_profile_registry());
      }

      bool Document::load_from_path(
          const Util::String &p_Path,
          const ContextRegistry &p_ContextRegistry)
      {
        return load_from_path(p_Path, p_ContextRegistry,
                              get_compile_profile_registry());
      }

      bool Document::load_from_path(
          const Util::String &p_Path,
          const ContextRegistry &p_ContextRegistry,
          const CompileProfileRegistry &p_ProfileRegistry)
      {
        if (p_Path.empty()) {
          return false;
        }

        if (!Util::FileIO::file_exists_sync(p_Path.c_str())) {
          LOW_LOG_WARN
              << "Could not load visual script document. File "
                 "does not exist: "
              << p_Path << LOW_LOG_END;
          return false;
        }

        initialize();

        Util::Serial::Node l_Root =
            Util::Serial::load_yaml_file(p_Path.c_str());
        if (l_Root["context"]) {
          const Util::Name l_ContextName =
              l_Root["context"].as<Util::Name>();
          const ContextDefinition *l_Context =
              p_ContextRegistry.find_context(l_ContextName);
          if (l_Context) {
            apply_context(*l_Context, p_ProfileRegistry);
          } else {
            context = l_ContextName;
          }
        }
        if (l_Root["output_path"]) {
          output_path = l_Root["output_path"].as<Util::String>();
        }

        if (l_Root["compile_profile"]) {
          compile_profile =
              l_Root["compile_profile"].as<Util::Name>();
        } else if (!compile_profile.is_valid()) {
          compile_profile = N(vs_compile_default);
        }

        if (!compile_settings ||
            compile_settings->get_profile_name() != compile_profile) {
          initialize_compile_settings(p_ProfileRegistry);
        }

        if (compile_settings && l_Root["compile_settings"]) {
          compile_settings->deserialize(l_Root["compile_settings"]);
        }

        if (l_Root["graph"]) {
          graph.deserialize(l_Root["graph"]);
        } else {
          graph.deserialize(l_Root);
        }

        path = p_Path;
        return true;
      }

      bool Document::save()
      {
        if (path.empty()) {
          return false;
        }

        return save_as(path);
      }

      bool Document::save_as(const Util::String &p_Path)
      {
        if (p_Path.empty()) {
          return false;
        }

        Util::Serial::Node l_Root;
        l_Root["type"] = "LowVisualScript";
        l_Root["version"] = Util::U64Id{1ull};
        if (context.is_valid()) {
          l_Root["context"] = context;
        }
        if (!output_path.empty()) {
          l_Root["output_path"] = output_path;
        }
        l_Root["compile_profile"] = compile_profile;
        if (compile_settings) {
          compile_settings->serialize(l_Root["compile_settings"]);
        }
        graph.serialize(l_Root["graph"]);

        Util::Serial::write_yaml_file(p_Path.c_str(), l_Root);
        path = p_Path;
        return true;
      }

      bool Document::compile(CompileContext &p_Context)
      {
        return compile(p_Context, get_compile_profile_registry());
      }

      bool Document::compile(
          CompileContext &p_Context,
          const CompileProfileRegistry &p_ProfileRegistry)
      {
        const CompileProfile *l_Profile =
            p_ProfileRegistry.find_profile(compile_profile);
        if (!l_Profile) {
          LOW_LOG_WARN
              << "Could not compile visual script document. Compile "
                 "profile was not registered: "
              << compile_profile << LOW_LOG_END;
          return false;
        }

        l_Profile->compile(*this, p_Context);
        return true;
      }

      bool Document::compile_and_write()
      {
        return compile_and_write(get_compile_profile_registry());
      }

      bool Document::compile_and_write(
          const CompileProfileRegistry &p_ProfileRegistry)
      {
        if (output_path.empty()) {
          LOW_LOG_WARN << "Could not compile visual script document. "
                          "Output path "
                          "is empty."
                       << LOW_LOG_END;
          return false;
        }

        CompileContext l_Context;
        if (!compile(l_Context, p_ProfileRegistry)) {
          return false;
        }

        LOW_LOG_DEBUG << "CODE: " << l_Context.main_code.get()
                      << LOW_LOG_END;

        Util::FileIO::File l_File = Util::FileIO::open(
            output_path.c_str(), Util::FileIO::FileMode::WRITE);
        Util::FileIO::write_sync(l_File, l_Context.main_code.get());
        Util::FileIO::close(l_File);

        return true;
      }

      void
      ContextRegistry::register_context(ContextDefinition &p_Context)
      {
        contexts[p_Context.get_name()] = &p_Context;
      }

      ContextDefinition *
      ContextRegistry::find_context(Util::Name p_ContextName)
      {
        auto it = contexts.find(p_ContextName);
        if (it == contexts.end()) {
          return nullptr;
        }
        return it->second;
      }

      const ContextDefinition *
      ContextRegistry::find_context(Util::Name p_ContextName) const
      {
        auto it = contexts.find(p_ContextName);
        if (it == contexts.end()) {
          return nullptr;
        }
        return it->second;
      }

      Util::Name DefaultContextDefinition::get_name() const
      {
        return N(vs_context_default);
      }

      Util::Name
      DefaultContextDefinition::get_default_compile_profile() const
      {
        return N(vs_compile_default);
      }

      void DefaultContextDefinition::register_node_libraries(
          Graph &p_Graph) const
      {
        register_common_node_libraries(p_Graph);
      }

      Util::Name UiControllerContextDefinition::get_name() const
      {
        return N(vs_context_ui_controller);
      }

      Util::Name
      UiControllerContextDefinition::get_default_compile_profile()
          const
      {
        return N(cp_ui_controller);
      }

      void UiControllerContextDefinition::register_node_libraries(
          Graph &p_Graph) const
      {
        register_common_node_libraries(p_Graph);
        UiControllerNodes::register_nodes(p_Graph);
      }

      void UiControllerContextDefinition::build_default_template(
          Document &p_Document) const
      {
        GraphBuilder l_Builder(p_Document.graph, &p_Document.schema);
      }

      void
      UiControllerContextDefinition::get_canvas_drop_payload_types(
          Util::List<Util::String> &p_Types) const
      {
        p_Types.push_back("UI_ELEMENT");
      }

      void UiControllerContextDefinition::get_canvas_drop_actions(
          Document &p_Document, const char *p_PayloadType,
          const void *p_Data, u32 p_DataSize,
          const Math::Vector2 &p_CanvasPosition,
          Util::List<CanvasDropAction> &p_Actions) const
      {
        (void)p_Document;
        (void)p_Data;
        (void)p_DataSize;
        (void)p_CanvasPosition;

        if (!p_PayloadType ||
            strcmp(p_PayloadType, "UI_ELEMENT") != 0) {
          return;
        }

        Core::UI::Element l_Element =
            *static_cast<const Core::UI::Element *>(p_Data);

        auto l_SpawnElementEventNode =
            [&p_Document,
             p_CanvasPosition](Core::UI::Element p_Element,
                               UiControllerNodes::InteractionType
                                   p_InteractionType) {
              NodeGraphMutationResult<Low::Editor::Node> l_Result =
                  p_Document.graph.create_node(
                      N(vs_uicontroller_element_event),
                      p_CanvasPosition, &p_Document.schema);
              if (!l_Result.succeeded() || !l_Result.value) {
                return;
              }

              UiControllerNodes::ElementEventNodeData *l_Data =
                  p_Document.graph.get_node_user_data<
                      UiControllerNodes::ElementEventNodeData>(
                      l_Result.value->id);
              if (!l_Data) {
                return;
              }

              l_Data->element_name = p_Element.get_name();
              l_Data->element_local_id = p_Element.get_local_id();
              l_Data->interaction_type = p_InteractionType;
              p_Document.graph.refresh_node_display_metadata(
                  l_Result.value->id);
            };

        auto l_AddElementAction =
            [&p_Actions, &l_SpawnElementEventNode,
             l_Element](const char *p_Label,
                        UiControllerNodes::InteractionType
                            p_InteractionType) {
              CanvasDropAction l_Action;
              l_Action.label = p_Label;
              l_Action.execute = [l_SpawnElementEventNode, l_Element,
                                  p_InteractionType]() {
                l_SpawnElementEventNode(l_Element, p_InteractionType);
              };
              p_Actions.push_back(l_Action);
            };

        l_AddElementAction("React to click",
                           UiControllerNodes::InteractionType::Click);
        l_AddElementAction(
            "React to mouse enter",
            UiControllerNodes::InteractionType::MouseEnter);
        l_AddElementAction(
            "React to mouse exit",
            UiControllerNodes::InteractionType::MouseExit);
      }

      void
      register_builtin_contexts(ContextRegistry &p_ContextRegistry)
      {
        static DefaultContextDefinition g_DefaultContext;
        static UiControllerContextDefinition g_UiControllerContext;

        p_ContextRegistry.register_context(g_DefaultContext);
        p_ContextRegistry.register_context(g_UiControllerContext);
      }

      ContextRegistry &get_context_registry()
      {
        if (!g_ContextRegistry) {
          g_ContextRegistry = new ContextRegistry();
          register_builtin_contexts(*g_ContextRegistry);
        }
        return *g_ContextRegistry;
      }

      CompileProfileRegistry &get_compile_profile_registry()
      {
        static CompileProfileRegistry *g_CompileProfileRegistry =
            nullptr;
        if (!g_CompileProfileRegistry) {
          g_CompileProfileRegistry = new CompileProfileRegistry();
          register_builtin_compile_profiles(
              *g_CompileProfileRegistry);
        }
        return *g_CompileProfileRegistry;
      }

      namespace {
        static const char *get_pin_type_label(PinType p_Type)
        {
          switch (p_Type) {
          case PinType::Bool:
            return "Bool";
          case PinType::Number:
            return "Number";
          case PinType::String:
            return "String";
          case PinType::Handle:
            return "Handle";
          case PinType::Vector2:
            return "Vector2";
          case PinType::Vector3:
            return "Vector3";
          case PinType::Vector4:
            return "Vector4";
          case PinType::Quaternion:
            return "Quaternion";
          default:
            return "Unsupported";
          }
        }

        static Pin
        make_variable_pin_metadata(const Variable &p_Variable)
        {
          Pin l_Pin;
          l_Pin.display_name = p_Variable.name;
          l_Pin.type = p_Variable.type;
          l_Pin.string_subtype = p_Variable.string_subtype;
          l_Pin.number_subtype = p_Variable.number_subtype;
          l_Pin.handle_type = p_Variable.handle_type;
          l_Pin.container_type = p_Variable.container_type;
          l_Pin.default_value = p_Variable.default_value;
          return l_Pin;
        }

        static Util::Variant make_default_variable_value(
            PinType p_Type, NumberSubtype p_NumberSubtype,
            StringSubtype p_StringSubtype,
            const Util::TypeIdentifier &p_HandleType)
        {
          switch (p_Type) {
          case PinType::Bool:
            return default_value_for_pin(make_bool_pin_metadata(""));
          case PinType::Number:
            return default_value_for_pin(
                make_number_pin_metadata("", p_NumberSubtype));
          case PinType::String:
            return default_value_for_pin(
                make_string_pin_metadata("", p_StringSubtype));
          case PinType::Handle:
            return default_value_for_pin(
                make_handle_pin_metadata("", p_HandleType));
          case PinType::Vector2: {
            Pin l_Pin;
            l_Pin.type = PinType::Vector2;
            return default_value_for_pin(l_Pin);
          }
          case PinType::Vector3: {
            Pin l_Pin;
            l_Pin.type = PinType::Vector3;
            return default_value_for_pin(l_Pin);
          }
          case PinType::Vector4: {
            Pin l_Pin;
            l_Pin.type = PinType::Vector4;
            return default_value_for_pin(l_Pin);
          }
          case PinType::Quaternion: {
            Pin l_Pin;
            l_Pin.type = PinType::Quaternion;
            return default_value_for_pin(l_Pin);
          }
          default:
            return Util::Variant(Util::String(""));
          }
        }

        static bool
        render_variable_default_editor(Variable &p_Variable)
        {
          switch (p_Variable.type) {
          case PinType::Bool: {
            return Gui::ToggleButtonSimple(
                "##default", (bool *)&p_Variable.default_value);
          }
          case PinType::Number:
            return Base::VariantEdit("##default",
                                     p_Variable.default_value, true);
          case PinType::String:
            if (p_Variable.string_subtype == StringSubtype::Name) {
              Util::Name l_Name = p_Variable.default_value.as_name();
              if (Base::NameEdit("##default", &l_Name)) {
                p_Variable.default_value = Util::Variant(l_Name);
                return true;
              }
              return false;
            } else {
              Util::String l_String =
                  p_Variable.default_value.as_string();
              if (Base::StringEdit("##default", &l_String)) {
                p_Variable.default_value = Util::Variant(l_String);
                return true;
              }
              return false;
            }
          case PinType::Quaternion: {
            Math::Quaternion l_Value = p_Variable.default_value;
            if (Gui::EulerEdit(l_Value)) {
              p_Variable.default_value = Util::Variant(l_Value);
              return true;
            }
            return false;
          }
          case PinType::Vector4: {
            Math::Vector4 l_Value = p_Variable.default_value;
            if (Gui::Vector4Edit(l_Value)) {
              p_Variable.default_value = Util::Variant(l_Value);
              return true;
            }
            return false;
          }
          case PinType::Vector2: {
            Math::Vector2 l_Value = p_Variable.default_value;
            if (Gui::Vector2Edit(l_Value)) {
              p_Variable.default_value = Util::Variant(l_Value);
              return true;
            }
            return false;
          }
          case PinType::Vector3:
            return Base::VariantEdit("##default",
                                     p_Variable.default_value, true);
          default:
            ImGui::TextDisabled("No default editor");
            return false;
          }
        }

        static void render_add_variable_form(Editor &p_Editor)
        {
          static const PinType l_TypeOptions[] = {
              PinType::Bool,    PinType::Number,    PinType::String,
              PinType::Handle,  PinType::Vector2,   PinType::Vector3,
              PinType::Vector4, PinType::Quaternion};

          Gui::InputText("##new_variable_name",
                         p_Editor.new_variable_name, 128,
                         ImGuiInputTextFlags_EnterReturnsTrue);

          if (ImGui::BeginCombo(
                  "##new_variable_type",
                  get_pin_type_label(p_Editor.new_variable_type))) {
            for (PinType i_Type : l_TypeOptions) {
              const bool l_Selected =
                  p_Editor.new_variable_type == i_Type;
              if (ImGui::Selectable(get_pin_type_label(i_Type),
                                    l_Selected)) {
                p_Editor.new_variable_type = i_Type;
              }
              if (l_Selected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }

          if (p_Editor.new_variable_type == PinType::Number) {
            if (ImGui::BeginCombo(
                    "##new_variable_number_subtype",
                    p_Editor.new_variable_number_subtype ==
                            NumberSubtype::Float
                        ? "Float"
                    : p_Editor.new_variable_number_subtype ==
                            NumberSubtype::Int32
                        ? "Int32"
                    : p_Editor.new_variable_number_subtype ==
                            NumberSubtype::UInt32
                        ? "UInt32"
                        : "UInt64")) {
              const NumberSubtype l_Subtypes[] = {
                  NumberSubtype::Float, NumberSubtype::Int32,
                  NumberSubtype::UInt32, NumberSubtype::UInt64};
              for (NumberSubtype i_Subtype : l_Subtypes) {
                const char *l_Label =
                    i_Subtype == NumberSubtype::Float    ? "Float"
                    : i_Subtype == NumberSubtype::Int32  ? "Int32"
                    : i_Subtype == NumberSubtype::UInt32 ? "UInt32"
                                                         : "UInt64";
                const bool l_Selected =
                    p_Editor.new_variable_number_subtype == i_Subtype;
                if (ImGui::Selectable(l_Label, l_Selected)) {
                  p_Editor.new_variable_number_subtype = i_Subtype;
                }
                if (l_Selected) {
                  ImGui::SetItemDefaultFocus();
                }
              }
              ImGui::EndCombo();
            }
          } else if (p_Editor.new_variable_type == PinType::String) {
            if (ImGui::BeginCombo(
                    "##new_variable_string_subtype",
                    p_Editor.new_variable_string_subtype ==
                            StringSubtype::Name
                        ? "Name"
                        : "String")) {
              const StringSubtype l_Subtypes[] = {
                  StringSubtype::String, StringSubtype::Name};
              for (StringSubtype i_Subtype : l_Subtypes) {
                const char *l_Label = i_Subtype == StringSubtype::Name
                                          ? "Name"
                                          : "String";
                const bool l_Selected =
                    p_Editor.new_variable_string_subtype == i_Subtype;
                if (ImGui::Selectable(l_Label, l_Selected)) {
                  p_Editor.new_variable_string_subtype = i_Subtype;
                }
                if (l_Selected) {
                  ImGui::SetItemDefaultFocus();
                }
              }
              ImGui::EndCombo();
            }
          } else if (p_Editor.new_variable_type == PinType::Handle) {
            Util::String l_CurrentTypeLabel =
                ((u64)p_Editor.new_variable_handle_type) != 0
                    ? (Util::String)p_Editor.new_variable_handle_type
                    : Util::String("Select handle type");
            if (ImGui::BeginCombo("##new_variable_handle_type",
                                  l_CurrentTypeLabel.c_str())) {
              for (auto &i_TypeEntry : get_type_metadata()) {
                const TypeMetadata &i_Type = i_TypeEntry.second;
                const bool l_Selected =
                    p_Editor.new_variable_handle_type ==
                    i_Type.identifier;
                if (ImGui::Selectable(i_Type.friendlyName.c_str(),
                                      l_Selected)) {
                  p_Editor.new_variable_handle_type =
                      i_Type.identifier;
                }
                if (l_Selected) {
                  ImGui::SetItemDefaultFocus();
                }
              }
              ImGui::EndCombo();
            }
          }

          const bool l_AddPressed = Gui::AddButton("Add Variable");
          if (!l_AddPressed) {
            return;
          }

          if (!p_Editor.document) {
            return;
          }

          Util::String l_Name = p_Editor.new_variable_name;
          if (l_Name.empty()) {
            return;
          }

          Variable l_Variable;
          l_Variable.name = l_Name;
          l_Variable.type = p_Editor.new_variable_type;
          l_Variable.number_subtype =
              p_Editor.new_variable_number_subtype;
          l_Variable.string_subtype =
              p_Editor.new_variable_string_subtype;
          l_Variable.handle_type = p_Editor.new_variable_handle_type;
          l_Variable.default_value = make_default_variable_value(
              l_Variable.type, l_Variable.number_subtype,
              l_Variable.string_subtype, l_Variable.handle_type);

          if (p_Editor.document->graph.add_variable(l_Variable)) {
            p_Editor.new_variable_name[0] = '\0';
          }
        }

        static void render_variables_sidebar(Editor &p_Editor)
        {
          if (!p_Editor.document) {
            return;
          }

          ImGui::TextUnformatted("Variables");
          ImGui::Separator();
          ImGui::Dummy(ImVec2(0.0f, 6.0f));

          render_add_variable_form(p_Editor);

          ImGui::Dummy(ImVec2(0.0f, 8.0f));
          ImGui::Separator();

          Graph &l_Graph = p_Editor.document->graph;
          Util::String l_DeleteVariable;

          for (u32 i = 0; i < l_Graph.variables.size(); ++i) {
            Variable &i_Variable = l_Graph.variables[i];
            ImGui::PushID((int)i);

            const bool l_HeaderOpen = ImGui::CollapsingHeader(
                i_Variable.name.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen);

            if (ImGui::BeginDragDropSource(
                    ImGuiDragDropFlags_SourceAllowNullID)) {
              ImGui::SetDragDropPayload(g_VariableDragDropPayloadType,
                                        i_Variable.name.c_str(),
                                        i_Variable.name.size() + 1);
              ImGui::Text("Variable: %s", i_Variable.name.c_str());
              ImGui::EndDragDropSource();
            }

            if (l_HeaderOpen) {
              ImGui::TextDisabled(
                  "%s", get_pin_type_label(i_Variable.type));

              if (i_Variable.type == PinType::Number) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)",
                                    i_Variable.number_subtype ==
                                            NumberSubtype::Float
                                        ? "Float"
                                    : i_Variable.number_subtype ==
                                            NumberSubtype::Int32
                                        ? "Int32"
                                    : i_Variable.number_subtype ==
                                            NumberSubtype::UInt32
                                        ? "UInt32"
                                        : "UInt64");
              } else if (i_Variable.type == PinType::String) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)",
                                    i_Variable.string_subtype ==
                                            StringSubtype::Name
                                        ? "Name"
                                        : "String");
              } else if (i_Variable.type == PinType::Handle &&
                         ((u64)i_Variable.handle_type) != 0) {
                ImGui::SameLine();
                ImGui::TextDisabled(
                    "(%s)",
                    ((Util::String)i_Variable.handle_type).c_str());
              }

              ImGui::PushItemWidth(-1.0f);
              render_variable_default_editor(i_Variable);
              ImGui::PopItemWidth();

              if (Gui::DeleteButton()) {
                l_DeleteVariable = i_Variable.name;
              }
            }

            ImGui::PopID();
          }

          if (!l_DeleteVariable.empty()) {
            l_Graph.remove_variable(l_DeleteVariable);
          }
        }

        static void render_toolbar(Editor &p_Editor)
        {
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                              ImVec2(8.0f, 6.0f));
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                              ImVec2(6.0f, 0.0f));
          ImGui::BeginChild("##visual_script_toolbar",
                            ImVec2(0.0f, 38.0f), false,
                            ImGuiWindowFlags_NoScrollbar |
                                ImGuiWindowFlags_NoScrollWithMouse |
                                ImGuiWindowFlags_NoSavedSettings);

          if (Gui::SaveButton()) {
            p_Editor.document->save();
          }

          ImGui::SameLine();
          if (Gui::Button("Compile", false, ICON_LC_PLAY,
                          theme_get_current().play)) {
            p_Editor.get_document()->compile_and_write();
          }

          ImGui::EndChild();
          ImGui::PopStyleVar(2);
        }

        static void append_variable_drop_actions(
            Editor &p_Editor, const void *p_Data, u32 p_DataSize,
            const Math::Vector2 &p_CanvasPosition,
            Util::List<CanvasDropAction> &p_Actions)
        {
          if (!p_Editor.document || !p_Data || !p_DataSize) {
            return;
          }

          const char *l_NameData = static_cast<const char *>(p_Data);
          Util::String l_VariableName(l_NameData);
          if (l_VariableName.empty() ||
              !p_Editor.document->graph.find_variable(
                  l_VariableName)) {
            return;
          }

          auto l_SpawnVariableNode =
              [&p_Editor, l_VariableName,
               p_CanvasPosition](Util::Name p_NodeClass,
                                 const Util::String &p_Title) {
                Low::Editor::Node l_Node;
                l_Node.id =
                    p_Editor.document->graph.allocate_node_id();
                l_Node.position = p_CanvasPosition;

                Node l_Metadata;
                l_Metadata.node = l_Node.id;
                l_Metadata.node_class = p_NodeClass;
                l_Metadata.variable_name = l_VariableName;
                l_Metadata.title = p_Title;
                l_Metadata.subtitle = "Variable";
                l_Metadata.category = "Syntax";

                NodeGraphMutationResult<Low::Editor::Node> l_Result =
                    p_Editor.document->graph.add_node(
                        l_Node, l_Metadata,
                        &p_Editor.document->schema);
                if (!l_Result.succeeded()) {
                  return;
                }

                const NodeClass *l_NodeClass =
                    p_Editor.document->graph.find_node_class(
                        p_NodeClass);
                if (l_NodeClass) {
                  l_NodeClass->setup_default_pins(
                      p_Editor.document->graph, l_Node.id,
                      &p_Editor.document->schema);
                }
              };

          if (p_Editor.document->graph.find_node_class(
                  N(vs_syntax_get_variable))) {
            CanvasDropAction l_Action;
            l_Action.label = Util::String("Get ") + l_VariableName;
            l_Action.execute = [l_SpawnVariableNode,
                                l_VariableName]() {
              l_SpawnVariableNode(N(vs_syntax_get_variable),
                                  Util::String("Get ") +
                                      l_VariableName);
            };
            p_Actions.push_back(l_Action);
          }

          if (p_Editor.document->graph.find_node_class(
                  N(vs_syntax_set_variable))) {
            CanvasDropAction l_Action;
            l_Action.label = Util::String("Set ") + l_VariableName;
            l_Action.execute = [l_SpawnVariableNode,
                                l_VariableName]() {
              l_SpawnVariableNode(N(vs_syntax_set_variable),
                                  Util::String("Set ") +
                                      l_VariableName);
            };
            p_Actions.push_back(l_Action);
          }
        }

        static void append_canvas_drop_actions(
            Editor &p_Editor, const char *p_PayloadType,
            const void *p_Data, u32 p_DataSize,
            const Math::Vector2 &p_CanvasPosition,
            Util::List<CanvasDropAction> &p_Actions)
        {
          if (!p_Editor.document || !p_PayloadType) {
            return;
          }

          if (strcmp(p_PayloadType, g_VariableDragDropPayloadType) ==
              0) {
            append_variable_drop_actions(p_Editor, p_Data, p_DataSize,
                                         p_CanvasPosition, p_Actions);
          }

          const ContextDefinition *l_Context =
              p_Editor.document->context.is_valid()
                  ? get_context_registry().find_context(
                        p_Editor.document->context)
                  : nullptr;
          if (l_Context) {
            l_Context->get_canvas_drop_actions(
                *p_Editor.document, p_PayloadType, p_Data, p_DataSize,
                p_CanvasPosition, p_Actions);
          }
        }

        static void render_canvas_drop_popup(Editor &p_Editor)
        {
          if (ImGui::BeginPopup(g_CanvasDropPopupId)) {
            for (u32 i = 0;
                 i < p_Editor.pending_canvas_drop_actions.size();
                 ++i) {
              CanvasDropAction &i_Action =
                  p_Editor.pending_canvas_drop_actions[i];
              if (!i_Action.is_valid()) {
                continue;
              }

              if (ImGui::MenuItem(i_Action.label.c_str())) {
                i_Action.execute();
                p_Editor.pending_canvas_drop_actions.clear();
                ImGui::CloseCurrentPopup();
                break;
              }
            }
            ImGui::EndPopup();
          } else if (!ImGui::IsPopupOpen(g_CanvasDropPopupId) &&
                     !p_Editor.pending_canvas_drop_actions.empty()) {
            p_Editor.pending_canvas_drop_actions.clear();
          }
        }
      } // namespace

      void Editor::render(const float p_Delta)
      {
        (void)p_Delta;
        render("Graph", Math::Vector2(0.0f, 0.0f));
      }

      void Editor::render(const char *p_Label,
                          const Math::Vector2 &p_Size)
      {
        if (!document) {
          return;
        }

        if (!embedded) {
          render_toolbar(*this);
        }

        const ImVec2 l_ContentAvail = ImGui::GetContentRegionAvail();
        const float l_SplitterWidth = 6.0f;
        const float l_MaxSidebarWidth =
            LOW_MATH_MIN(max_sidebar_width,
                         LOW_MATH_MAX(min_sidebar_width,
                                      l_ContentAvail.x - 220.0f));
        sidebar_width = LOW_MATH_CLAMP(
            sidebar_width, min_sidebar_width, l_MaxSidebarWidth);
        const float l_CanvasWidth = LOW_MATH_MAX(
            0.0f, l_ContentAvail.x - sidebar_width - l_SplitterWidth);

        auto l_RenderSidebar = [this]() {
          ImGui::BeginChild("##visual_script_sidebar",
                            ImVec2(sidebar_width, 0.0f), true,
                            ImGuiWindowFlags_NoSavedSettings);
          render_variables_sidebar(*this);
          ImGui::EndChild();
        };

        auto l_RenderSplitter = [this, l_SplitterWidth,
                                 l_ContentAvail,
                                 l_MaxSidebarWidth]() {
          ImGui::PushStyleColor(ImGuiCol_Button,
                                IM_COL32(55, 58, 68, 255));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                IM_COL32(78, 82, 95, 255));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                IM_COL32(92, 97, 112, 255));
          ImGui::Button("##visual_script_splitter",
                        ImVec2(l_SplitterWidth, l_ContentAvail.y));
          ImGui::PopStyleColor(3);

          if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
          }

          if (ImGui::IsItemActive()) {
            const float l_Delta = sidebar_left
                                      ? ImGui::GetIO().MouseDelta.x
                                      : -ImGui::GetIO().MouseDelta.x;
            sidebar_width =
                LOW_MATH_CLAMP(sidebar_width + l_Delta,
                               min_sidebar_width, l_MaxSidebarWidth);
          }
        };

        auto l_RenderCanvas = [this, p_Label, p_Size,
                               l_CanvasWidth]() {
          ImGui::BeginChild("##visual_script_canvas_host",
                            ImVec2(l_CanvasWidth, 0.0f), false,
                            ImGuiWindowFlags_NoSavedSettings);

          if (document->canvas.begin(p_Label, p_Size)) {
            NodeGraphEditorContext l_GraphContext{
                document->graph.graph,
                document->canvas,
                &document->schema,
                &document->state,
                document->canvas.get_draw_list(),
                document->canvas.get_canvas_origin(),
                document->canvas.get_canvas_min(),
                document->canvas.get_canvas_max()};
            render_canvas_watermark(*document);
            document->controller.render(l_GraphContext,
                                        document->renderer);
            document->canvas.end();
          }

          Util::List<Util::String> l_PayloadTypes;
          l_PayloadTypes.push_back(g_VariableDragDropPayloadType);

          const ContextDefinition *l_Context =
              document->context.is_valid()
                  ? get_context_registry().find_context(
                        document->context)
                  : nullptr;
          if (l_Context) {
            l_Context->get_canvas_drop_payload_types(l_PayloadTypes);
          }

          const ImRect l_DropRect(document->canvas.get_canvas_min(),
                                  document->canvas.get_canvas_max());
          if (ImGui::BeginDragDropTargetCustom(
                  l_DropRect,
                  ImGui::GetID(
                      "##visual_script_canvas_drop_target"))) {
            for (u32 i = 0; i < l_PayloadTypes.size(); ++i) {
              const Util::String &i_PayloadType = l_PayloadTypes[i];
              if (const ImGuiPayload *l_Payload =
                      ImGui::AcceptDragDropPayload(
                          i_PayloadType.c_str())) {
                if (!l_Payload->IsDelivery()) {
                  continue;
                }

                pending_canvas_drop_actions.clear();
                append_canvas_drop_actions(
                    *this, i_PayloadType.c_str(), l_Payload->Data,
                    l_Payload->DataSize,
                    document->canvas.screen_to_canvas(
                        Math::Vector2(ImGui::GetMousePos().x,
                                      ImGui::GetMousePos().y),
                        Math::Vector2(
                            document->canvas.get_canvas_origin().x,
                            document->canvas.get_canvas_origin().y)),
                    pending_canvas_drop_actions);

                if (!pending_canvas_drop_actions.empty()) {
                  ImGui::OpenPopup(g_CanvasDropPopupId);
                }
                break;
              }
            }

            ImGui::EndDragDropTarget();
          }

          render_canvas_drop_popup(*this);

          ImGui::EndChild();
        };

        if (sidebar_left) {
          l_RenderSidebar();
          ImGui::SameLine(0.0f, 0.0f);
          l_RenderSplitter();
          ImGui::SameLine(0.0f, 0.0f);
          l_RenderCanvas();
        } else {
          l_RenderCanvas();
          ImGui::SameLine(0.0f, 0.0f);
          l_RenderSplitter();
          ImGui::SameLine(0.0f, 0.0f);
          l_RenderSidebar();
        }
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
