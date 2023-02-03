#pragma once

#include "LowRendererContext.h"
#include "LowRendererFramebuffer.h"
#include "LowRendererCommandBuffer.h"
#include "LowRendererCommandPool.h"
#include "LowRendererImage2D.h"
#include "LowRendererRenderpass.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
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
        Util::List<Math::Color> clearColorsValues;
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
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
