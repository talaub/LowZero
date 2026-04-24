#include "LowEditorVisualScripting.h"
#include "LowEditorVisualScriptEditor.h"

#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"

#include <cfloat>
#include <cstring>
#include <cstdio>
#include <imgui.h>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace {
        static bool pin_type_is_execution(PinType p_PinType)
        {
          return p_PinType == PinType::Execution;
        }

        static bool pin_type_is_dynamic(PinType p_PinType)
        {
          return p_PinType == PinType::Dynamic;
        }

        static bool pin_types_are_compatible(PinType p_Left,
                                             PinType p_Right)
        {
          if (p_Left == p_Right) {
            return true;
          }

          if (pin_type_is_dynamic(p_Left) ||
              pin_type_is_dynamic(p_Right)) {
            return true;
          }

          return false;
        }

        static bool pin_metadata_are_compatible(const Pin &p_Left,
                                                const Pin &p_Right)
        {
          if (p_Left.container_type != p_Right.container_type) {
            return false;
          }

          if (pin_type_is_dynamic(p_Left.type) ||
              pin_type_is_dynamic(p_Right.type)) {
            return true;
          }

          if (p_Left.type != p_Right.type) {
            return false;
          }

          if (p_Left.type == PinType::String) {
            return true;
          }

          if (p_Left.type == PinType::Number) {
            return true;
          }

          if (p_Left.type == PinType::Handle) {
            return ((u64)p_Left.handle_type ==
                    (u64)p_Right.handle_type) ||
                   !((u64)p_Left.handle_type) ||
                   !((u64)p_Right.handle_type);
          }

          return true;
        }

        static ImU32 get_pin_color(const Pin *p_PinMetadata)
        {
          if (!p_PinMetadata) {
            return IM_COL32(180, 180, 190, 255);
          }

          switch (p_PinMetadata->type) {
          case PinType::Execution:
            return IM_COL32(255, 255, 255, 255);
          case PinType::Bool:
            return IM_COL32(220, 48, 48, 255);
          case PinType::Number:
            return IM_COL32(147, 226, 74, 255);
          case PinType::String:
            return IM_COL32(124, 21, 153, 255);
          case PinType::Handle:
            return IM_COL32(51, 150, 215, 255);
          case PinType::Vector2:
            return IM_COL32(68, 150, 126, 255);
          case PinType::Vector3:
            return IM_COL32(68, 201, 156, 255);
          case PinType::Vector4:
          case PinType::Quaternion:
            return IM_COL32(76, 200, 196, 255);
          case PinType::Dynamic:
            return IM_COL32(120, 120, 120, 255);
          }

          return IM_COL32(180, 180, 190, 255);
        }

        static void add_scaled_text(ImDrawList *p_DrawList,
                                    ImFont *p_Font, float p_Size,
                                    const ImVec2 &p_Position,
                                    ImU32 p_Color, const char *p_Text)
        {
          if (!p_DrawList || !p_Text || !p_Text[0]) {
            return;
          }

          ImFont *l_Font = p_Font ? p_Font : ImGui::GetFont();
          const float l_Size =
              p_Size > 0.0f ? p_Size : ImGui::GetFontSize();
          p_DrawList->AddText(l_Font, l_Size, p_Position, p_Color,
                              p_Text);
        }

        static ImVec2 calc_scaled_text_size(ImFont *p_Font,
                                            float p_Size,
                                            const char *p_Text)
        {
          if (!p_Text || !p_Text[0]) {
            return ImVec2(0.0f, 0.0f);
          }

          ImFont *l_Font = p_Font ? p_Font : ImGui::GetFont();
          const float l_Size =
              p_Size > 0.0f ? p_Size : ImGui::GetFontSize();
          return l_Font->CalcTextSizeA(l_Size, FLT_MAX, 0.0f, p_Text);
        }

        static void draw_execution_pin(ImDrawList *p_DrawList,
                                       const ImVec2 &p_Anchor,
                                       bool p_IsInput, float p_Size,
                                       ImU32 p_Color, bool p_Hovered)
        {
          const float l_HalfWidth = p_Size * 0.65f;
          const float l_HalfHeight = p_Size * 0.72f;
          const float l_HoverExpand =
              p_Hovered ? p_Size * 0.18f : 0.0f;

          ImVec2 l_Left =
              ImVec2(p_Anchor.x - l_HalfWidth - l_HoverExpand,
                     p_Anchor.y - l_HalfHeight - l_HoverExpand);
          ImVec2 l_Right =
              ImVec2(p_Anchor.x + l_HalfWidth + l_HoverExpand,
                     p_Anchor.y + l_HalfHeight + l_HoverExpand);

          if (false && p_IsInput) {
            p_DrawList->AddTriangleFilled(
                ImVec2(l_Left.x, p_Anchor.y),
                ImVec2(l_Right.x, l_Left.y),
                ImVec2(l_Right.x, l_Right.y), p_Color);
            return;
          }

          p_DrawList->AddTriangleFilled(ImVec2(l_Right.x, p_Anchor.y),
                                        ImVec2(l_Left.x, l_Left.y),
                                        ImVec2(l_Left.x, l_Right.y),
                                        p_Color);
        }

        static void draw_data_pin(ImDrawList *p_DrawList,
                                  const ImVec2 &p_Anchor,
                                  float p_Radius, ImU32 p_Color,
                                  bool p_Hovered)
        {
          const float l_Radius =
              p_Hovered ? p_Radius + 1.5f : p_Radius;
          p_DrawList->AddCircleFilled(p_Anchor, l_Radius, p_Color);
          p_DrawList->AddCircle(p_Anchor, l_Radius,
                                IM_COL32(22, 22, 27, 255), 0, 1.5f);
        }

        static Util::String
        escape_script_string(const Util::String &p_Value)
        {
          Util::String l_Result;

          for (char i_Char : p_Value) {
            switch (i_Char) {
            case '\\':
              l_Result += "\\\\";
              break;
            case '\"':
              l_Result += "\\\"";
              break;
            case '\n':
              l_Result += "\\n";
              break;
            case '\r':
              l_Result += "\\r";
              break;
            case '\t':
              l_Result += "\\t";
              break;
            default:
              l_Result += i_Char;
              break;
            }
          }

          return l_Result;
        }

        static Util::String
        make_script_identifier(const Util::String &p_Value,
                               const Util::String &p_Fallback)
        {
          Util::String l_Result;

          for (char i_Char : p_Value) {
            const bool l_IsAlpha = (i_Char >= 'a' && i_Char <= 'z') ||
                                   (i_Char >= 'A' && i_Char <= 'Z');
            const bool l_IsDigit = i_Char >= '0' && i_Char <= '9';
            const bool l_IsUnderscore = i_Char == '_';

            if (l_IsAlpha || l_IsDigit || l_IsUnderscore) {
              l_Result += i_Char;
            } else if (!l_Result.empty() &&
                       l_Result[l_Result.size() - 1] != '_') {
              l_Result += '_';
            }
          }

          if (l_Result.empty()) {
            l_Result = p_Fallback;
          }

          if (l_Result[0] >= '0' && l_Result[0] <= '9') {
            l_Result = Util::String("_") + l_Result;
          }

          return l_Result;
        }

        static void append_default_value_expression(
            const Pin &p_PinMetadata,
            CompileContext &p_CompileContext)
        {
          switch (p_PinMetadata.type) {
          case PinType::Bool:
            p_CompileContext.main_code.append(
                p_PinMetadata.default_value.as_bool() ? "true"
                                                      : "false");
            return;
          case PinType::Number:
            switch (p_PinMetadata.number_subtype) {
            case NumberSubtype::Int32:
              p_CompileContext.main_code.append(
                  (int)((int32_t)p_PinMetadata.default_value));
              return;
            case NumberSubtype::UInt32:
              p_CompileContext.main_code.append(
                  p_PinMetadata.default_value.as_u32());
              return;
            case NumberSubtype::UInt64:
              p_CompileContext.main_code.append(
                  p_PinMetadata.default_value.as_u64());
              return;
            case NumberSubtype::Float:
            default:
              p_CompileContext.main_code.append(
                  p_PinMetadata.default_value.as_float());
              return;
            }
          case PinType::String: {
            if (p_PinMetadata.string_subtype == StringSubtype::Name) {
              p_CompileContext.main_code.append(
                  "Low::Util::Name::from_string(\"");
              p_CompileContext.main_code.append(escape_script_string(
                  p_PinMetadata.default_value.as_string()));
              p_CompileContext.main_code.append("\")");
            } else {
              p_CompileContext.main_code.append("\"");
              p_CompileContext.main_code.append(escape_script_string(
                  p_PinMetadata.default_value.as_string()));
              p_CompileContext.main_code.append("\"");
            }
            return;
          }
          default:
            p_CompileContext.main_code.append("0");
            return;
          }
        }

        static bool variable_is_supported(const Variable &p_Variable)
        {
          return p_Variable.is_valid();
        }

        static const char *g_GetVariableSpawnPrefix =
            "vs_spawn_syntax_get_variable_";
        static const char *g_SetVariableSpawnPrefix =
            "vs_spawn_syntax_set_variable_";

        static bool string_starts_with(const char *p_Value,
                                       const char *p_Prefix)
        {
          if (!p_Value || !p_Prefix) {
            return false;
          }

          while (*p_Prefix) {
            if (*p_Value != *p_Prefix) {
              return false;
            }
            ++p_Value;
            ++p_Prefix;
          }

          return true;
        }

        static Util::String
        get_variable_name_from_spawn_id(Util::Name p_Id,
                                        const char *p_Prefix)
        {
          const char *l_Value = p_Id.c_str();
          if (!string_starts_with(l_Value, p_Prefix)) {
            return "";
          }

          return Util::String(l_Value + strlen(p_Prefix));
        }

        static void serialize_pin_metadata(const Pin &p_Pin,
                                           Util::Serial::Node &p_Node)
        {
          p_Node["display_name"] = p_Pin.display_name;
          p_Node["type"] = pin_type_to_string(p_Pin.type);
          p_Node["string_subtype"] =
              p_Pin.string_subtype == StringSubtype::Name ? "Name"
                                                          : "String";

          switch (p_Pin.number_subtype) {
          case NumberSubtype::Int32:
            p_Node["number_subtype"] = "Int32";
            break;
          case NumberSubtype::UInt32:
            p_Node["number_subtype"] = "UInt32";
            break;
          case NumberSubtype::UInt64:
            p_Node["number_subtype"] = "UInt64";
            break;
          case NumberSubtype::Float:
          default:
            p_Node["number_subtype"] = "Float";
            break;
          }

          p_Node["container_type"] =
              p_Pin.container_type == PinContainerType::List ? "List"
                                                             : "None";
          p_Node["widget"] = p_Pin.widget == PinWidget::DefaultValue
                                 ? "DefaultValue"
                                 : "None";
          p_Node["show_default_value_when_unlinked"] =
              p_Pin.show_default_value_when_unlinked;

          if ((u64)p_Pin.handle_type != 0) {
            p_Node["handle_type"] = (Util::String)p_Pin.handle_type;
          }

          Util::Serial::serialize_variant(p_Node["default_value"],
                                          p_Pin.default_value);
        }

        static void
        deserialize_pin_metadata(Util::Serial::Node &p_Node,
                                 Pin &p_Pin)
        {
          if (p_Node["display_name"]) {
            p_Pin.display_name =
                p_Node["display_name"].as<Util::String>();
          }
          if (p_Node["type"]) {
            p_Pin.type =
                string_to_pin_type(p_Node["type"].as<Util::String>());
          }
          if (p_Node["string_subtype"]) {
            p_Pin.string_subtype =
                p_Node["string_subtype"].as<Util::String>() == "Name"
                    ? StringSubtype::Name
                    : StringSubtype::String;
          }
          if (p_Node["number_subtype"]) {
            const Util::String l_Subtype =
                p_Node["number_subtype"].as<Util::String>();
            if (l_Subtype == "Int32") {
              p_Pin.number_subtype = NumberSubtype::Int32;
            } else if (l_Subtype == "UInt32") {
              p_Pin.number_subtype = NumberSubtype::UInt32;
            } else if (l_Subtype == "UInt64") {
              p_Pin.number_subtype = NumberSubtype::UInt64;
            } else {
              p_Pin.number_subtype = NumberSubtype::Float;
            }
          }
          if (p_Node["container_type"]) {
            p_Pin.container_type =
                p_Node["container_type"].as<Util::String>() == "List"
                    ? PinContainerType::List
                    : PinContainerType::None;
          }
          if (p_Node["widget"]) {
            p_Pin.widget =
                p_Node["widget"].as<Util::String>() == "DefaultValue"
                    ? PinWidget::DefaultValue
                    : PinWidget::None;
          }
          if (p_Node["show_default_value_when_unlinked"]) {
            p_Pin.show_default_value_when_unlinked =
                p_Node["show_default_value_when_unlinked"].as<bool>();
          }
          if (p_Node["handle_type"]) {
            p_Pin.handle_type = Util::TypeIdentifier::from_string(
                p_Node["handle_type"].as<Util::String>());
          }
          if (p_Node["default_value"]) {
            p_Pin.default_value = Util::Serial::deserialize_variant(
                p_Node["default_value"]);
          }
        }

        static void serialize_variable(const Variable &p_Variable,
                                       Util::Serial::Node &p_Node)
        {
          p_Node["name"] = p_Variable.name;
          p_Node["type"] = pin_type_to_string(p_Variable.type);
          p_Node["string_subtype"] =
              p_Variable.string_subtype == StringSubtype::Name
                  ? "Name"
                  : "String";
          switch (p_Variable.number_subtype) {
          case NumberSubtype::Int32:
            p_Node["number_subtype"] = "Int32";
            break;
          case NumberSubtype::UInt32:
            p_Node["number_subtype"] = "UInt32";
            break;
          case NumberSubtype::UInt64:
            p_Node["number_subtype"] = "UInt64";
            break;
          case NumberSubtype::Float:
          default:
            p_Node["number_subtype"] = "Float";
            break;
          }
          p_Node["container_type"] =
              p_Variable.container_type == PinContainerType::List
                  ? "List"
                  : "None";
          if ((u64)p_Variable.handle_type != 0) {
            p_Node["handle_type"] =
                (Util::String)p_Variable.handle_type;
          }
          Util::Serial::serialize_variant(p_Node["default_value"],
                                          p_Variable.default_value);
        }

        static void deserialize_variable(Util::Serial::Node &p_Node,
                                         Variable &p_Variable)
        {
          p_Variable.name = p_Node["name"].as<Util::String>();
          p_Variable.type =
              string_to_pin_type(p_Node["type"].as<Util::String>());
          if (p_Node["string_subtype"]) {
            p_Variable.string_subtype =
                p_Node["string_subtype"].as<Util::String>() == "Name"
                    ? StringSubtype::Name
                    : StringSubtype::String;
          }
          if (p_Node["number_subtype"]) {
            const Util::String l_Subtype =
                p_Node["number_subtype"].as<Util::String>();
            if (l_Subtype == "Int32") {
              p_Variable.number_subtype = NumberSubtype::Int32;
            } else if (l_Subtype == "UInt32") {
              p_Variable.number_subtype = NumberSubtype::UInt32;
            } else if (l_Subtype == "UInt64") {
              p_Variable.number_subtype = NumberSubtype::UInt64;
            } else {
              p_Variable.number_subtype = NumberSubtype::Float;
            }
          }
          if (p_Node["container_type"]) {
            p_Variable.container_type =
                p_Node["container_type"].as<Util::String>() == "List"
                    ? PinContainerType::List
                    : PinContainerType::None;
          }
          if (p_Node["handle_type"]) {
            p_Variable.handle_type =
                Util::TypeIdentifier::from_string(
                    p_Node["handle_type"].as<Util::String>());
          }
          if (p_Node["default_value"]) {
            p_Variable.default_value =
                Util::Serial::deserialize_variant(
                    p_Node["default_value"]);
          }
        }
      } // namespace

      void CompileContext::append_indent()
      {
        for (u32 i = 0; i < indentation; ++i) {
          main_code.append("  ");
        }
      }

      void CompileContext::append_line(const Util::String &p_Line)
      {
        append_indent();
        main_code.append(p_Line);
        main_code.append("\n");
      }

      void CompileContext::begin_block(const Util::String &p_Line)
      {
        append_indent();
        main_code.append(p_Line);
        main_code.append("\n");
        append_indent();
        main_code.append("{\n");
        ++indentation;
      }

      void CompileContext::end_block(const Util::String &p_Line)
      {
        if (indentation > 0) {
          --indentation;
        }

        append_indent();
        main_code.append("}");
        if (!p_Line.empty()) {
          main_code.append(p_Line);
        }
        main_code.append("\n");
      }

      void UiControllerCompileProfileSettings::serialize(
          Util::Serial::Node &p_Node) const
      {
        p_Node["class_name"] = class_name;
      }

      void UiControllerCompileProfileSettings::deserialize(
          const Util::Serial::Node &p_Node)
      {
        if (p_Node["class_name"]) {
          class_name = p_Node["class_name"].as<Util::String>();
        }
      }

      std::unique_ptr<CompileProfileSettings>
      UiControllerCompileProfile::create_settings() const
      {
        return std::make_unique<UiControllerCompileProfileSettings>();
      }

      Util::String UiControllerCompileProfile::get_class_name(
          const Document &p_Document) const
      {
        const UiControllerCompileProfileSettings *l_Settings =
            p_Document.get_compile_settings<
                UiControllerCompileProfileSettings>();
        if (l_Settings && !l_Settings->class_name.empty()) {
          return l_Settings->class_name;
        }

        return "VisualScriptUiController";
      }

      void UiControllerCompileProfile::emit_prelude(
          Graph &p_Graph, CompileContext &p_Context) const
      {
        (void)p_Graph;
        (void)p_Context;
      }

      void UiControllerCompileProfile::emit_members(
          Graph &p_Graph, CompileContext &p_Context) const
      {
        (void)p_Graph;
        (void)p_Context;
      }

      void UiControllerCompileProfile::collect_entry_points(
          Graph &p_Graph,
          Util::List<CompileEntryPoint> &p_EntryPoints) const
      {
        (void)p_Graph;
        (void)p_EntryPoints;
      }

      void UiControllerCompileProfile::emit_entry_point(
          Graph &p_Graph, const CompileEntryPoint &p_Entry,
          CompileContext &p_Context) const
      {
        if (!p_Entry.is_valid()) {
          return;
        }

        Util::String l_Signature = p_Entry.function_signature;
        if (l_Signature.empty()) {
          l_Signature =
              Util::String("void ") + p_Entry.function_name + "()";
        }

        p_Context.begin_block(l_Signature);
        p_Graph.continue_compilation(p_Entry.execution_output_pin,
                                     p_Context);
        p_Context.end_block();
        p_Context.main_code.append("\n");
      }

      void UiControllerCompileProfile::compile(
          Document &p_Document, CompileContext &p_Context) const
      {
        Graph &p_Graph = p_Document.graph;
        emit_prelude(p_Graph, p_Context);

        const Util::String l_ClassName = make_script_identifier(
            get_class_name(p_Document), "VisualScript");
        p_Context.begin_block(Util::String("class ") + l_ClassName +
                              Util::String(": UiController"));
        emit_members(p_Graph, p_Context);

        Util::List<CompileEntryPoint> l_EntryPoints;
        collect_entry_points(p_Graph, l_EntryPoints);

        for (const CompileEntryPoint &i_Entry : l_EntryPoints) {
          emit_entry_point(p_Graph, i_Entry, p_Context);
        }

        p_Context.append_line("void on_click(UI::Element element) {");
        p_Context.append_line("}");

        p_Context.end_block(";");
      }

      Util::Name DefaultCompileProfile::get_name() const
      {
        return N(vs_compile_default);
      }

      std::unique_ptr<CompileProfileSettings>
      DefaultCompileProfile::create_settings() const
      {
        return std::make_unique<DefaultCompileProfileSettings>();
      }

      void DefaultCompileProfile::collect_entry_points(
          Graph &p_Graph,
          Util::List<CompileEntryPoint> &p_EntryPoints) const
      {
        u32 l_EntryIndex = 0;

        for (const Low::Editor::Node &i_Node : p_Graph.graph.nodes) {
          Util::List<Low::Editor::Pin *> l_NodePins =
              p_Graph.graph.get_node_pins(i_Node.id);

          bool l_HasExecutionInput = false;
          PinId l_ExecutionOutputPin;

          for (const Low::Editor::Pin *i_Pin : l_NodePins) {
            const Pin *l_PinMetadata = p_Graph.find_pin(i_Pin->id);
            if (!l_PinMetadata ||
                l_PinMetadata->type != PinType::Execution) {
              continue;
            }

            if (i_Pin->direction == PinDirection::Input) {
              l_HasExecutionInput = true;
            } else if (!l_ExecutionOutputPin.is_valid()) {
              l_ExecutionOutputPin = i_Pin->id;
            }
          }

          if (l_HasExecutionInput ||
              !l_ExecutionOutputPin.is_valid()) {
            continue;
          }

          const Node *l_NodeMetadata = p_Graph.find_node(i_Node.id);
          Util::String l_FunctionName = "Entry";
          if (l_NodeMetadata && !l_NodeMetadata->title.empty()) {
            l_FunctionName = make_script_identifier(
                l_NodeMetadata->title, Util::String("Entry"));
          }

          if (l_EntryIndex > 0) {
            l_FunctionName += "_";
            l_FunctionName += l_EntryIndex;
          }

          CompileEntryPoint l_Entry;
          l_Entry.node = i_Node.id;
          l_Entry.execution_output_pin = l_ExecutionOutputPin;
          l_Entry.function_name = l_FunctionName;
          l_Entry.function_signature =
              Util::String("void ") + l_FunctionName + "()";
          p_EntryPoints.push_back(l_Entry);
          ++l_EntryIndex;
        }
      }

      void
      DefaultCompileProfile::compile(Document &p_Document,
                                     CompileContext &p_Context) const
      {
        Graph &p_Graph = p_Document.graph;
        p_Context.begin_block("class VisualScript");

        Util::List<CompileEntryPoint> l_EntryPoints;
        collect_entry_points(p_Graph, l_EntryPoints);

        for (const CompileEntryPoint &i_Entry : l_EntryPoints) {
          if (!i_Entry.is_valid()) {
            continue;
          }

          Util::String l_Signature = i_Entry.function_signature;
          if (l_Signature.empty()) {
            l_Signature =
                Util::String("void ") + i_Entry.function_name + "()";
          }

          p_Context.begin_block(l_Signature);
          p_Graph.continue_compilation(i_Entry.execution_output_pin,
                                       p_Context);
          p_Context.end_block();
          p_Context.main_code.append("\n");
        }

        p_Context.end_block(";");
      }

      void CompileProfileRegistry::register_profile(
          CompileProfile &p_Profile)
      {
        profiles[p_Profile.get_name()] = &p_Profile;
      }

      CompileProfile *
      CompileProfileRegistry::find_profile(Util::Name p_ProfileName)
      {
        auto it = profiles.find(p_ProfileName);
        if (it == profiles.end()) {
          return nullptr;
        }
        return it->second;
      }

      const CompileProfile *CompileProfileRegistry::find_profile(
          Util::Name p_ProfileName) const
      {
        auto it = profiles.find(p_ProfileName);
        if (it == profiles.end()) {
          return nullptr;
        }
        return it->second;
      }

      void register_builtin_compile_profiles(
          CompileProfileRegistry &p_ProfileRegistry)
      {
        static DefaultCompileProfile g_DefaultCompileProfile;
        static UiControllerCompileProfile
            g_UiControllerCompileProfile;
        p_ProfileRegistry.register_profile(g_DefaultCompileProfile);
        p_ProfileRegistry.register_profile(
            g_UiControllerCompileProfile);
      }

      NodeGraphMutationResult<Low::Editor::Node>
      Graph::add_node(const Low::Editor::Node &p_Node,
                      const Node &p_Metadata,
                      const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Low::Editor::Node> l_Result =
            graph.add_node(p_Node, p_Schema);

        if (l_Result.succeeded()) {
          node_metadata[p_Node.id] = p_Metadata;
          node_metadata[p_Node.id].node = p_Node.id;

          const NodeClass *l_NodeClass =
              find_node_class(node_metadata[p_Node.id].node_class);
          if (l_NodeClass) {
            if (node_metadata[p_Node.id].title.empty()) {
              node_metadata[p_Node.id].title =
                  l_NodeClass->get_title(*this, p_Node.id);
            }
            if (node_metadata[p_Node.id].subtitle.empty()) {
              node_metadata[p_Node.id].subtitle =
                  l_NodeClass->get_subtitle(*this, p_Node.id);
            }
            if (node_metadata[p_Node.id].category.empty()) {
              node_metadata[p_Node.id].category =
                  l_NodeClass->get_category(*this, p_Node.id);
            }
          }
        }

        return l_Result;
      }

      NodeGraphMutationResult<Low::Editor::Pin>
      Graph::add_pin(const Low::Editor::Pin &p_Pin, const Pin &p_Metadata,
                     const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Low::Editor::Pin> l_Result =
            graph.add_pin(p_Pin, p_Schema);

        if (l_Result.succeeded()) {
          pin_metadata[p_Pin.id] = p_Metadata;
          pin_metadata[p_Pin.id].pin = p_Pin.id;

          if (pin_metadata[p_Pin.id].type != PinType::Execution &&
              pin_metadata[p_Pin.id].type != PinType::Dynamic &&
              pin_metadata[p_Pin.id].default_value.m_Type !=
                  pin_to_variant_type(pin_metadata[p_Pin.id])) {
            pin_metadata[p_Pin.id].default_value =
                default_value_for_pin(pin_metadata[p_Pin.id]);
          }
        }

        return l_Result;
      }

      NodeGraphMutationResult<Low::Editor::Link>
      Graph::add_link(const Low::Editor::Link &p_Link,
                      const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Low::Editor::Link> l_Result =
            graph.add_link(p_Link, p_Schema);

        if (!l_Result.succeeded()) {
          return l_Result;
        }

        const Low::Editor::Pin *l_StartPin =
            graph.find_pin(p_Link.start_pin);
        const Low::Editor::Pin *l_EndPin = graph.find_pin(p_Link.end_pin);

        if (l_StartPin) {
          const Node *l_NodeMetadata = find_node(l_StartPin->node);
          const NodeClass *l_NodeClass =
              l_NodeMetadata
                  ? find_node_class(l_NodeMetadata->node_class)
                  : nullptr;
          if (l_NodeClass) {
            l_NodeClass->on_pin_connected(*this, l_StartPin->node,
                                          l_StartPin->id,
                                          p_Link.end_pin);
          }
        }

        if (l_EndPin) {
          const Node *l_NodeMetadata = find_node(l_EndPin->node);
          const NodeClass *l_NodeClass =
              l_NodeMetadata
                  ? find_node_class(l_NodeMetadata->node_class)
                  : nullptr;
          if (l_NodeClass) {
            l_NodeClass->on_pin_connected(*this, l_EndPin->node,
                                          l_EndPin->id,
                                          p_Link.start_pin);
          }
        }

        return l_Result;
      }

      bool Graph::remove_node(NodeId p_NodeId)
      {
        Util::List<PinId> l_PinIdsToRemove;

        for (auto it = pin_metadata.begin(); it != pin_metadata.end();
             ++it) {
          const Low::Editor::Pin *l_Pin = graph.find_pin(it->first);
          if (l_Pin && l_Pin->node == p_NodeId) {
            l_PinIdsToRemove.push_back(it->first);
          }
        }

        for (PinId i_PinId : l_PinIdsToRemove) {
          remove_pin(i_PinId);
        }

        node_metadata.erase(p_NodeId);
        return graph.remove_node(p_NodeId);
      }

      bool Graph::remove_pin(PinId p_PinId)
      {
        pin_metadata.erase(p_PinId);
        return graph.remove_pin(p_PinId);
      }

      bool Graph::remove_link(LinkId p_LinkId)
      {
        return graph.remove_link(p_LinkId);
      }

      Node *Graph::find_node(NodeId p_NodeId)
      {
        auto it = node_metadata.find(p_NodeId);
        return it != node_metadata.end() ? &it->second : nullptr;
      }

      const Node *Graph::find_node(NodeId p_NodeId) const
      {
        auto it = node_metadata.find(p_NodeId);
        return it != node_metadata.end() ? &it->second : nullptr;
      }

      Node *Graph::find_node_checked(NodeId p_NodeId)
      {
        Node *l_Node = find_node(p_NodeId);
        LOW_ASSERT(l_Node,
                   "Could not find visual scripting node by id");
        return l_Node;
      }

      const Node *Graph::find_node_checked(NodeId p_NodeId) const
      {
        const Node *l_Node = find_node(p_NodeId);
        LOW_ASSERT(l_Node,
                   "Could not find visual scripting node by id");
        return l_Node;
      }

      Pin *Graph::find_pin(PinId p_PinId)
      {
        auto it = pin_metadata.find(p_PinId);
        return it != pin_metadata.end() ? &it->second : nullptr;
      }

      const Pin *Graph::find_pin(PinId p_PinId) const
      {
        auto it = pin_metadata.find(p_PinId);
        return it != pin_metadata.end() ? &it->second : nullptr;
      }

      Pin *Graph::find_pin_checked(PinId p_PinId)
      {
        Pin *l_Pin = find_pin(p_PinId);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting pin by id");
        return l_Pin;
      }

      const Pin *Graph::find_pin_checked(PinId p_PinId) const
      {
        const Pin *l_Pin = find_pin(p_PinId);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting pin by id");
        return l_Pin;
      }

      Pin *Graph::find_input_pin(NodeId p_NodeId,
                                 const Util::String &p_DisplayName)
      {
        for (Low::Editor::Pin *i_Pin : graph.get_input_pins(p_NodeId)) {
          Pin *l_PinMetadata = find_pin(i_Pin->id);
          if (l_PinMetadata &&
              l_PinMetadata->display_name == p_DisplayName) {
            return l_PinMetadata;
          }
        }

        return nullptr;
      }

      const Pin *
      Graph::find_input_pin(NodeId p_NodeId,
                            const Util::String &p_DisplayName) const
      {
        for (const Low::Editor::Pin *i_Pin :
             graph.get_input_pins(p_NodeId)) {
          const Pin *l_PinMetadata = find_pin(i_Pin->id);
          if (l_PinMetadata &&
              l_PinMetadata->display_name == p_DisplayName) {
            return l_PinMetadata;
          }
        }

        return nullptr;
      }

      Pin *Graph::find_output_pin(NodeId p_NodeId,
                                  const Util::String &p_DisplayName)
      {
        for (Low::Editor::Pin *i_Pin : graph.get_output_pins(p_NodeId)) {
          Pin *l_PinMetadata = find_pin(i_Pin->id);
          if (l_PinMetadata &&
              l_PinMetadata->display_name == p_DisplayName) {
            return l_PinMetadata;
          }
        }

        return nullptr;
      }

      const Pin *
      Graph::find_output_pin(NodeId p_NodeId,
                             const Util::String &p_DisplayName) const
      {
        for (const Low::Editor::Pin *i_Pin :
             graph.get_output_pins(p_NodeId)) {
          const Pin *l_PinMetadata = find_pin(i_Pin->id);
          if (l_PinMetadata &&
              l_PinMetadata->display_name == p_DisplayName) {
            return l_PinMetadata;
          }
        }

        return nullptr;
      }

      Pin *
      Graph::find_input_pin_checked(NodeId p_NodeId,
                                    const Util::String &p_DisplayName)
      {
        Pin *l_Pin = find_input_pin(p_NodeId, p_DisplayName);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting input pin");
        return l_Pin;
      }

      const Pin *Graph::find_input_pin_checked(
          NodeId p_NodeId, const Util::String &p_DisplayName) const
      {
        const Pin *l_Pin = find_input_pin(p_NodeId, p_DisplayName);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting input pin");
        return l_Pin;
      }

      Pin *Graph::find_output_pin_checked(
          NodeId p_NodeId, const Util::String &p_DisplayName)
      {
        Pin *l_Pin = find_output_pin(p_NodeId, p_DisplayName);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting output pin");
        return l_Pin;
      }

      const Pin *Graph::find_output_pin_checked(
          NodeId p_NodeId, const Util::String &p_DisplayName) const
      {
        const Pin *l_Pin = find_output_pin(p_NodeId, p_DisplayName);
        LOW_ASSERT(l_Pin,
                   "Could not find visual scripting output pin");
        return l_Pin;
      }

      bool Graph::add_variable(const Variable &p_Variable)
      {
        if (!variable_is_supported(p_Variable) ||
            find_variable(p_Variable.name)) {
          return false;
        }

        variables.push_back(p_Variable);
        return true;
      }

      bool Graph::remove_variable(const Util::String &p_Name)
      {
        for (auto it = variables.begin(); it != variables.end();
             ++it) {
          if (it->name == p_Name) {
            variables.erase(it);
            return true;
          }
        }

        return false;
      }

      Variable *Graph::find_variable(const Util::String &p_Name)
      {
        for (Variable &i_Variable : variables) {
          if (i_Variable.name == p_Name) {
            return &i_Variable;
          }
        }

        return nullptr;
      }

      const Variable *
      Graph::find_variable(const Util::String &p_Name) const
      {
        for (const Variable &i_Variable : variables) {
          if (i_Variable.name == p_Name) {
            return &i_Variable;
          }
        }

        return nullptr;
      }

      void Graph::register_node_class(NodeClass &p_NodeClass)
      {
        node_classes[p_NodeClass.get_name()] = &p_NodeClass;
      }

      void
      Graph::register_spawn_entry(const NodeSpawnEntry &p_SpawnEntry)
      {
        spawn_entries[p_SpawnEntry.id] = p_SpawnEntry;
      }

      NodeClass *Graph::find_node_class(Util::Name p_NodeClass)
      {
        auto it = node_classes.find(p_NodeClass);
        return it != node_classes.end() ? it->second : nullptr;
      }

      const NodeClass *
      Graph::find_node_class(Util::Name p_NodeClass) const
      {
        auto it = node_classes.find(p_NodeClass);
        return it != node_classes.end() ? it->second : nullptr;
      }

      NodeSpawnEntry *
      Graph::find_spawn_entry(Util::Name p_SpawnEntryId)
      {
        auto it = spawn_entries.find(p_SpawnEntryId);
        return it != spawn_entries.end() ? &it->second : nullptr;
      }

      const NodeSpawnEntry *
      Graph::find_spawn_entry(Util::Name p_SpawnEntryId) const
      {
        auto it = spawn_entries.find(p_SpawnEntryId);
        return it != spawn_entries.end() ? &it->second : nullptr;
      }

      Util::List<const NodeSpawnEntry *>
      Graph::get_spawn_entries() const
      {
        Util::List<const NodeSpawnEntry *> l_Entries;

        for (auto it = spawn_entries.begin();
             it != spawn_entries.end(); ++it) {
          l_Entries.push_back(&it->second);
        }

        return l_Entries;
      }

      NodeGraphMutationResult<Low::Editor::Node>
      Graph::create_node(Util::Name p_NodeClass,
                         const Math::Vector2 &p_Position,
                         const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Low::Editor::Node> l_Result;
        const NodeClass *l_NodeClass = find_node_class(p_NodeClass);

        if (!l_NodeClass) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidNode;
          return l_Result;
        }

        Low::Editor::Node l_Node;
        l_Node.id = allocate_node_id();
        l_Node.position = p_Position;

        Node l_Metadata;
        l_Metadata.node = l_Node.id;
        l_Metadata.node_class = p_NodeClass;
        l_Metadata.title = l_NodeClass->get_title(*this, l_Node.id);
        l_Metadata.subtitle =
            l_NodeClass->get_subtitle(*this, l_Node.id);
        l_Metadata.category =
            l_NodeClass->get_category(*this, l_Node.id);

        l_Result = add_node(l_Node, l_Metadata, p_Schema);
        if (l_Result.succeeded()) {
          l_NodeClass->setup_default_pins(*this, l_Node.id, p_Schema);
        }

        return l_Result;
      }

      NodeGraphMutationResult<Low::Editor::Node>
      Graph::create_node_from_spawn_entry(
          Util::Name p_SpawnEntryId, const Math::Vector2 &p_Position,
          const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Low::Editor::Node> l_Result;
        const NodeSpawnEntry *l_SpawnEntry =
            find_spawn_entry(p_SpawnEntryId);

        if (!l_SpawnEntry || !l_SpawnEntry->is_valid()) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidNode;
          return l_Result;
        }

        const NodeClass *l_NodeClass =
            find_node_class(l_SpawnEntry->node_class);

        if (!l_NodeClass) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidNode;
          return l_Result;
        }

        Low::Editor::Node l_Node;
        l_Node.id = allocate_node_id();
        l_Node.position = p_Position;

        Node l_Metadata;
        l_Metadata.node = l_Node.id;
        l_Metadata.node_class = l_SpawnEntry->node_class;
        l_Metadata.spawn_entry = l_SpawnEntry->id;
        l_Metadata.title =
            !l_SpawnEntry->title.empty()
                ? l_SpawnEntry->title
                : l_NodeClass->get_title(*this, l_Node.id);
        l_Metadata.subtitle =
            !l_SpawnEntry->subtitle.empty()
                ? l_SpawnEntry->subtitle
                : l_NodeClass->get_subtitle(*this, l_Node.id);
        l_Metadata.category =
            !l_SpawnEntry->category.empty()
                ? l_SpawnEntry->category
                : l_NodeClass->get_category(*this, l_Node.id);

        if (l_SpawnEntry->initialize_node) {
          l_SpawnEntry->initialize_node(*this, l_Metadata);
        }

        l_Result = add_node(l_Node, l_Metadata, p_Schema);
        if (l_Result.succeeded()) {
          l_NodeClass->setup_default_pins(*this, l_Node.id, p_Schema);
        }

        return l_Result;
      }

      bool Graph::is_pin_connected(PinId p_PinId) const
      {
        for (const Low::Editor::Link &i_Link : graph.links) {
          if (i_Link.start_pin == p_PinId ||
              i_Link.end_pin == p_PinId) {
            return true;
          }
        }

        return false;
      }

      PinId Graph::get_connected_pin(PinId p_PinId) const
      {
        for (const Low::Editor::Link &i_Link : graph.links) {
          if (i_Link.start_pin == p_PinId) {
            return i_Link.end_pin;
          }
          if (i_Link.end_pin == p_PinId) {
            return i_Link.start_pin;
          }
        }

        return {};
      }

      Util::List<PinId> Graph::get_connected_pins(PinId p_PinId) const
      {
        Util::List<PinId> l_Result;

        for (const Low::Editor::Link &i_Link : graph.links) {
          if (i_Link.start_pin == p_PinId) {
            l_Result.push_back(i_Link.end_pin);
          } else if (i_Link.end_pin == p_PinId) {
            l_Result.push_back(i_Link.start_pin);
          }
        }

        return l_Result;
      }

      void Graph::compile_node(NodeId p_NodeId,
                               CompileContext &p_CompileContext) const
      {
        const Node *l_NodeMetadata = find_node(p_NodeId);
        if (!l_NodeMetadata) {
          return;
        }

        const NodeClass *l_NodeClass =
            find_node_class(l_NodeMetadata->node_class);
        if (!l_NodeClass) {
          return;
        }

        l_NodeClass->compile(*const_cast<Graph *>(this), p_NodeId,
                             p_CompileContext);
      }

      void Graph::continue_compilation(
          PinId p_ExecutionOutputPinId,
          CompileContext &p_CompileContext) const
      {
        const Low::Editor::Pin *l_OutputPin =
            graph.find_pin(p_ExecutionOutputPinId);
        const Pin *l_OutputPinMetadata =
            find_pin(p_ExecutionOutputPinId);

        if (!l_OutputPin || !l_OutputPinMetadata ||
            l_OutputPin->direction != PinDirection::Output ||
            l_OutputPinMetadata->type != PinType::Execution) {
          return;
        }

        for (PinId i_ConnectedPinId :
             get_connected_pins(p_ExecutionOutputPinId)) {
          const Low::Editor::Pin *l_ConnectedPin =
              graph.find_pin(i_ConnectedPinId);
          if (!l_ConnectedPin) {
            continue;
          }

          compile_node(l_ConnectedPin->node, p_CompileContext);
        }
      }

      void
      Graph::compile_input_pin(PinId p_InputPinId,
                               CompileContext &p_CompileContext) const
      {
        const Low::Editor::Pin *l_InputPin = graph.find_pin(p_InputPinId);
        const Pin *l_InputPinMetadata = find_pin(p_InputPinId);

        if (!l_InputPin || !l_InputPinMetadata ||
            l_InputPin->direction != PinDirection::Input) {
          return;
        }

        if (is_pin_connected(p_InputPinId)) {
          const PinId l_ConnectedPinId =
              get_connected_pin(p_InputPinId);
          const Low::Editor::Pin *l_ConnectedPin =
              graph.find_pin(l_ConnectedPinId);

          if (!l_ConnectedPin) {
            append_default_value_expression(*l_InputPinMetadata,
                                            p_CompileContext);
            return;
          }

          const Node *l_ConnectedNodeMetadata =
              find_node(l_ConnectedPin->node);
          if (!l_ConnectedNodeMetadata) {
            append_default_value_expression(*l_InputPinMetadata,
                                            p_CompileContext);
            return;
          }

          const NodeClass *l_ConnectedNodeClass =
              find_node_class(l_ConnectedNodeMetadata->node_class);
          if (!l_ConnectedNodeClass) {
            append_default_value_expression(*l_InputPinMetadata,
                                            p_CompileContext);
            return;
          }

          const Pin *l_ConnectedPinMetadata =
              find_pin(l_ConnectedPinId);
          if (l_ConnectedPinMetadata &&
              l_InputPinMetadata->type == PinType::String &&
              l_ConnectedPinMetadata->type == PinType::String &&
              l_InputPinMetadata->string_subtype !=
                  l_ConnectedPinMetadata->string_subtype) {
            if (l_InputPinMetadata->string_subtype ==
                    StringSubtype::String &&
                l_ConnectedPinMetadata->string_subtype ==
                    StringSubtype::Name) {
              p_CompileContext.main_code.append("Low::Util::String(");
              l_ConnectedNodeClass->compile_output_pin(
                  *const_cast<Graph *>(this), l_ConnectedPin->node,
                  l_ConnectedPinId, p_CompileContext);
              p_CompileContext.main_code.append(".c_str())");
              return;
            }

            if (l_InputPinMetadata->string_subtype ==
                    StringSubtype::Name &&
                l_ConnectedPinMetadata->string_subtype ==
                    StringSubtype::String) {
              p_CompileContext.main_code.append(
                  "Low::Util::Name::from_string(");
              l_ConnectedNodeClass->compile_output_pin(
                  *const_cast<Graph *>(this), l_ConnectedPin->node,
                  l_ConnectedPinId, p_CompileContext);
              p_CompileContext.main_code.append(")");
              return;
            }
          }

          l_ConnectedNodeClass->compile_output_pin(
              *const_cast<Graph *>(this), l_ConnectedPin->node,
              l_ConnectedPinId, p_CompileContext);
          return;
        }

        append_default_value_expression(*l_InputPinMetadata,
                                        p_CompileContext);
      }

      void Graph::serialize(Util::Serial::Node &p_Node) const
      {
        p_Node["id_counter"] = Util::U64Id{id_counter};

        for (const Variable &i_Variable : variables) {
          Util::Serial::Node i_VariableNode;
          serialize_variable(i_Variable, i_VariableNode);
          p_Node["variables"].push_back(i_VariableNode);
        }

        for (const Low::Editor::Node &i_Node : graph.nodes) {
          const Node *l_NodeMetadata = find_node(i_Node.id);
          if (!l_NodeMetadata) {
            continue;
          }

          Util::Serial::Node i_NodeNode;
          i_NodeNode["id"] = Util::U64Id{i_Node.id.value};
          i_NodeNode["position"] = i_Node.position;
          i_NodeNode["node_class"] = l_NodeMetadata->node_class;

          if (l_NodeMetadata->spawn_entry.is_valid()) {
            i_NodeNode["spawn_entry"] = l_NodeMetadata->spawn_entry;
          }
          if (!l_NodeMetadata->title.empty()) {
            i_NodeNode["title"] = l_NodeMetadata->title;
          }
          if (!l_NodeMetadata->subtitle.empty()) {
            i_NodeNode["subtitle"] = l_NodeMetadata->subtitle;
          }
          if (!l_NodeMetadata->category.empty()) {
            i_NodeNode["category"] = l_NodeMetadata->category;
          }
          if (!l_NodeMetadata->variable_name.empty()) {
            i_NodeNode["variable_name"] =
                l_NodeMetadata->variable_name;
          }
          if ((u64)l_NodeMetadata->handle_type != 0) {
            i_NodeNode["handle_type"] =
                (Util::String)l_NodeMetadata->handle_type;
          }
          if (l_NodeMetadata->member_name.is_valid()) {
            i_NodeNode["member_name"] = l_NodeMetadata->member_name;
          }

          const NodeClass *l_NodeClass =
              find_node_class(l_NodeMetadata->node_class);
          if (l_NodeClass) {
            l_NodeClass->serialize(*const_cast<Graph *>(this),
                                   i_Node.id,
                                   i_NodeNode["node_data"]);
          }

          for (const Low::Editor::Pin *i_Pin :
               graph.get_node_pins(i_Node.id)) {
            const Pin *l_PinMetadata = find_pin(i_Pin->id);
            if (!l_PinMetadata) {
              continue;
            }

            Util::Serial::Node i_PinNode;
            i_PinNode["id"] = Util::U64Id{i_Pin->id.value};
            i_PinNode["direction"] =
                i_Pin->direction == PinDirection::Input ? "Input"
                                                        : "Output";
            serialize_pin_metadata(*l_PinMetadata, i_PinNode);
            i_NodeNode["pins"].push_back(i_PinNode);
          }

          p_Node["nodes"].push_back(i_NodeNode);
        }

        for (const Low::Editor::Link &i_Link : graph.links) {
          Util::Serial::Node i_LinkNode;
          i_LinkNode["id"] = Util::U64Id{i_Link.id.value};
          i_LinkNode["start_pin"] =
              Util::U64Id{i_Link.start_pin.value};
          i_LinkNode["end_pin"] = Util::U64Id{i_Link.end_pin.value};
          p_Node["links"].push_back(i_LinkNode);
        }
      }

      void Graph::deserialize(Util::Serial::Node &p_Node)
      {
        graph.nodes.clear();
        graph.pins.clear();
        graph.links.clear();
        node_metadata.clear();
        pin_metadata.clear();
        variables.clear();

        id_counter = p_Node["id_counter"]
                         ? (u64)p_Node["id_counter"].as<Util::U64Id>()
                         : 1ull;

        if (p_Node["variables"]) {
          for (auto [_, i_VariableNode] : p_Node["variables"]) {
            Variable i_Variable;
            deserialize_variable(i_VariableNode, i_Variable);
            add_variable(i_Variable);
          }
        }

        u64 l_MaxId = 0;

        if (p_Node["nodes"]) {
          for (auto [_, i_NodeNode] : p_Node["nodes"]) {
            const Util::Name l_NodeClassName =
                i_NodeNode["node_class"].as<Util::Name>();
            const NodeClass *l_NodeClass =
                find_node_class(l_NodeClassName);
            LOW_ASSERT(
                l_NodeClass,
                "Could not find node class during VisualScript "
                "graph deserialization");
            if (!l_NodeClass) {
              continue;
            }

            Low::Editor::Node l_Node;
            l_Node.id =
                NodeId{(u64)i_NodeNode["id"].as<Util::U64Id>()};
            l_Node.position =
                i_NodeNode["position"].as<Math::Vector2>();
            l_MaxId = LOW_MATH_MAX(l_MaxId, l_Node.id.value);

            Node l_Metadata;
            l_Metadata.node = l_Node.id;
            l_Metadata.node_class = l_NodeClassName;
            if (i_NodeNode["spawn_entry"]) {
              l_Metadata.spawn_entry =
                  i_NodeNode["spawn_entry"].as<Util::Name>();
            }
            if (i_NodeNode["title"]) {
              l_Metadata.title =
                  i_NodeNode["title"].as<Util::String>();
            }
            if (i_NodeNode["subtitle"]) {
              l_Metadata.subtitle =
                  i_NodeNode["subtitle"].as<Util::String>();
            }
            if (i_NodeNode["category"]) {
              l_Metadata.category =
                  i_NodeNode["category"].as<Util::String>();
            }
            if (i_NodeNode["variable_name"]) {
              l_Metadata.variable_name =
                  i_NodeNode["variable_name"].as<Util::String>();
            }
            if (i_NodeNode["handle_type"]) {
              l_Metadata.handle_type =
                  Util::TypeIdentifier::from_string(
                      i_NodeNode["handle_type"].as<Util::String>());
            }
            if (i_NodeNode["member_name"]) {
              l_Metadata.member_name =
                  i_NodeNode["member_name"].as<Util::Name>();
            }

            NodeGraphMutationResult<Low::Editor::Node> l_AddNodeResult =
                add_node(l_Node, l_Metadata, nullptr);
            if (!l_AddNodeResult.succeeded()) {
              continue;
            }

            if (i_NodeNode["node_data"]) {
              l_NodeClass->deserialize(*this, l_Node.id,
                                       i_NodeNode["node_data"]);
            }

            l_NodeClass->setup_default_pins(*this, l_Node.id,
                                            nullptr);

            Util::List<Low::Editor::Pin *> l_NodePins =
                graph.get_node_pins(l_Node.id);
            const u32 l_SerializedPinCount =
                (u32)i_NodeNode["pins"].size();
            LOW_ASSERT(l_NodePins.size() == l_SerializedPinCount,
                       "Node pin count mismatch during VisualScript "
                       "graph deserialization");

            Util::List<PinId> l_OldPinIds;
            for (Low::Editor::Pin *i_Pin : l_NodePins) {
              l_OldPinIds.push_back(i_Pin->id);
            }
            for (PinId i_OldPinId : l_OldPinIds) {
              pin_metadata.erase(i_OldPinId);
            }

            const u32 l_PinCount = LOW_MATH_MIN(
                (u32)l_NodePins.size(), l_SerializedPinCount);
            for (u32 i = 0; i < l_PinCount; ++i) {
              Low::Editor::Pin *l_Pin = l_NodePins[i];
              Util::Serial::Node &l_PinNode = i_NodeNode["pins"][i];
              Pin l_PinMetadata;
              deserialize_pin_metadata(l_PinNode, l_PinMetadata);

              const PinId l_PinId =
                  PinId{(u64)l_PinNode["id"].as<Util::U64Id>()};
              l_Pin->id = l_PinId;
              l_PinMetadata.pin = l_PinId;
              pin_metadata[l_PinId] = l_PinMetadata;
              l_MaxId = LOW_MATH_MAX(l_MaxId, l_PinId.value);
            }
          }
        }

        if (p_Node["links"]) {
          for (auto [_, i_LinkNode] : p_Node["links"]) {
            Low::Editor::Link l_Link;
            l_Link.id =
                LinkId{(u64)i_LinkNode["id"].as<Util::U64Id>()};
            l_Link.start_pin =
                PinId{(u64)i_LinkNode["start_pin"].as<Util::U64Id>()};
            l_Link.end_pin =
                PinId{(u64)i_LinkNode["end_pin"].as<Util::U64Id>()};
            graph.add_link(l_Link, nullptr);
            l_MaxId = LOW_MATH_MAX(l_MaxId, l_Link.id.value);
          }
        }

        id_counter = LOW_MATH_MAX(id_counter, l_MaxId + 1);
      }

      NodeId Graph::allocate_node_id()
      {
        return NodeId{id_counter++};
      }

      PinId Graph::allocate_pin_id()
      {
        return PinId{id_counter++};
      }

      Util::String NodeClass::get_title(const Graph &p_Graph,
                                        NodeId p_NodeId) const
      {
        (void)p_Graph;
        (void)p_NodeId;
        return Util::StringHelper::prettify_name(get_name());
      }

      Util::String NodeClass::get_subtitle(const Graph &p_Graph,
                                           NodeId p_NodeId) const
      {
        (void)p_Graph;
        (void)p_NodeId;
        return "";
      }

      Util::String NodeClass::get_category(const Graph &p_Graph,
                                           NodeId p_NodeId) const
      {
        (void)p_Graph;
        (void)p_NodeId;
        return "";
      }

      Util::String NodeClass::get_icon(const Graph &p_Graph,
                                       NodeId p_NodeId) const
      {
        (void)p_Graph;
        (void)p_NodeId;
        return LOW_EDITOR_ICON_ELEMENT;
      }

      ImU32 NodeClass::get_color(const Graph &p_Graph,
                                 NodeId p_NodeId) const
      {
        (void)p_Graph;
        (void)p_NodeId;
        return IM_COL32(92, 96, 112, 255);
      }

      void NodeClass::compile_input_pin(
          Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
          CompileContext &p_CompileContext) const
      {
        (void)p_NodeId;
        p_Graph.compile_input_pin(p_PinId, p_CompileContext);
      }

      NodeGraphValidationResult
      Schema::can_create_pin(const NodeGraph &p_Graph,
                             const Low::Editor::Pin &p_Pin) const
      {
        return NodeGraphSchema::can_create_pin(p_Graph, p_Pin);
      }

      NodeGraphValidationResult
      Schema::validate_link(const NodeGraph &p_Graph,
                            const Low::Editor::Pin &p_StartPin,
                            const Low::Editor::Pin &p_EndPin) const
      {
        const Graph *l_Graph = get_graph(p_Graph);
        if (!l_Graph) {
          return NodeGraphValidationResult::InvalidPin;
        }

        const Pin *l_StartPinMetadata =
            l_Graph->find_pin(p_StartPin.id);
        const Pin *l_EndPinMetadata = l_Graph->find_pin(p_EndPin.id);

        if (!l_StartPinMetadata || !l_EndPinMetadata) {
          return NodeGraphValidationResult::InvalidPin;
        }

        const bool l_StartIsExecution =
            pin_type_is_execution(l_StartPinMetadata->type);
        const bool l_EndIsExecution =
            pin_type_is_execution(l_EndPinMetadata->type);

        if (l_StartIsExecution != l_EndIsExecution) {
          return NodeGraphValidationResult::CustomRejected;
        }

        if (!pin_types_are_compatible(l_StartPinMetadata->type,
                                      l_EndPinMetadata->type) ||
            !pin_metadata_are_compatible(*l_StartPinMetadata,
                                         *l_EndPinMetadata)) {
          return NodeGraphValidationResult::CustomRejected;
        }

        const Node *l_StartNodeMetadata =
            l_Graph->find_node(p_StartPin.node);
        const Node *l_EndNodeMetadata =
            l_Graph->find_node(p_EndPin.node);
        const NodeClass *l_StartNodeClass =
            l_StartNodeMetadata ? l_Graph->find_node_class(
                                      l_StartNodeMetadata->node_class)
                                : nullptr;
        const NodeClass *l_EndNodeClass =
            l_EndNodeMetadata ? l_Graph->find_node_class(
                                    l_EndNodeMetadata->node_class)
                              : nullptr;

        if (l_StartNodeClass &&
            !l_StartNodeClass->can_connect_pin(
                *const_cast<Graph *>(l_Graph), p_StartPin.node,
                p_StartPin.id, *l_StartPinMetadata,
                *l_EndPinMetadata)) {
          return NodeGraphValidationResult::CustomRejected;
        }

        if (l_EndNodeClass &&
            !l_EndNodeClass->can_connect_pin(
                *const_cast<Graph *>(l_Graph), p_EndPin.node,
                p_EndPin.id, *l_EndPinMetadata,
                *l_StartPinMetadata)) {
          return NodeGraphValidationResult::CustomRejected;
        }

        return NodeGraphValidationResult::Allowed;
      }

      bool Schema::allows_multiple_links_per_pin(
          const NodeGraph &p_Graph, const Low::Editor::Pin &p_Pin) const
      {
        const Graph *l_Graph = get_graph(p_Graph);
        if (!l_Graph) {
          return true;
        }

        const Pin *l_PinMetadata = l_Graph->find_pin(p_Pin.id);
        if (!l_PinMetadata) {
          return true;
        }

        if (pin_type_is_execution(l_PinMetadata->type)) {
          return p_Pin.direction == PinDirection::Output;
        }

        return p_Pin.direction == PinDirection::Output;
      }

      const Graph *Schema::get_graph(const NodeGraph &p_Graph) const
      {
        if (!graph) {
          return nullptr;
        }

        return &graph->graph == &p_Graph ? graph : nullptr;
      }

      const Pin *
      Schema::get_pin_metadata(const Low::Editor::Pin &p_Pin) const
      {
        if (!graph) {
          return nullptr;
        }

        return graph->find_pin(p_Pin.id);
      }

      Math::Vector2 NodeRenderer::get_node_size(
          const NodeGraphEditorContext &p_Context,
          const Low::Editor::Node &p_Node) const
      {
        (void)p_Context;

        if (!graph) {
          return default_node_size;
        }

        u32 l_InputCount = 0;
        u32 l_OutputCount = 0;
        Util::List<Low::Editor::Pin *> l_NodePins =
            graph->graph.get_node_pins(p_Node.id);

        for (Low::Editor::Pin *i_Pin : l_NodePins) {
          if (i_Pin->direction == PinDirection::Input) {
            ++l_InputCount;
          } else {
            ++l_OutputCount;
          }
        }

        const u32 l_MaxPins =
            LOW_MATH_MAX(l_InputCount, l_OutputCount);
        const Node *l_Metadata =
            graph ? graph->find_node(p_Node.id) : nullptr;
        const NodeClass *l_NodeClass =
            graph && l_Metadata
                ? graph->find_node_class(l_Metadata->node_class)
                : nullptr;
        if (l_NodeClass &&
            l_NodeClass->is_compact(*graph, p_Node.id)) {
          return Math::Vector2(
              196.0f, LOW_MATH_MAX(60.0f, 10.0f + l_MaxPins * 34.0f));
        }
        return Math::Vector2(default_node_size.x,
                             LOW_MATH_MAX(default_node_size.y,
                                          82.0f + l_MaxPins * 32.0f));
      }

      void NodeRenderer::render_node(
          NodeGraphEditorContext &p_Context, Low::Editor::Node &p_Node,
          const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax)
      {
        if (!p_Context.draw_list) {
          return;
        }

        const Node *l_Metadata =
            graph ? graph->find_node(p_Node.id) : nullptr;
        const NodeClass *l_NodeClass =
            graph && l_Metadata
                ? graph->find_node_class(l_Metadata->node_class)
                : nullptr;
        const bool l_Selected =
            p_Context.state &&
            p_Context.state->is_node_selected(p_Node.id);
        const bool l_Hovered =
            p_Context.state &&
            p_Context.state->hovered_node == p_Node.id;
        const ImU32 l_NodeColor =
            l_NodeClass ? l_NodeClass->get_color(*graph, p_Node.id)
                        : IM_COL32(92, 96, 112, 255);
        const ImU32 l_BackgroundColor = IM_COL32(34, 35, 41, 255);
        const ImU32 l_HeaderBackgroundColor =
            IM_COL32(38, 39, 46, 255);
        const ImU32 l_HeaderDividerColor = IM_COL32(58, 60, 69, 255);
        const ImU32 l_BorderColor =
            l_Selected ? IM_COL32(215, 220, 236, 255)
                       : (l_Hovered ? IM_COL32(130, 136, 154, 255)
                                    : IM_COL32(76, 79, 91, 255));
        const ImU32 l_TextColor = IM_COL32(232, 233, 239, 255);
        const ImU32 l_SubTextColor = IM_COL32(171, 173, 187, 255);
        const ImU32 l_ValueBackgroundColor =
            IM_COL32(52, 49, 80, 255);
        const bool l_Compact =
            l_NodeClass && l_NodeClass->is_compact(*graph, p_Node.id);

        const float l_Zoom = p_Context.canvas.m_Zoom;
        const float l_HeaderHeight =
            l_Compact ? 8.0f * l_Zoom
                      : title_height * l_Zoom + 24.0f * l_Zoom;
        const float l_AccentHeight =
            l_Compact ? 4.0f * l_Zoom : 7.0f * l_Zoom;
        const float l_IconBoxSize = 36.0f * l_Zoom;
        const float l_HeaderPaddingX =
            l_Compact ? 8.0f * l_Zoom : 14.0f * l_Zoom;
        const float l_HeaderPaddingY =
            l_Compact ? 8.0f * l_Zoom : 14.0f * l_Zoom;
        const float l_PinTextOffset = 18.0f * l_Zoom;
        const float l_DefaultValueWidth = 78.0f * l_Zoom;
        const float l_TitleFontSize = 18.0f * l_Zoom;
        const float l_SubtitleFontSize = 13.0f * l_Zoom;
        const float l_PinFontSize = 17.0f * l_Zoom;
        const float l_DefaultFontSize = 15.0f * l_Zoom;
        const float l_IconFontSize = 30.0f * l_Zoom;
        ImFont *l_TitleFont =
            Fonts::UI(18.0f * l_Zoom, Fonts::Weight::Medium);
        ImFont *l_SubtitleFont =
            Fonts::UI(13.0f * l_Zoom, Fonts::Weight::Light);
        ImFont *l_PinFont =
            Fonts::UI(17.0f * l_Zoom, Fonts::Weight::Regular);
        ImFont *l_DefaultFont =
            Fonts::UI(15.0f * l_Zoom, Fonts::Weight::Regular);
        ImFont *l_IconFont =
            Fonts::UI(22.0f * l_Zoom, Fonts::Weight::Regular);

        Util::String l_TitleString =
            l_Metadata && !l_Metadata->title.empty()
                ? l_Metadata->title
                : (l_NodeClass
                       ? l_NodeClass->get_title(*graph, p_Node.id)
                       : Util::String("Visual Script Node"));
        Util::String l_SubtitleString =
            l_Metadata && !l_Metadata->subtitle.empty()
                ? l_Metadata->subtitle
                : (l_NodeClass
                       ? l_NodeClass->get_subtitle(*graph, p_Node.id)
                       : Util::String(""));
        Util::String l_IconString =
            l_NodeClass ? l_NodeClass->get_icon(*graph, p_Node.id)
                        : Util::String("");

        if (l_Compact) {
          p_Context.draw_list->AddRectFilled(p_ScreenMin, p_ScreenMax,
                                             l_BackgroundColor,
                                             border_rounding);
          p_Context.draw_list->AddRectFilled(
              p_ScreenMin,
              ImVec2(p_ScreenMax.x, p_ScreenMin.y + l_AccentHeight),
              l_NodeColor, border_rounding,
              ImDrawFlags_RoundCornersTop);
          p_Context.draw_list->AddRect(p_ScreenMin, p_ScreenMax,
                                       l_BorderColor, border_rounding,
                                       0, l_Selected ? 2.0f : 1.0f);

          const Util::String l_CompactText =
              l_NodeClass
                  ? l_NodeClass->get_compact_title(*graph, p_Node.id)
                  : l_TitleString;
          ImFont *l_CompactFont =
              Fonts::UI(19.0f * l_Zoom, Fonts::Weight::Medium);
          const float l_CompactFontSize = 19.0f * l_Zoom;
          const ImVec2 l_CompactTextSize =
              calc_scaled_text_size(l_CompactFont, l_CompactFontSize,
                                    l_CompactText.c_str());
          const float l_TagPaddingX = 7.0f * l_Zoom;
          const float l_TagPaddingY = 3.0f * l_Zoom;
          const float l_CompactTabInset = 7.0f * l_Zoom;
          const ImVec2 l_CompactBadgeMin = ImVec2(
              p_ScreenMin.x + l_CompactTabInset,
              p_ScreenMin.y -
                  (l_CompactTextSize.y + l_TagPaddingY * 2.0f) +
                  l_AccentHeight + 1.0f * l_Zoom);
          const ImVec2 l_CompactBadgeMax =
              ImVec2(l_CompactBadgeMin.x + l_CompactTextSize.x +
                         l_TagPaddingX * 2.0f,
                     p_ScreenMin.y + l_AccentHeight - 2);
          const float l_CompactBadgeRounding = 8.0f * l_Zoom;
          const ImU32 l_CompactBadgeBackground = l_NodeColor;

          p_Context.draw_list->AddRectFilled(
              l_CompactBadgeMin, l_CompactBadgeMax,
              l_CompactBadgeBackground, l_CompactBadgeRounding,
              ImDrawFlags_RoundCornersTop);
          /*
          p_Context.draw_list->AddRect(
              l_CompactBadgeMin, l_CompactBadgeMax, l_BorderColor,
              l_CompactBadgeRounding, 0, 1.0f);
              */
          p_Context.draw_list->AddLine(
              ImVec2(l_CompactBadgeMin.x + 1.0f * l_Zoom,
                     l_CompactBadgeMax.y),
              ImVec2(l_CompactBadgeMax.x - 1.0f * l_Zoom,
                     l_CompactBadgeMax.y),
              l_NodeColor, 2.0f);
          add_scaled_text(
              p_Context.draw_list, l_CompactFont, l_CompactFontSize,
              ImVec2(l_CompactBadgeMin.x + l_TagPaddingX,
                     l_CompactBadgeMin.y + l_TagPaddingY),
              IM_COL32(232, 233, 239, 220), l_CompactText.c_str());
        } else {
          p_Context.draw_list->AddRectFilled(p_ScreenMin, p_ScreenMax,
                                             l_BackgroundColor,
                                             border_rounding);
          const ImVec2 l_IconBoxMin = ImVec2(
              p_ScreenMin.x + l_HeaderPaddingX,
              p_ScreenMin.y + l_HeaderPaddingY + 1.0f * l_Zoom);
          p_Context.draw_list->AddRectFilled(
              p_ScreenMin,
              ImVec2(p_ScreenMax.x, p_ScreenMin.y + l_HeaderHeight),
              l_HeaderBackgroundColor, border_rounding,
              ImDrawFlags_RoundCornersTop);
          p_Context.draw_list->AddRectFilled(
              p_ScreenMin,
              ImVec2(p_ScreenMax.x, p_ScreenMin.y + l_AccentHeight),
              l_NodeColor, border_rounding,
              ImDrawFlags_RoundCornersTop);
          p_Context.draw_list->AddLine(
              ImVec2(p_ScreenMin.x, p_ScreenMin.y + l_HeaderHeight),
              ImVec2(p_ScreenMax.x, p_ScreenMin.y + l_HeaderHeight),
              l_HeaderDividerColor, 1.0f);
          p_Context.draw_list->AddRect(p_ScreenMin, p_ScreenMax,
                                       l_BorderColor, border_rounding,
                                       0, l_Selected ? 2.0f : 1.0f);
          const ImVec2 l_HeaderTextPos = ImVec2(
              p_ScreenMin.x + l_HeaderPaddingX +
                  (l_IconString.empty()
                       ? 0.0f
                       : (l_IconBoxSize + 10.0f * l_Zoom)),
              p_ScreenMin.y + l_HeaderPaddingY + 1.0f * l_Zoom);

          if (!l_IconString.empty()) {
            add_scaled_text(p_Context.draw_list, l_IconFont,
                            l_IconFontSize,
                            ImVec2(l_IconBoxMin.x,
                                   l_IconBoxMin.y + 1.0f * l_Zoom),
                            l_TextColor, l_IconString.c_str());
          }

          add_scaled_text(p_Context.draw_list, l_TitleFont,
                          l_TitleFontSize, l_HeaderTextPos,
                          l_TextColor, l_TitleString.c_str());
          if (!l_SubtitleString.empty()) {
            add_scaled_text(
                p_Context.draw_list, l_SubtitleFont,
                l_SubtitleFontSize,
                ImVec2(l_HeaderTextPos.x,
                       l_HeaderTextPos.y + 22.0f * l_Zoom),
                l_SubTextColor, l_SubtitleString.c_str());
          }
        }

        Util::List<Low::Editor::Pin *> l_NodePins =
            graph ? graph->graph.get_node_pins(p_Node.id)
                  : Util::List<Low::Editor::Pin *>();

        for (Low::Editor::Pin *i_Pin : l_NodePins) {
          Pin *l_PinMetadata =
              graph ? graph->find_pin(i_Pin->id) : nullptr;
          ImVec2 l_PinAnchor;
          if (!get_pin_anchor(p_Context, p_Node, *i_Pin, p_ScreenMin,
                              p_ScreenMax, l_PinAnchor)) {
            continue;
          }

          const ImU32 l_PinColor = get_pin_color(l_PinMetadata);
          const bool l_PinHovered =
              p_Context.state &&
              p_Context.state->hovered_pin == i_Pin->id;
          const float l_Radius = pin_radius * l_Zoom;

          if (l_PinMetadata &&
              l_PinMetadata->type == PinType::Execution) {
            draw_execution_pin(p_Context.draw_list, l_PinAnchor,
                               i_Pin->is_input(), l_Radius * 1.45f,
                               l_PinColor, l_PinHovered);
          } else {
            draw_data_pin(p_Context.draw_list, l_PinAnchor, l_Radius,
                          l_PinColor, l_PinHovered);
          }

          const char *l_PinLabel =
              l_PinMetadata && !l_PinMetadata->display_name.empty()
                  ? l_PinMetadata->display_name.c_str()
                  : "";
          Util::String l_PinLabelString = l_PinLabel;
          if (l_PinMetadata && l_PinMetadata->container_type ==
                                   PinContainerType::List) {
            l_PinLabelString += "[]";
          }
          const ImVec2 l_LabelSize = calc_scaled_text_size(
              l_PinFont, l_PinFontSize, l_PinLabelString.c_str());
          const ImVec2 l_TextPos =
              i_Pin->direction == PinDirection::Input
                  ? ImVec2(l_PinAnchor.x + l_PinTextOffset,
                           l_PinAnchor.y - l_LabelSize.y * 0.5f)
                  : ImVec2(l_PinAnchor.x - l_PinTextOffset -
                               l_LabelSize.x,
                           l_PinAnchor.y - l_LabelSize.y * 0.5f);

          if (!l_Compact && !l_PinLabelString.empty()) {
            add_scaled_text(p_Context.draw_list, l_PinFont,
                            l_PinFontSize, l_TextPos, l_TextColor,
                            l_PinLabelString.c_str());
          }

          if (l_PinMetadata &&
              l_PinMetadata->show_default_value_when_unlinked &&
              i_Pin->direction == PinDirection::Input &&
              p_Context.graph.get_link_count(i_Pin->id) == 0 &&
              l_PinMetadata->type != PinType::Execution &&
              l_PinMetadata->type != PinType::Dynamic) {
            char l_DefaultValue[128];
            l_DefaultValue[0] = '\0';

            switch (l_PinMetadata->type) {
            case PinType::Bool:
              snprintf(l_DefaultValue, sizeof(l_DefaultValue), "%s",
                       l_PinMetadata->default_value.as_bool()
                           ? "true"
                           : "false");
              break;
            case PinType::Number:
              switch (l_PinMetadata->number_subtype) {
              case NumberSubtype::Float:
                snprintf(l_DefaultValue, sizeof(l_DefaultValue),
                         "%.2f",
                         l_PinMetadata->default_value.as_float());
                break;
              case NumberSubtype::Int32:
                snprintf(l_DefaultValue, sizeof(l_DefaultValue), "%d",
                         (i32)l_PinMetadata->default_value);
                break;
              case NumberSubtype::UInt32:
                snprintf(l_DefaultValue, sizeof(l_DefaultValue), "%u",
                         (u32)l_PinMetadata->default_value);
                break;
              case NumberSubtype::UInt64:
                snprintf(l_DefaultValue, sizeof(l_DefaultValue),
                         "%llu",
                         (unsigned long long)
                             l_PinMetadata->default_value.as_u64());
                break;
              }
              break;
            case PinType::Handle:
              snprintf(l_DefaultValue, sizeof(l_DefaultValue), "%llu",
                       (unsigned long long)
                           l_PinMetadata->default_value.as_u64());
              break;
            case PinType::String:
              if (l_PinMetadata->string_subtype ==
                  StringSubtype::Name) {
                snprintf(
                    l_DefaultValue, sizeof(l_DefaultValue), "%s",
                    l_PinMetadata->default_value.as_name().c_str());
              } else {
                snprintf(
                    l_DefaultValue, sizeof(l_DefaultValue), "%s",
                    l_PinMetadata->default_value.as_string().c_str());
              }
              break;
            default:
              snprintf(l_DefaultValue, sizeof(l_DefaultValue), "%s",
                       pin_type_to_string(l_PinMetadata->type));
              break;
            }

            const ImVec2 l_ValueSize = calc_scaled_text_size(
                l_DefaultFont, l_DefaultFontSize, l_DefaultValue);
            const float l_WidgetStartX =
                l_Compact
                    ? p_ScreenMin.x + 28.0f * l_Zoom
                    : l_TextPos.x + l_LabelSize.x + 16.0f * l_Zoom;
            const float l_WidgetEndX =
                l_Compact ? p_ScreenMax.x - 22.0f * l_Zoom
                          : p_ScreenMax.x - 16.0f * l_Zoom;
            const float l_WidgetWidth =
                l_Compact
                    ? LOW_MATH_MAX(
                          38.0f * l_Zoom,
                          LOW_MATH_MIN(104.0f * l_Zoom,
                                       l_WidgetEndX - l_WidgetStartX))
                    : LOW_MATH_MAX(44.0f * l_Zoom,
                                   l_WidgetEndX - l_WidgetStartX);
            const ImVec2 l_ValueMin = ImVec2(
                l_WidgetStartX, l_PinAnchor.y - 11.0f * l_Zoom);
            const ImVec2 l_ValueMax =
                ImVec2(l_WidgetStartX + l_WidgetWidth,
                       l_PinAnchor.y + 11.0f * l_Zoom);
            bool l_RenderedWidget = false;

            ImGui::PushID((int)i_Pin->id.value);
            if (l_PinMetadata->type == PinType::String &&
                l_PinMetadata->string_subtype ==
                    StringSubtype::String) {
              char l_TextBuffer[256];
              snprintf(
                  l_TextBuffer, sizeof(l_TextBuffer), "%s",
                  l_PinMetadata->default_value.as_string().c_str());

              ImGui::SetCursorScreenPos(
                  ImVec2(l_ValueMin.x, l_PinAnchor.y - 13.0f));
              ImGui::PushItemWidth(l_ValueMax.x - l_ValueMin.x);
              if (Gui::InputText("defaultvalue", l_TextBuffer,
                                 sizeof(l_TextBuffer))) {
                l_PinMetadata->default_value =
                    Util::String(l_TextBuffer);
              }
              if (p_Context.state &&
                  (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                p_Context.state->interacting_with_widget = true;
              }
              ImGui::PopItemWidth();
              l_RenderedWidget = true;
            } else if (l_PinMetadata->type == PinType::Bool) {
              bool l_Value = l_PinMetadata->default_value.as_bool();
              ImGui::SetCursorScreenPos(
                  ImVec2(l_ValueMin.x, l_PinAnchor.y - 9.0f));
              if (Gui::Checkbox("##defaultvalue", &l_Value)) {
                l_PinMetadata->default_value = l_Value;
              }

              if (p_Context.state &&
                  (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                p_Context.state->interacting_with_widget = true;
              }
              l_RenderedWidget = true;
            }

            else if (l_PinMetadata->type == PinType::Number) {
              ImGui::SetCursorScreenPos(ImVec2(
                  l_ValueMin.x,
                  l_PinAnchor.y - (l_Compact ? 10.0f : 13.0f)));
              ImGui::PushItemWidth(l_ValueMax.x - l_ValueMin.x);
              float l_Value = l_PinMetadata->default_value.as_float();

              if (Gui::DragFloatWithButtons(
                      "defaultvalue", &l_Value, 1.0f, 0.0f, 0.0f,
                      "%.3f", l_Compact ? l_Zoom * 0.82f : l_Zoom)) {
                l_PinMetadata->default_value = l_Value;
              }
              if (p_Context.state &&
                  (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                p_Context.state->interacting_with_widget = true;
              }
              ImGui::PopItemWidth();
              l_RenderedWidget = true;
            }

            if (!l_RenderedWidget) {
              p_Context.draw_list->AddRectFilled(
                  l_ValueMin, l_ValueMax, l_ValueBackgroundColor,
                  5.0f * l_Zoom);
              p_Context.draw_list->AddRect(l_ValueMin, l_ValueMax,
                                           IM_COL32(71, 67, 104, 255),
                                           5.0f * l_Zoom);

              const ImVec2 l_ValueTextPos =
                  ImVec2(l_ValueMin.x + 8.0f * l_Zoom,
                         l_PinAnchor.y - l_ValueSize.y * 0.5f);
              add_scaled_text(p_Context.draw_list, l_DefaultFont,
                              l_DefaultFontSize, l_ValueTextPos,
                              IM_COL32(214, 216, 229, 255),
                              l_DefaultValue);
            }
            ImGui::PopID();
          }
        }
      }

      bool NodeRenderer::get_pin_anchor(
          const NodeGraphEditorContext &p_Context,
          const Low::Editor::Node &p_Node, const Low::Editor::Pin &p_Pin,
          const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax,
          ImVec2 &p_Anchor) const
      {
        if (!graph) {
          return false;
        }

        const Node *l_Metadata =
            graph ? graph->find_node(p_Node.id) : nullptr;
        const NodeClass *l_NodeClass =
            graph && l_Metadata
                ? graph->find_node_class(l_Metadata->node_class)
                : nullptr;
        const bool l_Compact =
            l_NodeClass && l_NodeClass->is_compact(*graph, p_Node.id);

        Util::List<Low::Editor::Pin *> l_NodePins =
            graph->graph.get_node_pins(p_Pin.node);

        u32 l_SideIndex = 0;
        u32 l_SideCount = 0;
        u32 l_InputCount = 0;
        u32 l_OutputCount = 0;

        for (Low::Editor::Pin *i_Pin : l_NodePins) {
          if (i_Pin->direction == PinDirection::Input) {
            ++l_InputCount;
          } else {
            ++l_OutputCount;
          }

          if (i_Pin->direction == p_Pin.direction) {
            if (i_Pin->id == p_Pin.id) {
              l_SideIndex = l_SideCount;
            }
            ++l_SideCount;
          }
        }

        if (l_SideCount == 0) {
          return false;
        }

        const float l_HeaderHeight =
            l_Compact ? 8.0f * p_Context.canvas.m_Zoom
                      : title_height * p_Context.canvas.m_Zoom +
                            12.0f * p_Context.canvas.m_Zoom;
        const float l_ContentTop = p_ScreenMin.y + l_HeaderHeight +
                                   (l_Compact ? 14.0f : 4.0f);
        const float l_ContentBottom =
            p_ScreenMax.y - (l_Compact ? 14.0f : 14.0f);
        const u32 l_RowCount =
            LOW_MATH_MAX(l_InputCount, l_OutputCount);
        float l_Y = (l_ContentTop + l_ContentBottom) * 0.5f;
        if (l_Compact) {
          if (l_SideCount == 1) {
            l_Y = (l_ContentTop + l_ContentBottom) * 0.5f;
          } else {
            const float l_CompactStep =
                20.0f * p_Context.canvas.m_Zoom;
            const float l_ClusterHeight =
                l_CompactStep * (float)(l_SideCount - 1);
            const float l_ClusterTop =
                ((l_ContentTop + l_ContentBottom) * 0.5f) -
                (l_ClusterHeight * 0.5f);
            l_Y = l_ClusterTop + l_CompactStep * (float)l_SideIndex;
          }
        } else {
          const float l_Step = (l_ContentBottom - l_ContentTop) /
                               (float)(l_RowCount + 1);
          l_Y = l_ContentTop + l_Step * (float)(l_SideIndex + 1);
        }
        const float l_X =
            p_Pin.is_input() ? p_ScreenMin.x : p_ScreenMax.x;

        p_Anchor = ImVec2(l_X, l_Y);
        return true;
      }

      NodeGraphNodeRenderer *GraphRenderer::get_node_renderer(
          NodeGraphEditorContext &p_Context, Low::Editor::Node &p_Node)
      {
        (void)p_Context;
        (void)p_Node;
        return graph ? &node_renderer : nullptr;
      }

      NodeGraphSpawner *
      GraphRenderer::get_spawner(NodeGraphEditorContext &p_Context)
      {
        (void)p_Context;
        return graph ? &spawn_adapter : nullptr;
      }

      NodeGraphMutationResult<Low::Editor::Link>
      GraphRenderer::create_link(NodeGraphEditorContext &p_Context,
                                 const Low::Editor::Link &p_Link)
      {
        if (!graph) {
          NodeGraphMutationResult<Low::Editor::Link> l_Result;
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidLink;
          return l_Result;
        }

        return graph->add_link(p_Link, p_Context.schema);
      }

      Util::List<NodeGraphSpawnEntry>
      GraphRenderer::SpawnAdapter::get_spawn_entries(
          NodeGraphEditorContext &p_Context) const
      {
        (void)p_Context;

        Util::List<NodeGraphSpawnEntry> l_Entries;
        if (!graph) {
          return l_Entries;
        }

        for (const NodeSpawnEntry *i_Entry :
             graph->get_spawn_entries()) {
          if (!i_Entry || !i_Entry->is_valid()) {
            continue;
          }

          NodeGraphSpawnEntry l_Entry;
          l_Entry.id = i_Entry->id;
          l_Entry.category = i_Entry->category;
          l_Entry.title = i_Entry->title;
          l_Entry.subtitle = i_Entry->subtitle;
          l_Entry.search_text = i_Entry->search_text;
          l_Entries.push_back(l_Entry);
        }

        if (graph->find_node_class(N(vs_syntax_get_variable))) {
          for (const Variable &i_Variable : graph->variables) {
            if (!i_Variable.is_valid()) {
              continue;
            }

            NodeGraphSpawnEntry l_Entry;
            l_Entry.id =
                LOW_NAME((Util::String(g_GetVariableSpawnPrefix) +
                          i_Variable.name)
                             .c_str());
            l_Entry.category = "Syntax";
            l_Entry.title = Util::String("Get ") + i_Variable.name;
            l_Entry.subtitle = "Variable";
            l_Entry.search_text =
                Util::String("get variable ") + i_Variable.name;
            l_Entries.push_back(l_Entry);
          }
        }

        if (graph->find_node_class(N(vs_syntax_set_variable))) {
          for (const Variable &i_Variable : graph->variables) {
            if (!i_Variable.is_valid()) {
              continue;
            }

            NodeGraphSpawnEntry l_Entry;
            l_Entry.id =
                LOW_NAME((Util::String(g_SetVariableSpawnPrefix) +
                          i_Variable.name)
                             .c_str());
            l_Entry.category = "Syntax";
            l_Entry.title = Util::String("Set ") + i_Variable.name;
            l_Entry.subtitle = "Variable";
            l_Entry.search_text =
                Util::String("set variable ") + i_Variable.name;
            l_Entries.push_back(l_Entry);
          }
        }

        return l_Entries;
      }

      bool GraphRenderer::SpawnAdapter::spawn_entry(
          NodeGraphEditorContext &p_Context, Util::Name p_EntryId,
          const Math::Vector2 &p_Position)
      {
        if (!graph) {
          return false;
        }

        Util::String l_GetVariableName =
            get_variable_name_from_spawn_id(p_EntryId,
                                            g_GetVariableSpawnPrefix);
        if (!l_GetVariableName.empty()) {
          Low::Editor::Node l_Node;
          l_Node.id = graph->allocate_node_id();
          l_Node.position = p_Position;

          Node l_Metadata;
          l_Metadata.node = l_Node.id;
          l_Metadata.node_class = N(vs_syntax_get_variable);
          l_Metadata.variable_name = l_GetVariableName;
          l_Metadata.title = Util::String("Get ") + l_GetVariableName;
          l_Metadata.subtitle = "Variable";
          l_Metadata.category = "Syntax";

          NodeGraphMutationResult<Low::Editor::Node> l_Result =
              graph->add_node(l_Node, l_Metadata, p_Context.schema);
          if (!l_Result.succeeded()) {
            return false;
          }

          const NodeClass *l_NodeClass =
              graph->find_node_class(N(vs_syntax_get_variable));
          if (l_NodeClass) {
            l_NodeClass->setup_default_pins(*graph, l_Node.id,
                                            p_Context.schema);
          }
          return true;
        }

        Util::String l_SetVariableName =
            get_variable_name_from_spawn_id(p_EntryId,
                                            g_SetVariableSpawnPrefix);
        if (!l_SetVariableName.empty()) {
          Low::Editor::Node l_Node;
          l_Node.id = graph->allocate_node_id();
          l_Node.position = p_Position;

          Node l_Metadata;
          l_Metadata.node = l_Node.id;
          l_Metadata.node_class = N(vs_syntax_set_variable);
          l_Metadata.variable_name = l_SetVariableName;
          l_Metadata.title = Util::String("Set ") + l_SetVariableName;
          l_Metadata.subtitle = "Variable";
          l_Metadata.category = "Syntax";

          NodeGraphMutationResult<Low::Editor::Node> l_Result =
              graph->add_node(l_Node, l_Metadata, p_Context.schema);
          if (!l_Result.succeeded()) {
            return false;
          }

          const NodeClass *l_NodeClass =
              graph->find_node_class(N(vs_syntax_set_variable));
          if (l_NodeClass) {
            l_NodeClass->setup_default_pins(*graph, l_Node.id,
                                            p_Context.schema);
          }
          return true;
        }

        return graph
            ->create_node_from_spawn_entry(p_EntryId, p_Position,
                                           p_Context.schema)
            .succeeded();
      }

      void GraphRenderer::render_node_context_menu(
          NodeGraphEditorContext &p_Context, Low::Editor::Node &p_Node)
      {
        (void)p_Context;

        const Node *l_NodeMetadata =
            graph ? graph->find_node(p_Node.id) : nullptr;
        Util::String l_Title =
            l_NodeMetadata && !l_NodeMetadata->title.empty()
                ? l_NodeMetadata->title
                : Util::String("Node");

        ImGui::TextDisabled("%s", l_Title.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Delete")) {
          graph->remove_node(p_Node.id);
        }
      }

      void GraphRenderer::render_pin_context_menu(
          NodeGraphEditorContext &p_Context, Low::Editor::Pin &p_Pin)
      {
        (void)p_Context;

        const Pin *l_PinMetadata =
            graph ? graph->find_pin(p_Pin.id) : nullptr;
        Util::String l_Label =
            l_PinMetadata && !l_PinMetadata->display_name.empty()
                ? l_PinMetadata->display_name
                : Util::String("Pin");

        ImGui::TextDisabled("%s", l_Label.c_str());
        ImGui::Separator();

        const bool l_PinHasLinks =
            graph->graph.get_link_count(p_Pin.id) > 0;

        if (ImGui::MenuItem("Break all links", NULL, false,
                            l_PinHasLinks)) {
          graph->graph.clear_links_for_pin(p_Pin.id);
        }
      }

      const char *pin_type_to_string(PinType p_Type)
      {
        switch (p_Type) {
        case PinType::Execution:
          return "Execution";
        case PinType::Bool:
          return "Bool";
        case PinType::Number:
          return "Number";
        case PinType::Vector2:
          return "Vector2";
        case PinType::Vector3:
          return "Vector3";
        case PinType::Vector4:
          return "Vector4";
        case PinType::Quaternion:
          return "Quaternion";
        case PinType::String:
          return "String";
        case PinType::Handle:
          return "Handle";
        case PinType::Dynamic:
          return "Dynamic";
        }

        return "Unknown";
      }

      PinType string_to_pin_type(const Util::String &p_Type)
      {
        if (p_Type == "Execution") {
          return PinType::Execution;
        }
        if (p_Type == "Bool") {
          return PinType::Bool;
        }
        if (p_Type == "Number") {
          return PinType::Number;
        }
        if (p_Type == "Vector2") {
          return PinType::Vector2;
        }
        if (p_Type == "Vector3") {
          return PinType::Vector3;
        }
        if (p_Type == "Vector4") {
          return PinType::Vector4;
        }
        if (p_Type == "Quaternion") {
          return PinType::Quaternion;
        }
        if (p_Type == "String") {
          return PinType::String;
        }
        if (p_Type == "Handle") {
          return PinType::Handle;
        }
        if (p_Type == "Dynamic") {
          return PinType::Dynamic;
        }

        return PinType::Dynamic;
      }

      PinType variant_type_to_pin_type(u8 p_VariantType)
      {
        switch (p_VariantType) {
        case Util::VariantType::Bool:
          return PinType::Bool;
        case Util::VariantType::Int32:
        case Util::VariantType::UInt32:
        case Util::VariantType::UInt64:
        case Util::VariantType::Float:
          return PinType::Number;
        case Util::VariantType::Vector2:
          return PinType::Vector2;
        case Util::VariantType::Vector3:
          return PinType::Vector3;
        case Util::VariantType::Vector4:
          return PinType::Vector4;
        case Util::VariantType::Quaternion:
          return PinType::Quaternion;
        case Util::VariantType::Name:
        case Util::VariantType::String:
          return PinType::String;
        case Util::VariantType::Handle:
          return PinType::Handle;
        default:
          return PinType::Dynamic;
        }
      }

      u8 pin_to_variant_type(const Pin &p_Pin)
      {
        switch (p_Pin.type) {
        case PinType::Bool:
          return Util::VariantType::Bool;
        case PinType::Number:
          switch (p_Pin.number_subtype) {
          case NumberSubtype::Float:
            return Util::VariantType::Float;
          case NumberSubtype::Int32:
            return Util::VariantType::Int32;
          case NumberSubtype::UInt32:
            return Util::VariantType::UInt32;
          case NumberSubtype::UInt64:
            return Util::VariantType::UInt64;
          }
          return Util::VariantType::Float;
        case PinType::Vector2:
          return Util::VariantType::Vector2;
        case PinType::Vector3:
          return Util::VariantType::Vector3;
        case PinType::Vector4:
          return Util::VariantType::Vector4;
        case PinType::Quaternion:
          return Util::VariantType::Quaternion;
        case PinType::String:
          return p_Pin.string_subtype == StringSubtype::Name
                     ? Util::VariantType::Name
                     : Util::VariantType::String;
        case PinType::Handle:
          return Util::VariantType::Handle;
        default:
          return Util::VariantType::String;
        }
      }

      Util::Variant default_value_for_pin(const Pin &p_Pin)
      {
        switch (p_Pin.type) {
        case PinType::Bool:
          return Util::Variant(false);
        case PinType::Number:
          switch (p_Pin.number_subtype) {
          case NumberSubtype::Float:
            return Util::Variant(0.0f);
          case NumberSubtype::Int32:
            return Util::Variant((i32)0);
          case NumberSubtype::UInt32:
            return Util::Variant((u32)0);
          case NumberSubtype::UInt64:
            return Util::Variant((u64)0);
          }
          return Util::Variant(0.0f);
        case PinType::Execution:
          return Util::Variant((u64)0);
        case PinType::Vector2:
          return Util::Variant(Math::Vector2(0.0f, 0.0f));
        case PinType::Vector3:
          return Util::Variant(Math::Vector3(0.0f, 0.0f, 0.0f));
        case PinType::Vector4:
          return Util::Variant(Math::Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        case PinType::Quaternion:
          return Util::Variant(
              Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        case PinType::String:
          if (p_Pin.string_subtype == StringSubtype::Name) {
            return Util::Variant(Util::Name((u32)0));
          }
          return Util::Variant(Util::String(""));
        case PinType::Dynamic:
          return Util::Variant(Util::String(""));
        case PinType::Handle:
          return Util::Variant::from_handle(Util::Handle());
        }

        return Util::Variant(Util::String(""));
      }

      Low::Editor::Pin make_input_pin(Graph &p_Graph, NodeId p_NodeId)
      {
        Low::Editor::Pin l_Pin;
        l_Pin.id = p_Graph.allocate_pin_id();
        l_Pin.node = p_NodeId;
        l_Pin.direction = PinDirection::Input;
        return l_Pin;
      }

      Low::Editor::Pin make_output_pin(Graph &p_Graph, NodeId p_NodeId)
      {
        Low::Editor::Pin l_Pin;
        l_Pin.id = p_Graph.allocate_pin_id();
        l_Pin.node = p_NodeId;
        l_Pin.direction = PinDirection::Output;
        return l_Pin;
      }

      Pin make_execution_pin_metadata(Util::String p_DisplayName)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Execution;
        l_Pin.widget = PinWidget::None;
        l_Pin.show_default_value_when_unlinked = false;
        l_Pin.default_value = Util::Variant((u64)0);
        return l_Pin;
      }

      Pin make_bool_pin_metadata(Util::String p_DisplayName,
                                 bool p_DefaultValue)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Bool;
        l_Pin.default_value = Util::Variant(p_DefaultValue);
        return l_Pin;
      }

      Pin make_number_pin_metadata(Util::String p_DisplayName,
                                   NumberSubtype p_NumberSubtype,
                                   PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Number;
        l_Pin.number_subtype = p_NumberSubtype;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }

      Pin make_string_pin_metadata(Util::String p_DisplayName,
                                   StringSubtype p_StringSubtype,
                                   PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::String;
        l_Pin.string_subtype = p_StringSubtype;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }

      Pin make_handle_pin_metadata(Util::String p_DisplayName,
                                   Util::TypeIdentifier p_HandleType,
                                   PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Handle;
        l_Pin.handle_type = p_HandleType;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }

      Pin make_vector2_pin_metadata(Util::String p_DisplayName,
                                    PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Vector2;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }

      Pin make_vector3_pin_metadata(Util::String p_DisplayName,
                                    PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Vector3;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }

      Pin make_dynamic_pin_metadata(Util::String p_DisplayName,
                                    PinContainerType p_ContainerType)
      {
        Pin l_Pin;
        l_Pin.display_name = p_DisplayName;
        l_Pin.type = PinType::Dynamic;
        l_Pin.container_type = p_ContainerType;
        l_Pin.default_value = default_value_for_pin(l_Pin);
        return l_Pin;
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
