#pragma once

#include "vulkan/vulkan.h"
#include "VkBootstrap.h"

#include "LowMath.h"

#include "vk_mem_alloc.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      struct Context;

      struct Image
      {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D extent;
        VkFormat format;
      };

      struct FrameData
      {
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;

        VkFence renderFence;
      };

      struct Swapchain
      {
        Context *context;

        VkSwapchainKHR vkhandle;
        VkFormat imageFormat;
        VkExtent2D extent;

        Util::List<VkImage> images;
        Util::List<VkImageView> imageViews;

        Image drawImage;
        VkExtent2D drawExtent;
      };

      struct Context
      {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice gpu;
        VkDevice device;
        vkb::Device vkbDevice;
        VkSurfaceKHR surface;

        VmaAllocator allocator;

        Swapchain swapchain;

        u32 frameNumber = 0;
        u32 frameOverlap;

        VkQueue graphicsQueue;
        u32 graphicsQueueFamily;

        FrameData *frames;

        FrameData &get_current_frame()
        {
          return frames[frameNumber % frameOverlap];
        }
      };

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
