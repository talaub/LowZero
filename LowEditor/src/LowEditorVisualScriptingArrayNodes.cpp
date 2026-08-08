#include "LowCoreScripting.h"
#include "LowEditor.h"
#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace ArrayNodes {
        namespace {
          static ImU32 g_ArrayColor = IM_COL32(189, 128, 53, 255);
          static const char *g_ArrayPinName = "Array";
          static const char *g_ElementPinName = "Element";
          static const char *g_IndexPinName = "Index";
          static const char *g_ValuePinName = "Value";
          static const char *g_LengthPinName = "Length";

          struct AddToArrayNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_add_to_array);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Add To Array";
            }

            virtual Util::String
            get_subtitle(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_LIST_PLUS;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_ArrayColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecIn,
                              make_execution_pin_metadata("Exec"),
                              p_Schema);

              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ExecOut,
                              make_execution_pin_metadata("Then"),
                              p_Schema);

              Editor::Pin l_ArrayIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(
                  l_ArrayIn,
                  make_dynamic_pin_metadata(g_ArrayPinName,
                                            PinContainerType::List),
                  p_Schema);

              Editor::Pin l_ElementIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(
                  l_ElementIn,
                  make_dynamic_pin_metadata(g_ElementPinName),
                  p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {
              const Pin *l_ArrayPin = p_Graph.find_input_pin_checked(
                  p_NodeId, g_ArrayPinName);
              const Pin *l_ElementPin =
                  p_Graph.find_input_pin_checked(p_NodeId,
                                                 g_ElementPinName);
              LOW_ASSERT(l_ArrayPin && l_ElementPin,
                        "Add to array node is missing pins");

              if (l_ArrayPin && l_ElementPin) {
                p_Graph.compile_input_pin(l_ArrayPin->pin,
                                          p_CompileContext);
                p_CompileContext.main_code.append(".insertLast(");
                p_Graph.compile_input_pin(l_ElementPin->pin,
                                          p_CompileContext);
                p_CompileContext.main_code.append(");").endl();
              }

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }
          };

          struct GetArrayEntryNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_get_array_entry);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Get Array Entry";
            }

            virtual Util::String
            get_subtitle(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_LIST;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_ArrayColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ArrayIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(
                  l_ArrayIn,
                  make_dynamic_pin_metadata(g_ArrayPinName,
                                            PinContainerType::List),
                  p_Schema);

              Editor::Pin l_IndexIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(
                  l_IndexIn,
                  make_number_pin_metadata(g_IndexPinName,
                                          NumberSubtype::Int32),
                  p_Schema);

              Editor::Pin l_ValueOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ValueMetadata =
                  make_dynamic_pin_metadata(g_ValuePinName);
              l_ValueMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_ValueOut, l_ValueMetadata, p_Schema);
            }

            virtual void
            on_pin_connected(Graph &p_Graph, NodeId p_NodeId,
                            PinId p_PinId,
                            PinId p_OtherPinId) const override
            {
              const Pin *l_ThisPin = p_Graph.find_pin(p_PinId);
              if (!l_ThisPin ||
                  l_ThisPin->container_type !=
                      PinContainerType::List) {
                return;
              }

              const Pin *l_OtherPin = p_Graph.find_pin(p_OtherPinId);
              if (!l_OtherPin ||
                  l_OtherPin->type == PinType::Dynamic) {
                return;
              }

              Pin *l_ValuePin = p_Graph.find_output_pin_checked(
                  p_NodeId, g_ValuePinName);
              if (!l_ValuePin) {
                return;
              }

              l_ValuePin->type = l_OtherPin->type;
              l_ValuePin->number_subtype = l_OtherPin->number_subtype;
              l_ValuePin->string_subtype = l_OtherPin->string_subtype;
              l_ValuePin->handle_type = l_OtherPin->handle_type;
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const Pin *l_ArrayPin = p_Graph.find_input_pin_checked(
                  p_NodeId, g_ArrayPinName);
              const Pin *l_IndexPin = p_Graph.find_input_pin_checked(
                  p_NodeId, g_IndexPinName);
              LOW_ASSERT(l_ArrayPin && l_IndexPin,
                        "Get array entry node is missing pins");
              if (!l_ArrayPin || !l_IndexPin) {
                return;
              }

              p_Graph.compile_input_pin(l_ArrayPin->pin,
                                        p_CompileContext);
              p_CompileContext.main_code.append("[");
              p_Graph.compile_input_pin(l_IndexPin->pin,
                                        p_CompileContext);
              p_CompileContext.main_code.append("]");
            }
          };

          struct GetArrayLengthNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_get_array_length);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Get Array Length";
            }

            virtual Util::String
            get_subtitle(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_LIST;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_ArrayColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ArrayIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(
                  l_ArrayIn,
                  make_dynamic_pin_metadata(g_ArrayPinName,
                                            PinContainerType::List),
                  p_Schema);

              Editor::Pin l_LengthOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_LengthMetadata = make_number_pin_metadata(
                  g_LengthPinName, NumberSubtype::UInt32);
              l_LengthMetadata.show_default_value_when_unlinked =
                  false;
              p_Graph.add_pin(l_LengthOut, l_LengthMetadata,
                              p_Schema);
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              const Pin *l_ArrayPin = p_Graph.find_input_pin_checked(
                  p_NodeId, g_ArrayPinName);
              LOW_ASSERT(l_ArrayPin,
                        "Get array length node is missing array pin");
              if (!l_ArrayPin) {
                return;
              }

              p_Graph.compile_input_pin(l_ArrayPin->pin,
                                        p_CompileContext);
              p_CompileContext.main_code.append(".length()");
            }
          };

          struct MakeArrayNodeClass : public NodeClass
          {
            virtual Util::Name get_name() const override
            {
              return N(vs_syntax_make_array);
            }

            virtual Util::String
            get_title(const Graph &, NodeId) const override
            {
              return "Make Array";
            }

            virtual Util::String
            get_subtitle(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String
            get_category(const Graph &, NodeId) const override
            {
              return "Array";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_LIST;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_ArrayColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ElementIn =
                  make_input_pin(p_Graph, p_NodeId);
              p_Graph.add_pin(l_ElementIn,
                              make_dynamic_pin_metadata(""),
                              p_Schema);

              Editor::Pin l_ArrayOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ArrayMetadata = make_dynamic_pin_metadata(
                  "", PinContainerType::List);
              l_ArrayMetadata.show_default_value_when_unlinked = false;
              p_Graph.add_pin(l_ArrayOut, l_ArrayMetadata, p_Schema);
            }

            virtual bool can_connect_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                const Pin &p_PinMetadata,
                const Pin &p_OtherPinMetadata) const override
            {
              (void)p_Graph;
              (void)p_NodeId;
              (void)p_PinId;
              if (p_PinMetadata.type == PinType::Dynamic &&
                  p_PinMetadata.container_type ==
                      PinContainerType::None) {
                return p_OtherPinMetadata.container_type ==
                      PinContainerType::None;
              }
              return true;
            }

            virtual void
            on_pin_connected(Graph &p_Graph, NodeId p_NodeId,
                            PinId p_PinId,
                            PinId p_OtherPinId) const override
            {
              const Pin *l_ThisPin = p_Graph.find_pin(p_PinId);
              if (!l_ThisPin ||
                  l_ThisPin->container_type !=
                      PinContainerType::None) {
                return;
              }

              const Pin *l_OtherPin = p_Graph.find_pin(p_OtherPinId);
              if (!l_OtherPin ||
                  l_OtherPin->type == PinType::Dynamic) {
                return;
              }

              for (Editor::Pin *i_Pin :
                   p_Graph.graph.get_node_pins(p_NodeId)) {
                Pin *l_PinMetadata = p_Graph.find_pin(i_Pin->id);
                if (!l_PinMetadata) {
                  continue;
                }

                const bool l_WasDynamic =
                    l_PinMetadata->type == PinType::Dynamic;
                l_PinMetadata->type = l_OtherPin->type;
                l_PinMetadata->number_subtype =
                    l_OtherPin->number_subtype;
                l_PinMetadata->string_subtype =
                    l_OtherPin->string_subtype;
                l_PinMetadata->handle_type = l_OtherPin->handle_type;
                if (l_WasDynamic &&
                    i_Pin->direction == PinDirection::Input) {
                  l_PinMetadata->default_value =
                      default_value_for_pin(*l_PinMetadata);
                }
              }
            }

            virtual void compile_output_pin(
                Graph &p_Graph, NodeId p_NodeId, PinId p_PinId,
                CompileContext &p_CompileContext) const override
            {
              (void)p_PinId;
              Util::List<Editor::Pin *> l_ElementPins;
              for (Editor::Pin *i_Pin :
                   p_Graph.graph.get_node_pins(p_NodeId)) {
                if (i_Pin->direction == PinDirection::Input) {
                  l_ElementPins.push_back(i_Pin);
                }
              }

              Util::String l_ElementTypeStr = "float";
              if (!l_ElementPins.empty()) {
                const Pin *l_First =
                    p_Graph.find_pin(l_ElementPins[0]->id);
                if (l_First) {
                  Util::String l_Resolved =
                      pin_type_to_script_type_string(
                          l_First->type, l_First->number_subtype,
                          l_First->string_subtype,
                          l_First->handle_type);
                  if (!l_Resolved.empty()) {
                    l_ElementTypeStr = l_Resolved;
                  }
                }
              }

              p_CompileContext.main_code.append("array<");
              p_CompileContext.main_code.append(l_ElementTypeStr);
              p_CompileContext.main_code.append(">{");
              for (u32 i = 0; i < l_ElementPins.size(); ++i) {
                if (i > 0) {
                  p_CompileContext.main_code.append(", ");
                }
                p_Graph.compile_input_pin(l_ElementPins[i]->id,
                                          p_CompileContext);
              }
              p_CompileContext.main_code.append("}");
            }

            virtual float
            get_below_pins_height(const Graph &,
                                  NodeId) const override
            {
              return 26.0f;
            }

            virtual void render_below_pins(
                Graph &p_Graph, NodeId p_NodeId,
                NodeGraphEditorContext &p_Context,
                const ImVec2 &p_RectMin,
                const ImVec2 &p_RectMax) const override
            {
              (void)p_RectMax;
              Util::List<Editor::Pin *> l_ElementPins;
              for (Editor::Pin *i_Pin :
                   p_Graph.graph.get_node_pins(p_NodeId)) {
                if (i_Pin->direction == PinDirection::Input) {
                  l_ElementPins.push_back(i_Pin);
                }
              }

              ImGui::PushID((int)p_NodeId.value);
              ImGui::SetCursorScreenPos(
                  ImVec2(p_RectMin.x + 6.0f, p_RectMin.y + 1.0f));

              if (ImGui::Button("+", ImVec2(22.0f, 0.0f))) {
                Pin l_Template = make_dynamic_pin_metadata("");
                if (!l_ElementPins.empty()) {
                  const Pin *l_First =
                      p_Graph.find_pin(l_ElementPins[0]->id);
                  if (l_First) {
                    l_Template = *l_First;
                    l_Template.display_name = "";
                  }
                }
                Editor::Pin l_NewPin =
                    make_input_pin(p_Graph, p_NodeId);
                p_Graph.add_pin(l_NewPin, l_Template,
                                p_Context.schema);
              }
              if (p_Context.state &&
                  (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                p_Context.state->interacting_with_widget = true;
              }

              ImGui::SameLine();
              if (ImGui::Button("-", ImVec2(22.0f, 0.0f)) &&
                  l_ElementPins.size() > 1) {
                p_Graph.remove_pin(
                    l_ElementPins[l_ElementPins.size() - 1]->id);
              }
              if (p_Context.state &&
                  (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                p_Context.state->interacting_with_widget = true;
              }
              ImGui::PopID();
            }
          };

          static AddToArrayNodeClass g_AddToArrayNodeClass;
          static GetArrayEntryNodeClass g_GetArrayEntryNodeClass;
          static GetArrayLengthNodeClass g_GetArrayLengthNodeClass;
          static MakeArrayNodeClass g_MakeArrayNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_AddToArrayNodeClass);
          p_Graph.register_node_class(g_GetArrayEntryNodeClass);
          p_Graph.register_node_class(g_GetArrayLengthNodeClass);
          p_Graph.register_node_class(g_MakeArrayNodeClass);

          NodeSpawnEntry l_AddToArrayEntry;
          l_AddToArrayEntry.id = N(vs_spawn_array_add_to_array);
          l_AddToArrayEntry.category = "Array";
          l_AddToArrayEntry.title = "Add To Array";
          l_AddToArrayEntry.subtitle = "Array";
          l_AddToArrayEntry.search_text = "add to array insert append";
          l_AddToArrayEntry.node_class =
              g_AddToArrayNodeClass.get_name();
          p_Graph.register_spawn_entry(l_AddToArrayEntry);

          NodeSpawnEntry l_GetArrayEntryEntry;
          l_GetArrayEntryEntry.id = N(vs_spawn_array_get_entry);
          l_GetArrayEntryEntry.category = "Array";
          l_GetArrayEntryEntry.title = "Get Array Entry";
          l_GetArrayEntryEntry.subtitle = "Array";
          l_GetArrayEntryEntry.search_text =
              "get array entry index element";
          l_GetArrayEntryEntry.node_class =
              g_GetArrayEntryNodeClass.get_name();
          p_Graph.register_spawn_entry(l_GetArrayEntryEntry);

          NodeSpawnEntry l_GetArrayLengthEntry;
          l_GetArrayLengthEntry.id = N(vs_spawn_array_get_length);
          l_GetArrayLengthEntry.category = "Array";
          l_GetArrayLengthEntry.title = "Get Array Length";
          l_GetArrayLengthEntry.subtitle = "Array";
          l_GetArrayLengthEntry.search_text = "array length size count";
          l_GetArrayLengthEntry.node_class =
              g_GetArrayLengthNodeClass.get_name();
          p_Graph.register_spawn_entry(l_GetArrayLengthEntry);

          NodeSpawnEntry l_MakeArrayEntry;
          l_MakeArrayEntry.id = N(vs_spawn_array_make_array);
          l_MakeArrayEntry.category = "Array";
          l_MakeArrayEntry.title = "Make Array";
          l_MakeArrayEntry.subtitle = "Array";
          l_MakeArrayEntry.search_text = "make array construct list";
          l_MakeArrayEntry.node_class = g_MakeArrayNodeClass.get_name();
          p_Graph.register_spawn_entry(l_MakeArrayEntry);
        }
      } // namespace ArrayNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
