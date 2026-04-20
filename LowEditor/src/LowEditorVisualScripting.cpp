#include "LowEditorVisualScripting.h"

#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include "LowUtilLogger.h"

#include <cfloat>
#include <cstdio>
#include <imgui.h>

namespace Low {
  namespace Editor {
    namespace VisualScripting {
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
      } // namespace

      NodeGraphMutationResult<Editor::Node>
      Graph::add_node(const Editor::Node &p_Node,
                      const Node &p_Metadata,
                      const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Editor::Node> l_Result =
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

      NodeGraphMutationResult<Editor::Pin>
      Graph::add_pin(const Editor::Pin &p_Pin, const Pin &p_Metadata,
                     const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Editor::Pin> l_Result =
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

      NodeGraphMutationResult<Editor::Link>
      Graph::add_link(const Editor::Link &p_Link,
                      const NodeGraphSchema *p_Schema)
      {
        return graph.add_link(p_Link, p_Schema);
      }

      bool Graph::remove_node(NodeId p_NodeId)
      {
        Util::List<PinId> l_PinIdsToRemove;

        for (auto it = pin_metadata.begin(); it != pin_metadata.end();
             ++it) {
          const Editor::Pin *l_Pin = graph.find_pin(it->first);
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

      void Graph::register_node_class(NodeClass &p_NodeClass)
      {
        node_classes[p_NodeClass.get_name()] = &p_NodeClass;
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

      NodeGraphMutationResult<Editor::Node>
      Graph::create_node(Util::Name p_NodeClass,
                         const Math::Vector2 &p_Position,
                         const NodeGraphSchema *p_Schema)
      {
        NodeGraphMutationResult<Editor::Node> l_Result;
        const NodeClass *l_NodeClass = find_node_class(p_NodeClass);

        if (!l_NodeClass) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidNode;
          return l_Result;
        }

        Editor::Node l_Node;
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

      NodeGraphValidationResult
      Schema::can_create_pin(const NodeGraph &p_Graph,
                             const Editor::Pin &p_Pin) const
      {
        return NodeGraphSchema::can_create_pin(p_Graph, p_Pin);
      }

      NodeGraphValidationResult
      Schema::validate_link(const NodeGraph &p_Graph,
                            const Editor::Pin &p_StartPin,
                            const Editor::Pin &p_EndPin) const
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

        return NodeGraphValidationResult::Allowed;
      }

      bool Schema::allows_multiple_links_per_pin(
          const NodeGraph &p_Graph, const Editor::Pin &p_Pin) const
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
      Schema::get_pin_metadata(const Editor::Pin &p_Pin) const
      {
        if (!graph) {
          return nullptr;
        }

        return graph->find_pin(p_Pin.id);
      }

      Math::Vector2 NodeRenderer::get_node_size(
          const NodeGraphEditorContext &p_Context,
          const Editor::Node &p_Node) const
      {
        (void)p_Context;

        if (!graph) {
          return default_node_size;
        }

        u32 l_InputCount = 0;
        u32 l_OutputCount = 0;
        Util::List<Editor::Pin *> l_NodePins =
            graph->graph.get_node_pins(p_Node.id);

        for (Editor::Pin *i_Pin : l_NodePins) {
          if (i_Pin->direction == PinDirection::Input) {
            ++l_InputCount;
          } else {
            ++l_OutputCount;
          }
        }

        const u32 l_MaxPins =
            LOW_MATH_MAX(l_InputCount, l_OutputCount);
        return Math::Vector2(default_node_size.x,
                             LOW_MATH_MAX(default_node_size.y,
                                          82.0f + l_MaxPins * 32.0f));
      }

      void NodeRenderer::render_node(
          NodeGraphEditorContext &p_Context, Editor::Node &p_Node,
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

        const float l_Zoom = p_Context.canvas.m_Zoom;
        const float l_HeaderHeight =
            title_height * l_Zoom + 24.0f * l_Zoom;
        const float l_AccentHeight = 7.0f * l_Zoom;
        const float l_IconBoxSize = 36.0f * l_Zoom;
        const float l_HeaderPaddingX = 14.0f * l_Zoom;
        const float l_HeaderPaddingY = 14.0f * l_Zoom;
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

        p_Context.draw_list->AddRectFilled(p_ScreenMin, p_ScreenMax,
                                           l_BackgroundColor,
                                           border_rounding);
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

        const ImVec2 l_IconBoxMin =
            ImVec2(p_ScreenMin.x + l_HeaderPaddingX,
                   p_ScreenMin.y + l_HeaderPaddingY + 1.0f * l_Zoom);
        const ImVec2 l_HeaderTextPos =
            ImVec2(p_ScreenMin.x + l_HeaderPaddingX +
                       (l_IconString.empty()
                            ? 0.0f
                            : (l_IconBoxSize + 10.0f * l_Zoom)),
                   p_ScreenMin.y + l_HeaderPaddingY + 1.0f * l_Zoom);

        if (!l_IconString.empty()) {
          add_scaled_text(
              p_Context.draw_list, l_IconFont, l_IconFontSize,
              ImVec2(l_IconBoxMin.x, l_IconBoxMin.y + 1.0f * l_Zoom),
              l_TextColor, l_IconString.c_str());
        }

        add_scaled_text(p_Context.draw_list, l_TitleFont,
                        l_TitleFontSize, l_HeaderTextPos, l_TextColor,
                        l_TitleString.c_str());
        if (!l_SubtitleString.empty()) {
          add_scaled_text(p_Context.draw_list, l_SubtitleFont,
                          l_SubtitleFontSize,
                          ImVec2(l_HeaderTextPos.x,
                                 l_HeaderTextPos.y + 22.0f * l_Zoom),
                          l_SubTextColor, l_SubtitleString.c_str());
        }

        Util::List<Editor::Pin *> l_NodePins =
            graph ? graph->graph.get_node_pins(p_Node.id)
                  : Util::List<Editor::Pin *>();

        for (Editor::Pin *i_Pin : l_NodePins) {
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

          if (!l_PinLabelString.empty()) {
            add_scaled_text(p_Context.draw_list, l_PinFont,
                            l_PinFontSize, l_TextPos, l_TextColor,
                            l_PinLabelString.c_str());
          }

          if (l_PinMetadata &&
              l_PinMetadata->show_default_value_when_unlinked &&
              i_Pin->direction == PinDirection::Input &&
              p_Context.graph.get_link_count(i_Pin->id) == 0 &&
              l_PinMetadata->type != PinType::Execution) {
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
                l_TextPos.x + l_LabelSize.x + 16.0f * l_Zoom;
            const float l_WidgetEndX = p_ScreenMax.x - 16.0f * l_Zoom;
            const float l_WidgetWidth = LOW_MATH_MAX(
                44.0f * l_Zoom, l_WidgetEndX - l_WidgetStartX);
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
            ImGui::PopID();

            if (!l_RenderedWidget) {
              p_Context.draw_list->AddRectFilled(
                  l_ValueMin, l_ValueMax, l_ValueBackgroundColor,
                  4.0f * l_Zoom);
              add_scaled_text(
                  p_Context.draw_list, l_DefaultFont,
                  l_DefaultFontSize,
                  ImVec2(l_ValueMin.x + 8.0f * l_Zoom,
                         l_PinAnchor.y - l_ValueSize.y * 0.5f),
                  l_SubTextColor, l_DefaultValue);
            }
          }
        }
      }

      bool NodeRenderer::get_pin_anchor(
          const NodeGraphEditorContext &p_Context,
          const Editor::Node &p_Node, const Editor::Pin &p_Pin,
          const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax,
          ImVec2 &p_Anchor) const
      {
        (void)p_Node;

        if (!graph) {
          return false;
        }

        Util::List<Editor::Pin *> l_NodePins =
            graph->graph.get_node_pins(p_Pin.node);

        u32 l_SideIndex = 0;
        u32 l_SideCount = 0;
        u32 l_InputCount = 0;
        u32 l_OutputCount = 0;

        for (Editor::Pin *i_Pin : l_NodePins) {
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
            title_height * p_Context.canvas.m_Zoom +
            12.0f * p_Context.canvas.m_Zoom;
        const float l_ContentTop =
            p_ScreenMin.y + l_HeaderHeight + 4.0f;
        const float l_ContentBottom = p_ScreenMax.y - 14.0f;
        const u32 l_RowCount =
            LOW_MATH_MAX(l_InputCount, l_OutputCount);
        const float l_Step = (l_ContentBottom - l_ContentTop) /
                             (float)(l_RowCount + 1);
        const float l_Y =
            l_ContentTop + l_Step * (float)(l_SideIndex + 1);
        const float l_X =
            p_Pin.is_input() ? p_ScreenMin.x : p_ScreenMax.x;

        p_Anchor = ImVec2(l_X, l_Y);
        return true;
      }

      NodeGraphNodeRenderer *GraphRenderer::get_node_renderer(
          NodeGraphEditorContext &p_Context, Editor::Node &p_Node)
      {
        (void)p_Context;
        (void)p_Node;
        return graph ? &node_renderer : nullptr;
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

      Editor::Pin make_input_pin(Graph &p_Graph, NodeId p_NodeId)
      {
        Editor::Pin l_Pin;
        l_Pin.id = p_Graph.allocate_pin_id();
        l_Pin.node = p_NodeId;
        l_Pin.direction = PinDirection::Input;
        return l_Pin;
      }

      Editor::Pin make_output_pin(Graph &p_Graph, NodeId p_NodeId)
      {
        Editor::Pin l_Pin;
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
    } // namespace VisualScripting
  } // namespace Editor
} // namespace Low
