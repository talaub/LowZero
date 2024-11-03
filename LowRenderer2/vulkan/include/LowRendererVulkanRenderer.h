#pragma once

namespace Low {
  namespace Renderer {
    namespace Vulkan {

      bool initialize();
      bool cleanup();
      bool tick(float p_Delta);

      bool check_window_resize(float p_Delta);
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
