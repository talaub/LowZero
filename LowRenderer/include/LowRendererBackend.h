#pragma once

#include "LowRendererWindow.h"

#include "LowRendererVulkan.h"
#include <stdint.h>

#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      namespace ImageFormat {
        enum Enum
        {
          BGRA8_SRGB
        };
      }

      struct Context;
      struct ImageResource;

      struct Renderpass
      {
        union
        {
          Vulkan::Renderpass vk;
        };
        Context *context;
        ImageResource *renderTargets;
        uint8_t renderTargetCount;
        Math::Color *clearTargetColor;
        bool useDepth;
        Math::Vector2 clearDepthColor;
        Math::UVector2 dimensions;
      };

      struct Context
      {
        union
        {
          Vulkan::Context vk;
        };
        Window window;
        uint8_t framesInFlight;
        uint8_t imageCount;
        uint8_t currentFrameIndex;
        uint8_t currentImageIndex;
        Math::UVector2 dimensions;
        uint8_t imageFormat;
        Renderpass *renderpasses;
      };

      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
        uint8_t framesInFlight;
      };

      struct RenderpassCreateParams
      {
        Context *context;
        ImageResource *renderTargets;
        uint8_t renderTargetCount;
        Math::Color *clearTargetColor;
        bool useDepth;
        Math::Vector2 clearDepthColor;
        Math::UVector2 dimensions;
      };

      namespace ImageState {
        enum Enum
        {
          UNDEFINED,
          RENDERTARGET,
          STORAGE,
          SAMPLE,
          TRANSFER_SOURCE,
          TRANSFER_DESTINATION,
          DEPTH_RENDERTARGET
        };
      }

      struct ImageResource
      {
        union
        {
          Vulkan::ImageResource vk;
        };
        Context *context;
        uint8_t state;
        bool swapchainImage;
        uint8_t format;
        bool depth;
        Math::UVector2 dimensions;
      };

      struct ImageResourceCreateParams
      {
        Context *context;
        Math::UVector2 dimensions;
        uint8_t format;
        bool writable;
        bool depth;
        bool createImage;
        void *imageData;
        size_t imageDataSize;
      };

      namespace ContextState {
        enum Enum
        {
          SUCCESS,
          FAILED,
          OUT_OF_DATE
        };
      }

      namespace PipelineRasterizerCullMode {
        enum Enum
        {
          BACK,
          FRONT
        };
      }

      namespace PipelineRasterizerFrontFace {
        enum Enum
        {
          CLOCKWISE,
          COUNTER_CLOCKWISE
        };
      }

      namespace PipelineRasterizerPolygonMode {
        enum Enum
        {
          FILL,
          LINE
        };
      }

#define LOW_RENDERER_COLOR_WRITE_BIT_RED 1
#define LOW_RENDERER_COLOR_WRITE_BIT_GREEN 2
#define LOW_RENDERER_COLOR_WRITE_BIT_BLUE 4
#define LOW_RENDERER_COLOR_WRITE_BIT_ALPHA 8

      namespace ResourcePipelineStep {
        enum Enum
        {
          GRAPHICS,
          COMPUTE,
          VERTEX,
          FRAGMENT
        };
      }

      struct ApiBackendCallback
      {
        void (*context_create)(Context &, ContextCreateParams &);
        void (*context_cleanup)(Context &);
        void (*context_wait_idle)(Context &);

        uint8_t (*frame_prepare)(Context &);
        void (*frame_render)(Context &);

        void (*renderpass_create)(Renderpass &, RenderpassCreateParams &);
        void (*renderpass_begin)(Renderpass &);
        void (*renderpass_end)(Renderpass &);

        void (*imageresource_create)(ImageResource &,
                                     ImageResourceCreateParams &);
      };

      ApiBackendCallback &callbacks();

      void initialize();
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
