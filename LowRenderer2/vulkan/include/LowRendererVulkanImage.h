#pragma once

#include <vulkan/vulkan.h>

#include "LowMath.h"

#include "LowRendererVulkan.h"
#include "LowRendererVkImage.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace ImageUtil {
        namespace Internal {
          bool cmd_transition(VkCommandBuffer p_Cmd,
                              AllocatedImage &p_AllocatedImage,
                              VkImageLayout p_NewLayout);
          bool cmd_transition(VkCommandBuffer p_Cmd, VkImage p_Image,
                              VkImageLayout p_CurrentLayout,
                              VkImageLayout p_NewLayout);

          bool cmd_copy2D(VkCommandBuffer p_Cmd, VkImage p_Source,
                          VkImage p_Destination,
                          VkExtent2D p_SourceExtent,
                          VkExtent2D p_DestinationExtent);

          bool cmd_clear_color(VkCommandBuffer p_Cmd, VkImage p_Image,
                               VkImageLayout p_ImageLayout,
                               Math::Color p_Color);

          AllocatedImage create(VkExtent3D p_Size, VkFormat p_Format,
                                VkImageUsageFlags p_Usage,
                                bool p_Mipmapped);
          AllocatedImage create_cubemap(VkExtent3D p_Size,
                                        VkFormat p_Format,
                                        VkImageUsageFlags p_Usage,
                                        bool p_Mipmapped);
          bool destroy(AllocatedImage &p_Image);
        } // namespace Internal

        bool cmd_transition(VkCommandBuffer p_Cmd, Image p_Image,
                            VkImageLayout p_NewLayout);
        bool cmd_transition(VkCommandBuffer p_Cmd, Image p_Image,
                            VkImageLayout p_CurrentLayout,
                            VkImageLayout p_NewLayout);

        bool cmd_copy2D(VkCommandBuffer p_Cmd, Image p_Source,
                        Image p_Destination,
                        VkExtent2D p_SourceExtent,
                        VkExtent2D p_DestinationExtent);

        bool cmd_clear_color(VkCommandBuffer p_Cmd, Image p_Image,
                             VkImageLayout p_ImageLayout,
                             Math::Color p_Color);

        bool create(Image p_Image, VkExtent3D p_Size,
                    VkFormat p_Format, VkImageUsageFlags p_Usage,
                    bool p_Mipmapped);

        bool destroy(Image p_Image);
      } // namespace ImageUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
