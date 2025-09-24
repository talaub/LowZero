#pragma once

#include "LowRenderer2Api.h"

#include "LowRendererMesh.h"

namespace Low {
  namespace Renderer {
    void initialize_primitives();

    struct Primitives
    {
      Mesh unitQuad;
      Mesh unitCube;
      Mesh unitIcoSphere;
    };

    LOW_RENDERER2_API Primitives &get_primitives();

    LOW_RENDERER2_API Mesh create_quad(Math::Vector2 p_Size);
    LOW_RENDERER2_API Mesh create_cube(Math::Vector3 p_HalfExtents);
    LOW_RENDERER2_API Mesh create_icosphere(float p_Radius,
                                            u32 p_Subdivisions);
  } // namespace Renderer
} // namespace Low
