#pragma once

#include "LowEditorApi.h"

#include "LowMath.h"
#include "LowRendererPrimitives.h"
#include "LowRendererRenderStep.h"
#include "LowRendererRenderView.h"
#include "LowRendererRenderObject.h"
#include "LowRendererRenderScene.h"
#include "LowRendererUiCanvas.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API Viewport
    {
      Viewport(const Math::UVector2 p_Dimensions);
      virtual ~Viewport();

      virtual bool tick(const float p_Delta);
      bool render_viewport(const float p_Delta);

      Renderer::Texture get_out_texture() const
      {
        return m_RenderView.get_lit_image();
      }

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

    protected:
      Renderer::RenderView m_RenderView;
      Renderer::RenderScene m_RenderScene;

      bool m_ViewportHovered;

      Util::List<Renderer::RenderObject> m_RenderObjects;
    };

    struct LOW_EDITOR_API MeshViewer : public Viewport
    {
      MeshViewer(Renderer::Mesh p_Mesh,
                 const Math::UVector2 p_Dimensions)
          : Viewport(p_Dimensions), m_InitialCameraSetup(false),
            m_CameraOrbitDistance(0.0f)
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
