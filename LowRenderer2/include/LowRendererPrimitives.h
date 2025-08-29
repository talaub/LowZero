#pragma once

#include "LowRendererMesh.h"

namespace Low {
  namespace Renderer {
    void initialize_primitives();

    struct Primitives
    {
      Mesh unitQuad;
      Mesh unitCube;
    };

    Primitives &get_primitives();

    Mesh create_quad(Math::Vector2 p_Size);
    Mesh create_cube(Math::Vector3 p_HalfExtents);
  } // namespace Renderer
} // namespace Low
