#pragma once

#include "vulkan/vulkan.h"
#include "VkBootstrap.h"

#include "LowMath.h"

#include "vk_mem_alloc.h"

#include "LowRendererVulkanDescriptor.h"

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

        bool requireResize;

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

        DescriptorUtil::DescriptorAllocator globalDescriptorAllocator;
        VkDescriptorPool imguiPool;

        // TODO: These are right now used very specifically. they
        // could in future be repurposed for just global descriptors
        // like textures, materials, etc. But we need to evaluate this
        // when the time comes
        VkDescriptorSet drawImageDescriptors;
        VkDescriptorSetLayout drawImageDescriptorLayout;

        // TODO: They are very specific right now. Just for testing.
        // Please remove at some point in the future
        VkPipeline gradientPipeline;
        VkPipelineLayout gradientPipelineLayout;

        // TODO: They are very specific right now. Just for testing.
        // Please remove at some point in the future
        VkPipelineLayout trianglePipelineLayout;
        VkPipeline trianglePipeline;
      };

      struct AllocatedBuffer
      {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
      };

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
