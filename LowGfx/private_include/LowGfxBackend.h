#pragma once

#include "LowGfxContext.h"
#include "LowGfxImage.h"
#include "LowMath.h"
#include "LowGfxPipeline.h"

namespace Low {
  namespace Gfx {
    namespace Detail {
      struct BackendBuffer
      {
        u64 size = 0;
        BufferUsage usage = BufferUsage::None;
        void *backend_state = nullptr;
      };

      struct BackendImage
      {
        ImageFormat format = ImageFormat::Undefined;
        ImageDimension dimension = ImageDimension::Image2D;
        ImageUsage usage = ImageUsage::None;
        ImageState state = ImageState::Undefined;
        Math::UVector3 extent;
        u32 mip_levels = 1;
        u32 array_layers = 1;
        void *backend_state = nullptr;
      };

      struct BackendImageView
      {
        Image image;
        ImageFormat format = ImageFormat::Undefined;
        ImageAspect aspect = ImageAspect::Color;
        u32 base_mip = 0;
        u32 mip_count = 1;
        u32 base_layer = 0;
        u32 layer_count = 1;
        void *backend_state = nullptr;
      };

      struct BackendSampler
      {
        void *backend_state = nullptr;
      };

      struct BackendShaderModule
      {
        ShaderSourceFormat format = ShaderSourceFormat::Spirv;
        void *backend_state = nullptr;
      };

      struct BackendBindGroupLayout
      {
        Util::List<BindGroupLayoutEntry> entries;
        void *backend_state = nullptr;
      };

      struct BackendPipelineLayout
      {
        Util::List<BindGroupLayout> bind_group_layouts;
        void *backend_state = nullptr;
      };

      struct BackendBindGroup
      {
        BindGroupLayout layout;
        u32 in_use_count = 0;
        Util::List<BindGroupEntry> pending_entries;
        void *backend_state = nullptr;
      };

      struct BackendGraphicsPipeline
      {
        PipelineLayout layout;
        void *backend_state = nullptr;
      };

      struct BackendComputePipeline
      {
        PipelineLayout layout;
        void *backend_state = nullptr;
      };

      struct BackendCommandList
      {
        QueueRole queue_role = QueueRole::Graphics;
        CommandListState state = CommandListState::Initial;
        bool rendering_active = false;
        Math::UVector2 rendering_extent{0, 0};
        Util::List<BindGroup> used_bind_groups;
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
        Util::List<Image> images;
        Util::List<ImageView> image_views;
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
        void (*wait_idle)(ContextImpl &);

        BackendBuffer (*create_buffer)(ContextImpl &,
                                       const BufferDesc &);
        void (*destroy_buffer)(ContextImpl &, BackendBuffer &);

        BackendImage (*create_image)(ContextImpl &,
                                     const ImageDesc &);
        void (*destroy_image)(ContextImpl &, BackendImage &);

        BackendImageView (*create_image_view)(ContextImpl &,
                                              const ImageViewDesc &);
        void (*destroy_image_view)(ContextImpl &, BackendImageView &);

        BackendSampler (*create_sampler)(ContextImpl &,
                                         const SamplerDesc &);
        void (*destroy_sampler)(ContextImpl &, BackendSampler &);

        BackendShaderModule (*create_shader_module)(
            ContextImpl &, const ShaderModuleDesc &);
        void (*destroy_shader_module)(ContextImpl &,
                                      BackendShaderModule &);

        BackendBindGroupLayout (*create_bind_group_layout)(
            ContextImpl &, const BindGroupLayoutDesc &);
        void (*destroy_bind_group_layout)(ContextImpl &,
                                          BackendBindGroupLayout &);

        BackendPipelineLayout (*create_pipeline_layout)(
            ContextImpl &, const PipelineLayoutDesc &);
        void (*destroy_pipeline_layout)(ContextImpl &,
                                        BackendPipelineLayout &);

        BackendBindGroup (*create_bind_group)(
            ContextImpl &, const BindGroupDesc &);
        void (*update_bind_group)(
            ContextImpl &, BackendBindGroup &,
            Util::Span<const BindGroupEntry> p_Entries);
        void (*destroy_bind_group)(ContextImpl &, BackendBindGroup &);

        BackendGraphicsPipeline (*create_graphics_pipeline)(
            ContextImpl &, const GraphicsPipelineDesc &);
        void (*destroy_graphics_pipeline)(
            ContextImpl &, BackendGraphicsPipeline &);

