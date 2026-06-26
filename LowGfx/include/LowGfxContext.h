#pragma once

#include "LowGfxBuffer.h"
#include "LowGfxLog.h"
#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowGfxCommandList.h"
#include "LowGfxSwapchain.h"
#include "LowGfxSurface.h"
#include "LowGfxAdapter.h"

namespace Low {
  namespace Gfx {
    enum class Backend : u8
    {
      Vulkan
    };

    enum class WindowBackend : u8
    {
      None,
      SDL
    };

    enum class PresentMode : u8
    {
      Fifo,
      Mailbox,
      Immediate
    };

    namespace Detail {
      struct InstanceImpl;
      struct ContextImpl;
      struct ContextBackendApi;
      struct FrameContextAccess;
      struct SwapchainFrameAccess;
    } // namespace Detail

    class FrameContext
    {
    public:
      u64 get_frame_number() const { return m_FrameNumber; }
      u32 get_frame_index() const { return m_FrameIndex; }

    private:
      friend class Context;
      friend struct Detail::FrameContextAccess;

      u64 m_FrameNumber = 0;
      u32 m_FrameIndex = 0;
    };

    class SwapchainFrame
    {
    public:
      u64 get_frame_number() const { return m_FrameNumber; }
      u32 get_frame_index() const { return m_FrameIndex; }
      Swapchain get_swapchain() const { return m_Swapchain; }
      u32 get_swapchain_image_index() const
      {
        return m_SwapchainImageIndex;
      }

    private:
      friend class Context;
      friend struct Detail::SwapchainFrameAccess;

      u64 m_FrameNumber = 0;
      u32 m_FrameIndex = 0;
      Swapchain m_Swapchain;
      u32 m_SwapchainImageIndex = LOW_UINT32_MAX;
    };

    namespace Detail {
      struct FrameContextAccess
      {
        static u64 frame_number(const FrameContext &p_Frame)
        {
          return p_Frame.m_FrameNumber;
        }

        static u32 frame_index(const FrameContext &p_Frame)
        {
          return p_Frame.m_FrameIndex;
        }

      };

      struct SwapchainFrameAccess
      {
        static u64 frame_number(const SwapchainFrame &p_Frame)
        {
          return p_Frame.m_FrameNumber;
        }

        static u32 frame_index(const SwapchainFrame &p_Frame)
        {
          return p_Frame.m_FrameIndex;
        }

        static Swapchain swapchain(const SwapchainFrame &p_Frame)
        {
          return p_Frame.m_Swapchain;
        }

        static u32 swapchain_image_index(
            const SwapchainFrame &p_Frame)
        {
          return p_Frame.m_SwapchainImageIndex;
        }

        static void set_swapchain_image_index(
            SwapchainFrame &p_Frame, u32 p_ImageIndex)
        {
          p_Frame.m_SwapchainImageIndex = p_ImageIndex;
        }
      };
    } // namespace Detail

    struct WindowDesc
    {
      WindowBackend backend = WindowBackend::None;
      void *handle = nullptr;
    };

    struct SwapchainDesc
    {
      Surface surface;
      u32 width = 0;
      u32 height = 0;
      const char *debug_name = nullptr;
      PresentMode present_mode = PresentMode::Fifo;
    };

    struct SurfaceDesc
    {
      WindowDesc window;
    };

    struct InstanceDesc
    {
      Backend backend = Backend::Vulkan;
      WindowDesc surface_window;
      bool enable_validation = true;
      LogCallback log_callback = nullptr;
      void *log_user_data = nullptr;
    };

    struct ContextDesc
    {
      u32 frames_in_flight = 2;
    };

    struct AdapterSelectionDesc
    {
      Surface compatible_surface;
      PowerPreference power_preference = PowerPreference::Default;
    };

    struct DeviceCaps
    {
      bool compute = false;
      bool storage_buffers = false;
      bool storage_images = false;
      bool bindless_sampled_textures = false;
      bool sampled_texture_arrays = false;
      bool indirect_draw = false;
      bool multi_draw_indirect = false;
      bool timeline_sync = false;
      u32 max_bind_groups = 0;
      u32 max_sampled_textures_per_bind_group = 0;
      u32 max_storage_buffers_per_bind_group = 0;
      u32 max_storage_buffer_range = 0;
      u32 max_inline_uniform_bytes = 0;
    };

    class Instance
    {
    public:
      explicit Instance(const InstanceDesc &p_Desc);
      ~Instance();

      Instance(Instance &&) noexcept;
      Instance &operator=(Instance &&) noexcept;

      Instance(const Instance &) = delete;
      Instance &operator=(const Instance &) = delete;

      Backend get_backend() const;

      Surface create_surface(const SurfaceDesc &p_Desc);
      void destroy(Surface p_Surface);
      bool is_valid(Surface p_Surface) const;

      Adapter select_adapter(const AdapterSelectionDesc &p_Desc);
      bool is_valid(Adapter p_Adapter) const;

    private:
      friend class Context;

      Util::UniquePtr<Detail::InstanceImpl> m_Impl;
    };

    class Context
    {
    public:
      Context(Instance &p_Instance, Adapter p_Adapter,
              const ContextDesc &p_Desc);
      ~Context();

      Context(Context &&) noexcept;
      Context &operator=(Context &&) noexcept;

      Context(const Context &) = delete;
      Context &operator=(const Context &) = delete;

      Backend get_backend() const;
      const DeviceCaps &get_caps() const;

      Buffer create_buffer(const BufferDesc &p_Desc);
      void destroy(Buffer p_Buffer);
      bool is_valid(Buffer p_Buffer) const;

      CommandList request_command_list(const FrameContext &p_Frame,
                                       QueueRole p_QueueRole);
      CommandList
      request_immediate_command_list(QueueRole p_QueueRole);
      void destroy(CommandList p_CommandList);
      bool is_valid(CommandList p_CommandList) const;

      Swapchain create_swapchain(const SwapchainDesc &p_Desc);

      void destroy(Swapchain p_Swapchain);
      bool is_valid(Swapchain p_Swapchain) const;

      FrameContext begin_frame();
      SwapchainFrame acquire_swapchain(const FrameContext &p_Frame,
                                       Swapchain p_Swapchain);
      void present(const SwapchainFrame &p_SwapchainFrame);
      void end_frame(const FrameContext &p_Frame);

    private:
      Util::UniquePtr<Detail::ContextImpl> m_Impl;
    };
  } // namespace Gfx
} // namespace Low
