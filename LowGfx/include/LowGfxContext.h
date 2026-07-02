#pragma once

#include "LowGfxBuffer.h"
#include "LowGfxImage.h"
#include "LowGfxLog.h"
#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowGfxCommandList.h"
#include "LowGfxPipeline.h"
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
      u64 get_frame_number() const
      {
        return m_FrameNumber;
      }
      u32 get_frame_index() const
      {
        return m_FrameIndex;
      }

    private:
      friend class Context;
      friend struct Detail::FrameContextAccess;

      u64 m_FrameNumber = 0;
      u32 m_FrameIndex = 0;
    };

    class SwapchainFrame
    {
    public:
      u64 get_frame_number() const
      {
        return m_FrameNumber;
      }
      u32 get_frame_index() const
      {
        return m_FrameIndex;
      }
      Swapchain get_swapchain() const
      {
        return m_Swapchain;
      }
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

        static u32
        swapchain_image_index(const SwapchainFrame &p_Frame)
        {
          return p_Frame.m_SwapchainImageIndex;
        }

        static void set_swapchain_image_index(SwapchainFrame &p_Frame,
                                              u32 p_ImageIndex)
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

    struct RenderingInfo
    {
      Math::UVector2 extent;
      Util::Span<const ColorAttachmentDesc> color_attachments;
      const DepthAttachmentDesc *depth_attachment = nullptr;
    };

    struct Viewport
    {
      float x = 0.0f;
      float y = 0.0f;
      float width = 0.0f;
      float height = 0.0f;
      float min_depth = 0.0f;
      float max_depth = 1.0f;
    };

    struct Rect2D
    {
      Math::IVector2 offset{0, 0};
      Math::UVector2 extent{0, 0};
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
      void wait_idle();

      Buffer create_buffer(const BufferDesc &p_Desc);
      void destroy(Buffer p_Buffer);
      bool is_valid(Buffer p_Buffer) const;
      void *map_buffer(Buffer p_Buffer);
      void unmap_buffer(Buffer p_Buffer);
      void flush_buffer(Buffer p_Buffer, u64 p_Offset, u64 p_Size);
      void invalidate_buffer(Buffer p_Buffer, u64 p_Offset,
                             u64 p_Size);
      void write_buffer(Buffer p_Buffer, u64 p_Offset,
                        const void *p_Data, u64 p_Size);
      void read_buffer(Buffer p_Buffer, u64 p_Offset, void *p_Data,
                       u64 p_Size);

      Image create_image(const ImageDesc &p_Desc);
      void destroy(Image p_Image);
      bool is_valid(Image p_Image) const;
      ImageState get_image_state(Image p_Image) const;

      ImageView create_image_view(const ImageViewDesc &p_Desc);
      void destroy(ImageView p_ImageView);
      bool is_valid(ImageView p_ImageView) const;

      Sampler create_sampler(const SamplerDesc &p_Desc);
      void destroy(Sampler p_Sampler);
      bool is_valid(Sampler p_Sampler) const;

      ShaderModule create_shader_module(
          const ShaderModuleDesc &p_Desc);
      void destroy(ShaderModule p_ShaderModule);
      bool is_valid(ShaderModule p_ShaderModule) const;

      BindGroupLayout create_bind_group_layout(
          const BindGroupLayoutDesc &p_Desc);
      void destroy(BindGroupLayout p_BindGroupLayout);
      bool is_valid(BindGroupLayout p_BindGroupLayout) const;

      PipelineLayout create_pipeline_layout(
          const PipelineLayoutDesc &p_Desc);
      void destroy(PipelineLayout p_PipelineLayout);
      bool is_valid(PipelineLayout p_PipelineLayout) const;

      BindGroup create_bind_group(const BindGroupDesc &p_Desc);
      void update_bind_group(BindGroup p_BindGroup,
                             Util::Span<const BindGroupEntry>
                                 p_Entries);
      void destroy(BindGroup p_BindGroup);
      bool is_valid(BindGroup p_BindGroup) const;

      GraphicsPipeline create_graphics_pipeline(
          const GraphicsPipelineDesc &p_Desc);
      void destroy(GraphicsPipeline p_GraphicsPipeline);
      bool is_valid(GraphicsPipeline p_GraphicsPipeline) const;

      ComputePipeline create_compute_pipeline(
          const ComputePipelineDesc &p_Desc);
      void destroy(ComputePipeline p_ComputePipeline);
      bool is_valid(ComputePipeline p_ComputePipeline) const;

      CommandList request_command_list(const FrameContext &p_Frame,
                                       QueueRole p_QueueRole);
      CommandList
      request_immediate_command_list(QueueRole p_QueueRole);
      void destroy(CommandList p_CommandList);
      bool is_valid(CommandList p_CommandList) const;

      GpuFence submit(const SubmitDesc &p_Desc);
      bool is_complete(GpuFence p_Fence);
      void wait(GpuFence p_Fence);
      void destroy(GpuFence p_Fence);
      bool is_valid(GpuFence p_Fence) const;

      Swapchain create_swapchain(const SwapchainDesc &p_Desc);

      void destroy(Swapchain p_Swapchain);
      bool is_valid(Swapchain p_Swapchain) const;

      FrameContext begin_frame();
      SwapchainFrame acquire_swapchain(const FrameContext &p_Frame,
                                       Swapchain p_Swapchain);
      Image get_swapchain_image(
          const SwapchainFrame &p_SwapchainFrame) const;
      ImageView get_swapchain_image_view(
          const SwapchainFrame &p_SwapchainFrame) const;
      void present(const SwapchainFrame &p_SwapchainFrame);
      void end_frame(const FrameContext &p_Frame);

      void begin(CommandList p_CommandList);
      void end(CommandList p_CommandList);
      void submit(const FrameContext &p_Frame,
                  CommandList p_CommandList);

      void barrier(CommandList p_CommandList,
                   const ImageBarrier &p_Barrier);
      void barrier(CommandList p_CommandList,
                   const BufferBarrier &p_Barrier);
      void copy_buffer(
          CommandList p_CommandList, Buffer p_Source,
          Buffer p_Destination,
          Util::Span<const BufferCopyRegion> p_Regions);
      void copy_buffer_to_image(
          CommandList p_CommandList, Buffer p_Source,
          Image p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions);
      void copy_image_to_buffer(
          CommandList p_CommandList, Image p_Source,
          Buffer p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions);
      void copy_image(
          CommandList p_CommandList, Image p_Source,
          Image p_Destination,
          Util::Span<const ImageCopyRegion> p_Regions);
      void blit_image(
          CommandList p_CommandList, Image p_Source,
          Image p_Destination,
          Util::Span<const ImageBlitRegion> p_Regions,
          FilterMode p_Filter);

      void begin_rendering(CommandList p_CommandList,
                           const RenderingInfo &p_RenderingInfo);
      void end_rendering(CommandList p_CommandList);
      void set_viewport(CommandList p_CommandList,
                        const Viewport &p_Viewport);
      void set_scissor(CommandList p_CommandList,
                       const Rect2D &p_Scissor);
      void bind_graphics_pipeline(
          CommandList p_CommandList,
          GraphicsPipeline p_GraphicsPipeline);
      void bind_compute_pipeline(CommandList p_CommandList,
                                 ComputePipeline p_ComputePipeline);
      void bind_bind_group(CommandList p_CommandList,
                           PipelineLayout p_PipelineLayout,
                           u32 p_GroupIndex,
                           BindGroup p_BindGroup);
      void push_constants(CommandList p_CommandList,
                          PipelineLayout p_PipelineLayout,
                          ShaderStage p_Stages, u32 p_Offset,
                          u32 p_Size, const void *p_Data);
      void bind_vertex_buffer(CommandList p_CommandList, u32 p_Binding,
                              Buffer p_Buffer, u64 p_Offset = 0);
      void bind_index_buffer(CommandList p_CommandList,
                             Buffer p_Buffer, u64 p_Offset,
                             IndexType p_IndexType);
      void draw(CommandList p_CommandList, u32 p_VertexCount,
                u32 p_InstanceCount = 1, u32 p_FirstVertex = 0,
                u32 p_FirstInstance = 0);
      void draw_indexed(CommandList p_CommandList, u32 p_IndexCount,
                        u32 p_InstanceCount = 1,
                        u32 p_FirstIndex = 0,
                        i32 p_VertexOffset = 0,
                        u32 p_FirstInstance = 0);
      void dispatch(CommandList p_CommandList, u32 p_GroupCountX,
                    u32 p_GroupCountY, u32 p_GroupCountZ);

    private:
      Util::UniquePtr<Detail::ContextImpl> m_Impl;
    };
  } // namespace Gfx
} // namespace Low
