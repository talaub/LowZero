#pragma once

#include "LowRendererVulkanPipeline.h"
#include "LowRendererVkPipeline.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineManager {
        bool register_graphics_pipeline(
            Pipeline p_Pipeline,
            PipelineUtil::GraphicsPipelineBuilder p_Builder);

        bool compile_graphics_pipeline(Pipeline p_Pipeline,
                                       bool p_CompileShaders = true);

        bool tick(float p_Delta);
      } // namespace PipelineManager
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
