#pragma once

#include "LowRendererComputePipeline.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererContext.h"
#include "LowRendererPipelineResourceSignature.h"

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

      struct PipelineComputeCreateParams
      {
        Context context;
        Util::String shaderPath;
        Util::List<PipelineResourceSignature> signatures;
      };

      struct PipelineGraphicsCreateParams
      {
        Context context;
        Util::String vertexShaderPath;
        Util::String fragmentShaderPath;
        Math::UVector2 dimensions;
        Util::List<PipelineResourceSignature> signatures;
        uint8_t cullMode;
        uint8_t frontFace;
        uint8_t polygonMode;
        Util::List<Backend::GraphicsPipelineColorTarget> colorTargets;
        Renderpass renderpass;
        Util::List<uint8_t> vertexDataAttributeTypes;
      };

      namespace PipelineManager {
        void register_compute_pipeline(ComputePipeline p_Pipeline,
                                       PipelineComputeCreateParams &p_Params);
        void delist_compute_pipeline(ComputePipeline p_Pipeline);

        void register_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                        PipelineGraphicsCreateParams &p_Params);
        void delist_graphics_pipeline(GraphicsPipeline p_Pipeline);

        void tick(float p_Delta);
      } // namespace PipelineManager
    }   // namespace Interface
  }     // namespace Renderer
} // namespace Low
