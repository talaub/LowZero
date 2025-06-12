#include "LowRendererVulkanImage.h"

#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanBase.h"

#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace ImageUtil {
        namespace Internal {
          bool cmd_transition(VkCommandBuffer p_Cmd,
                              AllocatedImage &p_AllocatedImage,
                              VkImageLayout p_NewLayout)
          {
            // Early out if the layouts already match
            if (p_AllocatedImage.layout == p_NewLayout) {
              return true;
            }

            bool l_Result =
                cmd_transition(p_Cmd, p_AllocatedImage.image,
                               p_AllocatedImage.layout, p_NewLayout);

            if (l_Result) {
              p_AllocatedImage.layout = p_NewLayout;
            }
            return l_Result;
          }

          bool cmd_transition(VkCommandBuffer p_Cmd, VkImage p_Image,
                              VkImageLayout p_CurrentLayout,
                              VkImageLayout p_NewLayout)
          {
            VkImageMemoryBarrier2 l_ImageBarrier{};
            l_ImageBarrier.sType =
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            l_ImageBarrier.pNext = nullptr;

            l_ImageBarrier.srcStageMask =
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            l_ImageBarrier.srcAccessMask =
                VK_ACCESS_2_MEMORY_WRITE_BIT;
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
            l_DependencyInfo.sType =
                VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
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

            VkImageBlit2 l_BlitRegion{};
            l_BlitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
            l_BlitRegion.pNext = nullptr;

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
            // for copying the drawimage to the swapchain image. Both
            // of them obviously don't have mips.
            l_BlitRegion.srcSubresource.mipLevel = 0;

            l_BlitRegion.dstSubresource.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
            l_BlitRegion.dstSubresource.baseArrayLayer = 0;
            l_BlitRegion.dstSubresource.layerCount = 1;
            // TODO: This is especially fishy 2 :D - maybe it does
            // need multiple of those copy helpers because this one is
            // used for copying the drawimage to the swapchain image.
            // Both of them obviously don't have mips.
            l_BlitRegion.dstSubresource.mipLevel = 0;

            VkBlitImageInfo2 l_BlitInfo{};
            l_BlitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            l_BlitInfo.pNext = nullptr;
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

          AllocatedImage create(VkExtent3D p_Size, VkFormat p_Format,
                                VkImageUsageFlags p_Usage,
                                bool p_Mipmapped)
          {
            AllocatedImage l_Image;
            l_Image.format = p_Format;
            l_Image.extent = p_Size;
            l_Image.layout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageCreateInfo l_ImgInfo = InitUtil::image_create_info(
                p_Format,
                p_Usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                p_Size);
            if (p_Mipmapped) {
              l_ImgInfo.mipLevels = 4;
            } else {
              l_ImgInfo.mipLevels = 1;
            }

            VmaAllocationCreateInfo l_AllocInfo = {};
            l_AllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            l_AllocInfo.requiredFlags = VkMemoryPropertyFlags(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            LOWR_VK_CHECK(
                vmaCreateImage(Global::get_allocator(), &l_ImgInfo,
                               &l_AllocInfo, &l_Image.image,
                               &l_Image.allocation, nullptr),
                "Failed to create vulkan image");

            VkImageAspectFlags l_AspectFlag =
                VK_IMAGE_ASPECT_COLOR_BIT;
            if (p_Format == VK_FORMAT_D32_SFLOAT) {
              l_AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            VkImageViewCreateInfo l_ViewInfo =
                InitUtil::imageview_create_info(
                    p_Format, l_Image.image, l_AspectFlag);
            l_ViewInfo.subresourceRange.levelCount =
                l_ImgInfo.mipLevels;

            LOWR_VK_CHECK(vkCreateImageView(Global::get_device(),
                                            &l_ViewInfo, nullptr,
                                            &l_Image.imageView),
                          "Failed to create vulkan imageview");

            return l_Image;
          }

          bool destroy(AllocatedImage &p_Image)
          {
            vkDestroyImageView(Global::get_device(),
                               p_Image.imageView, nullptr);
            vmaDestroyImage(Global::get_allocator(), p_Image.image,
                            p_Image.allocation);
            return true;
          }
        } // namespace Internal

        bool cmd_transition(VkCommandBuffer p_Cmd, Image p_Image,
                            VkImageLayout p_NewLayout)
        {
          return Internal::cmd_transition(
              p_Cmd, p_Image.get_allocated_image(), p_NewLayout);
        }

        bool cmd_transition(VkCommandBuffer p_Cmd, Image p_Image,
                            VkImageLayout p_CurrentLayout,
                            VkImageLayout p_NewLayout)
        {
          return Internal::cmd_transition(
              p_Cmd, p_Image.get_allocated_image().image,
              p_CurrentLayout, p_NewLayout);
        }

        bool cmd_copy2D(VkCommandBuffer p_Cmd, Image p_Source,
                        Image p_Destination,
                        VkExtent2D p_SourceExtent,
                        VkExtent2D p_DestinationExtent)
        {
          return Internal::cmd_copy2D(p_Cmd,
                               p_Source.get_allocated_image().image,
                               p_Source.get_allocated_image().image,
                               p_SourceExtent, p_DestinationExtent);
        }

        bool cmd_clear_color(VkCommandBuffer p_Cmd, Image p_Image,
                             VkImageLayout p_ImageLayout,
                             Math::Color p_Color)
        {
          return Internal::cmd_clear_color(
              p_Cmd, p_Image.get_allocated_image().image,
              p_ImageLayout, p_Color);
        }

        bool create(Image p_Image, VkExtent3D p_Size,
                    VkFormat p_Format, VkImageUsageFlags p_Usage,
                    bool p_Mipmapped)
        {
          AllocatedImage l_Image = Internal::create(
              p_Size, p_Format, p_Usage, p_Mipmapped);

          p_Image.set_allocated_image(l_Image);

          return true;
        }

        bool destroy(Image p_Image)
        {
          return Internal::destroy(p_Image.get_allocated_image());
        }
      } // namespace ImageUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
