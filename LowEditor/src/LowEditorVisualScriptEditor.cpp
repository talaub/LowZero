#include "LowEditorVisualScriptEditor.h"

#include "LowEditor.h"
#include "LowEditorBase.h"
#include "LowEditorGui.h"
#include "LowEditorMetadata.h"
#include "IconsLucide.h"
#include "LowEditorThemes.h"
#include "LowMath.h"
#include "LowUtil.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"

#include <imgui.h>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      bool Document::load_from_path(const Util::String &p_Path)
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
        graph.serialize(l_Root["graph"]);

        Util::Serial::write_yaml_file(p_Path.c_str(), l_Root);
        path = p_Path;
        return true;
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

            if (ImGui::CollapsingHeader(
                    i_Variable.name.c_str(),
                    ImGuiTreeNodeFlags_DefaultOpen)) {
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
            LOW_LOG_DEBUG << "VisualScript compile requested"
                          << LOW_LOG_END;
          }

          ImGui::EndChild();
          ImGui::PopStyleVar(2);
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

        render_toolbar(*this);

        const ImVec2 l_ContentAvail = ImGui::GetContentRegionAvail();
        const float l_SplitterWidth = 6.0f;
        const float l_MaxSidebarWidth =
            LOW_MATH_MIN(max_sidebar_width,
                         LOW_MATH_MAX(min_sidebar_width,
                                      l_ContentAvail.x - 220.0f));
        sidebar_width = LOW_MATH_CLAMP(
            sidebar_width, min_sidebar_width, l_MaxSidebarWidth);

        ImGui::BeginChild("##visual_script_sidebar",
                          ImVec2(sidebar_width, 0.0f), true,
                          ImGuiWindowFlags_NoSavedSettings);
        render_variables_sidebar(*this);
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

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
          sidebar_width = LOW_MATH_CLAMP(
              sidebar_width + ImGui::GetIO().MouseDelta.x,
              min_sidebar_width, l_MaxSidebarWidth);
        }

        ImGui::SameLine(0.0f, 0.0f);

        ImGui::BeginChild("##visual_script_canvas_host",
                          ImVec2(0.0f, 0.0f), false,
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
          document->renderer.render(l_GraphContext);
          document->canvas.end();
        }

        ImGui::EndChild();
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
