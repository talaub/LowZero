#pragma once

#include "LowMath.h"
#include "LowUtilContainers.h"
#include <imgui.h>

namespace Low {
  namespace Editor {
    struct NodeGraphSchema;

    struct NodeId
    {
      u64 value = 0;

      bool is_valid() const
      {
        return value != 0;
      }

      bool operator==(const NodeId &p_Other) const
      {
        return value == p_Other.value;
      }

      bool operator!=(const NodeId &p_Other) const
      {
        return value != p_Other.value;
      }

      bool operator<(const NodeId &p_Other) const
      {
        return value < p_Other.value;
      }
    };

    struct PinId
    {
      u64 value = 0;

      bool is_valid() const
      {
        return value != 0;
      }

      bool operator==(const PinId &p_Other) const
      {
        return value == p_Other.value;
      }

      bool operator!=(const PinId &p_Other) const
      {
        return value != p_Other.value;
      }

      bool operator<(const PinId &p_Other) const
      {
        return value < p_Other.value;
      }
    };

    struct LinkId
    {
      u64 value = 0;

      bool is_valid() const
      {
        return value != 0;
      }

      bool operator==(const LinkId &p_Other) const
      {
        return value == p_Other.value;
      }

      bool operator!=(const LinkId &p_Other) const
      {
        return value != p_Other.value;
      }

      bool operator<(const LinkId &p_Other) const
      {
        return value < p_Other.value;
      }
    };

    enum class PinDirection
    {
      Input,
      Output
    };

    enum class NodeGraphValidationResult
    {
      Allowed,
      InvalidNode,
      InvalidPin,
      InvalidLink,
      DuplicateNode,
      DuplicatePin,
      DuplicateLinkId,
      SamePin,
      SameDirection,
      PinNodeMismatch,
      DuplicateLink,
      CustomRejected
    };

    template <typename TValue> struct NodeGraphMutationResult
    {
      NodeGraphValidationResult validation_result =
          NodeGraphValidationResult::Allowed;
      TValue *value = nullptr;

      bool succeeded() const
      {
        return validation_result ==
                   NodeGraphValidationResult::Allowed &&
               value != nullptr;
      }
    };

    struct Node
    {
      NodeId id;
      Math::Vector2 position = Math::Vector2(0.0f, 0.0f);

      bool is_valid() const
      {
        return id.is_valid();
      }

      void set_position(const Math::Vector2 &p_Position)
      {
        position = p_Position;
      }

      void translate(const Math::Vector2 &p_Delta)
      {
        position += p_Delta;
      }
    };

    struct Pin
    {
      PinId id;
      NodeId node;
      PinDirection direction = PinDirection::Input;

      bool is_valid() const
      {
        return id.is_valid() && node.is_valid();
      }

      bool is_input() const
      {
        return direction == PinDirection::Input;
      }

      bool is_output() const
      {
        return direction == PinDirection::Output;
      }
    };

    struct Link
    {
      LinkId id;
      PinId start_pin;
      PinId end_pin;

      bool is_valid() const
      {
        return id.is_valid() && start_pin.is_valid() &&
               end_pin.is_valid();
      }
    };

    struct NodeGraph
    {
      Util::List<Node> nodes;
      Util::List<Pin> pins;
      Util::List<Link> links;

      NodeGraphMutationResult<Node>
      add_node(const Node &p_Node,
               const NodeGraphSchema *p_Schema = nullptr);

      NodeGraphMutationResult<Pin>
      add_pin(const Pin &p_Pin,
              const NodeGraphSchema *p_Schema = nullptr);

      NodeGraphMutationResult<Link>
      add_link(const Link &p_Link,
               const NodeGraphSchema *p_Schema = nullptr);

      Node *find_node(NodeId p_Id)
      {
        for (Node &i_Node : nodes) {
          if (i_Node.id == p_Id) {
            return &i_Node;
          }
        }
        return nullptr;
      }

      const Node *find_node(NodeId p_Id) const
      {
        for (const Node &i_Node : nodes) {
          if (i_Node.id == p_Id) {
            return &i_Node;
          }
        }
        return nullptr;
      }

      Pin *find_pin(PinId p_Id)
      {
        for (Pin &i_Pin : pins) {
          if (i_Pin.id == p_Id) {
            return &i_Pin;
          }
        }
        return nullptr;
      }

      const Pin *find_pin(PinId p_Id) const
      {
        for (const Pin &i_Pin : pins) {
          if (i_Pin.id == p_Id) {
            return &i_Pin;
          }
        }
        return nullptr;
      }

