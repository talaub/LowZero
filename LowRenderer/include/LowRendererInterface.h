#pragma once

#include "LowRendererContext.h"
#include "LowRendererFramebuffer.h"
#include "LowRendererCommandBuffer.h"
#include "LowRendererCommandPool.h"
#include "LowRendererImage2D.h"
#include "LowRendererRenderpass.h"
#include "LowRendererSwapchain.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererPipelineInterface.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
      };

      struct CommandPoolCreateParams
      {
        Context context;
      };

      struct RenderpassCreateParams
      {
        Context context;
        Util::List<Backend::ImageFormat> formats;
        Util::List<bool> clearTargets;
        bool useDepth;
        bool clearDepth;
      };

      struct RenderpassStartParams
      {
        Framebuffer framebuffer;
        CommandBuffer commandbuffer;
        Util::List<Math::Color> clearColorValues;
        Math::Vector2 clearDepthValue;
      };

      struct RenderpassStopParams
      {
        CommandBuffer commandbuffer;
      };

      struct FramebufferCreateParams
      {
        Context context;
        Util::List<Image2D> renderTargets;
        uint8_t framesInFlight;
        Math::UVector2 dimensions;
        Renderpass renderpass;
      };

      struct Image2DCreateParams
      {
        Context context;
        Math::UVector2 dimensions;
        Backend::ImageFormat format;
        bool writeable;
        bool depth;
        bool create_image;
      };

      struct SwapchainCreateParams
      {
        Context context;
        CommandPool commandPool;
      };

      struct GraphicsPipelineCreateParams
      {
        Context context;
        PipelineInterface interface;
        Util::String vertexPath;
        Util::String fragmentPath;
        Renderpass renderpass;
        Math::UVector2 dimensions;
        uint8_t cullMode;
        uint8_t frontFace;
        uint8_t polygonMode;
        Util::List<Backend::GraphicsPipelineColorTarget> colorTargets;
        bool vertexInput;
      };

      struct PipelineInterfaceCreateParams
      {
        Context context;
      };

      struct DrawParams
      {
        CommandBuffer commandBuffer;
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
      };

      void draw(DrawParams &p_Params);

      namespace ShaderProgramUtils {
        Util::String compile(Util::String p_Path);

        void register_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                        GraphicsPipelineCreateParams &p_Params);

        void delist_graphics_pipeline(GraphicsPipeline p_Pipeline);

        void tick(float p_Delta);
      }; // namespace ShaderProgramUtils
    }    // namespace Interface
  }      // namespace Renderer
} // namespace Low
