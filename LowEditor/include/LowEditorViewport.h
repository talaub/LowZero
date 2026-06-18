#pragma once

#include "LowEditorApi.h"
#include "LowEditorEditingLayer.h"

#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererPrimitives.h"
#include "LowRendererRenderStep.h"
#include "LowRendererRenderView.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderScene.h"
#include "LowRendererSkeletalRenderObject.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererTexture.h"
#include "LowRendererUiCanvas.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API Viewport
    {
      Viewport(const Math::UVector2 p_Dimensions);
      Viewport(Renderer::RenderView p_RenderView);
      virtual ~Viewport();

      virtual bool tick(const float p_Delta);
      bool render_viewport(const float p_Delta);

      EditingLayerStack &get_editing_layers()
      {
        return m_EditingLayers;
      }

      const EditingLayerStack &get_editing_layers() const
      {
        return m_EditingLayers;
      }

      Renderer::RenderView get_render_view() const
      {
        return m_RenderView;
      }

      Renderer::Texture get_out_texture() const;

      void set_dimensions(const Math::UVector2 p_Dimensions);
      void set_dimensions(const u32 p_DimenionsX,
                          const u32 p_DimensionsY)
      {
        set_dimensions(Math::UVector2(p_DimenionsX, p_DimensionsY));
      }

      void set_camera_direction(const Math::Vector3 p_Direction);
      Math::Vector3 get_camera_direction() const;

      void set_camera_position(const Math::Vector3 p_Position);
      void set_camera_position(const float p_X, const float p_Y,
                               const float p_Z)
      {
        set_camera_position(Math::Vector3(p_X, p_Y, p_Z));
      }
      Math::Vector3 get_camera_position() const;

      Renderer::RenderObject spawn_mesh(Renderer::Mesh p_Mesh);
      Renderer::RenderObject
      spawn_mesh(Renderer::Mesh p_Mesh,
                 Renderer::Material p_Material);

      bool is_hovered() const
      {
        return m_ViewportHovered;
      }

      bool is_focused() const
      {
        return m_ViewportFocused;
      }

      Math::Vector2 get_relative_hover_position() const
      {
        return m_HoveredRelativePosition;
      }

      Math::Vector2 get_widget_position() const
      {
        return m_WidgetPosition;
      }

      Math::Vector2 get_widget_rect_position() const
      {
        return m_WidgetRectPosition;
      }

      Math::Vector2 get_widget_rect_size() const
      {
        return Math::Vector2((float)m_LastFrameDimensions.x,
                             (float)m_LastFrameDimensions.y);
      }

      Math::UVector2 get_widget_dimensions() const
      {
        return m_LastFrameDimensions;
      }

    protected:
      Renderer::RenderView m_RenderView;
      Renderer::RenderScene m_RenderScene;

      bool m_ViewportHovered;
      bool m_ViewportFocused;
      bool m_OwnsRenderResources;
      Math::UVector2 m_LastFrameDimensions = {0, 0};
      Math::Vector2 m_HoveredRelativePosition = {2.0f, 2.0f};
      Math::Vector2 m_WidgetPosition = {0.0f, 0.0f};
      Math::Vector2 m_WidgetRectPosition = {0.0f, 0.0f};

      Util::List<Renderer::RenderObject> m_RenderObjects;
      EditingLayerStack m_EditingLayers;
    };

    struct LOW_EDITOR_API MeshViewer : public Viewport
    {
      MeshViewer(Renderer::Mesh p_Mesh,
                 const Math::UVector2 p_Dimensions)
          : Viewport(p_Dimensions), m_InitialCameraSetup(false),
            m_CameraOrbitDistance(0.0f)
      {
        m_RenderScene.set_directional_light_color(1.0f, 1.0f, 1.0f);
        m_RenderScene.set_directional_light_intensity(0.75f);
        m_RenderScene.set_directional_light_direction(-0.15f, -1.0f,
                                                      -1.5f);

        m_RenderView.add_step_by_name(RENDERSTEP_SKY_GRADIENT_NAME);

        {
          m_RenderObject = spawn_mesh(p_Mesh);

          Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

          l_LocalMatrix =
              glm::translate(l_LocalMatrix, Math::Vector3(0.0f));
          l_LocalMatrix *=
              glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
          l_LocalMatrix =
              glm::scale(l_LocalMatrix, Math::Vector3(1.0f));

          m_RenderObject.set_world_transform(l_LocalMatrix);
        }

        {
          m_GroundRenderObject =
              spawn_mesh(Renderer::get_primitives().unitCube);

          Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

          l_LocalMatrix = glm::translate(
              l_LocalMatrix, Math::Vector3(0.0f, 0.0f, 0.0f));
          l_LocalMatrix *=
              glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
          l_LocalMatrix = glm::scale(
              l_LocalMatrix, Math::Vector3(50.0f, 0.2f, 50.0f));

          m_GroundRenderObject.set_world_transform(l_LocalMatrix);
        }
      }

      virtual ~MeshViewer();

      virtual bool tick(const float p_Delta) override;

      Renderer::RenderObject m_RenderObject;

      Renderer::RenderObject m_GroundRenderObject;

    private:
      bool m_InitialCameraSetup;
      float m_CameraOrbitDistance;
      bool m_LowSpotCalculated = false;
    };

    struct LOW_EDITOR_API SkeletalMeshViewer : public Viewport
    {
      SkeletalMeshViewer(Renderer::Mesh p_Mesh,
                         const Math::UVector2 p_Dimensions)
          : Viewport(p_Dimensions), m_InitialCameraSetup(false),
            m_CameraOrbitDistance(0.0f), m_BindPoseInitialized(false)
      {
        m_RenderScene.set_directional_light_color(1.0f, 1.0f, 1.0f);
        m_RenderScene.set_directional_light_intensity(0.75f);
        m_RenderScene.set_directional_light_direction(-0.15f, -1.0f,
                                                      -1.5f);

        m_RenderView.add_step_by_name(RENDERSTEP_SKY_GRADIENT_NAME);

        m_RenderObject = Renderer::SkeletalRenderObject::make(
            m_RenderScene, p_Mesh);
        m_RenderObject.set_material(Renderer::get_default_material());

        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);
        l_LocalMatrix =
            glm::translate(l_LocalMatrix, Math::Vector3(0.0f));
        l_LocalMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_LocalMatrix =
            glm::scale(l_LocalMatrix, Math::Vector3(1.0f));
        m_RenderObject.set_world_transform(l_LocalMatrix);

        m_Pose =
            Renderer::SkinningPose::make(N(Skeletal Mesh Viewer));
        m_Pose.set_skeleton(p_Mesh.get_skeleton());

        m_SkinningInstance = Renderer::SkinningInstance::make(
            N(Skeletal Mesh Viewer), p_Mesh);
        m_SkinningInstance.set_pose(m_Pose);
        m_SkinningInstance.set_render_object_id(
            m_RenderObject.get_id());
        m_RenderObject.set_skinning_instance(m_SkinningInstance);

        m_GroundRenderObject =
            spawn_mesh(Renderer::get_primitives().unitCube);

        Low::Math::Matrix4x4 l_GroundMatrix(1.0f);
        l_GroundMatrix = glm::translate(
            l_GroundMatrix, Math::Vector3(0.0f, 0.0f, 0.0f));
        l_GroundMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_GroundMatrix = glm::scale(
            l_GroundMatrix, Math::Vector3(50.0f, 0.2f, 50.0f));

        m_GroundRenderObject.set_world_transform(l_GroundMatrix);
      }

      virtual ~SkeletalMeshViewer();

      virtual bool tick(const float p_Delta) override;

      Renderer::SkeletalRenderObject m_RenderObject;
      Renderer::SkinningPose m_Pose;
      Renderer::SkinningInstance m_SkinningInstance;
      Renderer::RenderObject m_GroundRenderObject;

    private:
      bool m_InitialCameraSetup;
      float m_CameraOrbitDistance;
      bool m_BindPoseInitialized;
      bool m_BindPoseSubmitted = false;
      bool m_LowSpotCalculated = false;
      Util::List<Math::Matrix4x4> m_BindGlobalPose;
    };

    struct LOW_EDITOR_API MaterialViewer : public Viewport
    {
      MaterialViewer(Renderer::Material p_Material,
                     const Math::UVector2 p_Dimensions)
          : Viewport(p_Dimensions), m_InitialCameraSetup(false),
            m_CameraOrbitDistance(0.0f)
      {
        m_RenderObject =
            spawn_mesh(Renderer::get_primitives().unitIcoSphere);
        m_RenderObject.set_material(p_Material);

        Low::Math::Matrix4x4 l_LocalMatrix(1.0f);

        l_LocalMatrix =
            glm::translate(l_LocalMatrix, Math::Vector3(0.0f));
        l_LocalMatrix *=
            glm::toMat4(Math::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        l_LocalMatrix =
            glm::scale(l_LocalMatrix, Math::Vector3(1.0f));

        m_RenderObject.set_world_transform(l_LocalMatrix);

        m_RenderScene.set_directional_light_color(1.0f, 1.0f, 1.0f);
        m_RenderScene.set_directional_light_intensity(0.75f);
        m_RenderScene.set_directional_light_direction(-0.15f, -1.0f,
                                                      -1.5f);

        m_RenderView.add_step_by_name(RENDERSTEP_SKY_GRADIENT_NAME);
      }

      virtual bool tick(const float p_Delta) override;

      Renderer::RenderObject m_RenderObject;

    private:
      bool m_InitialCameraSetup;
      float m_CameraOrbitDistance;
    };

    struct LOW_EDITOR_API UiViewport : public Viewport
    {
      UiViewport(const Math::UVector2 p_Dimensions)
          : Viewport(p_Dimensions)
      {
        m_RenderView.add_step_by_name(RENDERSTEP_UI_NAME);
        m_Canvas = Renderer::UiCanvas::make(N(Viewport Canvas));
        m_RenderView.add_ui_canvas(m_Canvas);
      }

      virtual bool tick(const float p_Delta) override;

      Renderer::UiCanvas get_canvas() const
      {
        return m_Canvas;
      }

    protected:
      Renderer::UiCanvas m_Canvas;
    };
  } // namespace Editor
} // namespace Low
