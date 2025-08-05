#pragma once

#include "LowUtil.h"

#include "LowRendererMesh.h"
#include "LowRendererImageResource.h"

namespace Low {
  namespace Renderer {
    void initialize();
    void cleanup();
    void prepare_tick(float p_Delta);
    void tick(float p_Delta);

    void check_window_resize(float p_Delta);

    Mesh load_mesh(Util::String p_MeshPath);
    ImageResource load_image(Util::String p_ImagePath);
  } // namespace Renderer
} // namespace Low
