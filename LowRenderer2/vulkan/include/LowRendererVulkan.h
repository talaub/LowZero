#pragma once

#include "vulkan/vulkan.h"
#include "VkBootstrap.h"

#include "LowMath.h"

#include "vk_mem_alloc.h"

#include "LowRendererVulkanDescriptor.h"

#include "LowUtilContainers.h"

#define VK_FRAME_OVERLAP 2

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      struct AllocatedBuffer
      {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
      };

      struct StagingBuffer
      {
        AllocatedBuffer buffer;
        size_t size;
        size_t occupied;
      };

      namespace Global {
        bool initialize();
        bool cleanup();

        VkInstance get_instance();
        VkPhysicalDevice get_physical_device();
        VkDevice get_device();
        vkb::Device get_vkbdevice();
        VkSurfaceKHR get_surface();
        VmaAllocator get_allocator();
        DescriptorUtil::DescriptorAllocator &
        get_global_descriptor_allocator();

        VkFormat get_swapchain_format();

        VkQueue get_graphics_queue();
        u32 get_graphics_queue_family();

        VkQueue get_transfer_queue();
        u32 get_transfer_queue_family();

        u32 get_frame_overlap();
        u64 get_frame_number();
        u32 get_current_frame_index();
        VkCommandBuffer get_current_command_buffer();
        VkCommandPool get_current_command_pool();
        StagingBuffer &get_current_resource_staging_buffer();

        bool advance_frame_count();
      } // namespace Global

      struct Context;

      struct AllocatedImage
      {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D extent;
        VkFormat format;
        VkImageLayout layout;
      };

      struct FrameData
      {
        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;

        VkFence renderFence;

        StagingBuffer frameStagingBuffer;
      };

      struct Swapchain
      {
        Context *context;

        VkSwapchainKHR vkhandle;
        VkFormat imageFormat;
        VkExtent2D extent;

        Util::List<VkImage> images;
        Util::List<VkImageView> imageViews;

        AllocatedImage drawImage;
        VkExtent2D drawExtent;

        u32 imageIndex;
      };

      struct Context
      {
        bool requireResize;

        Swapchain swapchain;

        FrameData *frames;

        FrameData &get_current_frame()
        {
          return frames[Global::get_current_frame_index()];
        }

        // TODO: These are right now used very specifically. they
        // could in future be repurposed for just global descriptors
        // like textures, materials, etc. But we need to evaluate this
        // when the time comes
        VkDescriptorSet drawImageDescriptors;
        VkDescriptorSetLayout drawImageDescriptorLayout;
      };

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
