#include "LowCoreDebugGeometry.h"

#include "LowCoreMonoUtils.h"

namespace Low {
  namespace Core {
    namespace Mono {
      void dg_draw_sphere(MonoObject *p_Position, float p_Radius,
                          MonoObject *p_Color, bool p_DepthTested,
                          bool p_Wireframe)
      {
        Math::Sphere l_Sphere;
        l_Sphere.position = Math::Vector3(0.0f);
        l_Sphere.radius = p_Radius;

        get_vector3(p_Position, l_Sphere.position);
        Math::ColorRGB l_Color;
        get_vector3(p_Color, l_Color);

        DebugGeometry::render_sphere(l_Sphere, Math::Color(l_Color, 1.0f),
                                     p_DepthTested, p_Wireframe);
      }
    } // namespace Mono
  }   // namespace Core
} // namespace Low