      Link *find_link(LinkId p_Id)
      {
        for (Link &i_Link : links) {
          if (i_Link.id == p_Id) {
            return &i_Link;
          }
        }
        return nullptr;
      }

      const Link *find_link(LinkId p_Id) const
      {
        for (const Link &i_Link : links) {
          if (i_Link.id == p_Id) {
            return &i_Link;
          }
        }
        return nullptr;
      }

      Util::List<Pin *> get_node_pins(NodeId p_NodeId)
      {
        Util::List<Pin *> l_Result;

        for (Pin &i_Pin : pins) {
          if (i_Pin.node == p_NodeId) {
            l_Result.push_back(&i_Pin);
          }
        }

        return l_Result;
      }

      Util::List<const Pin *> get_node_pins(NodeId p_NodeId) const
      {
        Util::List<const Pin *> l_Result;

        for (const Pin &i_Pin : pins) {
          if (i_Pin.node == p_NodeId) {
            l_Result.push_back(&i_Pin);
          }
        }

        return l_Result;
      }

      bool has_link_between(PinId p_StartPinId,
                            PinId p_EndPinId) const
      {
        for (const Link &i_Link : links) {
          const bool l_SameOrder = i_Link.start_pin == p_StartPinId &&
                                   i_Link.end_pin == p_EndPinId;
          const bool l_ReverseOrder =
              i_Link.start_pin == p_EndPinId &&
              i_Link.end_pin == p_StartPinId;

          if (l_SameOrder || l_ReverseOrder) {
            return true;
          }
        }

        return false;
      }

      u32 get_link_count(PinId p_PinId) const
      {
        u32 l_Count = 0;

        for (const Link &i_Link : links) {
          if (i_Link.start_pin == p_PinId ||
              i_Link.end_pin == p_PinId) {
            ++l_Count;
          }
        }

        return l_Count;
      }

      bool remove_node(NodeId p_Id)
      {
        Util::List<PinId> l_PinIdsToRemove;

        for (const Pin &i_Pin : pins) {
          if (i_Pin.node == p_Id) {
            l_PinIdsToRemove.push_back(i_Pin.id);
          }
        }

        for (PinId i_PinId : l_PinIdsToRemove) {
          remove_pin(i_PinId);
        }

        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
          if (it->id == p_Id) {
            nodes.erase(it);
            return true;
          }
        }

        return false;
      }

      bool remove_pin(PinId p_Id)
      {
        for (auto it = links.begin(); it != links.end();) {
          if (it->start_pin == p_Id || it->end_pin == p_Id) {
            it = links.erase(it);
          } else {
            ++it;
          }
        }

        for (auto it = pins.begin(); it != pins.end(); ++it) {
          if (it->id == p_Id) {
            pins.erase(it);
            return true;
          }
        }

        return false;
      }

      bool remove_link(LinkId p_Id)
      {
        for (auto it = links.begin(); it != links.end(); ++it) {
          if (it->id == p_Id) {
            links.erase(it);
            return true;
          }
        }

        return false;
      }
    };

    struct NodeGraphSchema
    {
      virtual ~NodeGraphSchema() = default;

      virtual NodeGraphValidationResult
      can_create_node(const NodeGraph &p_Graph,
                      const Node &p_Node) const
      {
        (void)p_Graph;
        return p_Node.is_valid()
                   ? NodeGraphValidationResult::Allowed
                   : NodeGraphValidationResult::InvalidNode;
      }

      virtual NodeGraphValidationResult
      can_create_pin(const NodeGraph &p_Graph, const Pin &p_Pin) const
      {
        if (!p_Pin.is_valid()) {
          return NodeGraphValidationResult::InvalidPin;
        }

        if (!p_Graph.find_node(p_Pin.node)) {
          return NodeGraphValidationResult::InvalidNode;
        }

        return NodeGraphValidationResult::Allowed;
      }

