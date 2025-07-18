#pragma once

#include "vulkan/vulkan.h"
#include "VkBootstrap.h"

#include "LowMath.h"

#include "vk_mem_alloc.h"

#include "LowRendererVulkanDescriptor.h"

#include "LowUtilContainers.h"
#include <vulkan/vulkan_core.h>

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

      struct DynamicBufferFreeSlot
      {
        uint32_t start;
        uint32_t length;
      };

      struct DescriptorData
      {
        VkDescriptorSet set;
        VkDescriptorSetLayout layout;
      };

      struct ViewInfoFrameData
      {
        alignas(16) Math::Matrix4x4 viewMatrix;
        alignas(16) Math::Matrix4x4 projectionMatrix;
        alignas(16) Math::Matrix4x4 viewProjectionMatrix;
        alignas(16) Math::Vector2 dimensions;
        alignas(16) Math::Vector2 inverseDimensions;
      };

      struct DynamicBuffer
      {
        bool reserve(u32 p_ElementCount, u32 *p_StartOut);

        void free(u32 p_Position, u32 p_ElementCount);

        void clear();

        void destroy();

        u32 get_used_elements() const;

        AllocatedBuffer m_Buffer;

        u32 m_ElementSize;
        u32 m_ElementCount;
        Util::List<DynamicBufferFreeSlot> m_FreeSlots;
      };

      struct Samplers
      {
        VkSampler no_lod_nearest_repeat_black;
        VkSampler no_lod_nearest_repeat_white;

        Util::List<VkSampler> lod_nearest_repeat_black;
        Util::List<VkSampler> lod_linear_repeat_black;
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

        DynamicBuffer &get_mesh_vertex_buffer();
        DynamicBuffer &get_mesh_index_buffer();

        DynamicBuffer &get_drawcommand_buffer();

        AllocatedBuffer get_material_data_buffer();

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

        VkDescriptorSetLayout get_view_info_descriptor_set_layout();
        VkDescriptorSetLayout get_gbuffer_descriptor_set_layout();

        VkPipelineLayout get_lighting_pipeline_layout();

        Samplers &get_samplers();

        VkDescriptorSetLayout get_global_descriptor_set_layout();
        VkDescriptorSet get_global_descriptor_set();

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

        // StagingBuffer frameStagingBuffer;
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
