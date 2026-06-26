#include "LowGfxBackend.h"
#include "LowGfxContext.h"
#include "LowGfxLogInternal.h"

#include "LowUtilAssert.h"

#include "LowGfxVulkanState.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      static VkPresentModeKHR
      to_vulkan_present_mode(const PresentMode p_Mode)
      {
        switch (p_Mode) {
        case PresentMode::Fifo:
          return VK_PRESENT_MODE_FIFO_KHR;
        case PresentMode::Immediate:
          return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case PresentMode::Mailbox:
          return VK_PRESENT_MODE_MAILBOX_KHR;
        }

        LOW_ASSERT(false, "Unsupported present mode for swapchain.");

        return VK_PRESENT_MODE_FIFO_KHR;
      }

      Detail::BackendSwapchain
      create_swapchain(Detail::ContextImpl &p_Context,
                       const SwapchainDesc &p_Desc)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);

        LOW_ASSERT(
            l_State,
            "Cannot create Vulkan swapchain without context state");

        LOW_ASSERT(p_Context.instance,
                   "Cannot create Vulkan swapchain without instance");

        const Detail::BackendSurface *l_BackendSurface =
            p_Context.instance->surfaces.get(p_Desc.surface);
        LOW_ASSERT(l_BackendSurface,
                   "Cannot create Vulkan swapchain from invalid surface");
        VulkanSurfaceState *l_Surface =
            static_cast<VulkanSurfaceState *>(
                l_BackendSurface->backend_state);
        LOW_ASSERT(l_Surface && l_Surface->surface != VK_NULL_HANDLE,
                   "Cannot create Vulkan swapchain from empty surface");

        VulkanSwapchainState *l_Swapchain = new VulkanSwapchainState;
        l_Swapchain->surface = l_Surface->surface;

        vkb::SwapchainBuilder l_SwapchainBuilder{
            l_State->gpu, l_State->device, l_Swapchain->surface};

        VkSurfaceFormatKHR l_SurfaceFormat{};
        l_SurfaceFormat.format =
            VK_FORMAT_B8G8R8A8_UNORM; // TODO: Make desired format
                                      // requestable
        l_SurfaceFormat.colorSpace =
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        vkb::Result<vkb::Swapchain> l_SwapchainResult =
            l_SwapchainBuilder
                //.use_default_format_selection()
                .set_desired_format(l_SurfaceFormat)
                // use vsync present mode
                .set_desired_present_mode(
                    to_vulkan_present_mode(p_Desc.present_mode))
                .set_desired_extent(p_Desc.width, p_Desc.height)
                .add_image_usage_flags(
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build();

        LOW_ASSERT(l_SwapchainResult,
                   "Failed to create vulkan swapchain.");

        vkb::Swapchain l_VkbSwapchain = l_SwapchainResult.value();

        l_Swapchain->extent = l_VkbSwapchain.extent;
        l_Swapchain->format = l_VkbSwapchain.image_format;
        l_Swapchain->acquired_image_index = LOW_UINT32_MAX;
        l_Swapchain->acquired = false;
        l_Swapchain->resize_requested = false;
        l_Swapchain->swapchain = l_VkbSwapchain.swapchain;

        Util::List<VkImage> l_Images;
        {
          auto l_ImagesResult = l_VkbSwapchain.get_images();
          LOW_ASSERT(l_ImagesResult,
                     "Failed to fetch images for vulkan swapchain.");
          for (auto it : l_ImagesResult.value()) {
            l_Images.push_back(it);
          }
        }

        Util::List<VkImageView> l_ImageViews;
        {
          auto l_ImageViewsResult = l_VkbSwapchain.get_image_views();
          LOW_ASSERT(
              l_ImageViewsResult,
              "Failed to fetch image views for vulkan swapchain.");
          for (auto it : l_ImageViewsResult.value()) {
            l_ImageViews.push_back(it);
          }
        }

        LOW_ASSERT(l_Images.size() == l_ImageViews.size(),
                   "Vulkan swapchain returned mismatched image state");
        for (u32 i = 0; i < l_Images.size(); ++i) {
          VulkanSwapchainImageState l_ImageState;
          l_ImageState.image = l_Images[i];
          l_ImageState.image_view = l_ImageViews[i];

          VkSemaphoreCreateInfo l_SemaphoreInfo{};
          l_SemaphoreInfo.sType =
              VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
          VkResult l_SemaphoreResult = vkCreateSemaphore(
              l_State->device, &l_SemaphoreInfo, nullptr,
              &l_ImageState.render_finished);
          LOW_ASSERT(l_SemaphoreResult == VK_SUCCESS,
                     "Failed to create Vulkan swapchain image "
                     "render-finished semaphore");

          l_Swapchain->images.push_back(l_ImageState);
        }

        for (u32 i = 0; i < p_Context.frames_in_flight; ++i) {
          VkSemaphoreCreateInfo l_SemaphoreInfo{};
          l_SemaphoreInfo.sType =
              VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
          VkSemaphore l_ImageAvailable = VK_NULL_HANDLE;
          VkResult l_SemaphoreResult = vkCreateSemaphore(
              l_State->device, &l_SemaphoreInfo, nullptr,
              &l_ImageAvailable);
          LOW_ASSERT(l_SemaphoreResult == VK_SUCCESS,
                     "Failed to create Vulkan swapchain "
                     "image-available semaphore");

          l_Swapchain->image_available_semaphores.push_back(
              l_ImageAvailable);
        }

        Detail::BackendSwapchain l_Result;
        l_Result.backend_state = l_Swapchain;

        return l_Result;
      }

      void destroy_swapchain(Detail::ContextImpl &p_Context,
                             Detail::BackendSwapchain &p_Swapchain)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);

        LOW_ASSERT(
            l_State,
            "Cannot destroy Vulkan swapchain without context state");

        VulkanSwapchainState *l_Swapchain =
            static_cast<VulkanSwapchainState *>(
                p_Swapchain.backend_state);

        LOW_ASSERT(l_Swapchain, "Cannot destroy vulkan swapchain "
                                "without valid swapchain state.");

        if (l_State->device != VK_NULL_HANDLE) {
          vkDeviceWaitIdle(l_State->device);
        }

        for (VulkanSwapchainImageState &i_Image :
             l_Swapchain->images) {
          if (i_Image.render_finished != VK_NULL_HANDLE) {
            vkDestroySemaphore(l_State->device,
                               i_Image.render_finished, nullptr);
          }
          if (i_Image.image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(l_State->device, i_Image.image_view,
                               nullptr);
          }
        }

        for (VkSemaphore i_Semaphore :
             l_Swapchain->image_available_semaphores) {
          if (i_Semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(l_State->device, i_Semaphore, nullptr);
          }
        }

        vkDestroySwapchainKHR(l_State->device, l_Swapchain->swapchain,
                              nullptr);

        delete l_Swapchain;
        p_Swapchain.backend_state = nullptr;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
