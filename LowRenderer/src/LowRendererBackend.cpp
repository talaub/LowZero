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

      Framebuffer &swapchain_get_framebuffer(Swapchain &p_Swapchain,
                                             uint8_t p_Index)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_framebuffer(p_Swapchain, p_Index);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        Framebuffer l_Bfr;
        return l_Bfr;
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

      uint8_t swapchain_get_image_count(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_image_count(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        return 0;
#endif
      }

      uint8_t swapchain_get_current_frame_index(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_current_frame_index(p_Swapchain);
#else
        LOW_ASSERT(false, "No valid graphics api set");
        return 0;
#endif
      }

      uint8_t swapchain_get_current_image_index(Swapchain &p_Swapchain)
      {
#ifdef LOW_RENDERER_API_VULKAN
        return Vulkan::vk_swapchain_get_current_image_index(p_Swapchain);
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

      void pipeline_interface_create(PipelineInterface &p_PipelineInterface,
                                     PipelineInterfaceCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_pipeline_interface_create(p_PipelineInterface, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void pipeline_interface_cleanup(PipelineInterface &p_PipelineInterface)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_pipeline_interface_cleanup(p_PipelineInterface);
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

      void pipeline_bind(Pipeline &p_Pipeline, PipelineBindParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_pipeline_bind(p_Pipeline, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void image2d_create(Image2D &p_Image, Image2DCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_image2d_create(p_Image, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void image2d_cleanup(Image2D &p_Image)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_image2d_cleanup(p_Image);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void image2d_transition_state(Image2D &p_Image,
                                    Image2DTransitionStateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_image2d_transition_state(p_Image, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void draw(DrawParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_draw(p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void draw_indexed(DrawIndexedParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_draw_indexed(p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void draw_indexed_bindless(DrawIndexedBindlessParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_draw_indexed_bindless(p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_scope_interface_create(
          UniformScopeInterface &p_Interface,
          UniformScopeInterfaceCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_scope_interface_create(p_Interface, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_scope_interface_cleanup(UniformScopeInterface &p_Interface)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_scope_interface_cleanup(p_Interface);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_buffer_create(Uniform &p_Uniform,
                                 UniformBufferCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_buffer_create(p_Uniform, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_cleanup(Uniform &p_Uniform)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_cleanup(p_Uniform);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_buffer_set(Uniform &p_Uniform,
                              UniformBufferSetParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_buffer_set(p_Uniform, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_buffer_set(Uniform p_Uniform, void *p_Data)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_buffer_set(p_Uniform, p_Data);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_pool_create(UniformPool &p_Pool,
                               UniformPoolCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_pool_create(p_Pool, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_pool_cleanup(UniformPool &p_Pool)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_pool_cleanup(p_Pool);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_scope_create(UniformScope &p_Scope,
                                UniformScopeCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_scope_create(p_Scope, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void uniform_scopes_bind(UniformScopeBindParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_uniform_scopes_bind(p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void buffer_create(Buffer &p_Buffer, BufferCreateParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_buffer_create(p_Buffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void buffer_cleanup(Buffer &p_Buffer)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_buffer_cleanup(p_Buffer);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void buffer_bind_vertex(Buffer &p_Buffer,
                              BufferBindVertexParams &p_Params)
      {
        _LOW_ASSERT(p_Buffer.bufferUsageType ==
                    Backend::BufferUsageType::VERTEX);
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_buffer_bind_vertex(p_Buffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void buffer_bind_index(Buffer &p_Buffer, BufferBindIndexParams &p_Params)
      {
        _LOW_ASSERT(p_Buffer.bufferUsageType ==
                    Backend::BufferUsageType::INDEX);
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_buffer_bind_index(p_Buffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

      void buffer_write(Buffer &p_Buffer, BufferWriteParams &p_Params)
      {
#ifdef LOW_RENDERER_API_VULKAN
        Vulkan::vk_buffer_write(p_Buffer, p_Params);
#else
        LOW_ASSERT(false, "No valid graphics api set");
#endif
      }

    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
