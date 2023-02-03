#include "LowRendererBackend.h"

#include "LowRendererGraphicsPipeline.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include <stdlib.h>
#include <string>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char *pName, int flags, unsigned debugFlags,
                     const char *file, int line)
{
  return malloc(size);
}

namespace Low {
  namespace Renderer {
    namespace Backend {
      void imageformat_get_depth(Context &p_Context, ImageFormat &p_Format)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_imageformat_get_depth(p_Context, p_Format);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void context_create(Context &p_Context, ContextCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_create(p_Context, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void context_cleanup(Context &p_Context)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_cleanup(p_Context);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
        p_Context.m_Window.cleanup();
      }

      void context_wait_idle(Context &p_Context)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_context_wait_idle(p_Context);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
        p_Context.m_Window.cleanup();
      }

      void framebuffer_create(Framebuffer &p_Framebuffer,
                              FramebufferCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_create(p_Framebuffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void framebuffer_get_dimensions(Framebuffer &p_Framebuffer,
                                      Math::UVector2 &p_Dimensions)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_get_dimensions(p_Framebuffer, p_Dimensions);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void framebuffer_cleanup(Framebuffer &p_Framebuffer)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_framebuffer_cleanup(p_Framebuffer);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandpool_create(CommandPool &p_CommandPool,
                              CommandPoolCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandpool_create(p_CommandPool, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandpool_cleanup(CommandPool &p_CommandPool)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandpool_cleanup(p_CommandPool);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void swapchain_create(Swapchain &p_Swapchain,
                            SwapchainCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_swapchain_create(p_Swapchain, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void swapchain_cleanup(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_swapchain_cleanup(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      uint8_t swapchain_prepare(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_prepare(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        return 0;
#endif
      }

      void swapchain_swap(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_swapchain_swap(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      CommandBuffer &swapchain_get_current_commandbuffer(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_current_commandbuffer(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        CommandBuffer l_CmpBfr;
        return l_CmpBfr;
#endif
      }

      CommandBuffer &swapchain_get_commandbuffer(Swapchain &p_Swapchain,
                                                 uint8_t p_Index)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_commandbuffer(p_Swapchain, p_Index);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        CommandBuffer l_CmpBfr;
        return l_CmpBfr;
#endif
      }

      Framebuffer &swapchain_get_current_framebuffer(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_current_framebuffer(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        Framebuffer l_Framebuffer;
        return l_Framebuffer;
#endif
      }

      uint8_t swapchain_get_frames_in_flight(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_frames_in_flight(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        return 0;
#endif
      }

      void renderpass_create(Renderpass &p_Renderpass,
                             RenderpassCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_renderpass_create(p_Renderpass, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void renderpass_cleanup(Renderpass &p_Renderpass)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_renderpass_cleanup(p_Renderpass);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void renderpass_start(Renderpass &p_Renderpass,
                            RenderpassStartParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_renderpass_start(p_Renderpass, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void renderpass_stop(Renderpass &p_Renderpass,
                           RenderpassStopParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_renderpass_stop(p_Renderpass, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandbuffer_start(CommandBuffer &p_CommandBuffer)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandbuffer_start(p_CommandBuffer);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void commandbuffer_stop(CommandBuffer &p_CommandBuffer)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_commandbuffer_stop(p_CommandBuffer);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void pipeline_graphics_create(Pipeline &p_Pipeline,
                                    GraphicsPipelineCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_pipeline_graphics_create(p_Pipeline, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void pipeline_cleanup(Pipeline &p_Pipeline)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_pipeline_cleanup(p_Pipeline);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
