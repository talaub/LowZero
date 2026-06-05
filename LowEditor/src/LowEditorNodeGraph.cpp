#include "LowEditorNodeGraph.h"
#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorThemes.h"
#include "LowMath.h"
#include "LowUtilString.h"
#include "IconsLucide.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include "imgui_internal.h"

namespace Low {
  namespace Editor {
    namespace {
      static ImVec2 cubic_bezier(const ImVec2 &p_P0,
                                 const ImVec2 &p_P1,
                                 const ImVec2 &p_P2,
                                 const ImVec2 &p_P3, float p_T)
      {
        const float l_U = 1.0f - p_T;
        const float l_TT = p_T * p_T;
        const float l_UU = l_U * l_U;
        const float l_UUU = l_UU * l_U;
        const float l_TTT = l_TT * p_T;

        return ImVec2(l_UUU * p_P0.x + 3.0f * l_UU * p_T * p_P1.x +
                          3.0f * l_U * l_TT * p_P2.x + l_TTT * p_P3.x,
                      l_UUU * p_P0.y + 3.0f * l_UU * p_T * p_P1.y +
                          3.0f * l_U * l_TT * p_P2.y +
                          l_TTT * p_P3.y);
      }

      static float distance_squared(const ImVec2 &p_A,
                                    const ImVec2 &p_B)
      {
        const float l_Dx = p_A.x - p_B.x;
        const float l_Dy = p_A.y - p_B.y;
        return l_Dx * l_Dx + l_Dy * l_Dy;
      }
    } // namespace

    bool NodeGraphCanvas::begin(const char *p_Label,
                                const Math::Vector2 &p_Size)
    {
      ImVec2 l_Size = FROM_VEC2(p_Size);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(0.0f, 0.0f));

      ImGui::BeginChild(p_Label, l_Size, ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoScrollWithMouse |
                            ImGuiWindowFlags_NoScrollbar);
      ImGui::PopStyleVar();

      m_DrawList = ImGui::GetWindowDrawList();

      m_CanvasP0 = ImGui::GetCursorScreenPos();
      const ImVec2 l_CanvasSize = ImGui::GetContentRegionAvail();
      m_CanvasP1 = ImVec2(m_CanvasP0.x + l_CanvasSize.x,
                          m_CanvasP0.y + l_CanvasSize.y);
      const ImVec2 l_MousePos = ImGui::GetIO().MousePos;
      const bool l_Hovered = l_MousePos.x >= m_CanvasP0.x &&
                             l_MousePos.x <= m_CanvasP1.x &&
                             l_MousePos.y >= m_CanvasP0.y &&
                             l_MousePos.y <= m_CanvasP1.y;
      // Background
      m_DrawList->AddRectFilled(m_CanvasP0, m_CanvasP1,
                                IM_COL32(32, 32, 36, 255));
      m_DrawList->AddRect(m_CanvasP0, m_CanvasP1,
                          IM_COL32(80, 80, 90, 255));

      update_view_animation(ImGui::GetIO().DeltaTime);

      // Panning with middle mouse
      if (l_Hovered &&
          ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        m_Scrolling.x += ImGui::GetIO().MouseDelta.x;
        m_Scrolling.y += ImGui::GetIO().MouseDelta.y;
        sync_view_targets();
      }

      if (l_Hovered &&
          !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) &&
          ImGui::GetIO().MouseWheel != 0.0f) {
        const ImVec2 l_MousePos = ImGui::GetIO().MousePos;
        const ImVec2 l_CanvasMousePos =
            screen_to_canvas(l_MousePos, m_CanvasP0);

        m_Zoom += ImGui::GetIO().MouseWheel * m_ZoomStep;
        m_Zoom = std::clamp(m_Zoom, m_MinZoom, m_MaxZoom);

        m_Scrolling.x =
            l_MousePos.x - m_CanvasP0.x - l_CanvasMousePos.x * m_Zoom;
        m_Scrolling.y =
            l_MousePos.y - m_CanvasP0.y - l_CanvasMousePos.y * m_Zoom;
        sync_view_targets();
      }

      m_DrawList->PushClipRect(m_CanvasP0, m_CanvasP1, true);

      if (m_ShowMinorGrid) {
        draw_grid(m_DrawList, m_CanvasP0, m_CanvasP1, m_MinorGridStep,
                  IM_COL32(45, 45, 50, 255));
      }

      draw_grid(m_DrawList, m_CanvasP0, m_CanvasP1, m_GridStep,
                IM_COL32(60, 60, 68, 255));

      // Origin cross for orientation
      draw_origin(m_DrawList, m_CanvasP0, m_CanvasP1);