      virtual NodeGraphValidationResult
      can_create_link(const NodeGraph &p_Graph, const Pin &p_StartPin,
                      const Pin &p_EndPin) const
      {
        if (!p_StartPin.is_valid() || !p_EndPin.is_valid()) {
          return NodeGraphValidationResult::InvalidPin;
        }

        if (p_StartPin.id == p_EndPin.id) {
          return NodeGraphValidationResult::SamePin;
        }

        const Node *l_StartNode = p_Graph.find_node(p_StartPin.node);
        const Node *l_EndNode = p_Graph.find_node(p_EndPin.node);

        if (!l_StartNode || !l_EndNode) {
          return NodeGraphValidationResult::InvalidNode;
        }

        const Pin *l_StartPin = p_Graph.find_pin(p_StartPin.id);
        const Pin *l_EndPin = p_Graph.find_pin(p_EndPin.id);

        if (!l_StartPin || !l_EndPin) {
          return NodeGraphValidationResult::InvalidPin;
        }

        if (l_StartPin->node != p_StartPin.node ||
            l_EndPin->node != p_EndPin.node) {
          return NodeGraphValidationResult::PinNodeMismatch;
        }

        if (p_StartPin.direction == p_EndPin.direction) {
          return NodeGraphValidationResult::SameDirection;
        }

        if (p_Graph.has_link_between(p_StartPin.id, p_EndPin.id)) {
          return NodeGraphValidationResult::DuplicateLink;
        }

        NodeGraphValidationResult l_StartPinResult =
            can_add_link_to_pin(p_Graph, p_StartPin);
        if (l_StartPinResult != NodeGraphValidationResult::Allowed) {
          return l_StartPinResult;
        }

        NodeGraphValidationResult l_EndPinResult =
            can_add_link_to_pin(p_Graph, p_EndPin);
        if (l_EndPinResult != NodeGraphValidationResult::Allowed) {
          return l_EndPinResult;
        }

        return validate_link(p_Graph, p_StartPin, p_EndPin);
      }

      virtual NodeGraphValidationResult
      validate_link(const NodeGraph &p_Graph, const Pin &p_StartPin,
                    const Pin &p_EndPin) const
      {
        (void)p_Graph;
        (void)p_StartPin;
        (void)p_EndPin;
        return NodeGraphValidationResult::Allowed;
      }

      virtual bool
      allows_multiple_links_per_pin(const NodeGraph &p_Graph,
                                    const Pin &p_Pin) const
      {
        (void)p_Graph;
        (void)p_Pin;
        return true;
      }

      virtual NodeGraphValidationResult
      can_add_link_to_pin(const NodeGraph &p_Graph,
                          const Pin &p_Pin) const
      {
        if (!p_Pin.is_valid()) {
          return NodeGraphValidationResult::InvalidPin;
        }

        if (allows_multiple_links_per_pin(p_Graph, p_Pin)) {
          return NodeGraphValidationResult::Allowed;
        }

        return p_Graph.get_link_count(p_Pin.id) == 0
                   ? NodeGraphValidationResult::Allowed
                   : NodeGraphValidationResult::CustomRejected;
      }
    };

    inline const char *node_graph_validation_result_to_string(
        NodeGraphValidationResult p_Result)
    {
      switch (p_Result) {
      case NodeGraphValidationResult::Allowed:
        return "Allowed";
      case NodeGraphValidationResult::InvalidNode:
        return "InvalidNode";
      case NodeGraphValidationResult::InvalidPin:
        return "InvalidPin";
      case NodeGraphValidationResult::InvalidLink:
        return "InvalidLink";
      case NodeGraphValidationResult::DuplicateNode:
        return "DuplicateNode";
      case NodeGraphValidationResult::DuplicatePin:
        return "DuplicatePin";
      case NodeGraphValidationResult::DuplicateLinkId:
        return "DuplicateLinkId";
      case NodeGraphValidationResult::SamePin:
        return "SamePin";
      case NodeGraphValidationResult::SameDirection:
        return "SameDirection";
      case NodeGraphValidationResult::PinNodeMismatch:
        return "PinNodeMismatch";
      case NodeGraphValidationResult::DuplicateLink:
        return "DuplicateLink";
      case NodeGraphValidationResult::CustomRejected:
        return "CustomRejected";
      }

      return "Unknown";
    };

    inline NodeGraphMutationResult<Node>
    NodeGraph::add_node(const Node &p_Node,
                        const NodeGraphSchema *p_Schema)
    {
      NodeGraphMutationResult<Node> l_Result;

      if (find_node(p_Node.id)) {
        l_Result.validation_result =
            NodeGraphValidationResult::DuplicateNode;
        return l_Result;
      }

      if (p_Schema) {
        l_Result.validation_result =
            p_Schema->can_create_node(*this, p_Node);
        if (l_Result.validation_result !=
            NodeGraphValidationResult::Allowed) {
          return l_Result;
        }
      } else if (!p_Node.is_valid()) {
        l_Result.validation_result =
            NodeGraphValidationResult::InvalidNode;
        return l_Result;
      }

      nodes.push_back(p_Node);
      l_Result.value = &nodes.back();
      return l_Result;
    }

