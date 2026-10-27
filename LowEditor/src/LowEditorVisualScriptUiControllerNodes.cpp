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

        } // namespace

        Util::Name ElementEventNode::get_name() const
        {
          return N(vs_uicontroller_element_event);
        }

        Util::String
        ElementEventNode::get_title(const Graph &p_Graph) const
        {

          if (element_name.is_valid()) {
            Util::StringBuilder l_Builder;

            l_Builder.append("On ");
            switch (interaction_type) {
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

            l_Builder.append(" ").append(element_name);
            return l_Builder.get();
          }
          return "On UI element event";
        }

        Util::String
        ElementEventNode::get_subtitle(const Graph &) const
        {
          return "UI Event";
        }

        Util::String
        ElementEventNode::get_category(const Graph &) const
        {
          return "UI";
        }

        Util::String ElementEventNode::get_icon(const Graph &) const
        {
          return ICON_LC_MOUSE_POINTER_CLICK;
        }

        ImU32 ElementEventNode::get_color(const Graph &) const
        {
          return g_EventColor;
        }

        void ElementEventNode::setup_default_pins(
            Graph &p_Graph, const NodeGraphSchema *p_Schema) const
        {

          Pin l_ExecOutMetadata = make_execution_pin("Then");
          p_Graph.add_pin(
              make_output_pin(p_Graph, id, l_ExecOutMetadata),
              p_Schema);
        }

        void ElementEventNode::compile(
            Graph &p_Graph, CompileContext &p_CompileContext) const
        {

          /*
          p_CompileContext.main_code.append(
              l_Variable->name.c_str());
          p_CompileContext.main_code.append(" = ");
          compile_input_pin(p_Graph, l_ValuePin->id,
                            p_CompileContext);
          p_CompileContext.main_code.append(";").endl();
          */

          const Pin *l_ThenPin =
              p_Graph.find_output_pin_checked(id, "Then");
          p_Graph.continue_compilation(l_ThenPin->id,
                                       p_CompileContext);
        }

        void
        ElementEventNode::serialize(Util::Serial::Node &p_Data) const
        {

          if (element_name.is_valid()) {
            p_Data["element_name"] = element_name;
          }
          if (element_local_id != 0) {
            p_Data["element_local_id"] =
                Util::U64Id{element_local_id};
          }
          p_Data["interaction_type"] = (u32)interaction_type;
        }

        void ElementEventNode::deserialize(
            const Util::Serial::Node &p_Data)
        {

          if (p_Data["element_name"]) {
            element_name = p_Data["element_name"].as<Util::Name>();
          }
          if (p_Data["element_local_id"]) {
            element_local_id =
                (u64)p_Data["element_local_id"].as<Util::U64Id>();
          }
          if (p_Data["interaction_type"]) {
            interaction_type =
                (InteractionType)p_Data["interaction_type"].as<u32>();
          }
        }

        namespace {

        } // namespace

        void register_nodes(Graph &p_Graph)
        {
          const Util::Name l_ElementEventNodeType =
              p_Graph.register_node_type<ElementEventNode>();

          {
            NodeSpawnEntry l_Entry;
            l_Entry.id = N(vs_spawn_uicontroller_element_event);
            l_Entry.category = "UI";
            l_Entry.title = "Element interaction";
            l_Entry.subtitle = "UI Event";
            l_Entry.search_text =
                "ui element interaction click hover widget";
            l_Entry.node_type = l_ElementEventNodeType;
            p_Graph.register_spawn_entry(l_Entry);
          }
        }
      } // namespace UiControllerNodes
    }   // namespace VisualScript
  }     // namespace Editor
} // namespace Low
