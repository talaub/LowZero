#pragma once

#include <vulkan/vulkan.h>

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace InitUtil {
        VkCommandPoolCreateInfo command_pool_create_info(
            u32 p_QueueFamilyIndex,
            VkCommandPoolCreateFlags p_Flags = 0);

        VkCommandBufferAllocateInfo
        command_buffer_allocate_info(VkCommandPool p_CommandPool,
                                     u32 p_Count = 1);

        VkFenceCreateInfo
        fence_create_info(VkFenceCreateFlags p_Flags = 0);

        VkSemaphoreCreateInfo
        semaphore_create_info(VkSemaphoreCreateFlags p_Flags = 0);

        VkCommandBufferBeginInfo command_buffer_begin_info(
            VkCommandBufferUsageFlags p_Flags = 0);

        VkImageSubresourceRange
        image_subresource_range(VkImageAspectFlags p_AspectMask);

        VkSemaphoreSubmitInfo
        semaphore_submit_info(VkPipelineStageFlags2 p_StageMask,
                              VkSemaphore p_Semaphore);

        VkCommandBufferSubmitInfo
        command_buffer_submit_info(VkCommandBuffer p_Cmd);

        VkSubmitInfo2
        submit_info(VkCommandBufferSubmitInfo *p_Cmd,
                    VkSemaphoreSubmitInfo *p_SignalSemaphoreInfo,
                    VkSemaphoreSubmitInfo *p_WaitSemaphoreInfo);

        VkImageCreateInfo
        image_create_info(VkFormat p_Format,
                          VkImageUsageFlags p_UsageFlags,
                          VkExtent3D p_Extent);
        VkImageCreateInfo
        image_create_info_mipmapped(VkFormat p_Format,
                                    VkImageUsageFlags p_UsageFlags,
                                    VkExtent3D p_Extent);
        VkImageCreateInfo
        cubemap_create_info(VkFormat p_Format,
                            VkImageUsageFlags p_UsageFlags,
                            VkExtent3D p_Extent);
        VkImageCreateInfo
        cubemap_create_info_mipmapped(VkFormat p_Format,
                                      VkImageUsageFlags p_UsageFlags,
                                      VkExtent3D p_Extent);

        VkImageViewCreateInfo
        imageview_create_info(VkFormat p_Format, VkImage p_Image,
                              VkImageAspectFlags p_AspectFlags);

        VkRenderingAttachmentInfo
        attachment_info(VkImageView p_View, VkClearValue *p_Clear,
                        VkImageLayout p_Layout =
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingInfo
        rendering_info(VkExtent2D p_RenderExtent,
                       VkRenderingAttachmentInfo *p_ColorAttachment,
                       VkRenderingAttachmentInfo *p_DepthAttachment);
      } // namespace InitUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
