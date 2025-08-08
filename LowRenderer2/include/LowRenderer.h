#pragma once

#include "LowUtil.h"

#include "LowRendererMesh.h"
#include "LowRendererTexture.h"

namespace Low {
  namespace Renderer {
    void initialize();
    void cleanup();
    void prepare_tick(float p_Delta);
    void tick(float p_Delta);

    void check_window_resize(float p_Delta);

    void load_mesh(Mesh p_Mesh);
    Texture load_texture(Util::String p_ImagePath);
  } // namespace Renderer
} // namespace Low
