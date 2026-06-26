#pragma once

#include "LowGfxBackend.h"
#include "LowGfxContext.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      const Detail::BackendProvider &get_backend_provider();

      void *create_instance(Detail::InstanceImpl &p_Instance,
                            const InstanceDesc &p_Desc);
      void destroy_instance(Detail::InstanceImpl &p_Instance);
      void enumerate_adapters(Detail::InstanceImpl &p_Instance);
      Detail::BackendSurface
      create_surface(Detail::InstanceImpl &p_Instance,
                     const SurfaceDesc &p_Desc);
      void destroy_surface(Detail::InstanceImpl &p_Instance,
                           Detail::BackendSurface &p_Surface);
      Adapter select_adapter(const Detail::InstanceImpl &p_Instance,
                             const AdapterSelectionDesc &p_Desc);

      void *create_context(Detail::ContextImpl &p_Context,
                           Detail::InstanceImpl &p_Instance,
                           Adapter p_Adapter,
                           const ContextDesc &p_Desc);
      void destroy_context(Detail::ContextImpl &p_Context);
      DeviceCaps get_caps(const Detail::ContextImpl &p_Context);

      Detail::BackendBuffer
      create_buffer(Detail::ContextImpl &p_Context,
                    const BufferDesc &p_Desc);
      void destroy_buffer(Detail::ContextImpl &p_Context,
                          Detail::BackendBuffer &p_Buffer);

      Detail::BackendCommandList
      request_command_list(Detail::ContextImpl &p_Context,
                           const FrameContext &p_Frame,
                           QueueRole p_QueueRole);
      Detail::BackendCommandList
      request_immediate_command_list(Detail::ContextImpl &p_Context,
                           QueueRole p_QueueRole);
      void
      destroy_command_list(Detail::ContextImpl &p_Context,
                           Detail::BackendCommandList &p_CommandList);

      Detail::BackendSwapchain
      create_swapchain(Detail::ContextImpl &p_Context,
                       const SwapchainDesc &p_Desc);
      void destroy_swapchain(Detail::ContextImpl &p_Context,
                             Detail::BackendSwapchain &p_Swapchain);

      void begin_frame(Detail::ContextImpl &p_Context,
                       const FrameContext &p_Frame);
      void acquire_swapchain(Detail::ContextImpl &p_Context,
                             const FrameContext &p_Frame,
                             SwapchainFrame &p_SwapchainFrame);
      void present(Detail::ContextImpl &p_Context,
                   const SwapchainFrame &p_SwapchainFrame);
      void end_frame(Detail::ContextImpl &p_Context,
                     const FrameContext &p_Frame);

    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
