#pragma once

#include "LowRendererVulkanPipeline.h"
#include "LowRendererVkPipeline.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineManager {
        bool register_graphics_pipeline(
            Context &p_Context, Pipeline p_Pipeline,
            PipelineUtil::GraphicsPipelineBuilder p_Builder);

        bool compile_graphics_pipeline(Context &p_Context,
                                       Pipeline p_Pipeline,
                                       bool p_CompileShaders = true);

        bool tick(Context &p_Context, float p_Delta);
      } // namespace PipelineManager
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
