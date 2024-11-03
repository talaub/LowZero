#include "LowRendererVulkanBase.h"

#include "LowRendererBackend.h"
#include "LowRendererCompatibility.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanPipeline.h"

#include "VkBootstrap.h"

#include "LowUtilAssert.h"
#include "LowUtil.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_vulkan.h"

#define VK_FRAME_OVERLAP 2

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace Base {
#ifdef LOW_RENDERER_VALIDATION_ENABLED
        const bool g_ValidationEnabled = true;
#else
        const bool g_ValidationEnabled = false;
#endif

        bool swapchain_cleanup(Swapchain &p_Swapchain);

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
            LOW_LOG_ERROR << l_InstanceReturn.full_error()
                                 .type.message()
                                 .c_str()
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
                      .colorSpace =
                          VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
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
                            Swapchain &p_Swapchain,
                            Math::UVector2 p_Dimensions)
        {
          bool l_Result = swapchain_create(
              p_Context, p_Context.swapchain, p_Dimensions);

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

            VkImageCreateInfo l_ImageInfo =
                InitUtil::image_create_info(
                    p_Swapchain.drawImage.format, l_DrawImageUsages,
                    p_Swapchain.drawImage.extent);

            VmaAllocationCreateInfo l_ImgAllocInfo = {};
            l_ImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            l_ImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vmaCreateImage(
                p_Context.allocator, &l_ImageInfo, &l_ImgAllocInfo,
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

          return l_Result;
        }

        bool swapchain_resize(Context &p_Context)
        {
          if (!p_Context.requireResize) {
            return true;
          }

          vkDeviceWaitIdle(p_Context.device);

          swapchain_cleanup(p_Context.swapchain);

          // Get size
          int l_Width, l_Height;
          SDL_GetWindowSize(Util::Window::get_main_window().sdlwindow,
                            &l_Width, &l_Height);

          // Create swapchain
          swapchain_create(p_Context, p_Context.swapchain,
                           Math::UVector2(l_Width, l_Height));

          p_Context.requireResize = false;

          return true;
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
              InitUtil::fence_create_info(
                  VK_FENCE_CREATE_SIGNALED_BIT);
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

        // TODO: Most likely temporary
        // So what I mean by that is, that in theory this can very
        // well be used to create the global descriptor sets for
        // textures, materials, etc. BUT is is now mainly used for
        // hardocded things so maaaaybe we need to check that
        bool descriptors_init(Context &p_Context)
        {
          Util::List<
              DescriptorUtil::DescriptorAllocator::PoolSizeRatio>
              l_Sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

          p_Context.globalDescriptorAllocator.init_pool(
              p_Context.device, 10, l_Sizes);

          // Creates the descriptor layout for the compute draw
          // TODO: this part is definitely hardcoded for the current
          // use case
          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(0,
                                  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            p_Context.drawImageDescriptorLayout = l_Builder.build(
                p_Context.device, VK_SHADER_STAGE_COMPUTE_BIT);
          }

          // Allocate new descriptor for the drawimage
          p_Context.drawImageDescriptors =
              p_Context.globalDescriptorAllocator.allocate(
                  p_Context.device,
                  p_Context.drawImageDescriptorLayout);

          // This part assignes the drawImage to the descriptor set
          {
            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_image(
                0, p_Context.swapchain.drawImage.imageView,
                VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            l_Writer.update_set(p_Context.device,
                                p_Context.drawImageDescriptors);
          }

          return true;
        }

        bool imgui_init(Context &p_Context)
        {
          VkDescriptorPoolSize l_PoolSizes[] = {
              {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
              {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
              {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
              {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

          VkDescriptorPoolCreateInfo l_PoolInfo = {};
          l_PoolInfo.sType =
              VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
          l_PoolInfo.flags =
              VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
          l_PoolInfo.maxSets = 1000;
          l_PoolInfo.poolSizeCount = (uint32_t)std::size(l_PoolSizes);
          l_PoolInfo.pPoolSizes = l_PoolSizes;

          LOWR_VK_CHECK_RETURN(
              vkCreateDescriptorPool(p_Context.device, &l_PoolInfo,
                                     nullptr, &p_Context.imguiPool));

          ImGui::CreateContext();

          ImGui_ImplSDL2_InitForVulkan(
              Util::Window::get_main_window().sdlwindow);

          // this initializes imgui for Vulkan
          ImGui_ImplVulkan_InitInfo l_InitInfo = {};
          l_InitInfo.Instance = p_Context.instance;
          l_InitInfo.PhysicalDevice = p_Context.gpu;
          l_InitInfo.Device = p_Context.device;
          l_InitInfo.Queue = p_Context.graphicsQueue;
          l_InitInfo.DescriptorPool = p_Context.imguiPool;
          l_InitInfo.MinImageCount = 3;
          l_InitInfo.ImageCount = 3;
          l_InitInfo.UseDynamicRendering = true;

          // dynamic rendering parameters for imgui to use
          l_InitInfo.PipelineRenderingCreateInfo = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
          l_InitInfo.PipelineRenderingCreateInfo
              .colorAttachmentCount = 1;
          l_InitInfo.PipelineRenderingCreateInfo
              .pColorAttachmentFormats =
              &p_Context.swapchain.imageFormat;

          l_InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

          ImGui_ImplVulkan_Init(&l_InitInfo);

          ImGui_ImplVulkan_CreateFontsTexture();

          return true;
        }

        bool context_init(Context &p_Context,
                          Math::UVector2 p_Dimensions)
        {
          p_Context.requireResize = false;

          LOWR_VK_ASSERT(device_init(p_Context),
                         "Could not initialize device");

          LOWR_VK_ASSERT(allocator_init(p_Context),
                         "Could not initialize allocator");

          LOWR_VK_ASSERT(swapchain_init(p_Context,
                                        p_Context.swapchain,
                                        p_Dimensions),
                         "Could not initialize swapchain");

          LOWR_VK_ASSERT(graphics_queue_init(p_Context),
                         "Could not initialize graphics queue");

          LOWR_VK_ASSERT(framedata_init(p_Context),
                         "Could not initialize frame data");

          LOWR_VK_ASSERT(sync_structures_init(p_Context),
                         "Could not initialize sync structures");

          // TODO: Most likely temporary
          // Please read the comment on the function itself
          LOWR_VK_ASSERT(descriptors_init(p_Context),
                         "Could not initialize descriptors");

          LOWR_VK_ASSERT(imgui_init(p_Context),
                         "Could not initialize imgui");
        }

        bool initialize(Context &p_Context,
                        Math::UVector2 p_Dimensions)
        {
          LOWR_VK_ASSERT(context_init(p_Context, p_Dimensions),
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

        bool framedata_cleanup(Context &p_Context)
        {
          for (u32 i = 0; i < p_Context.frameOverlap; ++i) {
            vkFreeCommandBuffers(
                p_Context.device, p_Context.frames[i].commandPool, 1,
                &p_Context.frames[i].mainCommandBuffer);

            vkDestroyCommandPool(p_Context.device,
                                 p_Context.frames[i].commandPool,
                                 nullptr);
          }

          return true;
        }

        bool sync_structures_cleanup(const Context &p_Context)
        {
          for (u32 i = 0; i < p_Context.frameOverlap; ++i) {
            vkDestroyFence(p_Context.device,
                           p_Context.frames[i].renderFence, nullptr);
            vkDestroySemaphore(p_Context.device,
                               p_Context.frames[i].renderSemaphore,
                               nullptr);
            vkDestroySemaphore(p_Context.device,
                               p_Context.frames[i].swapchainSemaphore,
                               nullptr);
          }

          return true;
        }

        bool imgui_cleanup(const Context &p_Context)
        {
          ImGui_ImplVulkan_Shutdown();
          vkDestroyDescriptorPool(p_Context.device,
                                  p_Context.imguiPool, nullptr);

          return true;
        }

        bool descriptors_cleanup(Context &p_Context)
        {
          vkDestroyDescriptorSetLayout(
              p_Context.device, p_Context.drawImageDescriptorLayout,
              nullptr);

          p_Context.globalDescriptorAllocator.destroy_pool(
              p_Context.device);
          return true;
        }

        bool context_cleanup(Context &p_Context)
        {
          vkDeviceWaitIdle(p_Context.device);

          LOWR_VK_ASSERT_RETURN(imgui_cleanup(p_Context),
                                "Failed to cleanup imgui");

          LOWR_VK_ASSERT_RETURN(descriptors_cleanup(p_Context),
                                "Failed to cleanup descriptors");

          LOWR_VK_ASSERT_RETURN(framedata_cleanup(p_Context),
                                "Failed to cleanup frame data");

          LOWR_VK_ASSERT_RETURN(sync_structures_cleanup(p_Context),
                                "Failed to cleanup sync structures");

          vkDestroySurfaceKHR(p_Context.instance, p_Context.surface,
                              nullptr);

          vmaDestroyAllocator(p_Context.allocator);

          vkDestroyDevice(p_Context.device, nullptr);

          vkb::destroy_debug_utils_messenger(
              p_Context.instance, p_Context.debugMessenger);
          vkDestroyInstance(p_Context.instance, nullptr);

          return true;
        }

        bool cleanup(Context &p_Context)
        {
          vkDeviceWaitIdle(p_Context.device);

          // Destroy drawimage
          ImageUtil::destroy(p_Context,
                             p_Context.swapchain.drawImage);

          LOWR_VK_ASSERT(swapchain_cleanup(p_Context.swapchain),
                         "Failed to cleanup swapchain");

          LOWR_VK_ASSERT(context_cleanup(p_Context),
                         "Failed to cleanup context");

          return true;
        }

        bool imgui_draw(Context &p_Context,
                        VkImageView p_TargetImageView)
        {
          VkCommandBuffer l_Cmd =
              p_Context.get_current_frame().mainCommandBuffer;

          VkRenderingAttachmentInfo l_ColorAttachment =
              InitUtil::attachment_info(
                  p_TargetImageView, nullptr,
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
          VkRenderingInfo l_RenderInfo =
              InitUtil::rendering_info(p_Context.swapchain.extent,
                                       &l_ColorAttachment, nullptr);

          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                          l_Cmd);

          vkCmdEndRendering(l_Cmd);

          return true;
        }

        bool context_prepare_draw(Context &p_Context)
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
          VkResult l_Result = vkAcquireNextImageKHR(
              p_Context.device, p_Context.swapchain.vkhandle,
              100000000,
              p_Context.get_current_frame().swapchainSemaphore,
              nullptr, &l_SwapchainImageIndex);
          p_Context.swapchain.imageIndex = l_SwapchainImageIndex;

          if (l_Result == VK_ERROR_OUT_OF_DATE_KHR) {
            p_Context.requireResize = true;
            LOW_LOG_DEBUG << "Require resize" << LOW_LOG_END;
            return false;
          }

          VkCommandBuffer l_Cmd =
              p_Context.get_current_frame().mainCommandBuffer;

          VkExtent2D l_DrawExtent;
          l_DrawExtent.width =
              p_Context.swapchain.drawImage.extent.width;
          l_DrawExtent.height =
              p_Context.swapchain.drawImage.extent.height;

          p_Context.swapchain.drawExtent = l_DrawExtent;

          // Because we waited for the fence we know that the command
          // buffer is finished executing so we can reset it
          LOWR_VK_CHECK_RETURN(vkResetCommandBuffer(l_Cmd, 0));

          VkCommandBufferBeginInfo l_CmdBeginInfo =
              InitUtil::command_buffer_begin_info(
                  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

          LOWR_VK_CHECK_RETURN(
              vkBeginCommandBuffer(l_Cmd, &l_CmdBeginInfo));

          return true;
        }

        bool context_present(Context &p_Context)
        {
          VkCommandBuffer l_Cmd =
              p_Context.get_current_frame().mainCommandBuffer;

          ImageUtil::cmd_copy2D(
              l_Cmd, p_Context.swapchain.drawImage.image,
              p_Context.swapchain
                  .images[p_Context.swapchain.imageIndex],
              p_Context.swapchain.drawExtent,
              p_Context.swapchain.extent);

          ImageUtil::cmd_transition(
              l_Cmd,
              p_Context.swapchain
                  .images[p_Context.swapchain.imageIndex],
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          imgui_draw(p_Context,
                     p_Context.swapchain
                         .imageViews[p_Context.swapchain.imageIndex]);

          ImageUtil::cmd_transition(
              l_Cmd,
              p_Context.swapchain
                  .images[p_Context.swapchain.imageIndex],
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

          l_PresentInfo.pImageIndices =
              &p_Context.swapchain.imageIndex;

          VkResult l_PresentResult = vkQueuePresentKHR(
              p_Context.graphicsQueue, &l_PresentInfo);

          if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            LOW_LOG_DEBUG << "Require resize" << LOW_LOG_END;
            p_Context.requireResize = true;
          }

          // increase the number of frames drawn
          p_Context.frameNumber++;
        }
      } // namespace Base
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
