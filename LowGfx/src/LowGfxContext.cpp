#include "LowGfxContext.h"

#include "LowGfxBackend.h"
#include "LowGfxImage.h"
#include "LowGfxLogInternal.h"
#include "LowGfxVulkanBackend.h"

#include "LowGfxAssert.h"

#include <atomic>
#include <utility>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Low {
  namespace Gfx {
    static u32 allocate_instance_id()
    {
      static std::atomic<u32> g_NextInstanceId{1};
      return g_NextInstanceId.fetch_add(1, std::memory_order_relaxed);
    }

    static u32 allocate_context_id()
    {
      static std::atomic<u32> g_NextContextId{1};
      return g_NextContextId.fetch_add(1, std::memory_order_relaxed);
    }

    static const Detail::BackendProvider &
    get_backend_provider(Backend p_Backend)
    {
      switch (p_Backend) {
      case Backend::Vulkan:
        return Vulkan::get_backend_provider();
      }

      GFX_ASSERT(false, "Unsupported LowGfx backend");
      return Vulkan::get_backend_provider();
    }

    static void cleanup_instance(Detail::InstanceImpl *p_Impl)
    {
      if (p_Impl && p_Impl->api) {
        p_Impl->surfaces.for_each(
            [p_Impl](Detail::BackendSurface &p_Surface) {
              p_Impl->api->destroy_surface(*p_Impl, p_Surface);
            });
        p_Impl->surfaces.clear();

        p_Impl->api->destroy_instance(*p_Impl);
        p_Impl->adapters.clear();
        p_Impl->api = nullptr;
        p_Impl->context_api = nullptr;
      }
    }

    static void cleanup_context(Detail::ContextImpl *p_Impl)
    {
      if (p_Impl && p_Impl->api) {
        p_Impl->graphics_pipelines.for_each(
            [p_Impl](Detail::BackendGraphicsPipeline &p_Pipeline) {
              p_Impl->api->destroy_graphics_pipeline(*p_Impl,
                                                     p_Pipeline);
            });
        p_Impl->graphics_pipelines.clear();

        p_Impl->compute_pipelines.for_each(
            [p_Impl](Detail::BackendComputePipeline &p_Pipeline) {
              p_Impl->api->destroy_compute_pipeline(*p_Impl,
                                                    p_Pipeline);
            });
        p_Impl->compute_pipelines.clear();

        p_Impl->bind_groups.for_each(
            [p_Impl](Detail::BackendBindGroup &p_BindGroup) {
              p_Impl->api->destroy_bind_group(*p_Impl, p_BindGroup);
            });
        p_Impl->bind_groups.clear();

        p_Impl->pipeline_layouts.for_each(
            [p_Impl](Detail::BackendPipelineLayout &p_Layout) {
              p_Impl->api->destroy_pipeline_layout(*p_Impl, p_Layout);
            });
        p_Impl->pipeline_layouts.clear();

        p_Impl->bind_group_layouts.for_each(
            [p_Impl](Detail::BackendBindGroupLayout &p_Layout) {
              p_Impl->api->destroy_bind_group_layout(*p_Impl,
                                                     p_Layout);
            });
        p_Impl->bind_group_layouts.clear();

        p_Impl->shader_modules.for_each(
            [p_Impl](Detail::BackendShaderModule &p_Shader) {
              p_Impl->api->destroy_shader_module(*p_Impl, p_Shader);
            });
        p_Impl->shader_modules.clear();

        p_Impl->buffers.for_each(
            [p_Impl](Detail::BackendBuffer &p_Buffer) {
              p_Impl->api->destroy_buffer(*p_Impl, p_Buffer);
            });
        p_Impl->buffers.clear();

        p_Impl->image_views.for_each(
            [p_Impl](Detail::BackendImageView &p_ImageView) {
              p_Impl->api->destroy_image_view(*p_Impl, p_ImageView);
            });
        p_Impl->image_views.clear();

        p_Impl->images.for_each(
            [p_Impl](Detail::BackendImage &p_Image) {
              p_Impl->api->destroy_image(*p_Impl, p_Image);
            });
        p_Impl->images.clear();

        p_Impl->samplers.for_each(
            [p_Impl](Detail::BackendSampler &p_Sampler) {
              p_Impl->api->destroy_sampler(*p_Impl, p_Sampler);
            });
        p_Impl->samplers.clear();

        p_Impl->command_lists.for_each(
            [p_Impl](Detail::BackendCommandList &p_CommandList) {
              p_Impl->api->destroy_command_list(*p_Impl,
                                                p_CommandList);
            });
        p_Impl->command_lists.clear();

        p_Impl->swapchains.for_each(
            [p_Impl](Detail::BackendSwapchain &p_Swapchain) {
              p_Impl->api->destroy_swapchain(*p_Impl, p_Swapchain);
            });
        p_Impl->swapchains.clear();

        p_Impl->api->destroy_context(*p_Impl);
        p_Impl->api = nullptr;
        p_Impl->instance = nullptr;
      }
    }

    Instance::Instance(const InstanceDesc &p_Desc)
        : m_Impl(std::make_unique<Detail::InstanceImpl>())
    {
      const Detail::BackendProvider &l_Provider =
          get_backend_provider(p_Desc.backend);
      GFX_ASSERT(l_Provider.instance_api,
                 "LowGfx backend does not provide an instance API");
      GFX_ASSERT(l_Provider.context_api,
                 "LowGfx backend does not provide a context API");

      m_Impl->instance_id = allocate_instance_id();
      m_Impl->backend = p_Desc.backend;
      m_Impl->api = l_Provider.instance_api;
      m_Impl->context_api = l_Provider.context_api;
      m_Impl->log_callback = p_Desc.log_callback;
      m_Impl->log_user_data = p_Desc.log_user_data;
      m_Impl->surfaces.set_owner_id(m_Impl->instance_id);
      m_Impl->backend_state =
          m_Impl->api->create_instance(*m_Impl, p_Desc);
    }

    Instance::~Instance()
    {
      cleanup_instance(m_Impl.get());
    }

    Instance::Instance(Instance &&) noexcept = default;

    Instance &Instance::operator=(Instance &&p_Other) noexcept
    {
      if (this != &p_Other) {
        cleanup_instance(m_Impl.get());
        m_Impl = std::move(p_Other.m_Impl);
      }

      return *this;
    }

    Backend Instance::get_backend() const
    {
      return m_Impl->backend;
    }

    Surface Instance::create_surface(const SurfaceDesc &p_Desc)
    {
      Detail::BackendSurface l_BackendSurface =
          m_Impl->api->create_surface(*m_Impl, p_Desc);
      return m_Impl->surfaces.create(std::move(l_BackendSurface));
    }

    void Instance::destroy(Surface p_Surface)
    {
      Detail::BackendSurface *l_Surface =
          m_Impl->surfaces.get(p_Surface);
      if (!l_Surface) {
        return;
      }

      m_Impl->api->destroy_surface(*m_Impl, *l_Surface);
      m_Impl->surfaces.destroy(p_Surface);
    }

    bool Instance::is_valid(Surface p_Surface) const
    {
      return m_Impl->surfaces.is_valid(p_Surface);
    }

    Adapter
    Instance::select_adapter(const AdapterSelectionDesc &p_Desc)
    {
      Adapter l_Adapter =
          m_Impl->api->select_adapter(*m_Impl, p_Desc);
      GFX_ASSERT(is_valid(l_Adapter),
                 "Backend returned an invalid LowGfx adapter");
      return l_Adapter;
    }

    bool Instance::is_valid(Adapter p_Adapter) const
    {
      if (!p_Adapter || p_Adapter.owner_id != m_Impl->instance_id ||
          p_Adapter.index >= m_Impl->adapters.size()) {
        return false;
      }

      return m_Impl->adapters[p_Adapter.index].generation ==
             p_Adapter.generation;
    }

    Context::Context(Instance &p_Instance, Adapter p_Adapter,
                     const ContextDesc &p_Desc)
        : m_Impl(std::make_unique<Detail::ContextImpl>())
    {
      GFX_ASSERT(p_Desc.frames_in_flight > 0,
                 "LowGfx frames_in_flight must be greater than zero");
      GFX_ASSERT(p_Instance.is_valid(p_Adapter),
                 "Cannot create LowGfx context from invalid adapter");

      m_Impl->context_id = allocate_context_id();
      m_Impl->instance_id = p_Instance.m_Impl->instance_id;
      m_Impl->backend = p_Instance.m_Impl->backend;
      m_Impl->api = p_Instance.m_Impl->context_api;
      m_Impl->instance = p_Instance.m_Impl.get();
      m_Impl->adapter = p_Adapter;
      m_Impl->log_callback = p_Instance.m_Impl->log_callback;
      m_Impl->log_user_data = p_Instance.m_Impl->log_user_data;
      m_Impl->frames_in_flight = p_Desc.frames_in_flight;
      m_Impl->frame_bind_group_usages.resize(p_Desc.frames_in_flight);
      m_Impl->buffers.set_owner_id(m_Impl->context_id);
      m_Impl->images.set_owner_id(m_Impl->context_id);
      m_Impl->image_views.set_owner_id(m_Impl->context_id);
      m_Impl->samplers.set_owner_id(m_Impl->context_id);
      m_Impl->shader_modules.set_owner_id(m_Impl->context_id);
      m_Impl->bind_group_layouts.set_owner_id(m_Impl->context_id);
      m_Impl->pipeline_layouts.set_owner_id(m_Impl->context_id);
      m_Impl->bind_groups.set_owner_id(m_Impl->context_id);
      m_Impl->graphics_pipelines.set_owner_id(m_Impl->context_id);
      m_Impl->compute_pipelines.set_owner_id(m_Impl->context_id);
      m_Impl->command_lists.set_owner_id(m_Impl->context_id);
      m_Impl->swapchains.set_owner_id(m_Impl->context_id);
      m_Impl->backend_state = m_Impl->api->create_context(
          *m_Impl, *m_Impl->instance, p_Adapter, p_Desc);
      m_Impl->caps = m_Impl->api->get_caps(*m_Impl);
    }

    Context::~Context()
    {
      cleanup_context(m_Impl.get());
    }

    Context::Context(Context &&) noexcept = default;

    Context &Context::operator=(Context &&p_Other) noexcept
    {
      if (this != &p_Other) {
        cleanup_context(m_Impl.get());
        m_Impl = std::move(p_Other.m_Impl);
      }

      return *this;
    }

    Backend Context::get_backend() const
    {
      return m_Impl->backend;
    }

    const DeviceCaps &Context::get_caps() const
    {
      return m_Impl->caps;
    }

    Buffer Context::create_buffer(const BufferDesc &p_Desc)
    {
      GFX_ASSERT(p_Desc.size > 0, "Cannot create zero-sized buffer");
      GFX_ASSERT(p_Desc.usage != BufferUsage::None,
                 "Cannot create buffer without usage flags");

      Detail::BackendBuffer l_BackendBuffer =
          m_Impl->api->create_buffer(*m_Impl, p_Desc);
      return m_Impl->buffers.create(std::move(l_BackendBuffer));
    }

    void Context::destroy(Buffer p_Buffer)
    {
      Detail::BackendBuffer *l_Buffer = m_Impl->buffers.get(p_Buffer);
      if (!l_Buffer) {
        return;
      }

      m_Impl->api->destroy_buffer(*m_Impl, *l_Buffer);
      m_Impl->buffers.destroy(p_Buffer);
    }

    bool Context::is_valid(Buffer p_Buffer) const
    {
      return m_Impl->buffers.is_valid(p_Buffer);
    }

    Image Context::create_image(const ImageDesc &p_Desc)
    {
      GFX_ASSERT(p_Desc.format != ImageFormat::Undefined,
                 "Cannot create image with undefined format");
      GFX_ASSERT(p_Desc.usage != ImageUsage::None,
                 "Cannot create image without usage flags");
      GFX_ASSERT(p_Desc.extent.x > 0 && p_Desc.extent.y > 0 &&
                     p_Desc.extent.z > 0,
                 "Cannot create zero-sized image");
      GFX_ASSERT(p_Desc.mip_levels > 0,
                 "Cannot create image without mip levels");
      GFX_ASSERT(p_Desc.array_layers > 0,
                 "Cannot create image without array layers");
      GFX_ASSERT(
          p_Desc.dimension != ImageDimension::Image2D ||
              p_Desc.extent.z == 1,
          "Cannot create 2D image with depth greater than one");
      GFX_ASSERT(p_Desc.dimension != ImageDimension::Image3D ||
                     p_Desc.array_layers == 1,
                 "Cannot create 3D image arrays");

      Detail::BackendImage l_BackendImage =
          m_Impl->api->create_image(*m_Impl, p_Desc);
      return m_Impl->images.create(std::move(l_BackendImage));
    }

    void Context::destroy(Image p_Image)
    {
      Detail::BackendImage *l_Image = m_Impl->images.get(p_Image);
      if (!l_Image) {
        return;
      }

      m_Impl->image_views.for_each(
          [p_Image](Detail::BackendImageView &p_ImageView) {
            GFX_ASSERT(p_ImageView.image != p_Image,
                       "Cannot destroy image while image views still "
                       "reference it");
          });

      m_Impl->api->destroy_image(*m_Impl, *l_Image);
      m_Impl->images.destroy(p_Image);
    }

    bool Context::is_valid(Image p_Image) const
    {
      return m_Impl->images.is_valid(p_Image);
    }

    static bool is_depth_format(ImageFormat p_Format)
    {
      return p_Format == ImageFormat::D16_UNorm ||
             p_Format == ImageFormat::D32_Float ||
             p_Format == ImageFormat::D24_UNorm_S8_UInt ||
             p_Format == ImageFormat::D32_Float_S8_UInt;
    }

    static bool has_stencil(ImageFormat p_Format)
    {
      return p_Format == ImageFormat::D24_UNorm_S8_UInt ||
             p_Format == ImageFormat::D32_Float_S8_UInt;
    }

    static bool has_image_usage(ImageUsage p_Value, ImageUsage p_Flag)
    {
      return (static_cast<u32>(p_Value) & static_cast<u32>(p_Flag)) !=
             0;
    }

    static bool has_buffer_usage(BufferUsage p_Value,
                                 BufferUsage p_Flag)
    {
      return (static_cast<u32>(p_Value) & static_cast<u32>(p_Flag)) !=
             0;
    }

    static bool has_shader_stage(ShaderStage p_Value,
                                 ShaderStage p_Flag)
    {
      return (static_cast<u32>(p_Value) & static_cast<u32>(p_Flag)) !=
             0;
    }

    static u32 mip_extent(u32 p_Extent, u32 p_BaseMip)
    {
      const u32 l_Extent = p_Extent >> p_BaseMip;
      return l_Extent > 0 ? l_Extent : 1;
    }

    static bool view_covers_whole_image(
        const Detail::BackendImage &p_Image,
        const Detail::BackendImageView &p_ImageView)
    {
      return p_ImageView.base_mip == 0 &&
             p_ImageView.mip_count == p_Image.mip_levels &&
             p_ImageView.base_layer == 0 &&
             p_ImageView.layer_count == p_Image.array_layers;
    }

    ImageView Context::create_image_view(const ImageViewDesc &p_Desc)
    {
      Detail::BackendImage *l_Image =
          m_Impl->images.get(p_Desc.image);
      GFX_ASSERT(l_Image,
                 "Cannot create image view for invalid image");

      const ImageFormat l_ViewFormat =
          p_Desc.format == ImageFormat::Undefined ? l_Image->format
                                                  : p_Desc.format;
      GFX_ASSERT(l_ViewFormat != ImageFormat::Undefined,
                 "Cannot create image view with undefined format");
      GFX_ASSERT(p_Desc.mip_count > 0,
                 "Cannot create image view without mip levels");
      GFX_ASSERT(p_Desc.layer_count > 0,
                 "Cannot create image view without array layers");
      GFX_ASSERT(p_Desc.base_mip + p_Desc.mip_count <=
                     l_Image->mip_levels,
                 "Image view mip range exceeds image mip levels");
      GFX_ASSERT(p_Desc.base_layer + p_Desc.layer_count <=
                     l_Image->array_layers,
                 "Image view layer range exceeds image array layers");

      if (is_depth_format(l_ViewFormat)) {
        GFX_ASSERT(p_Desc.aspect != ImageAspect::Color,
                   "Cannot create color image view for depth format");
        if (p_Desc.aspect == ImageAspect::Stencil ||
            p_Desc.aspect == ImageAspect::DepthStencil) {
          GFX_ASSERT(
              has_stencil(l_ViewFormat),
              "Cannot create stencil image view for depth-only "
              "format");
        }
      } else {
        GFX_ASSERT(p_Desc.aspect == ImageAspect::Color,
                   "Cannot create depth/stencil image view for color "
                   "format");
      }

      if (l_Image->dimension == ImageDimension::Image3D) {
        GFX_ASSERT(p_Desc.type == ImageViewType::Image3D,
                   "3D images require 3D image views");
        GFX_ASSERT(p_Desc.base_layer == 0 && p_Desc.layer_count == 1,
                   "3D image views cannot select array layers");
      } else {
        GFX_ASSERT(p_Desc.type != ImageViewType::Image3D,
                   "2D images cannot use 3D image views");
      }

      if (p_Desc.type == ImageViewType::Cube) {
        GFX_ASSERT(p_Desc.layer_count == 6,
                   "Cube image views require exactly 6 layers");
        GFX_ASSERT(l_Image->extent.x == l_Image->extent.y,
                   "Cube image views require square images");
      }
      if (p_Desc.type == ImageViewType::CubeArray) {
        GFX_ASSERT(p_Desc.layer_count >= 6 &&
                       p_Desc.layer_count % 6 == 0,
                   "Cube array image views require layer count as a "
                   "multiple of 6");
        GFX_ASSERT(l_Image->extent.x == l_Image->extent.y,
                   "Cube array image views require square images");
      }

      Detail::BackendImageView l_BackendImageView =
          m_Impl->api->create_image_view(*m_Impl, p_Desc);
      return m_Impl->image_views.create(
          std::move(l_BackendImageView));
    }

    void Context::destroy(ImageView p_ImageView)
    {
      Detail::BackendImageView *l_ImageView =
          m_Impl->image_views.get(p_ImageView);
      if (!l_ImageView) {
        return;
      }

      m_Impl->api->destroy_image_view(*m_Impl, *l_ImageView);
      m_Impl->image_views.destroy(p_ImageView);
    }

    bool Context::is_valid(ImageView p_ImageView) const
    {
      return m_Impl->image_views.is_valid(p_ImageView);
    }

    Sampler Context::create_sampler(const SamplerDesc &p_Desc)
    {
      GFX_ASSERT(p_Desc.min_lod <= p_Desc.max_lod,
                 "Cannot create sampler with min_lod greater than "
                 "max_lod");
      GFX_ASSERT(p_Desc.max_anisotropy >= 1.0f,
                 "Cannot create sampler with max_anisotropy below 1");

      Detail::BackendSampler l_BackendSampler =
          m_Impl->api->create_sampler(*m_Impl, p_Desc);
      return m_Impl->samplers.create(std::move(l_BackendSampler));
    }

    void Context::destroy(Sampler p_Sampler)
    {
      Detail::BackendSampler *l_Sampler =
          m_Impl->samplers.get(p_Sampler);
      if (!l_Sampler) {
        return;
      }

      m_Impl->api->destroy_sampler(*m_Impl, *l_Sampler);
      m_Impl->samplers.destroy(p_Sampler);
    }

    bool Context::is_valid(Sampler p_Sampler) const
    {
      return m_Impl->samplers.is_valid(p_Sampler);
    }

    ShaderModule
    Context::create_shader_module(const ShaderModuleDesc &p_Desc)
    {
      GFX_ASSERT(
          p_Desc.format == ShaderSourceFormat::Spirv,
          "Only SPIR-V shader modules are supported right now");
      GFX_ASSERT(!p_Desc.code.empty(),
                 "Cannot create shader module without code");

      Detail::BackendShaderModule l_BackendShaderModule =
          m_Impl->api->create_shader_module(*m_Impl, p_Desc);
      return m_Impl->shader_modules.create(
          std::move(l_BackendShaderModule));
    }

    void Context::destroy(ShaderModule p_ShaderModule)
    {
      Detail::BackendShaderModule *l_ShaderModule =
          m_Impl->shader_modules.get(p_ShaderModule);
      if (!l_ShaderModule) {
        return;
      }

      m_Impl->api->destroy_shader_module(*m_Impl, *l_ShaderModule);
      m_Impl->shader_modules.destroy(p_ShaderModule);
    }

    bool Context::is_valid(ShaderModule p_ShaderModule) const
    {
      return m_Impl->shader_modules.is_valid(p_ShaderModule);
    }

    BindGroupLayout Context::create_bind_group_layout(
        const BindGroupLayoutDesc &p_Desc)
    {
      for (u32 i = 0; i < p_Desc.entries.size(); ++i) {
        const BindGroupLayoutEntry &i_Entry = p_Desc.entries[i];
        GFX_ASSERT(i_Entry.count > 0,
                   "Bind group layout entry count must be non-zero");
        GFX_ASSERT(i_Entry.stages != ShaderStage::None,
                   "Bind group layout entry must be visible to at "
                   "least one shader stage");

        for (u32 j = i + 1; j < p_Desc.entries.size(); ++j) {
          GFX_ASSERT(i_Entry.binding != p_Desc.entries[j].binding,
                     "Bind group layout contains duplicate binding");
        }
      }

      Detail::BackendBindGroupLayout l_BackendLayout =
          m_Impl->api->create_bind_group_layout(*m_Impl, p_Desc);
      return m_Impl->bind_group_layouts.create(
          std::move(l_BackendLayout));
    }

    void Context::destroy(BindGroupLayout p_BindGroupLayout)
    {
      Detail::BackendBindGroupLayout *l_Layout =
          m_Impl->bind_group_layouts.get(p_BindGroupLayout);
      if (!l_Layout) {
        return;
      }

      m_Impl->pipeline_layouts.for_each(
          [p_BindGroupLayout](
              Detail::BackendPipelineLayout &p_Layout) {
            for (BindGroupLayout i_Layout :
                 p_Layout.bind_group_layouts) {
              GFX_ASSERT(
                  i_Layout != p_BindGroupLayout,
                  "Cannot destroy bind group layout while pipeline "
                  "layouts still reference it");
            }
          });
      m_Impl->bind_groups.for_each(
          [p_BindGroupLayout](Detail::BackendBindGroup &p_BindGroup) {
            GFX_ASSERT(
                p_BindGroup.layout != p_BindGroupLayout,
                "Cannot destroy bind group layout while bind groups "
                "still reference it");
          });

      m_Impl->api->destroy_bind_group_layout(*m_Impl, *l_Layout);
      m_Impl->bind_group_layouts.destroy(p_BindGroupLayout);
    }

    bool Context::is_valid(BindGroupLayout p_BindGroupLayout) const
    {
      return m_Impl->bind_group_layouts.is_valid(p_BindGroupLayout);
    }

    PipelineLayout
    Context::create_pipeline_layout(const PipelineLayoutDesc &p_Desc)
    {
      for (BindGroupLayout i_Layout : p_Desc.bind_group_layouts) {
        GFX_ASSERT(m_Impl->bind_group_layouts.is_valid(i_Layout),
                   "Cannot create pipeline layout with invalid bind "
                   "group layout");
      }

      Detail::BackendPipelineLayout l_BackendLayout =
          m_Impl->api->create_pipeline_layout(*m_Impl, p_Desc);
      return m_Impl->pipeline_layouts.create(
          std::move(l_BackendLayout));
    }

    void Context::destroy(PipelineLayout p_PipelineLayout)
    {
      Detail::BackendPipelineLayout *l_Layout =
          m_Impl->pipeline_layouts.get(p_PipelineLayout);
      if (!l_Layout) {
        return;
      }

      m_Impl->graphics_pipelines.for_each(
          [p_PipelineLayout](
              Detail::BackendGraphicsPipeline &p_Pipeline) {
            GFX_ASSERT(
                p_Pipeline.layout != p_PipelineLayout,
                "Cannot destroy pipeline layout while graphics "
                "pipelines still reference it");
          });
      m_Impl->compute_pipelines.for_each(
          [p_PipelineLayout](
              Detail::BackendComputePipeline &p_Pipeline) {
            GFX_ASSERT(
                p_Pipeline.layout != p_PipelineLayout,
                "Cannot destroy pipeline layout while compute "
                "pipelines still reference it");
          });

      m_Impl->api->destroy_pipeline_layout(*m_Impl, *l_Layout);
      m_Impl->pipeline_layouts.destroy(p_PipelineLayout);
    }

    bool Context::is_valid(PipelineLayout p_PipelineLayout) const
    {
      return m_Impl->pipeline_layouts.is_valid(p_PipelineLayout);
    }

    static const BindGroupLayoutEntry *
    find_layout_entry(const Detail::BackendBindGroupLayout &p_Layout,
                      u32 p_Binding)
    {
      for (const BindGroupLayoutEntry &i_Entry : p_Layout.entries) {
        if (i_Entry.binding == p_Binding) {
          return &i_Entry;
        }
      }

      return nullptr;
    }

    static bool same_bind_group_slot(const BindGroupEntry &p_Left,
                                     const BindGroupEntry &p_Right)
    {
      return p_Left.binding == p_Right.binding &&
             p_Left.array_element == p_Right.array_element &&
             p_Left.type == p_Right.type;
    }

    static void queue_pending_bind_group_entries(
        Detail::BackendBindGroup &p_BindGroup,
        Util::Span<const BindGroupEntry> p_Entries)
    {
      for (const BindGroupEntry &i_Entry : p_Entries) {
        bool l_Replaced = false;
        for (BindGroupEntry &i_Pending :
             p_BindGroup.pending_entries) {
          if (same_bind_group_slot(i_Pending, i_Entry)) {
            i_Pending = i_Entry;
            l_Replaced = true;
            break;
          }
        }

        if (!l_Replaced) {
          p_BindGroup.pending_entries.push_back(i_Entry);
        }
      }
    }

    static bool command_list_has_bind_group(
        const Detail::BackendCommandList &p_CommandList,
        BindGroup p_BindGroup)
    {
      for (BindGroup i_BindGroup : p_CommandList.used_bind_groups) {
        if (i_BindGroup == p_BindGroup) {
          return true;
        }
      }

      return false;
    }

    static void validate_bind_group_entries(
        Detail::ContextImpl &p_Context,
        const Detail::BackendBindGroupLayout &p_Layout,
        Util::Span<const BindGroupEntry> p_Entries)
    {
      for (const BindGroupEntry &i_Entry : p_Entries) {
        const BindGroupLayoutEntry *i_LayoutEntry =
            find_layout_entry(p_Layout, i_Entry.binding);
        GFX_ASSERT(i_LayoutEntry,
                   "Bind group entry binding is not in the layout");
        GFX_ASSERT(i_LayoutEntry->type == i_Entry.type,
                   "Bind group entry type does not match layout");
        GFX_ASSERT(i_Entry.array_element < i_LayoutEntry->count,
                   "Bind group entry array element exceeds layout "
                   "count");

        if (i_Entry.type == DescriptorType::UniformBuffer ||
            i_Entry.type == DescriptorType::StorageBuffer) {
          Detail::BackendBuffer *i_Buffer =
              p_Context.buffers.get(i_Entry.buffer.buffer);
          GFX_ASSERT(i_Buffer,
                     "Bind group buffer entry references invalid "
                     "buffer");
          GFX_ASSERT(i_Entry.buffer.offset < i_Buffer->size,
                     "Bind group buffer offset exceeds buffer size");
          if (i_Entry.buffer.range != LOW_UINT64_MAX) {
            GFX_ASSERT(i_Entry.buffer.offset + i_Entry.buffer.range <=
                           i_Buffer->size,
                       "Bind group buffer range exceeds buffer size");
          }
        } else if (i_Entry.type == DescriptorType::SampledImage ||
                   i_Entry.type == DescriptorType::StorageImage) {
          GFX_ASSERT(
              p_Context.image_views.is_valid(i_Entry.image.view),
              "Bind group image entry references invalid image "
              "view");
        } else if (i_Entry.type == DescriptorType::Sampler) {
          GFX_ASSERT(p_Context.samplers.is_valid(i_Entry.sampler),
                     "Bind group sampler entry references invalid "
                     "sampler");
        } else if (i_Entry.type ==
                   DescriptorType::CombinedImageSampler) {
          GFX_ASSERT(
              p_Context.image_views.is_valid(i_Entry.image.view),
              "Combined image sampler entry references invalid "
              "image view");
          GFX_ASSERT(
              p_Context.samplers.is_valid(i_Entry.sampler),
              "Combined image sampler entry references invalid "
              "sampler");
        }
      }
    }

    static void
    release_frame_bind_group_usages(Detail::ContextImpl &p_Context,
                                    u32 p_FrameIndex)
    {
      GFX_ASSERT(p_FrameIndex <
                     p_Context.frame_bind_group_usages.size(),
                 "Cannot release bind group usages for invalid frame "
                 "index");

      Util::List<BindGroup> &l_Usages =
          p_Context.frame_bind_group_usages[p_FrameIndex];
      for (BindGroup i_BindGroup : l_Usages) {
        Detail::BackendBindGroup *i_BackendBindGroup =
            p_Context.bind_groups.get(i_BindGroup);
        if (!i_BackendBindGroup) {
          continue;
        }

        GFX_ASSERT(i_BackendBindGroup->in_use_count > 0,
                   "Bind group usage tracking underflow");
        --i_BackendBindGroup->in_use_count;

        if (i_BackendBindGroup->in_use_count == 0 &&
            !i_BackendBindGroup->pending_entries.empty()) {
          p_Context.api->update_bind_group(
              p_Context, *i_BackendBindGroup,
              Util::Span<const BindGroupEntry>(
                  i_BackendBindGroup->pending_entries.data(),
                  i_BackendBindGroup->pending_entries.size()));
          i_BackendBindGroup->pending_entries.clear();
        }
      }

      l_Usages.clear();
    }

    BindGroup Context::create_bind_group(const BindGroupDesc &p_Desc)
    {
      Detail::BackendBindGroupLayout *l_Layout =
          m_Impl->bind_group_layouts.get(p_Desc.layout);
      GFX_ASSERT(l_Layout,
                 "Cannot create bind group with invalid layout");

      validate_bind_group_entries(*m_Impl, *l_Layout, p_Desc.entries);

      Detail::BackendBindGroup l_BackendBindGroup =
          m_Impl->api->create_bind_group(*m_Impl, p_Desc);
      return m_Impl->bind_groups.create(
          std::move(l_BackendBindGroup));
    }

    void Context::update_bind_group(
        BindGroup p_BindGroup,
        Util::Span<const BindGroupEntry> p_Entries)
    {
      Detail::BackendBindGroup *l_BindGroup =
          m_Impl->bind_groups.get(p_BindGroup);
      GFX_ASSERT(l_BindGroup, "Cannot update invalid bind group");

      Detail::BackendBindGroupLayout *l_Layout =
          m_Impl->bind_group_layouts.get(l_BindGroup->layout);
      GFX_ASSERT(l_Layout,
                 "Cannot update bind group with invalid layout");

      validate_bind_group_entries(*m_Impl, *l_Layout, p_Entries);
      if (l_BindGroup->in_use_count > 0) {
        queue_pending_bind_group_entries(*l_BindGroup, p_Entries);
      } else {
        m_Impl->api->update_bind_group(*m_Impl, *l_BindGroup,
                                       p_Entries);
      }
    }

    void Context::destroy(BindGroup p_BindGroup)
    {
      Detail::BackendBindGroup *l_BindGroup =
          m_Impl->bind_groups.get(p_BindGroup);
      if (!l_BindGroup) {
        return;
      }

      GFX_ASSERT(l_BindGroup->in_use_count == 0,
                 "Cannot destroy bind group while it is in use");

      m_Impl->api->destroy_bind_group(*m_Impl, *l_BindGroup);
      m_Impl->bind_groups.destroy(p_BindGroup);
    }

    bool Context::is_valid(BindGroup p_BindGroup) const
    {
      return m_Impl->bind_groups.is_valid(p_BindGroup);
    }

    GraphicsPipeline Context::create_graphics_pipeline(
        const GraphicsPipelineDesc &p_Desc)
    {
      GFX_ASSERT(m_Impl->pipeline_layouts.is_valid(p_Desc.layout),
                 "Cannot create graphics pipeline with invalid "
                 "pipeline layout");
      GFX_ASSERT(!p_Desc.shaders.empty(),
                 "Cannot create graphics pipeline without shaders");
      GFX_ASSERT(!p_Desc.color_targets.empty() ||
                     p_Desc.depth_format != ImageFormat::Undefined,
                 "Graphics pipeline must declare at least one color "
                 "target or a depth format");

      bool l_HasVertexShader = false;
      bool l_HasFragmentShader = false;
      for (const ShaderStageDesc &i_Shader : p_Desc.shaders) {
        GFX_ASSERT(i_Shader.stage == ShaderStage::Vertex ||
                       i_Shader.stage == ShaderStage::Fragment,
                   "Graphics pipelines only support vertex and "
                   "fragment shader stages");
        GFX_ASSERT(
            m_Impl->shader_modules.is_valid(i_Shader.module),
            "Graphics pipeline shader stage references invalid "
            "shader module");
        GFX_ASSERT(i_Shader.entry_point && i_Shader.entry_point[0],
                   "Graphics pipeline shader stage needs an entry "
                   "point");
        l_HasVertexShader |=
            has_shader_stage(i_Shader.stage, ShaderStage::Vertex);
        l_HasFragmentShader |=
            has_shader_stage(i_Shader.stage, ShaderStage::Fragment);
      }
      GFX_ASSERT(l_HasVertexShader,
                 "Graphics pipeline requires a vertex shader");
      GFX_ASSERT(l_HasFragmentShader || p_Desc.color_targets.empty(),
                 "Graphics pipeline with color targets requires a "
                 "fragment shader");

      for (const VertexBufferLayoutDesc &i_Buffer :
           p_Desc.vertex_buffers) {
        GFX_ASSERT(i_Buffer.stride > 0,
                   "Vertex buffer layout stride must be non-zero");
      }
      for (const ColorTargetDesc &i_Target : p_Desc.color_targets) {
        GFX_ASSERT(i_Target.format != ImageFormat::Undefined,
                   "Color target format must be defined");
        GFX_ASSERT(!is_depth_format(i_Target.format),
                   "Color target cannot use a depth format");
      }
      if (p_Desc.depth_format != ImageFormat::Undefined) {
        GFX_ASSERT(is_depth_format(p_Desc.depth_format),
                   "Graphics pipeline depth format must be a depth "
                   "format");
      }

      Detail::BackendGraphicsPipeline l_BackendPipeline =
          m_Impl->api->create_graphics_pipeline(*m_Impl, p_Desc);
      return m_Impl->graphics_pipelines.create(
          std::move(l_BackendPipeline));
    }

    void Context::destroy(GraphicsPipeline p_GraphicsPipeline)
    {
      Detail::BackendGraphicsPipeline *l_Pipeline =
          m_Impl->graphics_pipelines.get(p_GraphicsPipeline);
      if (!l_Pipeline) {
        return;
      }

      m_Impl->api->destroy_graphics_pipeline(*m_Impl, *l_Pipeline);
      m_Impl->graphics_pipelines.destroy(p_GraphicsPipeline);
    }

    bool Context::is_valid(GraphicsPipeline p_GraphicsPipeline) const
    {
      return m_Impl->graphics_pipelines.is_valid(p_GraphicsPipeline);
    }

    ComputePipeline Context::create_compute_pipeline(
        const ComputePipelineDesc &p_Desc)
    {
      GFX_ASSERT(m_Impl->pipeline_layouts.is_valid(p_Desc.layout),
                 "Cannot create compute pipeline with invalid "
                 "pipeline layout");
      GFX_ASSERT(p_Desc.shader.stage == ShaderStage::Compute,
                 "Compute pipeline shader stage must be Compute");
      GFX_ASSERT(m_Impl->shader_modules.is_valid(p_Desc.shader.module),
                 "Compute pipeline shader references invalid shader "
                 "module");
      GFX_ASSERT(p_Desc.shader.entry_point &&
                     p_Desc.shader.entry_point[0],
                 "Compute pipeline shader stage needs an entry point");

      Detail::BackendComputePipeline l_BackendPipeline =
          m_Impl->api->create_compute_pipeline(*m_Impl, p_Desc);
      return m_Impl->compute_pipelines.create(
          std::move(l_BackendPipeline));
    }

    void Context::destroy(ComputePipeline p_ComputePipeline)
    {
      Detail::BackendComputePipeline *l_Pipeline =
          m_Impl->compute_pipelines.get(p_ComputePipeline);
      if (!l_Pipeline) {
        return;
      }

      m_Impl->api->destroy_compute_pipeline(*m_Impl, *l_Pipeline);
      m_Impl->compute_pipelines.destroy(p_ComputePipeline);
    }

    bool Context::is_valid(ComputePipeline p_ComputePipeline) const
    {
      return m_Impl->compute_pipelines.is_valid(p_ComputePipeline);
    }

    CommandList
    Context::request_command_list(const FrameContext &p_Frame,
                                  const QueueRole p_QueueRole)
    {
      GFX_ASSERT(m_Impl->frame_active,
                 "Cannot request a frame command list before "
                 "begin_frame");
      GFX_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot request a command list with a stale frame "
                 "context");

      Detail::BackendCommandList l_BackendCommandList =
          m_Impl->api->request_command_list(*m_Impl, p_Frame,
                                            p_QueueRole);
      CommandList l_CommandList = m_Impl->command_lists.create(
          std::move(l_BackendCommandList));
      m_Impl->frame_command_lists.push_back(l_CommandList);
      return l_CommandList;
    }

    CommandList Context::request_immediate_command_list(
        const QueueRole p_QueueRole)
    {
      Detail::BackendCommandList l_BackendCommandList =
          m_Impl->api->request_immediate_command_list(*m_Impl,
                                                      p_QueueRole);
      return m_Impl->command_lists.create(
          std::move(l_BackendCommandList));
    }

    void Context::destroy(CommandList p_CommandList)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      if (!l_CommandList) {
        return;
      }

      GFX_ASSERT(l_CommandList->state != CommandListState::Submitted,
                 "Cannot destroy a submitted command list");

      m_Impl->api->destroy_command_list(*m_Impl, *l_CommandList);
      m_Impl->command_lists.destroy(p_CommandList);
    }

    bool Context::is_valid(CommandList p_CommandList) const
    {
      return m_Impl->command_lists.is_valid(p_CommandList);
    }

    Swapchain Context::create_swapchain(const SwapchainDesc &p_Desc)
    {
      GFX_ASSERT(p_Desc.width > 0,
                 "Cannot create zero-width swapchain");
      GFX_ASSERT(p_Desc.height > 0,
                 "Cannot create zero-height swapchain");
      GFX_ASSERT(
          m_Impl->instance &&
              m_Impl->instance->surfaces.is_valid(p_Desc.surface),
          "Cannot create swapchain from invalid surface");

      Detail::BackendSwapchain l_BackendSwapchain =
          m_Impl->api->create_swapchain(*m_Impl, p_Desc);
      return m_Impl->swapchains.create(std::move(l_BackendSwapchain));
    }

    void Context::destroy(Swapchain p_Swapchain)
    {
      Detail::BackendSwapchain *l_Swapchain =
          m_Impl->swapchains.get(p_Swapchain);
      if (!l_Swapchain) {
        return;
      }

      GFX_ASSERT(!m_Impl->frame_active,
                 "Cannot destroy swapchain while a frame is active");

      m_Impl->api->destroy_swapchain(*m_Impl, *l_Swapchain);
      m_Impl->swapchains.destroy(p_Swapchain);
    }

    bool Context::is_valid(Swapchain p_Swapchain) const
    {
      return m_Impl->swapchains.is_valid(p_Swapchain);
    }

    FrameContext Context::begin_frame()
    {
      GFX_ASSERT(
          !m_Impl->frame_active,
          "Cannot begin a frame while another frame is active");

      FrameContext l_Frame;
      l_Frame.m_FrameIndex = m_Impl->frame_index;
      l_Frame.m_FrameNumber = m_Impl->frame_number;

      m_Impl->api->begin_frame(*m_Impl, l_Frame);
      release_frame_bind_group_usages(*m_Impl, l_Frame.m_FrameIndex);
      m_Impl->frame_active = true;

      return l_Frame;
    }

    SwapchainFrame
    Context::acquire_swapchain(const FrameContext &p_Frame,
                               Swapchain p_Swapchain)
    {
      GFX_ASSERT(m_Impl->frame_active,
                 "Cannot acquire swapchain before begin_frame");
      GFX_ASSERT(
          p_Frame.m_FrameIndex == m_Impl->frame_index &&
              p_Frame.m_FrameNumber == m_Impl->frame_number,
          "Cannot acquire swapchain with a stale frame context");
      GFX_ASSERT(m_Impl->swapchains.is_valid(p_Swapchain),
                 "Cannot acquire invalid swapchain");

      SwapchainFrame l_SwapchainFrame;
      l_SwapchainFrame.m_FrameIndex = p_Frame.m_FrameIndex;
      l_SwapchainFrame.m_FrameNumber = p_Frame.m_FrameNumber;
      l_SwapchainFrame.m_Swapchain = p_Swapchain;

      m_Impl->api->acquire_swapchain(*m_Impl, p_Frame,
                                     l_SwapchainFrame);
      return l_SwapchainFrame;
    }

    void Context::present(const SwapchainFrame &p_SwapchainFrame)
    {
      GFX_ASSERT(m_Impl->frame_active,
                 "Cannot present swapchain before begin_frame");
      GFX_ASSERT(
          p_SwapchainFrame.m_FrameIndex == m_Impl->frame_index &&
              p_SwapchainFrame.m_FrameNumber == m_Impl->frame_number,
          "Cannot present swapchain with a stale swapchain frame");
      GFX_ASSERT(
          m_Impl->swapchains.is_valid(p_SwapchainFrame.m_Swapchain),
          "Cannot present invalid swapchain");

      m_Impl->api->present(*m_Impl, p_SwapchainFrame);
    }

    void Context::end_frame(const FrameContext &p_Frame)
    {
      GFX_ASSERT(m_Impl->frame_active,
                 "Cannot end a frame before begin_frame");
      GFX_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot end frame with a stale frame context");

      m_Impl->api->end_frame(*m_Impl, p_Frame);

      for (CommandList i_CommandList : m_Impl->frame_command_lists) {
        Detail::BackendCommandList *l_CommandList =
            m_Impl->command_lists.get(i_CommandList);
        if (l_CommandList) {
          m_Impl->api->destroy_command_list(*m_Impl, *l_CommandList);
          m_Impl->command_lists.destroy(i_CommandList);
        }
      }
      m_Impl->frame_command_lists.clear();

      m_Impl->frame_active = false;

      m_Impl->frame_number++;
      m_Impl->frame_index =
          (m_Impl->frame_index + 1) % m_Impl->frames_in_flight;
    }

    void Context::begin(CommandList p_CommandList)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList, "Cannot begin invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Initial,
                 "Can only begin command lists in initial state");

      m_Impl->api->begin_command_list(*m_Impl, *l_CommandList);
      l_CommandList->state = CommandListState::Recording;
    }

    void Context::end(CommandList p_CommandList)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList, "Cannot end invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only end command lists in recording state");
      GFX_ASSERT(
          !l_CommandList->rendering_active,
          "Cannot end command list that is still in rendering state");

      m_Impl->api->end_command_list(*m_Impl, *l_CommandList);
      l_CommandList->state = CommandListState::Executable;
    }

    void Context::submit(const FrameContext &p_Frame,
                         CommandList p_CommandList)
    {
      GFX_ASSERT(m_Impl->frame_active,
                 "Cannot submit a command list before begin_frame");
      GFX_ASSERT(p_Frame.m_FrameIndex == m_Impl->frame_index &&
                     p_Frame.m_FrameNumber == m_Impl->frame_number,
                 "Cannot submit a command list with a stale frame "
                 "context");

      bool l_IsFrameCommandList = false;
      for (CommandList i_CommandList : m_Impl->frame_command_lists) {
        if (i_CommandList == p_CommandList) {
          l_IsFrameCommandList = true;
          break;
        }
      }
      GFX_ASSERT(
          l_IsFrameCommandList,
          "Cannot submit an immediate command list through frame "
          "submission");

      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList, "Cannot submit invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Executable,
                 "Can only submit executable command lists");
      GFX_ASSERT(
          l_CommandList->queue_role == QueueRole::Graphics,
          "Frame command-list submission currently supports only "
          "graphics command lists");

      m_Impl->api->submit_command_list(*m_Impl, p_Frame,
                                       *l_CommandList);

      for (BindGroup i_BindGroup : l_CommandList->used_bind_groups) {
        Detail::BackendBindGroup *i_BackendBindGroup =
            m_Impl->bind_groups.get(i_BindGroup);
        GFX_ASSERT(i_BackendBindGroup,
                   "Command list references invalid bind group");
        ++i_BackendBindGroup->in_use_count;
        m_Impl->frame_bind_group_usages[p_Frame.m_FrameIndex]
            .push_back(i_BindGroup);
      }

      l_CommandList->state = CommandListState::Submitted;
    }

    void Context::barrier(CommandList p_CommandList,
                          const ImageBarrier &p_Barrier)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot record barrier to invalid command list");
      GFX_ASSERT(
          l_CommandList->state == CommandListState::Recording,
          "Can only record barriers to recording command lists");

      Detail::BackendImage *l_Image =
          m_Impl->images.get(p_Barrier.image);
      GFX_ASSERT(l_Image, "Cannot record barrier for invalid image");
      GFX_ASSERT(p_Barrier.mip_count > 0,
                 "Cannot record image barrier without mip levels");
      GFX_ASSERT(p_Barrier.layer_count > 0,
                 "Cannot record image barrier without array layers");
      GFX_ASSERT(p_Barrier.base_mip + p_Barrier.mip_count <=
                     l_Image->mip_levels,
                 "Image barrier mip range exceeds image mip levels");
      GFX_ASSERT(
          p_Barrier.base_layer + p_Barrier.layer_count <=
              l_Image->array_layers,
          "Image barrier layer range exceeds image array layers");

      if (is_depth_format(l_Image->format)) {
        GFX_ASSERT(p_Barrier.aspect != ImageAspect::Color,
                   "Cannot record color barrier for depth image");
        if (p_Barrier.aspect == ImageAspect::Stencil ||
            p_Barrier.aspect == ImageAspect::DepthStencil) {
          GFX_ASSERT(has_stencil(l_Image->format),
                     "Cannot record stencil barrier for depth-only "
                     "image");
        }
      } else {
        GFX_ASSERT(p_Barrier.aspect == ImageAspect::Color,
                   "Cannot record depth/stencil barrier for color "
                   "image");
      }

      const bool l_CoversWholeImage =
          p_Barrier.base_mip == 0 &&
          p_Barrier.mip_count == l_Image->mip_levels &&
          p_Barrier.base_layer == 0 &&
          p_Barrier.layer_count == l_Image->array_layers;
      if (l_CoversWholeImage) {
        GFX_ASSERT(l_Image->state == p_Barrier.old_state,
                   "Image barrier old state does not match tracked "
                   "image state");
      }

      m_Impl->api->barrier_image_command_list(*m_Impl, *l_CommandList,
                                              p_Barrier);

      if (l_CoversWholeImage) {
        l_Image->state = p_Barrier.new_state;
      }
    }

    void
    Context::begin_rendering(CommandList p_CommandList,
                             const RenderingInfo &p_RenderingInfo)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot begin rendering to invalid command list");
      GFX_ASSERT(
          l_CommandList->state == CommandListState::Recording,
          "Can only begin rendering to recording command lists");
      GFX_ASSERT(!l_CommandList->rendering_active,
                 "Can not begin rendering command list that is "
                 "already rendering.");

      GFX_ASSERT(p_RenderingInfo.extent.x > 0 &&
                     p_RenderingInfo.extent.y > 0,
                 "Can only begin rendering if the extent of the "
                 "rendered-to area is non-zero");
      GFX_ASSERT(!p_RenderingInfo.color_attachments.empty() ||
                     p_RenderingInfo.depth_attachment,
                 "Cannot begin rendering without color or depth "
                 "attachments");

      for (const ColorAttachmentDesc &i_CA :
           p_RenderingInfo.color_attachments) {
        Detail::BackendImageView *i_ImageView =
            m_Impl->image_views.get(i_CA.view);
        GFX_ASSERT(i_ImageView,
                   "Cannot begin rendering to invalid image view");
        Detail::BackendImage *i_Image =
            m_Impl->images.get(i_ImageView->image);
        GFX_ASSERT(
            i_Image,
            "Cannot begin rendering to image view with invalid "
            "image");
        GFX_ASSERT(
            i_CA.state == ImageState::ColorAttachment,
            "Color attachments must use ColorAttachment state");
        GFX_ASSERT(i_ImageView->aspect == ImageAspect::Color,
                   "Color attachments require color image views");
        GFX_ASSERT(!is_depth_format(i_ImageView->format),
                   "Color attachments cannot use depth formats");
        GFX_ASSERT(has_image_usage(i_Image->usage,
                                   ImageUsage::ColorAttachment),
                   "Color attachment image was not created with "
                   "ColorAttachment usage");
        GFX_ASSERT(i_ImageView->layer_count == 1,
                   "Layered rendering is not supported by "
                   "RenderingInfo yet");

        const u32 l_ViewWidth =
            mip_extent(i_Image->extent.x, i_ImageView->base_mip);
        const u32 l_ViewHeight =
            mip_extent(i_Image->extent.y, i_ImageView->base_mip);
        GFX_ASSERT(p_RenderingInfo.extent.x <= l_ViewWidth &&
                       p_RenderingInfo.extent.y <= l_ViewHeight,
                   "Rendering extent exceeds color attachment view "
                   "extent");

        if (view_covers_whole_image(*i_Image, *i_ImageView)) {
          GFX_ASSERT(i_Image->state == i_CA.state,
                     "Color attachment state does not match tracked "
                     "image state");
        }
      }

      if (p_RenderingInfo.depth_attachment) {
        Detail::BackendImageView *l_DepthImageView =
            m_Impl->image_views.get(
                p_RenderingInfo.depth_attachment->view);
        GFX_ASSERT(l_DepthImageView,
                   "Cannot begin rendering with invalid depth image "
                   "view");
        Detail::BackendImage *l_DepthImage =
            m_Impl->images.get(l_DepthImageView->image);
        GFX_ASSERT(
            l_DepthImage,
            "Cannot begin rendering to depth image view with invalid "
            "image");
        GFX_ASSERT(
            p_RenderingInfo.depth_attachment->state ==
                    ImageState::DepthWrite ||
                p_RenderingInfo.depth_attachment->state ==
                    ImageState::DepthRead,
            "Depth attachments must use DepthWrite or DepthRead "
            "state");
        GFX_ASSERT(l_DepthImageView->aspect == ImageAspect::Depth,
                   "Depth attachments require depth image views");
        GFX_ASSERT(is_depth_format(l_DepthImageView->format),
                   "Depth attachments require depth formats");
        GFX_ASSERT(
            has_image_usage(l_DepthImage->usage,
                            ImageUsage::DepthStencilAttachment),
            "Depth attachment image was not created with "
            "DepthStencilAttachment usage");
        GFX_ASSERT(l_DepthImageView->layer_count == 1,
                   "Layered rendering is not supported by "
                   "RenderingInfo yet");

        const u32 l_ViewWidth = mip_extent(
            l_DepthImage->extent.x, l_DepthImageView->base_mip);
        const u32 l_ViewHeight = mip_extent(
            l_DepthImage->extent.y, l_DepthImageView->base_mip);
        GFX_ASSERT(p_RenderingInfo.extent.x <= l_ViewWidth &&
                       p_RenderingInfo.extent.y <= l_ViewHeight,
                   "Rendering extent exceeds depth attachment view "
                   "extent");

        if (view_covers_whole_image(*l_DepthImage,
                                    *l_DepthImageView)) {
          GFX_ASSERT(
              l_DepthImage->state ==
                  p_RenderingInfo.depth_attachment->state,
              "Depth attachment state does not match tracked image "
              "state");
        }
      }

      m_Impl->api->begin_dynamic_rendering(*m_Impl, *l_CommandList,
                                           p_RenderingInfo);
      l_CommandList->rendering_active = true;
      l_CommandList->rendering_extent = p_RenderingInfo.extent;
    }

    void Context::end_rendering(CommandList p_CommandList)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot end rendering to invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only end rendering to recording command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can not end rendering command list that is "
                 "not rendering.");

      m_Impl->api->end_dynamic_rendering(*m_Impl, *l_CommandList);
      l_CommandList->rendering_active = false;
      l_CommandList->rendering_extent = Math::UVector2{0, 0};
    }

    void Context::set_viewport(CommandList p_CommandList,
                               const Viewport &p_Viewport)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot set viewport on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only set viewport on recording command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only set viewport inside an active rendering "
                 "scope");
      GFX_ASSERT(p_Viewport.x >= 0.0f && p_Viewport.y >= 0.0f,
                 "Viewport offset must be non-negative");
      GFX_ASSERT(p_Viewport.width > 0.0f && p_Viewport.height > 0.0f,
                 "Viewport extent must be non-zero");
      GFX_ASSERT(p_Viewport.min_depth >= 0.0f &&
                     p_Viewport.min_depth <= 1.0f &&
                     p_Viewport.max_depth >= 0.0f &&
                     p_Viewport.max_depth <= 1.0f &&
                     p_Viewport.min_depth <= p_Viewport.max_depth,
                 "Viewport depth range must be ordered and inside "
                 "[0, 1]");
      GFX_ASSERT(p_Viewport.x + p_Viewport.width <=
                         l_CommandList->rendering_extent.x &&
                     p_Viewport.y + p_Viewport.height <=
                         l_CommandList->rendering_extent.y,
                 "Viewport exceeds active rendering extent");

      m_Impl->api->set_viewport(*m_Impl, *l_CommandList, p_Viewport);
    }

    void Context::set_scissor(CommandList p_CommandList,
                              const Rect2D &p_Scissor)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot set scissor on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only set scissor on recording command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only set scissor inside an active rendering "
                 "scope");
      GFX_ASSERT(p_Scissor.offset.x >= 0 && p_Scissor.offset.y >= 0,
                 "Scissor offset must be non-negative");
      GFX_ASSERT(p_Scissor.extent.x > 0 && p_Scissor.extent.y > 0,
                 "Scissor extent must be non-zero");

      const u64 l_ScissorMaxX = static_cast<u64>(p_Scissor.offset.x) +
                                static_cast<u64>(p_Scissor.extent.x);
      const u64 l_ScissorMaxY = static_cast<u64>(p_Scissor.offset.y) +
                                static_cast<u64>(p_Scissor.extent.y);
      GFX_ASSERT(l_ScissorMaxX <= l_CommandList->rendering_extent.x &&
                     l_ScissorMaxY <=
                         l_CommandList->rendering_extent.y,
                 "Scissor exceeds active rendering extent");

      m_Impl->api->set_scissor(*m_Impl, *l_CommandList, p_Scissor);
    }

    void Context::bind_graphics_pipeline(
        CommandList p_CommandList,
        GraphicsPipeline p_GraphicsPipeline)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot bind graphics pipeline on invalid command "
                 "list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only bind graphics pipeline on recording "
                 "command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only bind graphics pipeline inside an active "
                 "rendering scope");

      Detail::BackendGraphicsPipeline *l_Pipeline =
          m_Impl->graphics_pipelines.get(p_GraphicsPipeline);
      GFX_ASSERT(l_Pipeline, "Cannot bind invalid graphics pipeline");

      m_Impl->api->bind_graphics_pipeline(*m_Impl, *l_CommandList,
                                          *l_Pipeline);
    }

    void Context::bind_compute_pipeline(
        CommandList p_CommandList, ComputePipeline p_ComputePipeline)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot bind compute pipeline on invalid command "
                 "list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only bind compute pipeline on recording "
                 "command lists");
      GFX_ASSERT(!l_CommandList->rendering_active,
                 "Cannot bind compute pipeline inside an active "
                 "rendering scope");
      GFX_ASSERT(l_CommandList->queue_role == QueueRole::Graphics ||
                     l_CommandList->queue_role == QueueRole::Compute,
                 "Compute pipelines require a graphics or compute "
                 "command list");

      Detail::BackendComputePipeline *l_Pipeline =
          m_Impl->compute_pipelines.get(p_ComputePipeline);
      GFX_ASSERT(l_Pipeline, "Cannot bind invalid compute pipeline");

      m_Impl->api->bind_compute_pipeline(*m_Impl, *l_CommandList,
                                         *l_Pipeline);
    }

    void Context::bind_bind_group(CommandList p_CommandList,
                                  PipelineLayout p_PipelineLayout,
                                  u32 p_GroupIndex,
                                  BindGroup p_BindGroup)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot bind bind group on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only bind bind groups on recording command "
                 "lists");

      Detail::BackendPipelineLayout *l_PipelineLayout =
          m_Impl->pipeline_layouts.get(p_PipelineLayout);
      GFX_ASSERT(l_PipelineLayout,
                 "Cannot bind bind group with invalid pipeline "
                 "layout");
      GFX_ASSERT(p_GroupIndex <
                     l_PipelineLayout->bind_group_layouts.size(),
                 "Bind group index exceeds pipeline layout bind "
                 "group count");

      Detail::BackendBindGroup *l_BindGroup =
          m_Impl->bind_groups.get(p_BindGroup);
      GFX_ASSERT(l_BindGroup, "Cannot bind invalid bind group");
      GFX_ASSERT(
          l_BindGroup->layout ==
              l_PipelineLayout->bind_group_layouts[p_GroupIndex],
          "Bind group layout does not match pipeline layout "
          "slot");

      m_Impl->api->bind_bind_group(*m_Impl, *l_CommandList,
                                   *l_PipelineLayout, p_GroupIndex,
                                   *l_BindGroup);

      if (!command_list_has_bind_group(*l_CommandList, p_BindGroup)) {
        l_CommandList->used_bind_groups.push_back(p_BindGroup);
      }
    }

    void Context::bind_vertex_buffer(CommandList p_CommandList,
                                     u32 p_Binding, Buffer p_Buffer,
                                     u64 p_Offset)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot bind vertex buffer on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only bind vertex buffers on recording command "
                 "lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only bind vertex buffers inside an active "
                 "rendering scope");

      Detail::BackendBuffer *l_Buffer =
          m_Impl->buffers.get(p_Buffer);
      GFX_ASSERT(l_Buffer, "Cannot bind invalid vertex buffer");
      GFX_ASSERT(has_buffer_usage(l_Buffer->usage,
                                  BufferUsage::Vertex),
                 "Buffer was not created with Vertex usage");
      GFX_ASSERT(p_Offset < l_Buffer->size,
                 "Vertex buffer offset exceeds buffer size");

      m_Impl->api->bind_vertex_buffer(*m_Impl, *l_CommandList,
                                      p_Binding, *l_Buffer,
                                      p_Offset);
    }

    void Context::bind_index_buffer(CommandList p_CommandList,
                                    Buffer p_Buffer, u64 p_Offset,
                                    IndexType p_IndexType)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot bind index buffer on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only bind index buffers on recording command "
                 "lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only bind index buffers inside an active "
                 "rendering scope");

      Detail::BackendBuffer *l_Buffer =
          m_Impl->buffers.get(p_Buffer);
      GFX_ASSERT(l_Buffer, "Cannot bind invalid index buffer");
      GFX_ASSERT(has_buffer_usage(l_Buffer->usage,
                                  BufferUsage::Index),
                 "Buffer was not created with Index usage");
      GFX_ASSERT(p_Offset < l_Buffer->size,
                 "Index buffer offset exceeds buffer size");
      GFX_ASSERT(p_IndexType == IndexType::UInt16 ||
                     p_IndexType == IndexType::UInt32,
                 "Unsupported index type");

      m_Impl->api->bind_index_buffer(*m_Impl, *l_CommandList,
                                     *l_Buffer, p_Offset,
                                     p_IndexType);
    }

    void Context::draw(CommandList p_CommandList, u32 p_VertexCount,
                       u32 p_InstanceCount, u32 p_FirstVertex,
                       u32 p_FirstInstance)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot draw on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only draw on recording command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only draw inside an active rendering scope");
      GFX_ASSERT(p_VertexCount > 0, "Draw vertex count is zero");
      GFX_ASSERT(p_InstanceCount > 0, "Draw instance count is zero");

      m_Impl->api->draw(*m_Impl, *l_CommandList, p_VertexCount,
                        p_InstanceCount, p_FirstVertex,
                        p_FirstInstance);
    }

    void Context::draw_indexed(CommandList p_CommandList,
                               u32 p_IndexCount,
                               u32 p_InstanceCount,
                               u32 p_FirstIndex,
                               i32 p_VertexOffset,
                               u32 p_FirstInstance)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot draw indexed on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only draw indexed on recording command lists");
      GFX_ASSERT(l_CommandList->rendering_active,
                 "Can only draw indexed inside an active rendering "
                 "scope");
      GFX_ASSERT(p_IndexCount > 0,
                 "Indexed draw index count is zero");
      GFX_ASSERT(p_InstanceCount > 0,
                 "Indexed draw instance count is zero");

      m_Impl->api->draw_indexed(*m_Impl, *l_CommandList,
                                p_IndexCount, p_InstanceCount,
                                p_FirstIndex, p_VertexOffset,
                                p_FirstInstance);
    }

    void Context::dispatch(CommandList p_CommandList,
                           u32 p_GroupCountX, u32 p_GroupCountY,
                           u32 p_GroupCountZ)
    {
      Detail::BackendCommandList *l_CommandList =
          m_Impl->command_lists.get(p_CommandList);
      GFX_ASSERT(l_CommandList,
                 "Cannot dispatch on invalid command list");
      GFX_ASSERT(l_CommandList->state == CommandListState::Recording,
                 "Can only dispatch on recording command lists");
      GFX_ASSERT(!l_CommandList->rendering_active,
                 "Cannot dispatch inside an active rendering scope");
      GFX_ASSERT(l_CommandList->queue_role == QueueRole::Graphics ||
                     l_CommandList->queue_role == QueueRole::Compute,
                 "Dispatch requires a graphics or compute command "
                 "list");
      GFX_ASSERT(p_GroupCountX > 0 && p_GroupCountY > 0 &&
                     p_GroupCountZ > 0,
                 "Dispatch group counts must be non-zero");

      m_Impl->api->dispatch(*m_Impl, *l_CommandList, p_GroupCountX,
                            p_GroupCountY, p_GroupCountZ);
    }
  } // namespace Gfx
} // namespace Low
