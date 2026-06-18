#include "LowEditorConvexHullEditingLayer.h"

#include "LowCoreTransform.h"
#include "LowEditor.h"
#include "LowEditorEditingLayerHelpers.h"
#include "LowEditorViewport.h"

#include "LowCore.h"
#include "LowCorePhysicsShape.h"
#include "IconsLucide.h"
#include "LowMath.h"

#include "LowCoreDebugGeometry.h"
#include "LowRendererRenderView.h"
#include "ImGuizmo.h"
#include "LowUtilLogger.h"
#include <imgui.h>
#include <imgui_internal.h>

#define DEBOUNCE_TIME 0.6f

namespace Low {
  namespace Editor {
    static Math::Vector3
    local_to_world_point(Core::Component::Transform p_Transform,
                         const Math::Vector3 &p_LocalPoint)
    {
      return p_Transform.get_world_position() +
             (p_Transform.get_world_rotation() *
              (p_LocalPoint * p_Transform.get_world_scale()));
    }

    static Math::Vector3
    world_to_local_point(Core::Component::Transform p_Transform,
                         const Math::Vector3 &p_WorldPoint)
    {
      const Math::Vector3 l_LocalScaled =
          glm::inverse(p_Transform.get_world_rotation()) *
          (p_WorldPoint - p_Transform.get_world_position());
      const Math::Vector3 l_WorldScale =
          p_Transform.get_world_scale();

      Math::Vector3 l_LocalPoint(0.0f);
      l_LocalPoint.x = glm::abs(l_WorldScale.x) > LOW_MATH_EPSILON
                           ? l_LocalScaled.x / l_WorldScale.x
                           : l_LocalScaled.x;
      l_LocalPoint.y = glm::abs(l_WorldScale.y) > LOW_MATH_EPSILON
                           ? l_LocalScaled.y / l_WorldScale.y
                           : l_LocalScaled.y;
      l_LocalPoint.z = glm::abs(l_WorldScale.z) > LOW_MATH_EPSILON
                           ? l_LocalScaled.z / l_WorldScale.z
                           : l_LocalScaled.z;
      return l_LocalPoint;
    }

    static void destroy_points(
        Util::List<Util::WeakPtr<TransformSphere>> &p_Points)
    {
      for (Util::WeakPtr<TransformSphere> &i_Point : p_Points) {
        Util::SharedPtr<TransformSphere> i_Sphere = i_Point.lock();
        if (i_Sphere) {
          i_Sphere->destroy();
        }
      }

      p_Points.clear();
    }

    ConvexHullEditingLayer::ConvexHullEditingLayer(
        Core::Component::ConvexHullCollider p_Collider,
        const bool p_BlocksLowerLayers)
        : FlyingCameraEditingLayer(p_BlocksLowerLayers, true),
          m_Collider(p_Collider)
    {
      load_world_points_from_collider();

      add_toolbar(
          "ConvexHullClose", ViewportToolbarAnchor::TOP_RIGHT,
          ViewportToolbarSlideDirection::UP,
          [](ViewportToolbarContext &p_Context) {
            if (p_Context.icon_button("Close", ICON_LC_X, "Close")) {
              p_Context.viewport.get_editing_layers().pop();
            }

            return p_Context.get_button_toolbar_width(1u);
          });

      add_toolbar(
          "ConvexHullBottomTools",
          ViewportToolbarAnchor::BOTTOM_CENTER,
          ViewportToolbarSlideDirection::DOWN,
          [this](ViewportToolbarContext &p_Context) {
            u32 l_ButtonCount = 1u;

            if (p_Context.icon_button("AddPoint", ICON_LC_PLUS,
                                      "Add point")) {
              handle_toolbar_action(TOOLBAR_ACTION_ADD_POINT);
            }

            if (selection_valid()) {
              l_ButtonCount++;
              if (p_Context.icon_button("DeletePoint",
                                        ICON_LC_TRASH_2,
                                        "Delete selected point")) {
                handle_toolbar_action(TOOLBAR_ACTION_DELETE_POINT);
              }
            }

            return p_Context.get_button_toolbar_width(l_ButtonCount);
          });
    }

    void ConvexHullEditingLayer::set_collider(
        Core::Component::ConvexHullCollider p_Collider)
    {
      m_Collider = p_Collider;
      load_world_points_from_collider();
    }

    void ConvexHullEditingLayer::load_world_points_from_collider()
    {
      destroy_points(m_Points);
      if (!m_Collider.is_alive()) {
        m_SelectedIndex = -1;
        return;
      }

      Core::Entity l_Entity = m_Collider.get_entity();
      if (!l_Entity.is_alive()) {
        m_SelectedIndex = -1;
        return;
      }

      Core::Component::Transform l_Transform =
          l_Entity.get_transform();
      for (const Math::Vector3 &i_Point : m_Collider.get_points()) {
        m_Points.push_back(TransformSphere::create(
            local_to_world_point(l_Transform, i_Point),
            Renderer::RenderView()));
      }
      m_SelectedIndex =
          m_Points.empty() ? -1 : (int)m_Points.size() - 1;
    }