      m_Active = true;
      return true;
    }

    void NodeGraphCanvas::update_view_animation(float p_DeltaTime)
    {
      if (p_DeltaTime <= 0.0f) {
        return;
      }

      const float l_Factor = LOW_MATH_CLAMP(
          p_DeltaTime * m_FocusSmoothingSpeed, 0.0f, 1.0f);

      m_Zoom += (m_TargetZoom - m_Zoom) * l_Factor;
      m_Scrolling += (m_TargetScrolling - m_Scrolling) * l_Factor;

      if (LOW_MATH_ABS(m_TargetZoom - m_Zoom) < 0.001f) {
        m_Zoom = m_TargetZoom;
      }

      if (LOW_MATH_ABS(m_TargetScrolling.x - m_Scrolling.x) < 0.5f) {
        m_Scrolling.x = m_TargetScrolling.x;
      }
      if (LOW_MATH_ABS(m_TargetScrolling.y - m_Scrolling.y) < 0.5f) {
        m_Scrolling.y = m_TargetScrolling.y;
      }
    }

    void NodeGraphCanvas::sync_view_targets()
    {
      m_TargetScrolling = m_Scrolling;
      m_TargetZoom = m_Zoom;
    }

    void NodeGraphCanvas::end()
    {
      if (!m_Active) {
        return;
      }

      m_DrawList->PopClipRect();
      ImGui::EndChild();
      m_DrawList = nullptr;
      m_Active = false;
    }

    void NodeGraphCanvas::render(const char *p_Label,
                                 const Math::Vector2 &p_Size)
    {
      if (begin(p_Label, p_Size)) {
        end();
      }
    }

    void NodeGraphCanvas::draw_grid(ImDrawList *p_DrawList,
                                    const ImVec2 &p_Min,
                                    const ImVec2 &p_Max, float p_Step,
                                    ImU32 p_Color) const
    {
      if (p_Step <= 0.0f) {
        return;
      }

      const float l_ScaledStep = p_Step * m_Zoom;
      if (l_ScaledStep <= 0.0f) {
        return;
      }

      const float l_Width = p_Max.x - p_Min.x;
      const float l_Height = p_Max.y - p_Min.y;

      float l_OffsetX = std::fmod(m_Scrolling.x, l_ScaledStep);
      float l_OffsetY = std::fmod(m_Scrolling.y, l_ScaledStep);

      if (l_OffsetX < 0.0f) {
        l_OffsetX += l_ScaledStep;
      }
      if (l_OffsetY < 0.0f) {
        l_OffsetY += l_ScaledStep;
      }

      for (float x = l_OffsetX; x < l_Width; x += l_ScaledStep) {
        p_DrawList->AddLine(ImVec2(p_Min.x + x, p_Min.y),
                            ImVec2(p_Min.x + x, p_Max.y), p_Color);
      }

      for (float y = l_OffsetY; y < l_Height; y += l_ScaledStep) {
        p_DrawList->AddLine(ImVec2(p_Min.x, p_Min.y + y),
                            ImVec2(p_Max.x, p_Min.y + y), p_Color);
      }
    }

    void NodeGraphCanvas::draw_origin(ImDrawList *p_DrawList,
                                      const ImVec2 &p_Min,
                                      const ImVec2 &p_Max) const
    {
      const ImVec2 l_Origin =
          ImVec2(p_Min.x + m_Scrolling.x, p_Min.y + m_Scrolling.y);

      if (l_Origin.x >= p_Min.x && l_Origin.x <= p_Max.x) {
        p_DrawList->AddLine(ImVec2(l_Origin.x, p_Min.y),
                            ImVec2(l_Origin.x, p_Max.y),
                            IM_COL32(110, 70, 70, 255), 2.0f);
      }

      if (l_Origin.y >= p_Min.y && l_Origin.y <= p_Max.y) {
        p_DrawList->AddLine(ImVec2(p_Min.x, l_Origin.y),
                            ImVec2(p_Max.x, l_Origin.y),
                            IM_COL32(70, 110, 70, 255), 2.0f);
      }
    }

    void NodeGraphEditorController::render(
        NodeGraphEditorContext &p_Context,
        NodeGraphEditorRenderer &p_Renderer)
    {
      NodeGraphEditorState *l_State = p_Context.state;
      NodeId l_HoveredNodeId;
      NodeGraphNodeRenderer *l_HoveredNodeRenderer = nullptr;
      PinId l_HoveredPinId;
      const ImVec2 l_MousePosition = ImGui::GetIO().MousePos;
      const bool l_MouseInCanvas =
          l_MousePosition.x >= p_Context.canvas_min.x &&
          l_MousePosition.x <= p_Context.canvas_max.x &&
          l_MousePosition.y >= p_Context.canvas_min.y &&
          l_MousePosition.y <= p_Context.canvas_max.y;
      float l_BestPinDistanceSquared = FLT_MAX;

      if (l_State) {
        l_State->interacting_with_widget = false;
      }

      for (auto it = p_Context.graph.nodes.rbegin();
           it != p_Context.graph.nodes.rend(); ++it) {
        Node &i_Node = *it;
        NodeGraphNodeRenderer *l_NodeRenderer =
            p_Renderer.get_node_renderer(p_Context, i_Node);

        if (!l_NodeRenderer) {
          continue;
        }

        const Math::Vector2 l_NodeSize =
            l_NodeRenderer->get_node_size(p_Context, i_Node);
        const ImVec2 l_ScreenMin = p_Context.canvas.canvas_to_screen(
            ImVec2(i_Node.position.x, i_Node.position.y),
            p_Context.canvas_origin);
        const ImVec2 l_ScreenSize =
            p_Context.canvas.canvas_to_screen_size(
                ImVec2(l_NodeSize.x, l_NodeSize.y));
        const ImVec2 l_ScreenMax =
            ImVec2(l_ScreenMin.x + l_ScreenSize.x,
                   l_ScreenMin.y + l_ScreenSize.y);

        if (l_MouseInCanvas && l_NodeRenderer->hit_test_node(
                                   p_Context, i_Node, l_ScreenMin,
                                   l_ScreenMax, l_MousePosition)) {
          l_HoveredNodeId = i_Node.id;
          l_HoveredNodeRenderer = l_NodeRenderer;
          break;
        }
      }

      if (l_MouseInCanvas) {
        for (auto it = p_Context.graph.nodes.rbegin();
             it != p_Context.graph.nodes.rend(); ++it) {
          Node &i_Node = *it;
          NodeGraphNodeRenderer *l_NodeRenderer =
              p_Renderer.get_node_renderer(p_Context, i_Node);

          if (!l_NodeRenderer) {
            continue;
          }

          const Math::Vector2 l_NodeSize =
              l_NodeRenderer->get_node_size(p_Context, i_Node);
          const ImVec2 l_ScreenMin =
              p_Context.canvas.canvas_to_screen(
                  ImVec2(i_Node.position.x, i_Node.position.y),
                  p_Context.canvas_origin);
          const ImVec2 l_ScreenSize =
              p_Context.canvas.canvas_to_screen_size(
                  ImVec2(l_NodeSize.x, l_NodeSize.y));
          const ImVec2 l_ScreenMax =
              ImVec2(l_ScreenMin.x + l_ScreenSize.x,
                     l_ScreenMin.y + l_ScreenSize.y);

          Util::List<Pin *> l_NodePins =
              p_Context.graph.get_node_pins(i_Node.id);

          for (Pin *i_Pin : l_NodePins) {
            ImVec2 l_PinAnchor;
            if (!l_NodeRenderer->get_pin_anchor(
                    p_Context, i_Node, *i_Pin, l_ScreenMin,
                    l_ScreenMax, l_PinAnchor)) {
              continue;
            }

            const float l_PinHitRadius =
                12.0f * p_Context.canvas.m_Zoom;
            const float l_DistanceSquared =
                distance_squared(l_PinAnchor, l_MousePosition);

            if (l_DistanceSquared <=
                    l_PinHitRadius * l_PinHitRadius &&
                l_DistanceSquared < l_BestPinDistanceSquared) {
              l_BestPinDistanceSquared = l_DistanceSquared;
              l_HoveredPinId = i_Pin->id;
              l_HoveredNodeId = i_Node.id;
            }
          }
        }
      }

      if (l_State) {
        l_State->hovered_node = l_HoveredNodeId;
        l_State->hovered_pin = l_HoveredPinId;
      }

      p_Renderer.render_background(p_Context);
      p_Renderer.render_links(p_Context);

      for (Node &i_Node : p_Context.graph.nodes) {
        NodeGraphNodeRenderer *l_NodeRenderer =
            p_Renderer.get_node_renderer(p_Context, i_Node);

        if (!l_NodeRenderer) {
          continue;
        }

        const Math::Vector2 l_NodeSize =
            l_NodeRenderer->get_node_size(p_Context, i_Node);
        const ImVec2 l_ScreenMin = p_Context.canvas.canvas_to_screen(
            ImVec2(i_Node.position.x, i_Node.position.y),
            p_Context.canvas_origin);
        const ImVec2 l_ScreenSize =
            p_Context.canvas.canvas_to_screen_size(
                ImVec2(l_NodeSize.x, l_NodeSize.y));
        const ImVec2 l_ScreenMax =
            ImVec2(l_ScreenMin.x + l_ScreenSize.x,
                   l_ScreenMin.y + l_ScreenSize.y);

        l_NodeRenderer->render_node(p_Context, i_Node, l_ScreenMin,
                                    l_ScreenMax);
      }

      if (l_State) {
        const bool l_BlockCanvasInteraction =
            l_State->interacting_with_widget;

        if (!l_BlockCanvasInteraction && l_MouseInCanvas &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          if (l_HoveredPinId.is_valid()) {
            l_State->context_menu_pin = l_HoveredPinId;
            l_State->context_menu_node = NodeId{};
            ImGui::OpenPopup("NodeGraphPinContextMenu");
          } else if (l_HoveredNodeId.is_valid()) {
            l_State->context_menu_node = l_HoveredNodeId;
            l_State->context_menu_pin = PinId{};
            l_State->select_node(l_HoveredNodeId, false);
            ImGui::OpenPopup("NodeGraphNodeContextMenu");
          }
        }

        if (!l_BlockCanvasInteraction && l_MouseInCanvas &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          if (l_HoveredPinId.is_valid()) {
            l_State->link_drag_start_pin = l_HoveredPinId;
            l_State->dragging_nodes = false;
            l_State->box_selecting = false;
          } else if (l_HoveredNodeId.is_valid()) {
            const bool l_AdditiveSelection = ImGui::GetIO().KeyCtrl;
            const bool l_NodeAlreadySelected =
                l_State->is_node_selected(l_HoveredNodeId);

            if (l_AdditiveSelection) {
              l_State->select_node(l_HoveredNodeId, true);
            } else if (!l_NodeAlreadySelected) {
              l_State->select_node(l_HoveredNodeId, false);
            }

            if (l_HoveredNodeRenderer) {
              Node *l_HoveredNode =
                  p_Context.graph.find_node(l_HoveredNodeId);
              if (l_HoveredNode &&
                  l_HoveredNodeRenderer->can_drag_node(
                      p_Context, *l_HoveredNode)) {
                l_State->dragging_nodes = true;
                l_State->box_selecting = false;
              }
            }
          } else {
            l_State->dragging_nodes = false;
            l_State->link_drag_start_pin = PinId{};
            l_State->box_selecting = true;
            l_State->box_select_additive = ImGui::GetIO().KeyCtrl;
            l_State->box_select_start = l_MousePosition;
            l_State->box_select_current = l_MousePosition;
            if (!l_State->box_select_additive) {
              l_State->clear_selection();
            }
          }
        }

        if (!l_BlockCanvasInteraction && l_State->dragging_nodes &&
            !l_State->link_drag_start_pin.is_valid() &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          const ImVec2 l_CanvasDelta =
              p_Context.canvas.screen_to_canvas_size(
                  ImGui::GetIO().MouseDelta);

          if (l_CanvasDelta.x != 0.0f || l_CanvasDelta.y != 0.0f) {
            for (NodeId i_NodeId : l_State->selected_nodes) {
              Node *l_Node = p_Context.graph.find_node(i_NodeId);
              if (l_Node) {
                l_Node->translate(
                    Math::Vector2(l_CanvasDelta.x, l_CanvasDelta.y));
              }
            }
          }
        }

        if (!l_BlockCanvasInteraction && l_State->box_selecting &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
          l_State->box_select_current = l_MousePosition;

          const ImVec2 l_BoxMin(
              LOW_MATH_MIN(l_State->box_select_start.x,
                           l_State->box_select_current.x),
              LOW_MATH_MIN(l_State->box_select_start.y,
                           l_State->box_select_current.y));
          const ImVec2 l_BoxMax(
              LOW_MATH_MAX(l_State->box_select_start.x,
                           l_State->box_select_current.x),
              LOW_MATH_MAX(l_State->box_select_start.y,
                           l_State->box_select_current.y));

          if (!l_State->box_select_additive) {
            l_State->clear_selection();
          }

          for (Node &i_Node : p_Context.graph.nodes) {
            NodeGraphNodeRenderer *l_NodeRenderer =
                p_Renderer.get_node_renderer(p_Context, i_Node);

            if (!l_NodeRenderer) {
              continue;
            }

            const Math::Vector2 l_NodeSize =
                l_NodeRenderer->get_node_size(p_Context, i_Node);
            const ImVec2 l_ScreenMin =
                p_Context.canvas.canvas_to_screen(
                    ImVec2(i_Node.position.x, i_Node.position.y),
                    p_Context.canvas_origin);
            const ImVec2 l_ScreenSize =
                p_Context.canvas.canvas_to_screen_size(
                    ImVec2(l_NodeSize.x, l_NodeSize.y));
            const ImVec2 l_ScreenMax =
                ImVec2(l_ScreenMin.x + l_ScreenSize.x,
                       l_ScreenMin.y + l_ScreenSize.y);

            const bool l_Intersects = l_ScreenMin.x <= l_BoxMax.x &&
                                      l_ScreenMax.x >= l_BoxMin.x &&
                                      l_ScreenMin.y <= l_BoxMax.y &&
                                      l_ScreenMax.y >= l_BoxMin.y;

            if (l_Intersects) {
              l_State->select_node(i_Node.id, true);
            }
          }
        }

        if (l_State->dragging_nodes &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          l_State->dragging_nodes = false;
        }

        if (l_State->box_selecting &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          l_State->box_selecting = false;
        }

        if (!l_BlockCanvasInteraction &&
            l_State->link_drag_start_pin.is_valid() &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          const Pin *l_StartPin =
              p_Context.graph.find_pin(l_State->link_drag_start_pin);
          const Pin *l_EndPin =
              p_Context.graph.find_pin(l_HoveredPinId);

          if (l_StartPin && l_EndPin &&
              l_StartPin->id != l_EndPin->id) {
            Link l_NewLink;
            l_NewLink.id = p_Renderer.allocate_link_id(p_Context);

            if (l_StartPin->direction == PinDirection::Output) {
              l_NewLink.start_pin = l_StartPin->id;
              l_NewLink.end_pin = l_EndPin->id;
            } else {
              l_NewLink.start_pin = l_EndPin->id;
              l_NewLink.end_pin = l_StartPin->id;
            }

            p_Renderer.create_link(p_Context, l_NewLink);
          }

          l_State->link_drag_start_pin = PinId{};
        }

        if (l_State->box_selecting) {
          const ImVec2 l_BoxMin(
              LOW_MATH_MIN(l_State->box_select_start.x,
                           l_State->box_select_current.x),
              LOW_MATH_MIN(l_State->box_select_start.y,
                           l_State->box_select_current.y));
          const ImVec2 l_BoxMax(
              LOW_MATH_MAX(l_State->box_select_start.x,
                           l_State->box_select_current.x),
              LOW_MATH_MAX(l_State->box_select_start.y,
                           l_State->box_select_current.y));

          p_Context.draw_list->AddRectFilled(
              l_BoxMin, l_BoxMax, IM_COL32(120, 170, 255, 28));
          p_Context.draw_list->AddRect(l_BoxMin, l_BoxMax,
                                       IM_COL32(120, 170, 255, 180),
                                       0.0f, 0, 1.5f);
        }
      }

      handle_shortcuts(p_Context, p_Renderer,
                       l_State ? l_State->interacting_with_widget
                               : false);
      render_create_node_popup(p_Context, p_Renderer);
      render_context_menus(p_Context, p_Renderer);
    }

    void NodeGraphEditorController::handle_shortcuts(
        NodeGraphEditorContext &p_Context,
        NodeGraphEditorRenderer &p_Renderer,
        bool p_BlockCanvasInteraction)
    {
      (void)p_Renderer;

      NodeGraphEditorState *l_State = p_Context.state;
      if (!l_State || p_BlockCanvasInteraction) {
        return;
      }

      if ((ImGui::IsKeyPressed(ImGuiKey_Delete) ||
           ImGui::IsKeyPressed(ImGuiKey_Backspace)) &&
          !l_State->selected_nodes.empty()) {
        Util::List<NodeId> l_NodesToDelete = l_State->selected_nodes;

        l_State->clear_selection();
        l_State->dragging_nodes = false;
        l_State->hovered_node = NodeId{};
        l_State->hovered_pin = PinId{};

        for (NodeId i_NodeId : l_NodesToDelete) {
          p_Context.graph.remove_node(i_NodeId);
        }
      }
      if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        if (p_Context.state->selected_nodes.empty()) {

          p_Renderer.focus_graph(p_Context);
        } else {
          p_Renderer.focus_selection(p_Context);
        }
      }
    }

    bool NodeGraphEditorRenderer::get_node_canvas_bounds(
        NodeGraphEditorContext &p_Context, const Node &p_Node,
        Math::Vector2 &p_Min, Math::Vector2 &p_Max)
    {
      Node *l_Node = p_Context.graph.find_node(p_Node.id);
      if (!l_Node) {
        return false;
      }

      NodeGraphNodeRenderer *l_NodeRenderer =
          get_node_renderer(p_Context, *l_Node);
      if (!l_NodeRenderer) {
        return false;
      }

      const Math::Vector2 l_Size =
          l_NodeRenderer->get_node_size(p_Context, *l_Node);
      p_Min = l_Node->position;
      p_Max = l_Node->position + l_Size;
      return true;
    }

    bool NodeGraphEditorRenderer::focus_node(
        NodeGraphEditorContext &p_Context, NodeId p_NodeId,
        float p_PaddingScreen)
    {
      const Node *l_Node = p_Context.graph.find_node(p_NodeId);
      if (!l_Node) {
        return false;
      }

      Math::Vector2 l_Min;
      Math::Vector2 l_Max;
      if (!get_node_canvas_bounds(p_Context, *l_Node, l_Min, l_Max)) {
        return false;
      }

      NodeGraphCanvas &l_Canvas =
          const_cast<NodeGraphCanvas &>(p_Context.canvas);
      const Math::Vector2 l_Size = l_Max - l_Min;
      if (l_Size.x <= 1.0f && l_Size.y <= 1.0f) {
        l_Canvas.focus_canvas_point((l_Min + l_Max) * 0.5f);
      } else {
        l_Canvas.focus_canvas_bounds(l_Min, l_Max, p_PaddingScreen);
      }
      return true;
    }

    bool NodeGraphEditorRenderer::focus_nodes(
        NodeGraphEditorContext &p_Context,
        const Util::List<NodeId> &p_NodeIds, float p_PaddingScreen)
    {
      bool l_HaveBounds = false;
      Math::Vector2 l_Min;
      Math::Vector2 l_Max;

      for (NodeId i_NodeId : p_NodeIds) {
        const Node *l_Node = p_Context.graph.find_node(i_NodeId);
        if (!l_Node) {
          continue;
        }

        Math::Vector2 i_Min;
        Math::Vector2 i_Max;
        if (!get_node_canvas_bounds(p_Context, *l_Node, i_Min,
                                    i_Max)) {
          continue;
        }

        if (!l_HaveBounds) {
          l_Min = i_Min;
          l_Max = i_Max;
          l_HaveBounds = true;
        } else {
          l_Min.x = LOW_MATH_MIN(l_Min.x, i_Min.x);
          l_Min.y = LOW_MATH_MIN(l_Min.y, i_Min.y);
          l_Max.x = LOW_MATH_MAX(l_Max.x, i_Max.x);
          l_Max.y = LOW_MATH_MAX(l_Max.y, i_Max.y);
        }
      }

      if (!l_HaveBounds) {
        return false;
      }

      NodeGraphCanvas &l_Canvas =
          const_cast<NodeGraphCanvas &>(p_Context.canvas);
      const Math::Vector2 l_Size = l_Max - l_Min;
      if (l_Size.x <= 1.0f && l_Size.y <= 1.0f) {
        l_Canvas.focus_canvas_point((l_Min + l_Max) * 0.5f);
      } else {
        l_Canvas.focus_canvas_bounds(l_Min, l_Max, p_PaddingScreen);
      }
      return true;
    }

    bool NodeGraphEditorRenderer::focus_selection(
        NodeGraphEditorContext &p_Context, float p_PaddingScreen)
    {
      if (!p_Context.state ||
          p_Context.state->selected_nodes.empty()) {
        return false;
      }

      return focus_nodes(p_Context, p_Context.state->selected_nodes,
                         p_PaddingScreen);
    }

    bool NodeGraphEditorRenderer::focus_graph(
        NodeGraphEditorContext &p_Context, float p_PaddingScreen)
    {
      Util::List<NodeId> l_NodeIds;
      for (const Node &i_Node : p_Context.graph.nodes) {
        l_NodeIds.push_back(i_Node.id);
      }

      return focus_nodes(p_Context, l_NodeIds, p_PaddingScreen);
    }

    void NodeGraphEditorController::render_create_node_popup(
        NodeGraphEditorContext &p_Context,
        NodeGraphEditorRenderer &p_Renderer)
    {
      NodeGraphSpawner *l_Spawner = p_Renderer.get_spawner(p_Context);
      if (!l_Spawner || !p_Context.state) {
        return;
      }

      const ImVec2 l_MousePosition = ImGui::GetIO().MousePos;
      const bool l_MouseInCanvas =
          l_MousePosition.x >= p_Context.canvas_min.x &&
          l_MousePosition.x <= p_Context.canvas_max.x &&
          l_MousePosition.y >= p_Context.canvas_min.y &&
          l_MousePosition.y <= p_Context.canvas_max.y;
      const bool l_HoveringGraphElement =
          p_Context.state->hovered_node.is_valid() ||
          p_Context.state->hovered_pin.is_valid();

      if (!p_Context.state->interacting_with_widget &&
          l_MouseInCanvas && !l_HoveringGraphElement &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        create_node_position =
            TO_VEC2(p_Context.canvas.screen_to_canvas(
                l_MousePosition, p_Context.canvas_origin));
        create_node_popup_just_opened = true;
        create_node_search[0] = '\0';
        ImGui::OpenPopup("NodeGraphCreateNode");
      }

      ImGui::SetNextWindowSize(ImVec2(300.0f, 380.0f),
                               ImGuiCond_Always);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(8.0f, 8.0f));
      if (ImGui::BeginPopup("NodeGraphCreateNode",
                            ImGuiWindowFlags_NoScrollbar |
                                ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::PopStyleVar();

        if (create_node_popup_just_opened) {
          ImGui::SetKeyboardFocusHere();
          create_node_popup_just_opened = false;
        }

        ImGui::SetNextItemWidth(-1.0f);
        Gui::SearchField(
            "##nodegraph_create_node_search", create_node_search,
            IM_ARRAYSIZE(create_node_search), ImVec2(0.0f, 3.0f));

        ImGui::Spacing();

        Util::String l_Search = create_node_search;
        l_Search.make_lower();
        Util::Map<Util::String, Util::List<NodeGraphSpawnEntry>>
            l_CategorizedEntries;

        for (const NodeGraphSpawnEntry &i_Entry :
             l_Spawner->get_spawn_entries(p_Context)) {
          if (!i_Entry.is_valid()) {
            continue;
          }

          if (!l_Search.empty()) {
            Util::String l_FilterText = i_Entry.title;
            l_FilterText += " ";
            l_FilterText += i_Entry.subtitle;
            l_FilterText += " ";
            l_FilterText += i_Entry.category;
            l_FilterText += " ";
            l_FilterText += i_Entry.search_text;
            l_FilterText.make_lower();

            if (!Util::StringHelper::contains(l_FilterText,
                                              l_Search)) {
              continue;
            }
          }

          l_CategorizedEntries[i_Entry.category].push_back(i_Entry);
        }

        const float l_ListH =
            ImGui::GetContentRegionAvail().y - 2.0f;
        ImGui::BeginChild("##spawn_list",
                          ImVec2(0.0f, l_ListH), false,
                          ImGuiWindowFlags_NoSavedSettings);

        if (l_CategorizedEntries.empty()) {
          ImGui::Dummy(ImVec2(0.0f, 6.0f));
          ImGui::Indent(6.0f);
          ImGui::TextDisabled(ICON_LC_SEARCH "  No nodes found");
          ImGui::Unindent(6.0f);
        } else {
          ImDrawList *l_DrawList =
              ImGui::GetWindowDrawList();
          const Theme &l_Theme = theme_get_current();
          const float l_EntryH = 28.0f;
          const float l_Rounding = 4.0f;

          bool l_First = true;
          for (auto it = l_CategorizedEntries.begin();
               it != l_CategorizedEntries.end(); ++it) {

            if (!l_First) {
              ImGui::Dummy(ImVec2(0.0f, 4.0f));
            }
            l_First = false;

            // Category header (collapsible)
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            const ImGuiID l_CatId = ImGui::GetID(
                it->first.c_str());
            bool *l_CatOpen =
                ImGui::GetStateStorage()->GetBoolRef(
                    l_CatId, true);

            const float l_CatAvailW =
                ImGui::GetContentRegionAvail().x;
            const float l_CatH = 22.0f;
            const ImVec2 l_CatStart =
                ImGui::GetCursorScreenPos();

            const bool l_CatClicked =
                ImGui::InvisibleButton(
                    it->first.c_str(),
                    ImVec2(l_CatAvailW, l_CatH));
            if (l_CatClicked) {
              *l_CatOpen = !*l_CatOpen;
            }

            const float l_CatMidY =
                l_CatStart.y +
                (l_CatH - ImGui::GetTextLineHeight()) *
                    0.5f;
            const char *l_CatChevron =
                *l_CatOpen ? ICON_LC_CHEVRON_DOWN
                           : ICON_LC_CHEVRON_RIGHT;
            l_DrawList->AddText(
                ImVec2(l_CatStart.x, l_CatMidY),
                ImGui::GetColorU32(ImGuiCol_TextDisabled),
                l_CatChevron);
            const float l_ChevW =
                ImGui::CalcTextSize(l_CatChevron).x +
                4.0f;

            // Category name — regular weight, normal size
            l_DrawList->AddText(
                ImVec2(l_CatStart.x + l_ChevW, l_CatMidY),
                ImGui::GetColorU32(ImGuiCol_Text),
                it->first.c_str());

            // Separator line after name
            const float l_CatTextW =
                ImGui::CalcTextSize(it->first.c_str()).x;
            const ImVec2 l_LineStart(
                l_CatStart.x + l_ChevW + l_CatTextW + 6.0f,
                l_CatStart.y + l_CatH * 0.5f);
            const ImVec2 l_LineEnd(
                l_CatStart.x + l_CatAvailW,
                l_LineStart.y);
            l_DrawList->AddLine(
                l_LineStart, l_LineEnd,
                ImGui::GetColorU32(ImGuiCol_Separator),
                1.0f);

            if (!*l_CatOpen) {
              continue;
            }

            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            // Entries
            for (const NodeGraphSpawnEntry &i_Entry :
                 it->second) {
              const float l_AvailW =
                  ImGui::GetContentRegionAvail().x;
              const ImVec2 l_Start =
                  ImGui::GetCursorScreenPos();
              const ImVec2 l_End(l_Start.x + l_AvailW,
                                 l_Start.y + l_EntryH);

              ImGui::PushID(i_Entry.id.m_Index);
              const bool l_Clicked =
                  ImGui::InvisibleButton(
                      "##entry",
                      ImVec2(l_AvailW, l_EntryH));
              const bool l_Hovered =
                  ImGui::IsItemHovered();
              ImGui::PopID();

              if (l_Hovered) {
                l_DrawList->AddRectFilled(
                    l_Start, l_End,
                    color_to_imcolor(l_Theme.headerHover),
                    l_Rounding);
              }

              const float l_MidY =
                  l_Start.y +
                  (l_EntryH -
                   ImGui::GetTextLineHeight()) *
                      0.5f;

              // Subtitle — smaller light font, right-aligned
              ImGui::PushFont(
                  Fonts::UI(12, Fonts::Weight::Light));
              const ImVec2 l_SubSize =
                  i_Entry.subtitle.empty()
                      ? ImVec2(0.0f, 0.0f)
                      : ImGui::CalcTextSize(
                            i_Entry.subtitle.c_str());
              ImGui::PopFont();

              const float l_SubX =
                  l_End.x - l_SubSize.x - 6.0f;
              const ImVec2 l_TitleClip(
                  l_SubX - 8.0f, l_End.y);

              ImGui::RenderTextEllipsis(
                  l_DrawList,
                  ImVec2(l_Start.x + 8.0f, l_MidY),
                  l_TitleClip, l_TitleClip.x,
                  i_Entry.title.c_str(), nullptr,
                  nullptr);

              if (!i_Entry.subtitle.empty()) {
                const float l_SubMidY =
                    l_Start.y +
                    (l_EntryH - l_SubSize.y) * 0.5f;
                ImGui::PushFont(
                    Fonts::UI(12, Fonts::Weight::Light));
                l_DrawList->AddText(
                    ImVec2(l_SubX, l_SubMidY),
                    ImGui::GetColorU32(
                        ImGuiCol_TextDisabled),
                    i_Entry.subtitle.c_str());
                ImGui::PopFont();
              }

              if (l_Clicked) {
                l_Spawner->spawn_entry(
                    p_Context, i_Entry.id,
                    create_node_position);
                ImGui::CloseCurrentPopup();
              }
            }
          }
        }

        ImGui::EndChild();
        ImGui::EndPopup();
      } else {
        ImGui::PopStyleVar();
      }
    }

    void NodeGraphEditorController::render_context_menus(
        NodeGraphEditorContext &p_Context,
        NodeGraphEditorRenderer &p_Renderer)
    {
      if (p_Context.state &&
          ImGui::BeginPopup("NodeGraphNodeContextMenu")) {
        Node *l_Node = p_Context.graph.find_node(
            p_Context.state->context_menu_node);
        if (l_Node) {
          p_Renderer.render_node_context_menu(p_Context, *l_Node);
        }
        ImGui::EndPopup();
      }

      if (p_Context.state &&
          ImGui::BeginPopup("NodeGraphPinContextMenu")) {
        Pin *l_Pin = p_Context.graph.find_pin(
            p_Context.state->context_menu_pin);
        if (l_Pin) {
          p_Renderer.render_pin_context_menu(p_Context, *l_Pin);
        }
        ImGui::EndPopup();
      }
    }

    void NodeGraphEditorRenderer::render_links(
        NodeGraphEditorContext &p_Context)
    {
      if (!p_Context.draw_list) {
        return;
      }

      LinkId l_HoveredLinkId;
      float l_BestDistance = FLT_MAX;
      const ImVec2 l_MousePosition = ImGui::GetIO().MousePos;
      const bool l_MouseInCanvas =
          l_MousePosition.x >= p_Context.canvas_min.x &&
          l_MousePosition.x <= p_Context.canvas_max.x &&
          l_MousePosition.y >= p_Context.canvas_min.y &&
          l_MousePosition.y <= p_Context.canvas_max.y;

      for (const Link &i_Link : p_Context.graph.links) {
        ImVec2 l_Start;
        ImVec2 l_End;

        if (!get_link_endpoints(p_Context, i_Link, l_Start, l_End)) {
          continue;
        }

        const float l_Tangent =
            LOW_MATH_MAX(60.0f * p_Context.canvas.m_Zoom,
                         LOW_MATH_ABS(l_End.x - l_Start.x) * 0.5f);
        const ImVec2 l_ControlStart(l_Start.x + l_Tangent, l_Start.y);
        const ImVec2 l_ControlEnd(l_End.x - l_Tangent, l_End.y);

        ImU32 l_Color = IM_COL32(180, 180, 190, 255);
        float l_Thickness = 3.0f;

        if (l_MouseInCanvas) {
          const float l_Distance =
              distance_to_link(p_Context, i_Link, l_MousePosition);
          if (l_Distance < l_BestDistance) {
            l_BestDistance = l_Distance;
            l_HoveredLinkId = i_Link.id;
          }
        }

        if (p_Context.state &&
            p_Context.state->hovered_link == i_Link.id) {
          l_Color = IM_COL32(255, 210, 120, 255);
          l_Thickness = 4.0f;
        }

        p_Context.draw_list->AddBezierCubic(l_Start, l_ControlStart,
                                            l_ControlEnd, l_End,
                                            l_Color, l_Thickness);
      }

      if (p_Context.state) {
        if (l_BestDistance <= 10.0f) {
          p_Context.state->hovered_link = l_HoveredLinkId;
        } else {
          p_Context.state->hovered_link = LinkId{};
        }

        if (p_Context.state->link_drag_start_pin.is_valid()) {
          const Pin *l_StartPin = p_Context.graph.find_pin(
              p_Context.state->link_drag_start_pin);
          ImVec2 l_Start;

          if (l_StartPin &&
              get_pin_anchor(p_Context, *l_StartPin, l_Start)) {
            const ImVec2 l_End = ImGui::GetIO().MousePos;
            const float l_Tangent = LOW_MATH_MAX(
                60.0f * p_Context.canvas.m_Zoom,
                LOW_MATH_ABS(l_End.x - l_Start.x) * 0.5f);
            ImVec2 l_ControlStart;
            ImVec2 l_ControlEnd;

            if (l_StartPin->direction == PinDirection::Output) {
              l_ControlStart =
                  ImVec2(l_Start.x + l_Tangent, l_Start.y);
              l_ControlEnd = ImVec2(l_End.x - l_Tangent, l_End.y);
            } else {
              l_ControlStart =
                  ImVec2(l_Start.x - l_Tangent, l_Start.y);
              l_ControlEnd = ImVec2(l_End.x + l_Tangent, l_End.y);
            }

            p_Context.draw_list->AddBezierCubic(
                l_Start, l_ControlStart, l_ControlEnd, l_End,
                IM_COL32(255, 255, 255, 160), 3.0f);
          }
        }
      }
    }

    bool NodeGraphEditorRenderer::get_pin_anchor(
        NodeGraphEditorContext &p_Context, const Pin &p_Pin,
        ImVec2 &p_Anchor)
    {
      const Node *l_Node = p_Context.graph.find_node(p_Pin.node);
      if (!l_Node) {
        return false;
      }

      Node &l_NodeRef = *p_Context.graph.find_node(p_Pin.node);
      NodeGraphNodeRenderer *l_NodeRenderer =
          get_node_renderer(p_Context, l_NodeRef);
      if (!l_NodeRenderer) {
        return false;
      }

      const Math::Vector2 l_NodeSize =
          l_NodeRenderer->get_node_size(p_Context, l_NodeRef);
      const ImVec2 l_ScreenMin = p_Context.canvas.canvas_to_screen(
          ImVec2(l_Node->position.x, l_Node->position.y),
          p_Context.canvas_origin);
      const ImVec2 l_ScreenSize =
          p_Context.canvas.canvas_to_screen_size(
              ImVec2(l_NodeSize.x, l_NodeSize.y));
      const ImVec2 l_ScreenMax =
          ImVec2(l_ScreenMin.x + l_ScreenSize.x,
                 l_ScreenMin.y + l_ScreenSize.y);

      return l_NodeRenderer->get_pin_anchor(p_Context, *l_Node, p_Pin,
                                            l_ScreenMin, l_ScreenMax,
                                            p_Anchor);
    }

    bool NodeGraphEditorRenderer::get_link_endpoints(
        NodeGraphEditorContext &p_Context, const Link &p_Link,
        ImVec2 &p_Start, ImVec2 &p_End)
    {
      const Pin *l_StartPin =
          p_Context.graph.find_pin(p_Link.start_pin);
      const Pin *l_EndPin = p_Context.graph.find_pin(p_Link.end_pin);

      if (!l_StartPin || !l_EndPin) {
        return false;
      }

      return get_pin_anchor(p_Context, *l_StartPin, p_Start) &&
             get_pin_anchor(p_Context, *l_EndPin, p_End);
    }

    LinkId NodeGraphEditorRenderer::allocate_link_id(
        NodeGraphEditorContext &p_Context)
    {
      u64 l_MaxId = 0;

      for (const Link &i_Link : p_Context.graph.links) {
        l_MaxId = LOW_MATH_MAX(l_MaxId, i_Link.id.value);
      }

      return LinkId{l_MaxId + 1};
    }

    float NodeGraphEditorRenderer::distance_to_link(
        NodeGraphEditorContext &p_Context, const Link &p_Link,
        const ImVec2 &p_ScreenPosition)
    {
      ImVec2 l_Start;
      ImVec2 l_End;

      if (!get_link_endpoints(p_Context, p_Link, l_Start, l_End)) {
        return FLT_MAX;
      }

      const float l_Tangent =
          LOW_MATH_MAX(60.0f * p_Context.canvas.m_Zoom,
                       LOW_MATH_ABS(l_End.x - l_Start.x) * 0.5f);
      const ImVec2 l_ControlStart(l_Start.x + l_Tangent, l_Start.y);
      const ImVec2 l_ControlEnd(l_End.x - l_Tangent, l_End.y);

      float l_BestDistanceSquared = FLT_MAX;

      for (u32 i = 0; i <= 24; ++i) {
        const float l_T = (float)i / 24.0f;
        const ImVec2 l_Point = cubic_bezier(l_Start, l_ControlStart,
                                            l_ControlEnd, l_End, l_T);
        l_BestDistanceSquared =
            LOW_MATH_MIN(l_BestDistanceSquared,
                         distance_squared(l_Point, p_ScreenPosition));
      }

      return sqrtf(l_BestDistanceSquared);
    }

    void NodeGraphBoxNodeRenderer::render_node(
        NodeGraphEditorContext &p_Context, Node &p_Node,
        const ImVec2 &p_ScreenMin, const ImVec2 &p_ScreenMax)
    {
      if (!p_Context.draw_list) {
        return;
      }

      const float l_TitleHeight =
          title_height * p_Context.canvas.m_Zoom;

      p_Context.draw_list->AddRectFilled(p_ScreenMin, p_ScreenMax,
                                         background_color,
                                         border_rounding);
      p_Context.draw_list->AddRectFilled(
          p_ScreenMin,
          ImVec2(p_ScreenMax.x, p_ScreenMin.y + l_TitleHeight),
          title_color, border_rounding, ImDrawFlags_RoundCornersTop);
      p_Context.draw_list->AddRect(p_ScreenMin, p_ScreenMax,
                                   border_color, border_rounding, 0,
                                   1.5f);

      char l_NodeLabel[64];
      snprintf(l_NodeLabel, sizeof(l_NodeLabel), "Node %llu",
               (unsigned long long)p_Node.id.value);

      p_Context.draw_list->AddText(
          ImVec2(p_ScreenMin.x + 12.0f * p_Context.canvas.m_Zoom,
                 p_ScreenMin.y + 6.0f * p_Context.canvas.m_Zoom),
          text_color, l_NodeLabel);

      char l_NodePosition[96];
      snprintf(l_NodePosition, sizeof(l_NodePosition), "(%.0f, %.0f)",
               p_Node.position.x, p_Node.position.y);

      p_Context.draw_list->AddText(
          ImVec2(p_ScreenMin.x + 12.0f * p_Context.canvas.m_Zoom,
                 p_ScreenMin.y + l_TitleHeight +
                     10.0f * p_Context.canvas.m_Zoom),
          IM_COL32(180, 180, 190, 255), l_NodePosition);

      Util::List<Pin *> l_NodePins =
          p_Context.graph.get_node_pins(p_Node.id);

      for (Pin *i_Pin : l_NodePins) {
        ImVec2 l_PinAnchor;
        if (!get_pin_anchor(p_Context, p_Node, *i_Pin, p_ScreenMin,
                            p_ScreenMax, l_PinAnchor)) {
          continue;
        }

        ImU32 l_PinColor = i_Pin->is_input()
                               ? IM_COL32(120, 180, 255, 255)
                               : IM_COL32(255, 170, 120, 255);
        p_Context.draw_list->AddCircleFilled(
            l_PinAnchor, pin_radius * p_Context.canvas.m_Zoom,
            l_PinColor);
      }
    }

    bool NodeGraphBoxNodeRenderer::get_pin_anchor(
        const NodeGraphEditorContext &p_Context, const Node &p_Node,
        const Pin &p_Pin, const ImVec2 &p_ScreenMin,
        const ImVec2 &p_ScreenMax, ImVec2 &p_Anchor) const
    {
      (void)p_Node;

      const NodeGraph &l_Graph = p_Context.graph;
      Util::List<const Pin *> l_NodePins =
          l_Graph.get_node_pins(p_Pin.node);

      u32 l_SideIndex = 0;
      u32 l_SideCount = 0;

      for (const Pin *i_Pin : l_NodePins) {
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

      const float l_Step =
          (p_ScreenMax.y - p_ScreenMin.y) / (float)(l_SideCount + 1);
      const float l_Y =
          p_ScreenMin.y + l_Step * (float)(l_SideIndex + 1);
      const float l_X =
          p_Pin.is_input() ? p_ScreenMin.x : p_ScreenMax.x;

      p_Anchor = ImVec2(l_X, l_Y);
      return true;
    }
  } // namespace Editor
} // namespace Low