        BackendComputePipeline (*create_compute_pipeline)(
            ContextImpl &, const ComputePipelineDesc &);
        void (*destroy_compute_pipeline)(ContextImpl &,
                                         BackendComputePipeline &);

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

        void (*begin_command_list)(ContextImpl &p_Context,
                                   BackendCommandList &p_CommandList);
        void (*end_command_list)(ContextImpl &p_Context,
                                 BackendCommandList &p_CommandList);
        void (*submit_command_list)(
            ContextImpl &p_Context, const FrameContext &p_Frame,
            BackendCommandList &p_CommandList);
        void (*barrier_image_command_list)(
            ContextImpl &p_Context, BackendCommandList &p_CommandList,
            const ImageBarrier &p_Barrier);

        void (*begin_dynamic_rendering)(
            ContextImpl &p_Context, BackendCommandList &p_CommandList,
            const RenderingInfo &p_RenderingInfo);
        void (*end_dynamic_rendering)(
            ContextImpl &p_Context,
            BackendCommandList &p_CommandList);
        void (*set_viewport)(ContextImpl &p_Context,
                             BackendCommandList &p_CommandList,
                             const Viewport &p_Viewport);
        void (*set_scissor)(ContextImpl &p_Context,
                            BackendCommandList &p_CommandList,
                            const Rect2D &p_Scissor);
        void (*bind_graphics_pipeline)(
            ContextImpl &p_Context, BackendCommandList &p_CommandList,
            BackendGraphicsPipeline &p_GraphicsPipeline);
        void (*bind_compute_pipeline)(
            ContextImpl &p_Context, BackendCommandList &p_CommandList,
            BackendComputePipeline &p_ComputePipeline);
        void (*bind_bind_group)(
            ContextImpl &p_Context, BackendCommandList &p_CommandList,
            BackendPipelineLayout &p_PipelineLayout, u32 p_GroupIndex,
            BackendBindGroup &p_BindGroup);
        void (*bind_vertex_buffer)(ContextImpl &p_Context,
                                   BackendCommandList &p_CommandList,
                                   u32 p_Binding,
                                   BackendBuffer &p_Buffer,
                                   u64 p_Offset);
        void (*bind_index_buffer)(ContextImpl &p_Context,
                                  BackendCommandList &p_CommandList,
                                  BackendBuffer &p_Buffer,
                                  u64 p_Offset,
                                  IndexType p_IndexType);
        void (*draw)(ContextImpl &p_Context,
                     BackendCommandList &p_CommandList,
                     u32 p_VertexCount, u32 p_InstanceCount,
                     u32 p_FirstVertex, u32 p_FirstInstance);
        void (*draw_indexed)(ContextImpl &p_Context,
                             BackendCommandList &p_CommandList,
                             u32 p_IndexCount, u32 p_InstanceCount,
                             u32 p_FirstIndex, i32 p_VertexOffset,
                             u32 p_FirstInstance);
        void (*dispatch)(ContextImpl &p_Context,
                         BackendCommandList &p_CommandList,
                         u32 p_GroupCountX, u32 p_GroupCountY,
                         u32 p_GroupCountZ);
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
        Pool<Image, BackendImage> images;
        Pool<ImageView, BackendImageView> image_views;
        Pool<Sampler, BackendSampler> samplers;
        Pool<ShaderModule, BackendShaderModule> shader_modules;
        Pool<BindGroupLayout, BackendBindGroupLayout>
            bind_group_layouts;
        Pool<PipelineLayout, BackendPipelineLayout> pipeline_layouts;
        Pool<BindGroup, BackendBindGroup> bind_groups;
        Pool<GraphicsPipeline, BackendGraphicsPipeline>
            graphics_pipelines;
        Pool<ComputePipeline, BackendComputePipeline>
            compute_pipelines;
        Pool<CommandList, BackendCommandList> command_lists;
        Pool<Swapchain, BackendSwapchain> swapchains;
        Util::List<CommandList> frame_command_lists;
        Util::List<Util::List<BindGroup>> frame_bind_group_usages;

        void *backend_state = nullptr;

        u32 frames_in_flight = 0;

        u64 frame_number = 0;
        u32 frame_index = 0;
        bool frame_active = false;
      };
    } // namespace Detail
  } // namespace Gfx
} // namespace Low
