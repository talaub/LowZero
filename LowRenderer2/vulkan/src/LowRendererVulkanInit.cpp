#include "LowRendererVulkanInit.h"

#include "LowRendererGlobals.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace InitUtil {
        VkCommandPoolCreateInfo
        command_pool_create_info(u32 p_QueueFamilyIndex,
                                 VkCommandPoolCreateFlags p_Flags)
        {
          VkCommandPoolCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          l_Info.pNext = nullptr;
          l_Info.queueFamilyIndex = p_QueueFamilyIndex;
          l_Info.flags = p_Flags;
          return l_Info;
        }

        VkCommandBufferAllocateInfo
        command_buffer_allocate_info(VkCommandPool p_CommandPool,
                                     u32 p_Count)
        {
          VkCommandBufferAllocateInfo l_Info = {};
          l_Info.sType =
              VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
          l_Info.pNext = nullptr;

          l_Info.commandPool = p_CommandPool;
          l_Info.commandBufferCount = p_Count;
          l_Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
          return l_Info;
        }

        VkFenceCreateInfo
        fence_create_info(VkFenceCreateFlags p_Flags)
        {
          VkFenceCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
          l_Info.pNext = nullptr;

          l_Info.flags = p_Flags;

          return l_Info;
        }

        VkSemaphoreCreateInfo
        semaphore_create_info(VkSemaphoreCreateFlags p_Flags)
        {
          VkSemaphoreCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
          l_Info.pNext = nullptr;
          l_Info.flags = p_Flags;
          return l_Info;
        }

        VkCommandBufferBeginInfo
        command_buffer_begin_info(VkCommandBufferUsageFlags p_Flags)
        {
          VkCommandBufferBeginInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          l_Info.pNext = nullptr;

          l_Info.pInheritanceInfo = nullptr;
          l_Info.flags = p_Flags;
          return l_Info;
        }

        VkImageSubresourceRange
        image_subresource_range(VkImageAspectFlags p_AspectMask)
        {
          VkImageSubresourceRange l_SubImage{};
          l_SubImage.aspectMask = p_AspectMask;
          l_SubImage.baseMipLevel = 0;
          l_SubImage.levelCount = VK_REMAINING_MIP_LEVELS;
          l_SubImage.baseArrayLayer = 0;
          l_SubImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

          return l_SubImage;
        }

        VkSemaphoreSubmitInfo
        semaphore_submit_info(VkPipelineStageFlags2 p_StageMask,
                              VkSemaphore p_Semaphore)
        {
          VkSemaphoreSubmitInfo l_Info{};
          l_Info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
          l_Info.pNext = nullptr;
          l_Info.semaphore = p_Semaphore;
          l_Info.stageMask = p_StageMask;
          l_Info.deviceIndex = 0;
          l_Info.value = 1;

          return l_Info;
        }

        VkCommandBufferSubmitInfo
        command_buffer_submit_info(VkCommandBuffer p_Cmd)
        {
          VkCommandBufferSubmitInfo l_Info{};
          l_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
          l_Info.pNext = nullptr;
          l_Info.commandBuffer = p_Cmd;
          l_Info.deviceMask = 0;

          return l_Info;
        }

        VkSubmitInfo2
        submit_info(VkCommandBufferSubmitInfo *p_Cmd,
                    VkSemaphoreSubmitInfo *p_SignalSemaphoreInfo,
                    VkSemaphoreSubmitInfo *p_WaitSemaphoreInfo)
        {
          VkSubmitInfo2 l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
          l_Info.pNext = nullptr;

          l_Info.waitSemaphoreInfoCount =
              p_WaitSemaphoreInfo == nullptr ? 0 : 1;
          l_Info.pWaitSemaphoreInfos = p_WaitSemaphoreInfo;

          l_Info.signalSemaphoreInfoCount =
              p_SignalSemaphoreInfo == nullptr ? 0 : 1;
          l_Info.pSignalSemaphoreInfos = p_SignalSemaphoreInfo;

          l_Info.commandBufferInfoCount = 1;
          l_Info.pCommandBufferInfos = p_Cmd;

          return l_Info;
        }

        VkImageCreateInfo
        image_create_info(VkFormat p_Format,
                          VkImageUsageFlags p_UsageFlags,
                          VkExtent3D p_Extent)
        {
          VkImageCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
          l_Info.pNext = nullptr;

          // TODO: Probabaly change
          // This probably has to be dynamic
          l_Info.imageType = VK_IMAGE_TYPE_2D;

          l_Info.format = p_Format;
          l_Info.extent = p_Extent;

          l_Info.mipLevels = 1;
          l_Info.arrayLayers = 1;

          // MSAA
          l_Info.samples = VK_SAMPLE_COUNT_1_BIT;

          // TODO: Check that again and learn what it does
          // May have to be dynamic (change based on input params)
          l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
          l_Info.usage = p_UsageFlags;

          return l_Info;
        }

        VkImageCreateInfo
        image_create_info_mipmapped(VkFormat p_Format,
                                    VkImageUsageFlags p_UsageFlags,
                                    VkExtent3D p_Extent)
        {
          VkImageCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
          l_Info.pNext = nullptr;

          // TODO: Probabaly change
          // This probably has to be dynamic
          l_Info.imageType = VK_IMAGE_TYPE_2D;

          l_Info.format = p_Format;
          l_Info.extent = p_Extent;

          l_Info.mipLevels = IMAGE_MIPMAP_COUNT;
          l_Info.arrayLayers = 1;

          // MSAA
          l_Info.samples = VK_SAMPLE_COUNT_1_BIT;

          // TODO: Check that again and learn what it does
          // May have to be dynamic (change based on input params)
          l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
          l_Info.usage = p_UsageFlags;

          return l_Info;
        }

        VkImageCreateInfo
        cubemap_create_info(VkFormat p_Format,
                            VkImageUsageFlags p_UsageFlags,
                            VkExtent3D p_Extent)
        {
          VkImageCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
          l_Info.pNext = nullptr;

          // TODO: Probabaly change
          // This probably has to be dynamic
          l_Info.imageType = VK_IMAGE_TYPE_2D;

          l_Info.format = p_Format;
          l_Info.extent = p_Extent;

          l_Info.mipLevels = 1;
          l_Info.arrayLayers = 6;

          // MSAA
          l_Info.samples = VK_SAMPLE_COUNT_1_BIT;

          // TODO: Check that again and learn what it does
          // May have to be dynamic (change based on input params)
          l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
          l_Info.usage = p_UsageFlags;

          return l_Info;
        }

        VkImageCreateInfo
        cubemap_create_info_mipmapped(VkFormat p_Format,
                                      VkImageUsageFlags p_UsageFlags,
                                      VkExtent3D p_Extent)
        {
          VkImageCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
          l_Info.pNext = nullptr;

          // TODO: Probabaly change
          // This probably has to be dynamic
          l_Info.imageType = VK_IMAGE_TYPE_2D;

          l_Info.format = p_Format;
          l_Info.extent = p_Extent;

          l_Info.mipLevels = IMAGE_MIPMAP_COUNT;
          l_Info.arrayLayers = 1;

          // MSAA
          l_Info.samples = VK_SAMPLE_COUNT_1_BIT;

          // TODO: Check that again and learn what it does
          // May have to be dynamic (change based on input params)
          l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
          l_Info.usage = p_UsageFlags;

          return l_Info;
        }

        VkImageViewCreateInfo
        imageview_create_info(VkFormat p_Format, VkImage p_Image,
                              VkImageAspectFlags p_AspectFlags)
        {
          VkImageViewCreateInfo l_Info = {};
          l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          l_Info.pNext = nullptr;

          // TODO: Check again that is probably required to be dynamic
          l_Info.viewType = VK_IMAGE_VIEW_TYPE_2D;
          l_Info.image = p_Image;
          l_Info.format = p_Format;

          // TODO: Check all of those. They definitely need to be
          // dynamic Of course I need to learn about mipmaps first :)
          l_Info.subresourceRange.baseMipLevel = 0;
          l_Info.subresourceRange.levelCount = 1;
          l_Info.subresourceRange.baseArrayLayer = 0;
          l_Info.subresourceRange.layerCount = 1;

          l_Info.subresourceRange.aspectMask = p_AspectFlags;

          return l_Info;
        }

        VkRenderingAttachmentInfo
        attachment_info(VkImageView p_View, VkClearValue *p_Clear,
                        VkImageLayout p_Layout)
        {
          VkRenderingAttachmentInfo l_ColorAttachment{};
          l_ColorAttachment.sType =
              VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          l_ColorAttachment.pNext = nullptr;

          l_ColorAttachment.imageView = p_View;
          l_ColorAttachment.imageLayout = p_Layout;
          l_ColorAttachment.loadOp = p_Clear
                                         ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                         : VK_ATTACHMENT_LOAD_OP_LOAD;
          l_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          if (p_Clear) {
            l_ColorAttachment.clearValue = *p_Clear;
          }

          return l_ColorAttachment;
        }

        VkRenderingInfo
        rendering_info(VkExtent2D p_RenderExtent,
                       VkRenderingAttachmentInfo *p_ColorAttachment,
                       u32 p_ColorAttachmentCount,
                       VkRenderingAttachmentInfo *p_DepthAttachment)
        {
          VkRenderingInfo l_RenderInfo{};
          l_RenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
          l_RenderInfo.pNext = nullptr;

          l_RenderInfo.renderArea =
              VkRect2D{VkOffset2D{0, 0}, p_RenderExtent};
          l_RenderInfo.layerCount = 1;
          l_RenderInfo.colorAttachmentCount = p_ColorAttachmentCount;
          l_RenderInfo.pColorAttachments = p_ColorAttachment;
          l_RenderInfo.pDepthAttachment = p_DepthAttachment;
          l_RenderInfo.pStencilAttachment = nullptr;

          return l_RenderInfo;
        }
      } // namespace InitUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
