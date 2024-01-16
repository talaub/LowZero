#pragma once

#include "LowCoreApi.h"

#include "LowCoreMeshResource.h"

#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace DebugGeometry {
      void initialize();

      // Helpers
      LOW_CORE_API float
      screen_space_multiplier(Renderer::RenderFlow p_RenderFlow,
                              Math::Vector3 p_Position);
      LOW_CORE_API Renderer::Material
      create_spherical_billboard_material(Util::String p_Path);

      // Primitives
      LOW_CORE_API void render_box(Math::Box p_Box, Math::Color p_Color,
                                   bool p_DepthTest, bool p_Wireframe);
      LOW_CORE_API void render_sphere(Math::Sphere p_Sphere,
                                      Math::Color p_Color, bool p_DepthTest,
                                      bool p_Wireframe);
      LOW_CORE_API void render_cylinder(Math::Cylinder p_Cylinder,
                                        Math::Color p_Color, bool p_DepthTest,
                                        bool p_Wireframe);
      LOW_CORE_API void render_cone(Math::Cone p_Cone, Math::Color p_Color,
                                    bool p_DepthTest, bool p_Wireframe);

      LOW_CORE_API void render_line(Math::Vector3 p_Start, Math::Vector3 p_End,
                                    Math::Color p_Color, bool p_DepthTest,
                                    bool p_Wireframe,
                                    float p_Thickness = 0.02f);

      LOW_CORE_API void render_triangle(Math::Vector3 p_Vertex0,
                                        Math::Vector3 p_Vertex1,
                                        Math::Vector3 p_Vertex2,
                                        Math::Color p_Color, bool p_DepthTest,
                                        bool p_Wireframe);

      // Complex predefined objects
      LOW_CORE_API
      void render_arrow(Math::Vector3 p_Position, Math::Quaternion p_Rotation,
                        float p_Length, float p_Thickness, float p_HeadRadius,
                        float p_HeadLength, Math::Color p_Color,
                        bool p_DepthTest, bool p_Wireframe);

      // Billboards
      LOW_CORE_API void
      render_spherical_billboard(Math::Vector3 p_Position, float p_Size,
                                 Renderer::Material p_Material);
    } // namespace DebugGeometry
  }   // namespace Core
} // namespace Low
