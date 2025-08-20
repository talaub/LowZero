#pragma once

#include "LowRendererMesh.h"

namespace Low {
  namespace Renderer {
    void initialize_primitives();

    struct Primitives
    {
      Mesh unitQuad;
    };

    Primitives &get_primitives();

    Mesh create_quad(Math::Vector2 p_Size);
  } // namespace Renderer
} // namespace Low
