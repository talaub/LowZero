#include "LowRendererVulkanBase.h"

#include "LowRendererBackend.h"
#include "LowRendererCompatibility.h"

#include "VkBootstrap.h"

#include "LowUtilAssert.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_vulkan.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {

#ifdef LOW_RENDERER_VALIDATION_ENABLED
      const bool g_ValidationEnabled = true;
#else
      const bool g_ValidationEnabled = false;
#endif

      Context g_Context;

      bool device_init(Context &p_Context)
      {
        // Begin initialize instance
        vkb::InstanceBuilder l_InstanceBuilder;

        vkb::Result<vkb::Instance> l_InstanceReturn =
            l_InstanceBuilder.set_app_name("LowEngine")
                .request_validation_layers(g_ValidationEnabled)
                .use_default_debug_messenger()
                .require_api_version(1, 3, 0)
                .build();

        if (!l_InstanceReturn) {
          LOW_LOG_ERROR
              << l_InstanceReturn.full_error().type.message().c_str()
              << LOW_LOG_END;
        }
        LOWR_VK_ASSERT(l_InstanceReturn,
                       "Failed to initialize vulkan instance");
        vkb::Instance l_VkbInstance = l_InstanceReturn.value();

        p_Context.instance = l_VkbInstance.instance;
        p_Context.debugMessenger = l_VkbInstance.debug_messenger;
        // End initialize instance

        // Begin initialize surface
        SDL_Vulkan_CreateSurface(Util::Window::get_main_window()
                                     .get_main_window()
                                     .sdlwindow,
                                 p_Context.instance,
                                 &p_Context.surface);
        // End initialize surface

        // Begin initialize gpu
        // vulkan 1.3 features
        VkPhysicalDeviceVulkan13Features l_Features{};
        l_Features.dynamicRendering = true;
        l_Features.synchronization2 = true;

        // vulkan 1.2 features
        VkPhysicalDeviceVulkan12Features l_Features12{};
        l_Features12.bufferDeviceAddress = true;
        l_Features12.descriptorIndexing = true;

        // use vkbootstrap to select a gpu.
        // We want a gpu that can write to the SDL surface and
        // supports vulkan 1.3 with the correct features
        vkb::PhysicalDeviceSelector l_GpuSelector{l_VkbInstance};
        vkb::PhysicalDevice l_VkbGpu =
            l_GpuSelector.set_minimum_version(1, 3)
                .set_required_features_13(l_Features)
                .set_required_features_12(l_Features12)
                .set_surface(p_Context.surface)
                .select()
                .value();

        p_Context.gpu = l_VkbGpu.physical_device;
        // End initialize gpu

        // Begin initialize device
        vkb::DeviceBuilder l_DeviceBuilder{l_VkbGpu};
        vkb::Device l_VkbDevice = l_DeviceBuilder.build().value();

        p_Context.device = l_VkbDevice.device;
        // End initialize device

        return true;
      }

      bool swapchain_create(Context &p_Context,
                            Swapchain &p_Swapchain,
                            Math::UVector2 p_Dimensions)
      {
        vkb::SwapchainBuilder l_SwapchainBuilder{
            p_Context.gpu, p_Context.device, p_Context.surface};

        p_Swapchain.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain l_VkbSwapchain =
            l_SwapchainBuilder
                //.use_default_format_selection()
                .set_desired_format(VkSurfaceFormatKHR{
                    .format = p_Swapchain.imageFormat,
                    .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                // use vsync present mode
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                .set_desired_extent(p_Dimensions.x, p_Dimensions.y)
                .add_image_usage_flags(
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build()
                .value();

        p_Swapchain.extent = l_VkbSwapchain.extent;
        // store swapchain and its related images
        p_Swapchain.vkhandle = l_VkbSwapchain.swapchain;

        p_Swapchain.images.clear();
        for (auto it : l_VkbSwapchain.get_images().value()) {
          p_Swapchain.images.push_back(it);
        }

        p_Swapchain.imageViews.clear();
        for (auto it : l_VkbSwapchain.get_image_views().value()) {
          p_Swapchain.imageViews.push_back(it);
        }

        p_Swapchain.context = &p_Context;

        return true;
      }

      bool swapchain_init(Context &p_Context,
                          Math::UVector2 p_Dimensions)
      {
        return swapchain_create(p_Context, p_Context.swapchain,
                                p_Dimensions);
      }

      bool initialize(Math::UVector2 p_Dimensions)
      {
        LOWR_VK_ASSERT(device_init(g_Context),
                       "Could not initialize device");

        LOWR_VK_ASSERT(swapchain_init(g_Context, p_Dimensions),
                       "Could not initialize swapchain");

        return true;
      }

      bool swapchain_cleanup(Swapchain &p_Swapchain)
      {
        vkDestroySwapchainKHR(p_Swapchain.context->device,
                              p_Swapchain.vkhandle, nullptr);

        // destroy swapchain resources
        for (int i = 0; i < p_Swapchain.imageViews.size(); i++) {

          vkDestroyImageView(p_Swapchain.context->device,
                             p_Swapchain.imageViews[i], nullptr);
        }

        return true;
      }

      bool context_cleanup(Context &p_Context)
      {
        vkDestroySurfaceKHR(p_Context.instance, p_Context.surface,
                            nullptr);
        vkDestroyDevice(p_Context.device, nullptr);

        vkb::destroy_debug_utils_messenger(p_Context.instance,
                                           p_Context.debugMessenger);
        vkDestroyInstance(p_Context.instance, nullptr);

        return true;
      }

      bool cleanup()
      {
        LOWR_VK_ASSERT(swapchain_cleanup(g_Context.swapchain),
                       "Failed to cleanup swapchain");

        LOWR_VK_ASSERT(context_cleanup(g_Context),
                       "Failed to cleanup context");

        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
