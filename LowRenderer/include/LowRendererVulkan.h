#pragma once

#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <vulkan/vulkan.h>

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct ImageFormat;

      struct Context;
      struct ContextCreateParams;

      struct Swapchain;
      struct SwapchainCreateParams;

      struct Framebuffer;
      struct FramebufferCreateParams;

      struct Image2D;
      struct Image2DCreateParams;

      struct CommandPool;
      struct CommandPoolCreateParams;

      struct CommandBuffer;

      struct Renderpass;
      struct RenderpassCreateParams;
      struct RenderpassStartParams;
      struct RenderpassStopParams;

      struct PipelineInterface;
      struct PipelineInterfaceCreateParams;

      struct Pipeline;
      struct GraphicsPipelineCreateParams;
      struct PipelineBindParams;

      struct DrawParams;

      struct UniformScopeInterface;
      struct UniformScopeInterfaceCreateParams;

      struct Uniform;
      struct UniformBufferCreateParams;
      struct UniformBufferSetParams;

      struct UniformPool;
      struct UniformPoolCreateParams;

      struct UniformScope;
      struct UniformScopeCreateParams;
    } // namespace Backend

    namespace Vulkan {
      struct ImageFormat
      {
        VkFormat m_Handle;
      };

      void vk_imageformat_get_depth(Backend::Context &p_Context,
                                    Backend::ImageFormat &p_Format);

      struct Context
      {
        VkSurfaceKHR m_Surface;
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        bool m_ValidationEnabled;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
      };

      void vk_context_create(Backend::Context &p_Context,
                             Backend::ContextCreateParams &p_Params);
      void vk_context_cleanup(Backend::Context &p_Context);
      void vk_context_wait_idle(Backend::Context &p_Context);

      struct Framebuffer
      {
        VkFramebuffer m_Handle;
        Math::UVector2 m_Dimensions;
      };

      void vk_framebuffer_create(Backend::Framebuffer &p_Framebuffer,
                                 Backend::FramebufferCreateParams &p_Params);
      void vk_framebuffer_get_dimensions(Backend::Framebuffer &p_Framebuffer,
                                         Math::UVector2 &p_Dimensions);
      void vk_framebuffer_cleanup(Backend::Framebuffer &p_Framebuffer);

      struct Image2D
      {
        VkImage m_Image;
        VkImageView m_ImageView;
        VkSampler m_Sampler;
        VkDeviceMemory m_Memory;
      };

      void vk_image2d_create(Backend::Image2D &p_Image2d,
                             Backend::Image2DCreateParams &p_Params);
      void vk_image2d_cleanup(Backend::Image2D &p_Image2d);

      struct Renderpass
      {
        VkRenderPass m_Handle;
      };

      void vk_renderpass_create(Backend::Renderpass &p_Renderpass,
                                Backend::RenderpassCreateParams &p_Params);
      void vk_renderpass_cleanup(Backend::Renderpass &p_Renderpass);
      void vk_renderpass_start(Backend::Renderpass &p_Renderpass,
                               Backend::RenderpassStartParams &p_Params);
      void vk_renderpass_stop(Backend::Renderpass &p_Renderpass,
                              Backend::RenderpassStopParams &p_Params);

      struct CommandBuffer
      {
        VkCommandBuffer m_Handle;
      };

      void vk_commandbuffer_start(Backend::CommandBuffer &p_CommandBuffer);
      void vk_commandbuffer_stop(Backend::CommandBuffer &p_CommandBuffer);

      struct Swapchain
      {
        VkSwapchainKHR m_Handle;
        uint8_t m_FramesInFlight;
        uint8_t m_ImageCount;
        uint8_t m_CurrentFrameIndex;
        uint8_t m_CurrentImageIndex;
        ImageFormat m_ImageFormat;
        Math::UVector2 m_Dimensions;
        VkSemaphore *m_ImageAvailableSemaphores;
        VkSemaphore *m_RenderFinishedSemaphores;
        VkFence *m_InFlightFences;
        Backend::Image2D *m_RenderTargets;
        Backend::CommandBuffer *m_CommandBuffers;
        Backend::Framebuffer *m_Framebuffers;
      };

      void vk_swapchain_create(Backend::Swapchain &p_Swapchain,
                               Backend::SwapchainCreateParams &p_Params);
      void vk_swapchain_cleanup(Backend::Swapchain &p_Swapchain);
      uint8_t vk_swapchain_prepare(Backend::Swapchain &p_Swapchain);
      void vk_swapchain_swap(Backend::Swapchain &p_Swapchain);
      Backend::CommandBuffer &
      vk_swapchain_get_current_commandbuffer(Backend::Swapchain &p_Swapchain);
      Backend::CommandBuffer &
      vk_swapchain_get_commandbuffer(Backend::Swapchain &p_Swapchain,
                                     uint8_t p_Index);
      Backend::Framebuffer &
      vk_swapchain_get_framebuffer(Backend::Swapchain &p_Swapchain,
                                   uint8_t p_Index);
      Backend::Framebuffer &
      vk_swapchain_get_current_framebuffer(Backend::Swapchain &p_Swapchain);
      uint8_t
      vk_swapchain_get_frames_in_flight(Backend::Swapchain &p_Swapchain);
      uint8_t vk_swapchain_get_image_count(Backend::Swapchain &p_Swapchain);
      uint8_t
      vk_swapchain_get_current_frame_index(Backend::Swapchain &p_Swapchain);
      uint8_t
      vk_swapchain_get_current_image_index(Backend::Swapchain &p_Swapchain);

      struct CommandPool
      {
        VkCommandPool m_Handle;
      };

      void vk_commandpool_create(Backend::CommandPool &p_CommandPool,
                                 Backend::CommandPoolCreateParams &p_Params);
      void vk_commandpool_cleanup(Backend::CommandPool &p_CommandPool);

      struct PipelineInterface
      {
        VkPipelineLayout m_Handle;
      };

      void vk_pipeline_interface_create(
          Backend::PipelineInterface &p_PipelineInterface,
          Backend::PipelineInterfaceCreateParams &p_Params);

      void vk_pipeline_interface_cleanup(
          Backend::PipelineInterface &p_PipelineInterface);

      struct Pipeline
      {
        VkPipeline m_Handle;
      };

      void vk_pipeline_graphics_create(
          Backend::Pipeline &p_Pipeline,
          Backend::GraphicsPipelineCreateParams &p_Params);

      void vk_pipeline_cleanup(Backend::Pipeline &p_Pipeline);

      void vk_pipeline_bind(Backend::Pipeline &p_Pipeline,
                            Backend::PipelineBindParams &p_Params);

      void vk_draw(Backend::DrawParams &p_Params);

      struct UniformScopeInterface
      {
        VkDescriptorSetLayout m_Layout;
      };

      void vk_uniform_scope_interface_create(
          Backend::UniformScopeInterface &p_Interface,
          Backend::UniformScopeInterfaceCreateParams &p_Params);

      struct Uniform
      {
        union
        {
          struct
          {
            VkBuffer *buffers;
            VkDeviceMemory *bufferMemories;
            uint32_t bufferSize;
          };
        };
      };

      void
      vk_uniform_buffer_create(Backend::Uniform &p_Uniform,
                               Backend::UniformBufferCreateParams &p_Params);

      void vk_uniform_buffer_set(Backend::Uniform &p_Uniform,
                                 Backend::UniformBufferSetParams &p_Params);

      struct UniformPool
      {
        VkDescriptorPool handle;
      };

      void vk_uniform_pool_create(Backend::UniformPool &p_Pool,
                                  Backend::UniformPoolCreateParams &p_Params);

      void vk_uniform_pool_cleanup(Backend::UniformPool &p_Scope);

      struct UniformScope
      {
        VkDescriptorSet *sets;
      };

      void vk_uniform_scope_create(Backend::UniformScope &p_Scope,
                                   Backend::UniformScopeCreateParams &p_Params);
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
