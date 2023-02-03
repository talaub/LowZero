#pragma once

#include "LowRendererWindow.h"

#include "LowRendererVulkan.h"
#include <stdint.h>

#include "LowUtilName.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct Framebuffer;
      struct CommandBuffer;
      struct GraphicsPipeline;

      struct ImageFormat
      {
        union
        {
          Vulkan::ImageFormat vk;
        };
      };

      void imageformat_get_depth(Context &p_Context, ImageFormat &p_Format);

      struct Context
      {
        union
        {
          Vulkan::Context vk;
        };
        Window m_Window;
      };

      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
      };

      void context_create(Context &p_Context, ContextCreateParams &p_Params);
      void context_cleanup(Context &p_Context);
      void context_wait_idle(Context &p_Context);

      struct Renderpass
      {
        union
        {
          Vulkan::Renderpass vk;
        };
        Context *context;
        uint8_t formatCount;
        bool *clearTarget;
        bool useDepth;
        bool clearDepth;
      };

      struct RenderpassCreateParams
      {
        Context *context;
        ImageFormat *formats;
        uint8_t formatCount;
        bool *clearTarget;
        bool useDepth;
        bool clearDepth;
      };

      struct RenderpassStartParams
      {

        Framebuffer *framebuffer;
        CommandBuffer *commandBuffer;
        Math::Color *clearColorValues;
        Math::Vector2 clearDepthValue;
      };

      struct RenderpassStopParams
      {
        CommandBuffer *commandBuffer;
      };

      void renderpass_create(Renderpass &p_Renderpass,
                             RenderpassCreateParams &p_Params);
      void renderpass_cleanup(Renderpass &p_Renderpass);
      void renderpass_start(Renderpass &p_Renderpass,
                            RenderpassStartParams &p_Params);
      void renderpass_stop(Renderpass &p_Renderpass,
                           RenderpassStopParams &p_Params);

      struct Framebuffer
      {
        union
        {
          Vulkan::Framebuffer vk;
        };
        Context *context;
      };

      struct FramebufferCreateParams
      {
        Context *context;
        Image2D *renderTargets;
        uint8_t renderTargetCount;
        uint8_t framesInFlight;
        Math::UVector2 dimensions;
        Renderpass *renderpass;
      };

      void framebuffer_create(Framebuffer &p_Framebuffer,
                              FramebufferCreateParams &p_Params);
      void framebuffer_get_dimensions(Framebuffer &p_Framebuffer,
                                      Math::UVector2 &p_Dimensions);
      void framebuffer_cleanup(Framebuffer &p_Framebuffer);

      namespace ImageState {
        enum Enum
        {
          UNDEFINED,
          RENDERTARGET,
          STORAGE,
          SAMPLE,
          SOURCE,
          DESTINATION,
          DEPTH_RENDERTARGET
        };
      }

      struct Image2D
      {
        union
        {
          Vulkan::Image2D vk;
        };
        Context *context;
        uint8_t state;
        bool swapchainImage;
      };

      struct Image2DCreateParams
      {
        Context *context;
        Math::UVector2 dimensions;
        ImageFormat *format;
        bool writable;
        bool depth;
        bool create_image;
      };

      // Transition methode here
      // void image_state_transition_rendertarget(Image2D &p_Image);

      struct CommandBuffer
      {
        union
        {
          Vulkan::CommandBuffer vk;
        };
        Context *context;
      };

      void commandbuffer_start(CommandBuffer &p_CommandBuffer);
      void commandbuffer_stop(CommandBuffer &p_CommandBuffer);

      struct Swapchain
      {
        union
        {
          Vulkan::Swapchain vk;
        };
        Context *context;
        Renderpass renderpass;
      };

      struct SwapchainCreateParams
      {
        Context *context;
        CommandPool *commandPool;
      };

      namespace SwapchainState {
        enum Enum
        {
          SUCCESS,
          FAILED,
          OUT_OF_DATE
        };
      }

      void swapchain_create(Swapchain &p_Swapchain,
                            SwapchainCreateParams &p_Params);
      void swapchain_cleanup(Swapchain &p_Swapchain);
      uint8_t swapchain_prepare(Swapchain &p_Swapchain);
      void swapchain_swap(Swapchain &p_Swapchain);
      CommandBuffer &
      swapchain_get_current_commandbuffer(Swapchain &p_Swapchain);
      CommandBuffer &swapchain_get_commandbuffer(Swapchain &p_Swapchain,
                                                 uint8_t p_Index);
      Framebuffer &swapchain_get_current_framebuffer(Swapchain &p_Swapchain);
      uint8_t swapchain_get_frames_in_flight(Swapchain &p_Swapchain);

      struct CommandPool
      {
        union
        {
          Vulkan::CommandPool vk;
        };
        Context *context;
      };

      struct CommandPoolCreateParams
      {
        Context *context;
      };

      void commandpool_create(CommandPool &p_CommandPool,
                              CommandPoolCreateParams &p_Params);
      void commandpool_cleanup(CommandPool &p_CommandPool);

      struct PipelineInterface
      {
        union
        {
          Vulkan::PipelineInterface vk;
        };
        Context *context;
      };

      struct PipelineInterfaceCreateParams
      {
        Context *context;
      };

      void pipeline_interface_create(PipelineInterface &p_PipelineInterface,
                                     PipelineInterfaceCreateParams &p_Params);

      struct Pipeline
      {
        union
        {
          Vulkan::Pipeline vk;
        };

        Context *context;
      };

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

      struct GraphicsPipelineColorTarget
      {
        bool blendEnable;
        uint32_t wirteMask;
      };

      struct GraphicsPipelineCreateParams
      {
        Context *context;
        const char *vertexShaderPath;
        const char *fragmentShaderPath;
        PipelineInterface *interface;
        Renderpass *renderpass;
        Math::UVector2 dimensions;
        uint8_t cullMode;
        uint8_t frontFace;
        uint8_t polygonMode;
        GraphicsPipelineColorTarget *colorTargets;
        uint8_t colorTargetCount;
      };

      void pipeline_graphics_create(Pipeline &p_Pipeline,
                                    GraphicsPipelineCreateParams &p_Params);

      void pipeline_cleanup(Pipeline &p_Pipeline);

    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
