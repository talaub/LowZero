#pragma once

#include "vulkan/vulkan.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      struct Context;

      struct Swapchain
      {
        Context *context;

        VkSwapchainKHR vkhandle;
        VkFormat imageFormat;
        VkExtent2D extent;

        Util::List<VkImage> images;
        Util::List<VkImageView> imageViews;
      };

      struct Context
      {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice gpu;
        VkDevice device;
        VkSurfaceKHR surface;

        Swapchain swapchain;
      };

    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
