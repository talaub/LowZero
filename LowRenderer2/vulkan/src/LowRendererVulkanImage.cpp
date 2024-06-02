#include "LowRendererVulkanImage.h"

#include "LowRendererVulkanInit.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace ImageUtil {
        bool cmd_transition(VkCommandBuffer p_Cmd, VkImage p_Image,
                            VkImageLayout p_CurrentLayout,
                            VkImageLayout p_NewLayout)
        {
          VkImageMemoryBarrier2 l_ImageBarrier{
              .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
          l_ImageBarrier.pNext = nullptr;

          l_ImageBarrier.srcStageMask =
              VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
          l_ImageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
          l_ImageBarrier.dstStageMask =
              VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
          l_ImageBarrier.dstAccessMask =
              VK_ACCESS_2_MEMORY_WRITE_BIT |
              VK_ACCESS_2_MEMORY_READ_BIT;

          l_ImageBarrier.oldLayout = p_CurrentLayout;
          l_ImageBarrier.newLayout = p_NewLayout;

          VkImageAspectFlags l_AspectMask =
              (p_NewLayout ==
               VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                  ? VK_IMAGE_ASPECT_DEPTH_BIT
                  : VK_IMAGE_ASPECT_COLOR_BIT;
          l_ImageBarrier.subresourceRange =
              InitUtil::image_subresource_range(l_AspectMask);
          l_ImageBarrier.image = p_Image;

          VkDependencyInfo l_DependencyInfo{};
          l_DependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
          l_DependencyInfo.pNext = nullptr;

          l_DependencyInfo.imageMemoryBarrierCount = 1;
          l_DependencyInfo.pImageMemoryBarriers = &l_ImageBarrier;

          vkCmdPipelineBarrier2(p_Cmd, &l_DependencyInfo);

          return true;
        }

        bool cmd_copy2D(VkCommandBuffer p_Cmd, VkImage p_Source,
                        VkImage p_Destination,
                        VkExtent2D p_SourceExtent,
                        VkExtent2D p_DestinationExtent)
        {
          // TODO: Please check entire function for MIP stuff and
          // other things that may need to be dynamic

          VkImageBlit2 l_BlitRegion{
              .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
              .pNext = nullptr};

          l_BlitRegion.srcOffsets[1].x = p_SourceExtent.width;
          l_BlitRegion.srcOffsets[1].y = p_SourceExtent.height;
          l_BlitRegion.srcOffsets[1].z = 1;

          l_BlitRegion.dstOffsets[1].x = p_DestinationExtent.width;
          l_BlitRegion.dstOffsets[1].y = p_DestinationExtent.height;
          l_BlitRegion.dstOffsets[1].z = 1;

          l_BlitRegion.srcSubresource.aspectMask =
              VK_IMAGE_ASPECT_COLOR_BIT;
          l_BlitRegion.srcSubresource.baseArrayLayer = 0;
          l_BlitRegion.srcSubresource.layerCount = 1;
          // TODO: This is especially fishy - maybe it does need
          // multiple of those copy helpers because this one is used
          // for copying the drawimage to the swapchain image. Both of
          // them obviously don't have mips.
          l_BlitRegion.srcSubresource.mipLevel = 0;

          l_BlitRegion.dstSubresource.aspectMask =
              VK_IMAGE_ASPECT_COLOR_BIT;
          l_BlitRegion.dstSubresource.baseArrayLayer = 0;
          l_BlitRegion.dstSubresource.layerCount = 1;
          // TODO: This is especially fishy 2 :D - maybe it does need
          // multiple of those copy helpers because this one is used
          // for copying the drawimage to the swapchain image. Both of
          // them obviously don't have mips.
          l_BlitRegion.dstSubresource.mipLevel = 0;

          VkBlitImageInfo2 l_BlitInfo{
              .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
              .pNext = nullptr};
          l_BlitInfo.dstImage = p_Destination;
          l_BlitInfo.dstImageLayout =
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
          l_BlitInfo.srcImage = p_Source;
          l_BlitInfo.srcImageLayout =
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
          l_BlitInfo.filter = VK_FILTER_LINEAR;
          l_BlitInfo.regionCount = 1;
          l_BlitInfo.pRegions = &l_BlitRegion;

          vkCmdBlitImage2(p_Cmd, &l_BlitInfo);

          return true;
        }

        bool cmd_clear_color(VkCommandBuffer p_Cmd, VkImage p_Image,
                             VkImageLayout p_ImageLayout,
                             Math::Color p_Color)
        {
          VkClearColorValue l_ClearValue = {
              {p_Color.r, p_Color.g, p_Color.b, p_Color.a}};

          VkImageSubresourceRange l_ClearRange =
              InitUtil::image_subresource_range(
                  VK_IMAGE_ASPECT_COLOR_BIT);

          vkCmdClearColorImage(p_Cmd, p_Image, p_ImageLayout,
                               &l_ClearValue, 1, &l_ClearRange);

          return true;
        }

        bool destroy(const Context &p_Context, Image &p_Image)
        {
          vkDestroyImageView(p_Context.device, p_Image.imageView,
                             nullptr);
          vmaDestroyImage(p_Context.allocator, p_Image.image,
                          p_Image.allocation);
          return true;
        }
      } // namespace ImageUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
