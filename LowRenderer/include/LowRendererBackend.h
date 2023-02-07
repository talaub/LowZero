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
      Framebuffer &swapchain_get_framebuffer(Swapchain &p_Swapchain,
                                             uint8_t p_Index);
      Framebuffer &swapchain_get_current_framebuffer(Swapchain &p_Swapchain);
      uint8_t swapchain_get_frames_in_flight(Swapchain &p_Swapchain);
      uint8_t swapchain_get_image_count(Swapchain &p_Swapchain);
      uint8_t swapchain_get_current_frame_index(Swapchain &p_Swapchain);
      uint8_t swapchain_get_current_image_index(Swapchain &p_Swapchain);

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
        UniformScopeInterface *uniformScopeInterfaces;
        uint32_t uniformScopeInterfaceCount;
      };

      void pipeline_interface_create(PipelineInterface &p_PipelineInterface,
                                     PipelineInterfaceCreateParams &p_Params);

      void pipeline_interface_cleanup(PipelineInterface &p_PipelineInterface);

      struct Pipeline
      {
        union
        {
          Vulkan::Pipeline vk;
        };

        Context *context;
        PipelineInterface *interface;
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
        bool vertexInput;
      };

      struct PipelineBindParams
      {
        CommandBuffer *commandBuffer;
      };

      void pipeline_graphics_create(Pipeline &p_Pipeline,
                                    GraphicsPipelineCreateParams &p_Params);

      void pipeline_cleanup(Pipeline &p_Pipeline);

      void pipeline_bind(Pipeline &p_Pipeline, PipelineBindParams &p_Params);

      struct DrawParams
      {
        CommandBuffer *commandBuffer;
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
      };

      void draw(DrawParams &p_Params);

      struct UniformScopeInterface
      {
        union
        {
          Vulkan::UniformScopeInterface vk;
        };

        Context *context;
      };

      namespace UniformPipelineStep {
        enum Enum
        {
          GRAPHICS,
          COMPUTE,
          VERTEX,
          FRAGMENT
        };
      }

      namespace UniformType {
        enum Enum
        {
          SAMPLER,
          RENDERTARGET,
          UNIFORM_BUFFER,
          STORAGE_BUFFER
        };
      }

      struct UniformInterface
      {
        uint8_t pipelineStep;
        uint8_t type;
        uint32_t uniformCount;
      };

      struct UniformScopeInterfaceCreateParams
      {
        Context *context;
        UniformInterface *uniformInterfaces;
        uint32_t uniformInterfaceCount;
      };

      void uniform_scope_interface_create(
          UniformScopeInterface &p_Interface,
          UniformScopeInterfaceCreateParams &p_Params);
      void uniform_scope_interface_cleanup(UniformScopeInterface &p_Interface);

      struct Uniform
      {
        union
        {
          Vulkan::Uniform vk;
        };

        Context *context;
        uint8_t framesInFlight;
        uint8_t type;
        uint32_t binding;
        uint32_t arrayIndex;
      };

      namespace UniformBufferType {
        enum Enum
        {
          UNIFORM_BUFFER,
          STORAGE_BUFFER
        };
      }

      struct UniformBufferCreateParams
      {
        Context *context;
        Swapchain *swapchain;
        uint8_t bufferType;
        size_t bufferSize;
        uint32_t binding;
        uint32_t arrayIndex;
      };

      struct UniformBufferSetParams
      {
        Context *context;
        Swapchain *swapchain;
        void *value;
      };

      void uniform_buffer_create(Uniform &p_Uniform,
                                 UniformBufferCreateParams &p_Params);

      void uniform_buffer_set(Uniform &p_Uniform,
                              UniformBufferSetParams &p_Params);

      struct UniformPool
      {
        union
        {
          Vulkan::UniformPool vk;
        };

        Context *context;
      };

      struct UniformPoolCreateParams
      {
        Context *context;
        uint32_t uniformBufferCount;
        uint32_t storageBufferCount;
        uint32_t samplerCount;
        uint32_t rendertargetCount;
        uint32_t scopeCount;
      };

      void uniform_pool_create(UniformPool &p_Pool,
                               UniformPoolCreateParams &p_Params);
      void uniform_pool_cleanup(UniformPool &p_Pool);

      struct UniformScope
      {
        union
        {
          Vulkan::UniformScope vk;
        };

        Context *context;
        uint8_t framesInFlight;
      };

      struct UniformScopeCreateParams
      {
        Context *context;
        Swapchain *swapchain;
        UniformPool *pool;
        Uniform *uniforms;
        uint32_t uniformCount;
        UniformScopeInterface *interface;
      };

      void uniform_scope_create(UniformScope &p_Scope,
                                UniformScopeCreateParams &p_Params);

      struct UniformScopeBindParams
      {
        UniformScope *scopes;
        uint32_t scopeCount;
        Context *context;
        Swapchain *swapchain;
        uint32_t startIndex;
        Pipeline *pipeline;
      };

      void uniform_scopes_bind(UniformScopeBindParams &p_Params);
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
