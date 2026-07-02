#pragma once

#include "LowMath.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

#include "LowUtilContainers.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      struct VulkanQueue
      {
        VkQueue queue = VK_NULL_HANDLE;
        u32 family_index = LOW_UINT32_MAX;
      };

      struct VulkanSwapchainState;
      struct VulkanSwapchainImageState;

      struct VulkanPendingPresent
      {
        VulkanSwapchainState *swapchain = nullptr;
        VulkanSwapchainImageState *image = nullptr;
        u32 image_index = LOW_UINT32_MAX;
        VkSemaphore image_available = VK_NULL_HANDLE;
        VkCommandBuffer present_transition_command = VK_NULL_HANDLE;
      };

      struct VulkanCommandListState
      {
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        QueueRole queue_role = QueueRole::Graphics;
        u32 queue_family_index = LOW_UINT32_MAX;
        bool owns_command_pool = true;
      };

      struct VulkanFrameCommandPool
      {
        VkCommandPool pool = VK_NULL_HANDLE;
        Util::List<VkCommandBuffer> command_buffers;
        u32 next_command_buffer = 0;
      };

      struct FrameState
      {
        VulkanFrameCommandPool graphics;
        VulkanFrameCommandPool transfer;
        VulkanFrameCommandPool compute;
        VkFence frame_fence = VK_NULL_HANDLE;
        Util::List<VkCommandBuffer> pending_graphics_submits;
        Util::List<VulkanPendingPresent> pending_presents;
      };

      struct VulkanSwapchainImageState
      {
        Image image_token;
        ImageView image_view_token;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkSemaphore render_finished = VK_NULL_HANDLE;
        VkFence in_flight_fence = VK_NULL_HANDLE;
      };

      struct VulkanImageState
      {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        VmaAllocationInfo info{};
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent3D extent{};
        u32 mip_levels = 1;
        u32 array_layers = 1;
      };

      struct VulkanImageViewState
      {
        VkImageView image_view = VK_NULL_HANDLE;
      };

      struct VulkanSamplerState
      {
        VkSampler sampler = VK_NULL_HANDLE;
      };

      struct VulkanBufferState
      {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        VmaAllocationInfo info{};
      };

      struct VulkanShaderModuleState
      {
        VkShaderModule shader_module = VK_NULL_HANDLE;
      };

      struct VulkanBindGroupLayoutState
      {
        VkDescriptorSetLayout descriptor_set_layout =
            VK_NULL_HANDLE;
        Util::List<VkDescriptorPoolSize> descriptor_pool_sizes;
      };

      struct VulkanPipelineLayoutState
      {
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
      };

      struct VulkanDescriptorPoolSizeRatio
      {
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        float ratio = 1.0f;
      };

      struct VulkanDescriptorAllocatorGrowable
      {
        Util::List<VulkanDescriptorPoolSizeRatio> ratios;
        Util::List<VkDescriptorPool> full_pools;
        Util::List<VkDescriptorPool> ready_pools;
        u32 sets_per_pool = 0;
      };

      struct VulkanBindGroupState
      {
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
      };

      struct VulkanGraphicsPipelineState
      {
        VkPipeline pipeline = VK_NULL_HANDLE;
      };

      struct VulkanComputePipelineState
      {
        VkPipeline pipeline = VK_NULL_HANDLE;
      };

      struct VulkanFenceState
      {
        VkFence fence = VK_NULL_HANDLE;
      };

      struct VulkanSemaphoreState
      {
        VkSemaphore semaphore = VK_NULL_HANDLE;
      };

      struct VulkanInstanceState
      {
        bool validation_enabled = false;

        vkb::Instance vkb_instance;
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
      };

      struct VulkanAdapterState
      {
        vkb::PhysicalDevice physical_device;
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
      };

      struct VulkanContextState
      {
        u32 frames_in_flight = 0;

        VulkanInstanceState *instance_state = nullptr;

        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        vkb::Device vkb_device;
        VmaAllocator allocator = nullptr;

        VulkanQueue graphics_queue;
        VulkanQueue compute_queue;
        VulkanQueue transfer_queue;
        VulkanQueue present_queue;

        VulkanDescriptorAllocatorGrowable descriptor_allocator;

        Util::List<FrameState> frames;
      };

      struct VulkanSurfaceState
      {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
      };

      struct VulkanSwapchainState
      {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat format;
        VkExtent2D extent;
        Util::List<VulkanSwapchainImageState> images;
        Util::List<VkSemaphore> image_available_semaphores;
        u32 acquired_image_index = LOW_UINT32_MAX;
        bool acquired = false;
        bool resize_requested = false;
      };

      VkCommandBuffer
      acquire_frame_command_buffer(VulkanContextState &p_State,
                                   VulkanFrameCommandPool &p_Pool);
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
