#pragma once

namespace Low {
  namespace Renderer {
    void initialize();
    void cleanup();
    void tick(float p_Delta);

    void check_window_resize(float p_Delta);
  } // namespace Renderer
} // namespace Low
