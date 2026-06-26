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
        Util::List<VulkanPendingPresent> pending_presents;
      };

      struct VulkanSwapchainImageState
      {
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkSemaphore render_finished = VK_NULL_HANDLE;
        VkFence in_flight_fence = VK_NULL_HANDLE;
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
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
