#pragma once

#include "LowGfxContext.h"

namespace Low {
  namespace Gfx {
    namespace Detail {
      struct BackendBuffer
      {
        u64 size = 0;
        BufferUsage usage = BufferUsage::None;
        void *backend_state = nullptr;
      };

      struct BackendCommandList
      {
        QueueRole queue_role = QueueRole::Graphics;
        CommandListState state = CommandListState::Initial;
        void *backend_state = nullptr;
      };

      struct BackendSurface
      {
        void *backend_state = nullptr;
      };

      struct BackendAdapter
      {
        void *backend_state = nullptr;
        u32 generation = 1;
      };

      struct BackendSwapchain
      {
        void *backend_state = nullptr;
      };

      struct InstanceBackendApi
      {
        void *(*create_instance)(InstanceImpl &,
                                 const InstanceDesc &);
        void (*destroy_instance)(InstanceImpl &);

        void (*enumerate_adapters)(InstanceImpl &);
        BackendSurface (*create_surface)(InstanceImpl &,
                                         const SurfaceDesc &);
        void (*destroy_surface)(InstanceImpl &, BackendSurface &);

        Adapter (*select_adapter)(const InstanceImpl &,
                                  const AdapterSelectionDesc &);
      };

      struct ContextBackendApi
      {
        void *(*create_context)(ContextImpl &, InstanceImpl &,
                                Adapter, const ContextDesc &);
        void (*destroy_context)(ContextImpl &);

        DeviceCaps (*get_caps)(const ContextImpl &);

        BackendBuffer (*create_buffer)(ContextImpl &,
                                       const BufferDesc &);
        void (*destroy_buffer)(ContextImpl &, BackendBuffer &);

        BackendCommandList (*request_command_list)(
            ContextImpl &, const FrameContext &, QueueRole);
        BackendCommandList (*request_immediate_command_list)(
            ContextImpl &, QueueRole);
        void (*destroy_command_list)(ContextImpl &,
                                     BackendCommandList &);

        BackendSwapchain (*create_swapchain)(ContextImpl &,
                                             const SwapchainDesc &);
        void (*destroy_swapchain)(ContextImpl &, BackendSwapchain &);

        void (*begin_frame)(ContextImpl &p_Context,
                            const FrameContext &p_Frame);
        void (*acquire_swapchain)(ContextImpl &p_Context,
                                  const FrameContext &p_Frame,
                                  SwapchainFrame &p_SwapchainFrame);
        void (*present)(ContextImpl &p_Context,
                        const SwapchainFrame &p_SwapchainFrame);
        void (*end_frame)(ContextImpl &p_Context,
                          const FrameContext &p_Frame);
      };

      struct BackendProvider
      {
        Backend backend = Backend::Vulkan;
        const InstanceBackendApi *instance_api = nullptr;
        const ContextBackendApi *context_api = nullptr;
      };

      struct InstanceImpl
      {
        u32 instance_id = 0;
        const InstanceBackendApi *api = nullptr;
        const ContextBackendApi *context_api = nullptr;
        Backend backend = Backend::Vulkan;

        void *backend_state = nullptr;
        LogCallback log_callback = nullptr;
        void *log_user_data = nullptr;

        Pool<Surface, BackendSurface> surfaces;
        Util::List<BackendAdapter> adapters;
      };

      struct ContextImpl
      {
        u32 context_id = 0;
        u32 instance_id = 0;
        Backend backend = Backend::Vulkan;
        const ContextBackendApi *api = nullptr;
        InstanceImpl *instance = nullptr;
        Adapter adapter;
        DeviceCaps caps;
        LogCallback log_callback = nullptr;
        void *log_user_data = nullptr;

        Pool<Buffer, BackendBuffer> buffers;
        Pool<CommandList, BackendCommandList> command_lists;
        Pool<Swapchain, BackendSwapchain> swapchains;
        Util::List<CommandList> frame_command_lists;

        void *backend_state = nullptr;

        u32 frames_in_flight = 0;

        u64 frame_number = 0;
        u32 frame_index = 0;
        bool frame_active = false;
      };
    } // namespace Detail
  } // namespace Gfx
} // namespace Low
