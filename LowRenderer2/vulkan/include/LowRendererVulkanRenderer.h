#pragma once

#include "LowMath.h"

#include "vulkan/vulkan.h"

#include "LowRendererRenderObject.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {

      bool initialize();
      bool cleanup();
      bool prepare_tick(float p_Delta);
      bool tick(float p_Delta);

      bool wait_idle();

      size_t request_resource_staging_buffer_space(
          const size_t p_RequestedSize, size_t *p_OutOffset);
      bool resource_staging_buffer_write(void *p_Data,
                                         const size_t p_DataSize,
                                         const size_t p_Offset);

      bool check_window_resize(float p_Delta);
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
