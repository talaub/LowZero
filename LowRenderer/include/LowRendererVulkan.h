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

        VkQueue m_TransferQueue;
        VkSemaphore *m_TransferFinishedSemaphores;

        VkCommandPool m_CommandPool;
        VkCommandPool m_TransferCommandPool;
        VkDescriptorPool m_DescriptorPool;

        VkPipelineLayout m_BoundPipelineLayout;

        VmaAllocator m_Alloc;

        Backend::ImageResource *m_SwapchainRenderTargets;

        uint32_t m_PipelineResourceSignatureIndex;
        PipelineResourceSignatureInternal *m_PipelineResourceSignatures;
        uint32_t *m_CommittedPipelineResourceSignatures;

        VkDescriptorPool m_ImGuiDescriptorPool;

        VkCommandBuffer m_TransferCommandBuffer;
        VkBuffer m_StagingBuffer;
        VkDeviceMemory m_StagingBufferMemory;
        VkDeviceSize m_StagingBufferSize;
        uint32_t m_StagingBufferUsage;

        VkBuffer m_ReadStagingBuffer;
        VkDeviceMemory m_ReadStagingBufferMemory;
        VkDeviceSize m_ReadStagingBufferSize;
        uint32_t m_ReadStagingBufferUsage;

        VkFence m_StagingBufferFence;
        VkSemaphore m_StagingBufferSemaphore;
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
          GENERAL,
          SHADER_READ_ONLY_OPTIMAL,
          DESTINATION_OPTIMAL,
          PRESENT_SRC,
          DEPTH_STENCIL_ATTACHMENT
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
        uint32_t m_StagingBufferReadOffset;
      };

      struct ImGuiImage
      {
        VkDescriptorSet *m_DescriptorSets;
      };
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