    void
    ConvexHullEditingLayer::tick(const EditingLayerContext &p_Context)
    {
      if (!m_Collider.is_alive()) {
        return;
      }

      Core::Entity l_Entity = m_Collider.get_entity();
      Core::Component::Transform l_Transform =
          l_Entity.get_transform();

      if (m_AwaitWriteback) {
        if (m_DebounceTimer <= 0.0f) {
          m_AwaitWriteback = false;

          Util::List<Math::Vector3> l_LocalPoints;
          l_LocalPoints.reserve(m_Points.size());
          for (const Util::WeakPtr<TransformSphere> &i_Point :
               m_Points) {
            Util::SharedPtr<TransformSphere> i_Sphere =
                i_Point.lock();
            if (!i_Sphere) {
              continue;
            }

            l_LocalPoints.push_back(world_to_local_point(
                l_Transform, i_Sphere->get_position()));
          }
          m_Collider.set_points(l_LocalPoints);
          m_Collider.mark_dirty();
        }

        m_DebounceTimer -= p_Context.delta;
      }

      u32 l_PickId = read_hovered_pick_id(p_Context.viewport);

      FlyingCameraEditingLayer::tick(p_Context);

      Math::Color l_LineColor(1.0f, 0.0f, 0.5f, 1.0f);

      const bool l_Clicked =
          ImGui::IsMouseClicked(ImGuiMouseButton_Left);

      bool l_CurrentlyEditing = false;

      bool l_Interacted = is_ui_hovered();

      if (m_Points.size() >= 4) {
        Core::Physics::Shape l_Shape = m_Collider.get_shape();
        if (l_Shape.is_alive()) {
          l_Shape.debug_visualize(
              p_Context.viewport.get_render_view(),
              l_Transform.get_world_position(),
              l_Transform.get_world_rotation(),
              Math::Color(0.0f, 1.0f, 0.0f, 1.0f), true, true);
        }
      }

      for (int i = 0; i < m_Points.size(); ++i) {
        Util::SharedPtr<TransformSphere> i_Point = m_Points[i].lock();
        if (!i_Point) {
          continue;
        }

        const bool i_Hovered = i == l_PickId;
        i_Point->set_pick_id(static_cast<u32>(i));
        TransformSphere::State i_State =
            TransformSphere::State::Normal;
        if (i == m_SelectedIndex) {
          i_State = TransformSphere::State::Selected;
        } else if (i_Hovered) {
          i_State = TransformSphere::State::Hovered;
        }
        i_Point->set_state(i_State);

        if (i) {
          Util::SharedPtr<TransformSphere> i_PreviousPoint =
              m_Points[i - 1].lock();
          if (!i_PreviousPoint) {
            continue;
          }

          Core::DebugGeometry::render_line(
              i_PreviousPoint->get_position(),
              i_Point->get_position(), l_LineColor, true);
        }

        if (l_Clicked && i_Hovered) {
          l_Interacted = true;
          m_SelectedIndex = i;
        }
      }

      if (m_SelectedIndex >= 0 &&
          m_SelectedIndex <= m_Points.size() - 1) {
        Util::SharedPtr<TransformSphere> l_SelectedPoint =
            m_Points[m_SelectedIndex].lock();
        if (!l_SelectedPoint) {
          m_SelectedIndex = -1;
        } else {
          Util::List<Math::Vector3> l_Positions = {
              {l_SelectedPoint->get_position()}};
          TransformGizmoConfig l_Config;
          l_Config.set_viewport(
              p_Context.viewport,
              TransformGizmoViewportMatrices::Scene3D);

          TransformGizmoResult l_Result =
              render_transform_gizmo(l_Config, l_Positions);

          if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
            l_Interacted = true;
          }

          if (l_Result.changed) {
            l_Interacted = true;
            l_SelectedPoint->set_position(l_Positions[0]);
            l_CurrentlyEditing = true;
          }
        }
      }

      if (l_Clicked && !l_Interacted) {
        m_SelectedIndex = -1;
      }

      if (m_Editing && !l_CurrentlyEditing) {
        m_DebounceTimer = DEBOUNCE_TIME;
        m_AwaitWriteback = true;
        m_Editing = false;
      }

      m_Editing = l_CurrentlyEditing;
    }

    void
    ConvexHullEditingLayer::handle_toolbar_action(const u32 p_Action)
    {
      switch (p_Action) {
      case TOOLBAR_ACTION_ADD_POINT: {
        // Point insertion will be wired once the point editing gizmos
        // decide where new points should be placed.
        Core::Component::Transform l_Transform =
            m_Collider.get_entity().get_transform();
        m_Points.push_back(TransformSphere::create(
            get_new_position(), Renderer::RenderView()));
        m_SelectedIndex = m_Points.size() - 1;
        break;
      }
      case TOOLBAR_ACTION_DELETE_POINT: {
        if (!selection_valid()) {
          break;
        }

        Util::SharedPtr<TransformSphere> l_Point =
            m_Points[m_SelectedIndex].lock();
        if (l_Point) {
          l_Point->destroy();
        }

        m_Points.erase(m_Points.begin() + m_SelectedIndex);
        if (m_Points.empty()) {
          m_SelectedIndex = -1;
        } else if (m_SelectedIndex >= m_Points.size()) {
          m_SelectedIndex = (int)m_Points.size() - 1;
        }

        m_Editing = false;
        force_write_update();
        break;
      }
      default:
        break;
      }
    }

    void ConvexHullEditingLayer::on_close()
    {
      destroy_points(m_Points);
    }
  } // namespace Editor
} // namespace Low
