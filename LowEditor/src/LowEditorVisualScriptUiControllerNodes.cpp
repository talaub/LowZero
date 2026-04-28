#include "LowEditorVisualScriptNodes.h"

#include "LowEditorIcons.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace UiControllerNodes {
        namespace {
          static ImU32 g_EventColor = IM_COL32(240, 100, 100, 255);

          struct ElementEventNodeClass : public NodeClass
          {
          public:
            virtual Util::Name get_name() const override
            {
              return N(vs_uicontroller_element_event);
            }

            virtual Util::SharedPtr<NodeUserData>
            create_user_data() const override
            {
              return Util::make_shared<ElementEventNodeData>();
            }

            virtual Util::String
            get_title(const Graph &p_Graph,
                      NodeId p_NodeId) const override
            {
              const ElementEventNodeData *l_Data =
                  p_Graph.get_node_user_data<ElementEventNodeData>(
                      p_NodeId);

              if (l_Data && l_Data->element_name.is_valid()) {
                Util::StringBuilder l_Builder;

                l_Builder.append("On ");
                switch (l_Data->interaction_type) {
                case InteractionType::Click:
                  l_Builder.append("click");
                  break;
                case InteractionType::MouseEnter:
                  l_Builder.append("mouse enters");
                  break;
                case InteractionType::MouseExit:
                  l_Builder.append("mouse exits");
                  break;
                default:
                  l_Builder.append("interact");
                  break;
                }

                l_Builder.append(" ").append(l_Data->element_name);
                return l_Builder.get();
              }
              return "On UI element event";
            }

            virtual Util::String get_subtitle(const Graph &,
                                              NodeId) const override
            {
              return "UI Event";
            }

            virtual Util::String get_category(const Graph &,
                                              NodeId) const override
            {
              return "UI";
            }

            virtual Util::String get_icon(const Graph &,
                                          NodeId) const override
            {
              return ICON_LC_MOUSE_POINTER_CLICK;
            }

            virtual ImU32 get_color(const Graph &,
                                    NodeId) const override
            {
              return g_EventColor;
            }

            virtual void setup_default_pins(
                Graph &p_Graph, NodeId p_NodeId,
                const NodeGraphSchema *p_Schema) const override
            {
              Editor::Pin l_ExecOut =
                  make_output_pin(p_Graph, p_NodeId);
              Pin l_ExecOutMetadata =
                  make_execution_pin_metadata("Then");
              p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);
            }

            virtual void
            compile(Graph &p_Graph, NodeId p_NodeId,
                    CompileContext &p_CompileContext) const override
            {

              /*
              p_CompileContext.main_code.append(
                  l_Variable->name.c_str());
              p_CompileContext.main_code.append(" = ");
              compile_input_pin(p_Graph, p_NodeId, l_ValuePin->pin,
                                p_CompileContext);
              p_CompileContext.main_code.append(";").endl();
              */

              const Pin *l_ThenPin =
                  p_Graph.find_output_pin_checked(p_NodeId, "Then");
              p_Graph.continue_compilation(l_ThenPin->pin,
                                           p_CompileContext);
            }

            virtual void serialize(Graph &p_Graph, NodeId p_NodeId,
                                   Util::Serial::Node &p_Data) const override
            {
              const ElementEventNodeData *l_Data =
                  p_Graph.get_node_user_data<ElementEventNodeData>(
                      p_NodeId);
              if (!l_Data) {
                return;
              }

              if (l_Data->element_name.is_valid()) {
                p_Data["element_name"] = l_Data->element_name;
              }
              if (l_Data->element_local_id != 0) {
                p_Data["element_local_id"] =
                    Util::U64Id{l_Data->element_local_id};
              }
              p_Data["interaction_type"] =
                  (u32)l_Data->interaction_type;
            }

            virtual void deserialize(
                Graph &p_Graph, NodeId p_NodeId,
                Util::Serial::Node &p_Data) const override
            {
              ElementEventNodeData *l_Data =
                  p_Graph.get_node_user_data<ElementEventNodeData>(
                      p_NodeId);
              if (!l_Data) {
                return;
              }

              if (p_Data["element_name"]) {
                l_Data->element_name =
                    p_Data["element_name"].as<Util::Name>();
              }
              if (p_Data["element_local_id"]) {
                l_Data->element_local_id =
                    (u64)p_Data["element_local_id"]
                        .as<Util::U64Id>();
              }
              if (p_Data["interaction_type"]) {
                l_Data->interaction_type = (InteractionType)
                    p_Data["interaction_type"].as<u32>();
              }

              p_Graph.refresh_node_display_metadata(p_NodeId);
            }
          };

          static ElementEventNodeClass g_ElementEventNodeClass;
        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          p_Graph.register_node_class(g_ElementEventNodeClass);

          {
            NodeSpawnEntry l_Entry;
            l_Entry.id = N(vs_spawn_uicontroller_element_event);
            l_Entry.category = "UI";
            l_Entry.title = "Element interaction";
            l_Entry.subtitle = "UI Event";
            l_Entry.search_text =
                "ui element interaction click hover widget";
            l_Entry.node_class = g_ElementEventNodeClass.get_name();
            p_Graph.register_spawn_entry(l_Entry);
          }
        }
      } // namespace UiControllerNodes
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
