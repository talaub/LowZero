#pragma once

#include "vulkan/vulkan_core.h"
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

      struct CommandPool;
      struct CommandPoolCreateParams;

      struct Renderpass;
      struct RenderpassCreateParams;
      struct RenderpassStartParams;
      struct RenderpassStopParams;
    } // namespace Backend

    namespace Vulkan {
      struct ImageFormat
      {
        VkFormat m_Handle;
      };

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

      struct Swapchain
      {
        VkSwapchainKHR m_Handle;
        uint8_t m_FramesInFlight;
        uint8_t m_CurrentFrameIndex;
        ImageFormat m_ImageFormat;
        Math::UVector2 m_Dimensions;
        VkSemaphore *m_ImageAvailableSemaphores;
        VkSemaphore *m_RenderFinishedSemaphores;
        VkFence *m_InFlightFences;
        Image2D *m_RenderTargets;
        Renderpass m_Renderpass;
      };

      void vk_swapchain_create(Backend::Swapchain &p_Swapchain,
                               Backend::SwapchainCreateParams &p_Params);

      struct CommandPool
      {
        VkCommandPool m_Handle;
      };

      void vk_commandpool_create(Backend::CommandPool &p_CommandPool,
                                 Backend::CommandPoolCreateParams &p_Params);
      void vk_commandpool_cleanup(Backend::CommandPool &p_CommandPool);
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
