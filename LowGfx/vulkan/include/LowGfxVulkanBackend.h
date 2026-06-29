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
      void wait_idle(Detail::ContextImpl &p_Context);

      Detail::BackendBuffer
      create_buffer(Detail::ContextImpl &p_Context,
                    const BufferDesc &p_Desc);
      void destroy_buffer(Detail::ContextImpl &p_Context,
                          Detail::BackendBuffer &p_Buffer);
      void *map_buffer(Detail::ContextImpl &p_Context,
                       Detail::BackendBuffer &p_Buffer);
      void unmap_buffer(Detail::ContextImpl &p_Context,
                        Detail::BackendBuffer &p_Buffer);
      void flush_buffer(Detail::ContextImpl &p_Context,
                        Detail::BackendBuffer &p_Buffer,
                        u64 p_Offset, u64 p_Size);
      void invalidate_buffer(Detail::ContextImpl &p_Context,
                             Detail::BackendBuffer &p_Buffer,
                             u64 p_Offset, u64 p_Size);

      Detail::BackendImage
      create_image(Detail::ContextImpl &p_Context,
                   const ImageDesc &p_Desc);
      void destroy_image(Detail::ContextImpl &p_Context,
                         Detail::BackendImage &p_Image);

      Detail::BackendImageView
      create_image_view(Detail::ContextImpl &p_Context,
                        const ImageViewDesc &p_Desc);
      void destroy_image_view(Detail::ContextImpl &p_Context,
                              Detail::BackendImageView &p_ImageView);

      Detail::BackendSampler
      create_sampler(Detail::ContextImpl &p_Context,
                     const SamplerDesc &p_Desc);
      void destroy_sampler(Detail::ContextImpl &p_Context,
                           Detail::BackendSampler &p_Sampler);

      Detail::BackendShaderModule create_shader_module(
          Detail::ContextImpl &p_Context,
          const ShaderModuleDesc &p_Desc);
      void destroy_shader_module(
          Detail::ContextImpl &p_Context,
          Detail::BackendShaderModule &p_ShaderModule);

      Detail::BackendBindGroupLayout create_bind_group_layout(
          Detail::ContextImpl &p_Context,
          const BindGroupLayoutDesc &p_Desc);
      void destroy_bind_group_layout(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroupLayout &p_BindGroupLayout);

      Detail::BackendPipelineLayout create_pipeline_layout(
          Detail::ContextImpl &p_Context,
          const PipelineLayoutDesc &p_Desc);
      void destroy_pipeline_layout(
          Detail::ContextImpl &p_Context,
          Detail::BackendPipelineLayout &p_PipelineLayout);

      Detail::BackendBindGroup
      create_bind_group(Detail::ContextImpl &p_Context,
                        const BindGroupDesc &p_Desc);
      void update_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroup &p_BindGroup,
          Util::Span<const BindGroupEntry> p_Entries);
      void destroy_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroup &p_BindGroup);

      Detail::BackendGraphicsPipeline create_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          const GraphicsPipelineDesc &p_Desc);
      void destroy_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendGraphicsPipeline &p_GraphicsPipeline);

      Detail::BackendComputePipeline create_compute_pipeline(
          Detail::ContextImpl &p_Context,
          const ComputePipelineDesc &p_Desc);
      void destroy_compute_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendComputePipeline &p_ComputePipeline);

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

      void begin_command_list(Detail::ContextImpl &p_Context,
                              Detail::BackendCommandList &);
      void end_command_list(Detail::ContextImpl &p_Context,
                            Detail::BackendCommandList &);
      void submit_command_list(Detail::ContextImpl &p_Context,
                               const FrameContext &p_Frame,
                               Detail::BackendCommandList &);
      void barrier_image_command_list(Detail::ContextImpl &p_Context,
                                      Detail::BackendCommandList &,
                                      const ImageBarrier &);
      void copy_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Source,
          Detail::BackendBuffer &p_Destination,
          Util::Span<const BufferCopyRegion> p_Regions);
      void copy_buffer_to_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions);
      void copy_image_to_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendBuffer &p_Destination,
          Util::Span<const BufferImageCopyRegion> p_Regions);
      void copy_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const ImageCopyRegion> p_Regions);
      void blit_image(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendImage &p_Source,
          Detail::BackendImage &p_Destination,
          Util::Span<const ImageBlitRegion> p_Regions,
          FilterMode p_Filter);

      void begin_dynamic_rendering(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          const RenderingInfo &p_RenderingInfo);
      void end_dynamic_rendering(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList);
      void set_viewport(Detail::ContextImpl &p_Context,
                        Detail::BackendCommandList &p_CommandList,
                        const Viewport &p_Viewport);
      void set_scissor(Detail::ContextImpl &p_Context,
                       Detail::BackendCommandList &p_CommandList,
                       const Rect2D &p_Scissor);
      void bind_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendGraphicsPipeline &p_GraphicsPipeline);
      void bind_compute_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendComputePipeline &p_ComputePipeline);
      void bind_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendPipelineLayout &p_PipelineLayout,
          u32 p_GroupIndex, Detail::BackendBindGroup &p_BindGroup);
      void bind_vertex_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList, u32 p_Binding,
          Detail::BackendBuffer &p_Buffer, u64 p_Offset);
      void bind_index_buffer(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendBuffer &p_Buffer, u64 p_Offset,
          IndexType p_IndexType);
      void draw(Detail::ContextImpl &p_Context,
                Detail::BackendCommandList &p_CommandList,
                u32 p_VertexCount, u32 p_InstanceCount,
                u32 p_FirstVertex, u32 p_FirstInstance);
      void draw_indexed(Detail::ContextImpl &p_Context,
                        Detail::BackendCommandList &p_CommandList,
                        u32 p_IndexCount, u32 p_InstanceCount,
                        u32 p_FirstIndex, i32 p_VertexOffset,
                        u32 p_FirstInstance);
      void dispatch(Detail::ContextImpl &p_Context,
                    Detail::BackendCommandList &p_CommandList,
                    u32 p_GroupCountX, u32 p_GroupCountY,
                    u32 p_GroupCountZ);

    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
