#pragma once

#include "LowRendererComputePipeline.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererContext.h"

namespace Low {
  namespace Renderer {
    namespace Interface {

      struct RenderpassCreateParams
      {
        Context context;
        Util::List<Resource::Image> renderTargets;
        Util::List<Math::Color> clearColors;
        bool useDepth;
        Math::Vector2 clearDepthColor;
        Math::UVector2 dimensions;
      };

      namespace PipelineManager {
        void register_compute_pipeline(
            ComputePipeline p_Pipeline,
            Backend::PipelineComputeCreateParams &p_Params);
        void delist_compute_pipeline(ComputePipeline p_Pipeline);

        void register_graphics_pipeline(
            GraphicsPipeline p_Pipeline,
            Backend::PipelineGraphicsCreateParams &p_Params);
        void delist_graphics_pipeline(GraphicsPipeline p_Pipeline);

        void tick(float p_Delta);
      } // namespace PipelineManager
    }   // namespace Interface
  }     // namespace Renderer
} // namespace Low
