#include "LowRendererVulkanBase.h"

#include "LowRendererBackend.h"
#include "LowRendererCompatibility.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanBuffer.h"

#include "VkBootstrap.h"

#include "LowUtilAssert.h"
#include "LowUtil.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#define FRAME_STAGING_BUFFER_SIZE 30000u

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace Base {

        bool swapchain_cleanup(Swapchain &p_Swapchain);

        bool swapchain_create(Context &p_Context,
                              Swapchain &p_Swapchain,
                              Math::UVector2 p_Dimensions)
        {
          vkb::SwapchainBuilder l_SwapchainBuilder{
              Global::get_physical_device(), Global::get_device(),
              Global::get_surface()};

          p_Swapchain.imageFormat = Global::get_swapchain_format();
          
            VkSurfaceFormatKHR l_SurfaceFormat;
				  l_SurfaceFormat.format = p_Swapchain.imageFormat;
				  l_SurfaceFormat.colorSpace =
					  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

          vkb::Swapchain l_VkbSwapchain =
              l_SwapchainBuilder
                  //.use_default_format_selection()
                  .set_desired_format(l_SurfaceFormat)
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

            VkImageCreateInfo l_ImageInfo =
                InitUtil::image_create_info(
                    p_Swapchain.drawImage.format, l_DrawImageUsages,
                    p_Swapchain.drawImage.extent);

            VmaAllocationCreateInfo l_ImgAllocInfo = {};
            l_ImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            l_ImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vmaCreateImage(
                Global::get_allocator(), &l_ImageInfo,
                &l_ImgAllocInfo, &p_Swapchain.drawImage.image,
                &p_Swapchain.drawImage.allocation, nullptr);

            VkImageViewCreateInfo l_ImgViewInfo =
                InitUtil::imageview_create_info(
                    p_Swapchain.drawImage.format,
                    p_Swapchain.drawImage.image,
                    VK_IMAGE_ASPECT_COLOR_BIT);

            LOWR_VK_CHECK_RETURN(vkCreateImageView(
                Global::get_device(), &l_ImgViewInfo, nullptr,
                &p_Swapchain.drawImage.imageView));
          }

          return true;
        }

        bool swapchain_init(Context &p_Context,
                            Swapchain &p_Swapchain,
                            Math::UVector2 p_Dimensions)
        {
          bool l_Result = swapchain_create(
              p_Context, p_Context.swapchain, p_Dimensions);

          return l_Result;
        }

        bool swapchain_resize(Context &p_Context)
        {
          if (!p_Context.requireResize) {
            return true;
          }

          vkDeviceWaitIdle(Global::get_device());

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

        bool sync_structures_init(Context &p_Context)
        {
          VkFenceCreateInfo l_FenceCreateInfo =
              InitUtil::fence_create_info(
                  VK_FENCE_CREATE_SIGNALED_BIT);
          VkSemaphoreCreateInfo l_SemaphoreCreateInfo =
              InitUtil::semaphore_create_info();

          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
            LOWR_VK_CHECK_RETURN(vkCreateFence(
                Global::get_device(), &l_FenceCreateInfo, nullptr,
                &p_Context.frames[i].renderFence));

            LOWR_VK_CHECK_RETURN(vkCreateSemaphore(
                Global::get_device(), &l_SemaphoreCreateInfo, nullptr,
                &p_Context.frames[i].swapchainSemaphore));
            LOWR_VK_CHECK_RETURN(vkCreateSemaphore(
                Global::get_device(), &l_SemaphoreCreateInfo, nullptr,
                &p_Context.frames[i].renderSemaphore));
          }
        }

        // TODO: Most likely temporary
        // So what I mean by that is, that in theory this can very
        // well be used to create the global descriptor sets for
        // textures, materials, etc. BUT is is now mainly used for
        // hardocded things so maaaaybe we need to check that
        bool descriptors_init(Context &p_Context)
        {
          // Creates the descriptor layout for the compute draw
          // TODO: this part is definitely hardcoded for the current
          // use case
          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(0,
                                  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            p_Context.drawImageDescriptorLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_COMPUTE_BIT);
          }

          // Allocate new descriptor for the drawimage
          p_Context.drawImageDescriptors =
              Global::get_global_descriptor_allocator().allocate(
                  Global::get_device(),
                  p_Context.drawImageDescriptorLayout);

          // This part assignes the drawImage to the descriptor set
          {
            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_image(
                0, p_Context.swapchain.drawImage.imageView,
                VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            l_Writer.update_set(Global::get_device(),
                                p_Context.drawImageDescriptors);
          }

          return true;
        }

        static bool framedata_init(Context &p_Context)
        {
          p_Context.frames = (FrameData *)malloc(
              sizeof(FrameData) * Global::get_frame_overlap());

          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
          }

          return true;
        }

        bool context_init(Context &p_Context,
                          Math::UVector2 p_Dimensions)
        {
          p_Context.requireResize = false;

          LOWR_VK_ASSERT(swapchain_init(p_Context,
                                        p_Context.swapchain,
                                        p_Dimensions),
                         "Could not initialize swapchain");

          LOWR_VK_ASSERT(framedata_init(p_Context),
                         "Could not initialize frame data");

          LOWR_VK_ASSERT(sync_structures_init(p_Context),
                         "Could not initialize sync structures");

          // TODO: Most likely temporary
          // Please read the comment on the function itself
          LOWR_VK_ASSERT(descriptors_init(p_Context),
                         "Could not initialize descriptors");
        }

        bool context_initialize(Context &p_Context,
                                Math::UVector2 p_Dimensions)
        {
          LOWR_VK_ASSERT(context_init(p_Context, p_Dimensions),
                         "Failed to initialize context");

          return true;
        }

        bool initialize()
        {
          return Global::initialize();
        }

        bool swapchain_cleanup(Swapchain &p_Swapchain)
        {

          // Destroy drawimage
          ImageUtil::Internal::destroy(p_Swapchain.drawImage);

          vkDestroySwapchainKHR(Global::get_device(),
                                p_Swapchain.vkhandle, nullptr);

          // destroy swapchain resources
          for (int i = 0; i < p_Swapchain.imageViews.size(); i++) {

            vkDestroyImageView(Global::get_device(),
                               p_Swapchain.imageViews[i], nullptr);
          }

          return true;
        }

        bool framedata_cleanup(Context &p_Context)
        {
          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
          }

          return true;
        }

        bool sync_structures_cleanup(const Context &p_Context)
        {
          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
            vkDestroyFence(Global::get_device(),
                           p_Context.frames[i].renderFence, nullptr);
            vkDestroySemaphore(Global::get_device(),
                               p_Context.frames[i].renderSemaphore,
                               nullptr);
            vkDestroySemaphore(Global::get_device(),
                               p_Context.frames[i].swapchainSemaphore,
                               nullptr);
          }

          return true;
        }

        bool imgui_cleanup(const Context &p_Context)
        {

          return true;
        }

        bool descriptors_cleanup(Context &p_Context)
        {
          vkDestroyDescriptorSetLayout(
              Global::get_device(),
              p_Context.drawImageDescriptorLayout, nullptr);

          Global::get_global_descriptor_allocator().destroy_pools(
              Global::get_device());
          return true;
        }

        bool context_cleanup(Context &p_Context)
        {
          vkDeviceWaitIdle(Global::get_device());

          LOWR_VK_ASSERT(swapchain_cleanup(p_Context.swapchain),
                         "Failed to cleanup swapchain");

          LOWR_VK_ASSERT_RETURN(imgui_cleanup(p_Context),
                                "Failed to cleanup imgui");

          LOWR_VK_ASSERT_RETURN(descriptors_cleanup(p_Context),
                                "Failed to cleanup descriptors");

          LOWR_VK_ASSERT_RETURN(framedata_cleanup(p_Context),
                                "Failed to cleanup frame data");

          LOWR_VK_ASSERT_RETURN(sync_structures_cleanup(p_Context),
                                "Failed to cleanup sync structures");
        }

        bool cleanup()
        {
          return Global::cleanup();
        }

        bool imgui_draw(Context &p_Context,
                        VkImageView p_TargetImageView)
        {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VkRenderingAttachmentInfo l_ColorAttachment =
              InitUtil::attachment_info(
                  p_TargetImageView, nullptr,
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              p_Context.swapchain.extent, &l_ColorAttachment, 1,
              nullptr);

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
              Global::get_device(), 1,
              &p_Context.get_current_frame().renderFence, true,
              1000000000));
          LOWR_VK_CHECK_RETURN(vkResetFences(
              Global::get_device(), 1,
              &p_Context.get_current_frame().renderFence));

          // Acquire next swapchain image
          u32 l_SwapchainImageIndex;
          VkResult l_Result = vkAcquireNextImageKHR(
              Global::get_device(), p_Context.swapchain.vkhandle,
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
              Global::get_current_command_buffer();

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
              Global::get_current_command_buffer();

          ImageUtil::Internal::cmd_copy2D(
              l_Cmd, p_Context.swapchain.drawImage.image,
              p_Context.swapchain
                  .images[p_Context.swapchain.imageIndex],
              p_Context.swapchain.drawExtent,
              p_Context.swapchain.extent);

          ImageUtil::Internal::cmd_transition(
              l_Cmd,
              p_Context.swapchain
                  .images[p_Context.swapchain.imageIndex],
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          imgui_draw(p_Context,
                     p_Context.swapchain
                         .imageViews[p_Context.swapchain.imageIndex]);

          ImageUtil::Internal::cmd_transition(
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
              Global::get_graphics_queue(), 1, &l_SubmitInfo,
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
              Global::get_graphics_queue(), &l_PresentInfo);

          if (l_PresentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            LOW_LOG_DEBUG << "Require resize" << LOW_LOG_END;
            p_Context.requireResize = true;
          }
        }
      } // namespace Base
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
