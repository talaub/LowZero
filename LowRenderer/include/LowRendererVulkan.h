#pragma once

#include "vulkan/vulkan_core.h"
#include <stdint.h>
#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1002000
#include "../../LowDependencies/VulkanMemoryAllocator/include/vk_mem_alloc.h"

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Backend {
      struct ApiBackendCallback;
      struct ImageResource;
      struct PipelineResourceBinding;
    } // namespace Backend

    namespace Vulkan {
      void initialize_callback(Backend::ApiBackendCallback &p_Callbacks);

      struct PipelineResourceSignatureInternal
      {
        VkDescriptorSet *m_DescriptorSets;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        Backend::PipelineResourceBinding *m_Bindings;
        uint32_t m_BindingCount;
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

        VkSwapchainKHR m_Swapchain;
        VkSemaphore *m_ImageAvailableSemaphores;
        VkSemaphore *m_RenderFinishedSemaphores;
        VkFence *m_InFlightFences;
        VkCommandBuffer *m_CommandBuffers;

        VkCommandPool m_CommandPool;
        VkDescriptorPool m_DescriptorPool;

        VkPipelineLayout m_BoundPipelineLayout;

        VmaAllocator m_Alloc;

        Backend::ImageResource *m_SwapchainRenderTargets;

        uint32_t m_PipelineResourceSignatureIndex;
        PipelineResourceSignatureInternal *m_PipelineResourceSignatures;
        uint32_t *m_CommittedPipelineResourceSignatures;
      };

      struct Renderpass
      {
        VkRenderPass m_Renderpass;
        VkFramebuffer m_Framebuffer;
      };

      namespace ImageState {
        enum Enum
        {
          UNDEFINED,
          GENERAL
        };
      };

      struct ImageResource
      {
        VkImage m_Image;
        VkImageView m_ImageView;
        VkSampler m_Sampler;
        VkDeviceMemory m_Memory;
        uint8_t m_State;
      };

      struct PipelineResourceSignature
      {
        uint32_t m_Index;
      };

      struct Pipeline
      {
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
      };

      struct Buffer
      {
        VkBuffer m_Buffer;
        VkDeviceMemory m_Memory;
      };
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
