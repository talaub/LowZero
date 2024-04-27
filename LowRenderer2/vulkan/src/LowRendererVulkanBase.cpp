#include "LowRendererVulkanBase.h"

#include "LowRendererBackend.h"
#include "LowRendererCompatibility.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanImage.h"

#include "VkBootstrap.h"

#include "LowUtilAssert.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_vulkan.h"

#define VK_FRAME_OVERLAP 2

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

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
        p_Context.vkbDevice = l_DeviceBuilder.build().value();

        p_Context.device = p_Context.vkbDevice.device;
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

        {
          VkExtent3D l_DrawImageExtent = {p_Dimensions.x,
                                          p_Dimensions.y, 1};

          p_Swapchain.drawImage.format =
              VK_FORMAT_R16G16B16A16_SFLOAT;
          p_Swapchain.drawImage.extent = l_DrawImageExtent;

          VkImageUsageFlags l_DrawImageUsages{};
          l_DrawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          l_DrawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
          l_DrawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
          l_DrawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

          VkImageCreateInfo l_ImageInfo = InitUtil::image_create_info(
              p_Swapchain.drawImage.format, l_DrawImageUsages,
              p_Swapchain.drawImage.extent);

          VmaAllocationCreateInfo l_ImgAllocInfo = {};
          l_ImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
          l_ImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

          vmaCreateImage(p_Context.allocator, &l_ImageInfo,
                         &l_ImgAllocInfo,
                         &p_Swapchain.drawImage.image,
                         &p_Swapchain.drawImage.allocation, nullptr);

          VkImageViewCreateInfo l_ImgViewInfo =
              InitUtil::imageview_create_info(
                  p_Swapchain.drawImage.format,
                  p_Swapchain.drawImage.image,
                  VK_IMAGE_ASPECT_COLOR_BIT);

          LOWR_VK_CHECK_RETURN(vkCreateImageView(
              p_Context.device, &l_ImgViewInfo, nullptr,
              &p_Swapchain.drawImage.imageView));
        }

        return true;
      }

      bool swapchain_init(Context &p_Context,
                          Math::UVector2 p_Dimensions)
      {
        return swapchain_create(p_Context, p_Context.swapchain,
                                p_Dimensions);
      }

      bool graphics_queue_init(Context &p_Context)
      {
        p_Context.graphicsQueue =
            p_Context.vkbDevice.get_queue(vkb::QueueType::graphics)
                .value();
        p_Context.graphicsQueueFamily =
            p_Context.vkbDevice
                .get_queue_index(vkb::QueueType::graphics)
                .value();

        return true;
      }

      bool framedata_init(Context &p_Context)
      {
        p_Context.frameOverlap = VK_FRAME_OVERLAP;
        p_Context.frameNumber = 0;

        VkCommandPoolCreateInfo l_CommandPoolInfo =
            InitUtil::command_pool_create_info(
                p_Context.graphicsQueueFamily,
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        p_Context.frames = (FrameData *)malloc(
            sizeof(FrameData) * p_Context.frameOverlap);

        for (u32 i = 0; i < p_Context.frameOverlap; ++i) {
          LOWR_VK_CHECK_RETURN(vkCreateCommandPool(
              p_Context.device, &l_CommandPoolInfo, nullptr,
              &p_Context.frames[i].commandPool));

          VkCommandBufferAllocateInfo l_CmdAllocInfo =
              InitUtil::command_buffer_allocate_info(
                  p_Context.frames[i].commandPool);

          LOWR_VK_CHECK_RETURN(vkAllocateCommandBuffers(
              p_Context.device, &l_CmdAllocInfo,
              &p_Context.frames[i].mainCommandBuffer));
        }
      }

      bool sync_structures_init(Context &p_Context)
      {
        VkFenceCreateInfo l_FenceCreateInfo =
            InitUtil::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo l_SemaphoreCreateInfo =
            InitUtil::semaphore_create_info();

        for (u32 i = 0; i < p_Context.frameOverlap; ++i) {
          LOWR_VK_CHECK_RETURN(vkCreateFence(
              p_Context.device, &l_FenceCreateInfo, nullptr,
              &p_Context.frames[i].renderFence));

          LOWR_VK_CHECK_RETURN(vkCreateSemaphore(
              p_Context.device, &l_SemaphoreCreateInfo, nullptr,
              &p_Context.frames[i].swapchainSemaphore));
          LOWR_VK_CHECK_RETURN(vkCreateSemaphore(
              p_Context.device, &l_SemaphoreCreateInfo, nullptr,
              &p_Context.frames[i].renderSemaphore));
        }
      }

      bool allocator_init(Context &p_Context)
      {
        VmaAllocatorCreateInfo l_AllocatorInfo = {};
        l_AllocatorInfo.physicalDevice = p_Context.gpu;
        l_AllocatorInfo.device = p_Context.device;
        l_AllocatorInfo.instance = p_Context.instance;
        l_AllocatorInfo.flags =
            VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&l_AllocatorInfo, &p_Context.allocator);

        return true;
      }

      bool context_init(Context &p_Context,
                        Math::UVector2 p_Dimensions)
      {
        LOWR_VK_ASSERT(device_init(p_Context),
                       "Could not initialize device");

        LOWR_VK_ASSERT(allocator_init(p_Context),
                       "Could not initialize allocator");

        LOWR_VK_ASSERT(swapchain_init(p_Context, p_Dimensions),
                       "Could not initialize swapchain");

        LOWR_VK_ASSERT(graphics_queue_init(p_Context),
                       "Could not initialize graphics queue");

        LOWR_VK_ASSERT(framedata_init(p_Context),
                       "Could not initialize frame data");

        LOWR_VK_ASSERT(sync_structures_init(p_Context),
                       "Could not initialize sync structures");
      }

      bool initialize(Math::UVector2 p_Dimensions)
      {
        LOWR_VK_ASSERT(context_init(g_Context, p_Dimensions),
                       "Failed to initialize context");

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

      bool context_draw(Context &p_Context)
      {
        // Wait for fence and reset right after
        LOWR_VK_CHECK_RETURN(vkWaitForFences(
            p_Context.device, 1,
            &p_Context.get_current_frame().renderFence, true,
            1000000000));
        LOWR_VK_CHECK_RETURN(vkResetFences(
            p_Context.device, 1,
            &p_Context.get_current_frame().renderFence));

        // Acquire next swapchain image
        u32 l_SwapchainImageIndex;
        LOWR_VK_CHECK_RETURN(vkAcquireNextImageKHR(
            p_Context.device, p_Context.swapchain.vkhandle, 100000000,
            p_Context.get_current_frame().swapchainSemaphore, nullptr,
            &l_SwapchainImageIndex));

        VkCommandBuffer l_Cmd =
            p_Context.get_current_frame().mainCommandBuffer;

        VkExtent2D l_DrawExtent;
        l_DrawExtent.width =
            p_Context.swapchain.drawImage.extent.width;
        l_DrawExtent.height =
            p_Context.swapchain.drawImage.extent.height;

        // Because we waited for the fence we know that the command
        // buffer is finished executing so we can reset it
        LOWR_VK_CHECK_RETURN(vkResetCommandBuffer(l_Cmd, 0));

        VkCommandBufferBeginInfo l_CmdBeginInfo =
            InitUtil::command_buffer_begin_info(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        LOWR_VK_CHECK_RETURN(
            vkBeginCommandBuffer(l_Cmd, &l_CmdBeginInfo));

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        float l_Flash = abs(sin(p_Context.frameNumber / 120.f));
        Math::Color l_ClearColor = {0.0f, 0.0f, l_Flash, 1.0f};

        ImageUtil::cmd_clear_color(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_GENERAL, l_ClearColor);

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.images[l_SwapchainImageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        ImageUtil::cmd_copy2D(
            l_Cmd, p_Context.swapchain.drawImage.image,
            p_Context.swapchain.images[l_SwapchainImageIndex],
            l_DrawExtent, p_Context.swapchain.extent);

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.images[l_SwapchainImageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        LOWR_VK_CHECK_RETURN(vkEndCommandBuffer(l_Cmd));

        VkCommandBufferSubmitInfo l_CmdInfo =
            InitUtil::command_buffer_submit_info(l_Cmd);

        VkSemaphoreSubmitInfo l_WaitInfo =
            InitUtil::semaphore_submit_info(
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                p_Context.get_current_frame().swapchainSemaphore);
        VkSemaphoreSubmitInfo l_SignalInfo =
            InitUtil::semaphore_submit_info(
                VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                p_Context.get_current_frame().renderSemaphore);

        VkSubmitInfo2 l_SubmitInfo = InitUtil::submit_info(
            &l_CmdInfo, &l_SignalInfo, &l_WaitInfo);

        LOWR_VK_CHECK_RETURN(vkQueueSubmit2(
            p_Context.graphicsQueue, 1, &l_SubmitInfo,
            p_Context.get_current_frame().renderFence));

        VkPresentInfoKHR l_PresentInfo = {};
        l_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        l_PresentInfo.pNext = nullptr;
        l_PresentInfo.pSwapchains = &p_Context.swapchain.vkhandle;
        l_PresentInfo.swapchainCount = 1;

        l_PresentInfo.pWaitSemaphores =
            &p_Context.get_current_frame().renderSemaphore;
        l_PresentInfo.waitSemaphoreCount = 1;

        l_PresentInfo.pImageIndices = &l_SwapchainImageIndex;

        LOWR_VK_CHECK_RETURN(vkQueuePresentKHR(
            p_Context.graphicsQueue, &l_PresentInfo));

        // increase the number of frames drawn
        p_Context.frameNumber++;

        return true;
      }

      bool draw()
      {
        LOWR_VK_ASSERT(context_draw(g_Context), "Failed to draw");
        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