    inline NodeGraphMutationResult<Pin>
    NodeGraph::add_pin(const Pin &p_Pin,
                       const NodeGraphSchema *p_Schema)
    {
      NodeGraphMutationResult<Pin> l_Result;

      if (find_pin(p_Pin.id)) {
        l_Result.validation_result =
            NodeGraphValidationResult::DuplicatePin;
        return l_Result;
      }

      if (p_Schema) {
        l_Result.validation_result =
            p_Schema->can_create_pin(*this, p_Pin);
        if (l_Result.validation_result !=
            NodeGraphValidationResult::Allowed) {
          return l_Result;
        }
      } else {
        if (!p_Pin.is_valid()) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidPin;
          return l_Result;
        }

        if (!find_node(p_Pin.node)) {
          l_Result.validation_result =
              NodeGraphValidationResult::InvalidNode;
          return l_Result;
        }
      }

      pins.push_back(p_Pin);
      l_Result.value = &pins.back();
      return l_Result;
    }

    inline NodeGraphMutationResult<Link>
    NodeGraph::add_link(const Link &p_Link,
                        const NodeGraphSchema *p_Schema)
    {
      NodeGraphMutationResult<Link> l_Result;

      if (find_link(p_Link.id)) {
        l_Result.validation_result =
            NodeGraphValidationResult::DuplicateLinkId;
        return l_Result;
      }

      if (!p_Link.is_valid()) {
        l_Result.validation_result =
            NodeGraphValidationResult::InvalidLink;
        return l_Result;
      }

      const Pin *l_StartPin = find_pin(p_Link.start_pin);
      const Pin *l_EndPin = find_pin(p_Link.end_pin);

      if (!l_StartPin || !l_EndPin) {
        l_Result.validation_result =
            NodeGraphValidationResult::InvalidPin;
        return l_Result;
      }

      if (p_Schema) {
        l_Result.validation_result =
            p_Schema->can_create_link(*this, *l_StartPin, *l_EndPin);
        if (l_Result.validation_result !=
            NodeGraphValidationResult::Allowed) {
          return l_Result;
        }
      } else {
        if (l_StartPin->direction == l_EndPin->direction) {
          l_Result.validation_result =
              NodeGraphValidationResult::SameDirection;
          return l_Result;
        }

        if (has_link_between(p_Link.start_pin, p_Link.end_pin)) {
          l_Result.validation_result =
              NodeGraphValidationResult::DuplicateLink;
          return l_Result;
        }
      }

      links.push_back(p_Link);
      l_Result.value = &links.back();
      return l_Result;
    }

    struct NodeGraphCanvas
    {
      Math::Vector2 m_Scrolling = Math::Vector2(0, 0);
      float m_Zoom = 1.0f;
      float m_MinZoom = 0.25f;
      float m_MaxZoom = 2.5f;
      float m_ZoomStep = 0.1f;
      float m_GridStep = 64.0f;
      float m_MinorGridStep = 16.0f;
      bool m_ShowMinorGrid = true;

      bool begin(const char *p_Label,
                 const Math::Vector2 &p_Size = Math::Vector2(0, 0));
      void end();

      void render(const char *p_Label,
                  const Math::Vector2 &p_Size = Math::Vector2(0, 0));

      ImDrawList *get_draw_list() const
      {
        return m_DrawList;
      }

      ImVec2 get_canvas_origin() const
      {
        return m_CanvasP0;
      }

      ImVec2 get_canvas_min() const
      {
        return m_CanvasP0;
      }

      ImVec2 get_canvas_max() const
      {
        return m_CanvasP1;
      }

      Math::Vector2
      screen_to_canvas(const Math::Vector2 &p_ScreenPos,
                       const Math::Vector2 &p_CanvasOrigin) const
      {
        const ImVec2 l_CanvasPos = screen_to_canvas(
            ImVec2(p_ScreenPos.x, p_ScreenPos.y),
            ImVec2(p_CanvasOrigin.x, p_CanvasOrigin.y));

        return Math::Vector2(l_CanvasPos.x, l_CanvasPos.y);
      }

      Math::Vector2
      canvas_to_screen(const Math::Vector2 &p_CanvasPos,
                       const Math::Vector2 &p_CanvasOrigin) const
      {
        const ImVec2 l_ScreenPos = canvas_to_screen(
            ImVec2(p_CanvasPos.x, p_CanvasPos.y),
            ImVec2(p_CanvasOrigin.x, p_CanvasOrigin.y));

        return Math::Vector2(l_ScreenPos.x, l_ScreenPos.y);
      }

      ImVec2 screen_to_canvas(const ImVec2 &p_ScreenPos,
                              const ImVec2 &p_CanvasOrigin) const
      {
        return ImVec2(
            (p_ScreenPos.x - p_CanvasOrigin.x - m_Scrolling.x) /
                m_Zoom,
            (p_ScreenPos.y - p_CanvasOrigin.y - m_Scrolling.y) /
                m_Zoom);
      }

      ImVec2 canvas_to_screen(const ImVec2 &p_CanvasPos,
                              const ImVec2 &p_CanvasOrigin) const
      {
        return ImVec2(p_CanvasOrigin.x + p_CanvasPos.x * m_Zoom +
                          m_Scrolling.x,
                      p_CanvasOrigin.y + p_CanvasPos.y * m_Zoom +
                          m_Scrolling.y);
      }

      Math::Vector2
      screen_to_canvas_size(const Math::Vector2 &p_ScreenSize) const
      {
        const ImVec2 l_CanvasSize = screen_to_canvas_size(
            ImVec2(p_ScreenSize.x, p_ScreenSize.y));

        return Math::Vector2(l_CanvasSize.x, l_CanvasSize.y);
      }

      Math::Vector2
      canvas_to_screen_size(const Math::Vector2 &p_CanvasSize) const
      {
        const ImVec2 l_ScreenSize = canvas_to_screen_size(
            ImVec2(p_CanvasSize.x, p_CanvasSize.y));

        return Math::Vector2(l_ScreenSize.x, l_ScreenSize.y);
      }

      ImVec2 screen_to_canvas_size(const ImVec2 &p_ScreenSize) const
      {
        return ImVec2(p_ScreenSize.x / m_Zoom,
                      p_ScreenSize.y / m_Zoom);
      }

      ImVec2 canvas_to_screen_size(const ImVec2 &p_CanvasSize) const
      {
        return ImVec2(p_CanvasSize.x * m_Zoom,
                      p_CanvasSize.y * m_Zoom);
      }

    private:
      ImDrawList *m_DrawList = nullptr;
      ImVec2 m_CanvasP0 = ImVec2(0.0f, 0.0f);
      ImVec2 m_CanvasP1 = ImVec2(0.0f, 0.0f);
      bool m_Active = false;

      void draw_grid(ImDrawList *p_DrawList, const ImVec2 &p_Min,
                     const ImVec2 &p_Max, float p_Step,
                     ImU32 p_Color) const;
      void draw_origin(ImDrawList *p_DrawList, const ImVec2 &p_Min,
                       const ImVec2 &p_Max) const;
    };

    struct NodeGraphEditorContext
    {
      NodeGraph &graph;
      const NodeGraphCanvas &canvas;
      const NodeGraphSchema *schema = nullptr;
      struct NodeGraphEditorState *state = nullptr;
      ImDrawList *draw_list = nullptr;
      ImVec2 canvas_origin = ImVec2(0.0f, 0.0f);
      ImVec2 canvas_min = ImVec2(0.0f, 0.0f);
      ImVec2 canvas_max = ImVec2(0.0f, 0.0f);
    };

    struct NodeGraphEditorState
    {
      Util::List<NodeId> selected_nodes;
      NodeId hovered_node;
      PinId hovered_pin;
      LinkId hovered_link;
      PinId link_drag_start_pin;
      bool dragging_nodes = false;
      bool interacting_with_widget = false;

      bool is_node_selected(NodeId p_NodeId) const
      {
        for (NodeId i_NodeId : selected_nodes) {
          if (i_NodeId == p_NodeId) {
            return true;
          }
        }

        return false;
      }

      void clear_selection()
      {
        selected_nodes.clear();
      }

      void select_node(NodeId p_NodeId, bool p_Additive = false)
      {
        if (!p_Additive) {
          selected_nodes.clear();
        }

        if (!is_node_selected(p_NodeId)) {
          selected_nodes.push_back(p_NodeId);
        }
      }

      void deselect_node(NodeId p_NodeId)
      {
        for (auto it = selected_nodes.begin();
             it != selected_nodes.end(); ++it) {
          if (*it == p_NodeId) {
            selected_nodes.erase(it);
            return;
          }
        }
      }
    };

    struct NodeGraphNodeRenderer
    {
      virtual ~NodeGraphNodeRenderer() = default;

      virtual Math::Vector2
      get_node_size(const NodeGraphEditorContext &p_Context,
                    const Node &p_Node) const = 0;

      virtual void render_node(NodeGraphEditorContext &p_Context,
                               Node &p_Node,
                               const ImVec2 &p_ScreenMin,
                               const ImVec2 &p_ScreenMax) = 0;

      virtual bool hit_test_node(
          const NodeGraphEditorContext &p_Context,
          const Node &p_Node, const ImVec2 &p_ScreenMin,
          const ImVec2 &p_ScreenMax,
          const ImVec2 &p_ScreenPosition) const
      {
        (void)p_Context;
        (void)p_Node;
        return p_ScreenPosition.x >= p_ScreenMin.x &&
               p_ScreenPosition.x <= p_ScreenMax.x &&
               p_ScreenPosition.y >= p_ScreenMin.y &&
               p_ScreenPosition.y <= p_ScreenMax.y;
      }

      virtual bool can_drag_node(
          const NodeGraphEditorContext &p_Context,
          const Node &p_Node) const
      {
        (void)p_Context;
        return p_Node.is_valid();
      }

      virtual bool get_pin_anchor(
          const NodeGraphEditorContext &p_Context,
          const Node &p_Node, const Pin &p_Pin,
          const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax,
          ImVec2 &p_Anchor) const
      {
        (void)p_Context;
        (void)p_Node;
        (void)p_Pin;
        (void)p_ScreenMin;
        (void)p_ScreenMax;
        (void)p_Anchor;
        return false;
      }
    };

    struct NodeGraphEditorRenderer
    {
      virtual ~NodeGraphEditorRenderer() = default;

      virtual void
      render_background(NodeGraphEditorContext &p_Context)
      {
        (void)p_Context;
      }

      virtual void render_links(NodeGraphEditorContext &p_Context)
      ;

      virtual NodeGraphNodeRenderer *
      get_node_renderer(NodeGraphEditorContext &p_Context,
                        Node &p_Node) = 0;

      virtual void
      render_foreground(NodeGraphEditorContext &p_Context)
      {
        (void)p_Context;
      }

      virtual void render(NodeGraphEditorContext &p_Context);

    protected:
      bool get_pin_anchor(NodeGraphEditorContext &p_Context,
                          const Pin &p_Pin, ImVec2 &p_Anchor);
      bool get_link_endpoints(NodeGraphEditorContext &p_Context,
                              const Link &p_Link, ImVec2 &p_Start,
                              ImVec2 &p_End);
      LinkId allocate_link_id(NodeGraphEditorContext &p_Context);
      float distance_to_link(NodeGraphEditorContext &p_Context,
                             const Link &p_Link,
                             const ImVec2 &p_ScreenPosition);
    };

    struct NodeGraphBoxNodeRenderer : public NodeGraphNodeRenderer
    {
      Math::Vector2 node_size = Math::Vector2(220.0f, 120.0f);
      ImU32 background_color = IM_COL32(52, 52, 60, 255);
      ImU32 border_color = IM_COL32(96, 96, 108, 255);
      ImU32 title_color = IM_COL32(72, 72, 84, 255);
      ImU32 text_color = IM_COL32(230, 230, 235, 255);
      float border_rounding = 8.0f;
      float title_height = 28.0f;
      float pin_radius = 6.0f;

      virtual Math::Vector2
      get_node_size(const NodeGraphEditorContext &p_Context,
                    const Node &p_Node) const override
      {
        (void)p_Context;
        (void)p_Node;
        return node_size;
      }

      virtual void render_node(NodeGraphEditorContext &p_Context,
                               Node &p_Node,
                               const ImVec2 &p_ScreenMin,
                               const ImVec2 &p_ScreenMax) override;

      virtual bool get_pin_anchor(
          const NodeGraphEditorContext &p_Context,
          const Node &p_Node, const Pin &p_Pin,
          const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax,
          ImVec2 &p_Anchor) const override;
    };

    struct NodeGraphBoxRenderer : public NodeGraphEditorRenderer
    {
      NodeGraphBoxNodeRenderer node_renderer;

      virtual NodeGraphNodeRenderer *
      get_node_renderer(NodeGraphEditorContext &p_Context,
                        Node &p_Node) override
      {
        (void)p_Context;
        (void)p_Node;
        return &node_renderer;
      }
    };
  } // namespace Editor
} // namespace Low
