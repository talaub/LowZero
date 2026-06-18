#pragma once

#include "LowEditorApi.h"
#include "LowEditorFlyingCameraEditingLayer.h"

#include "LowCoreConvexHullCollider.h"
#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API ConvexHullEditingLayer
        : public FlyingCameraEditingLayer
    {
      ConvexHullEditingLayer(
          Core::Component::ConvexHullCollider p_Collider,
          bool p_BlocksLowerLayers = true);

      void tick(const EditingLayerContext &p_Context) override;

      Core::Component::ConvexHullCollider get_collider() const
      {
        return m_Collider;
      }

      void
      set_collider(Core::Component::ConvexHullCollider p_Collider);

      virtual void on_close() override;

    private:
      enum ToolbarAction : u32
      {
        TOOLBAR_ACTION_NONE = 0u,
        TOOLBAR_ACTION_ADD_POINT = 1u,
        TOOLBAR_ACTION_DELETE_POINT = 2u
      };

      void handle_toolbar_action(u32 p_Action);
      void load_world_points_from_collider();

      Core::Component::ConvexHullCollider m_Collider;
      Util::List<Util::WeakPtr<TransformSphere>> m_Points;
      int m_SelectedIndex = -1;

      float m_DebounceTimer = -1.0f;
      bool m_AwaitWriteback = false;
      bool m_Editing = false;

      void force_write_update()
      {
        m_AwaitWriteback = true;
        m_DebounceTimer = -1.0f;
      }

      bool selection_valid()
      {
        return m_SelectedIndex >= 0 &&
               m_SelectedIndex < m_Points.size();
      }

      Math::Vector3 get_new_position()
      {
        if (selection_valid()) {
          return m_Points[m_SelectedIndex].lock()->get_position();
        }
        return m_Collider.get_entity()
            .get_transform()
            .get_world_position();
      }
    };
  } // namespace Editor
} // namespace Low
