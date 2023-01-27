#pragma once

#include "LowRendererWindow.h"

#include "LowRendererVulkan.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct Framebuffer;
      struct CommandBuffer;

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

      void swapchain_create(Swapchain &p_Swapchain,
                            SwapchainCreateParams &p_Params);
      void swapchain_cleanup(Swapchain &p_Swapchain);

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
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
