#pragma once

#include "LowRendererVulkan.h"
#include "LowRendererWindow.h"

#include <stdint.h>

#include "LowUtilName.h"
#include "vulkan/vulkan_core.h"

#define LOW_RENDERER_BACKEND_MAX_COMMITTED_PRS 4

#define LOW_RENDERER_BEGIN_RENDERDOC_SECTION(ctx, label, color)                \
  Backend::callbacks().renderdoc_section_begin(ctx, label, color)

#define LOW_RENDERER_END_RENDERDOC_SECTION(ctx)                                \
  Backend::callbacks().renderdoc_section_end(ctx)

namespace Low {
  namespace Renderer {
    namespace Resource {
      struct Image;
      struct Buffer;
    } // namespace Resource
    namespace Backend {
      namespace ImageFormat {
        enum Enum
        {
          BGRA8_SRGB,
          BGRA8_UNORM,
          RGBA32_SFLOAT,
          RGBA16_SFLOAT,
          RGBA8_UNORM,
          R8_UNORM,
          R32_UINT,
          D32_SFLOAT,
          D32_SFLOAT_S8_UINT,
          D24_UNORM_S8_UINT
        };
      }

      namespace ImageSampleFilter {
        enum Enum
        {
          LINEAR,
          CUBIC
        };
      }

      uint32_t imageformat_get_pipeline_write_mask(uint8_t p_Format);

      struct Context;
      struct ImageResource;
      struct PipelineResourceSignature;
      struct Buffer;

      struct Renderpass
      {
        union
        {
          Vulkan::Renderpass vk;
        };
        Context *context;
        ImageResource *renderTargets;
        uint8_t renderTargetCount;
        Math::Color *clearTargetColor;
        bool useDepth;
        uint64_t depthRenderTargetHandleId;
        Math::Vector2 clearDepthColor;
        Math::UVector2 dimensions;
        bool swapchainRenderpass;
      };

      struct Context
      {
        union
        {
          Vulkan::Context vk;
        };
        Window window;
        uint8_t framesInFlight;
        uint8_t imageCount;
        uint8_t currentFrameIndex;
        uint8_t currentImageIndex;
        uint8_t state;
        Math::UVector2 dimensions;
        uint8_t imageFormat;
        Renderpass *renderpasses;
        bool debugEnabled;
        bool running;
      };

      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
        uint8_t framesInFlight;
      };

      struct RenderpassCreateParams
      {
        Context *context;
        ImageResource *renderTargets;
        uint8_t renderTargetCount;
        Math::Color *clearTargetColor;
        bool useDepth;
        ImageResource *depthRenderTarget;
        Math::Vector2 clearDepthColor;
        Math::UVector2 dimensions;
      };

      namespace ResourcePipelineStep {
        enum Enum
        {
          GRAPHICS,
          COMPUTE,
          VERTEX,
          FRAGMENT,
          ALL
        };
      }

      namespace ResourceType {
        enum Enum
        {
          CONSTANT_BUFFER,
          BUFFER,
          SAMPLER,
          IMAGE,
          TEXTURE2D,
          UNBOUND_SAMPLER
        };
      };

      struct PipelineResourceDescription
      {
        Util::Name name;
        uint8_t type;
        uint8_t step;
        uint32_t arraySize;
      };

      struct PipelineResourceBinding
      {
        PipelineResourceDescription description;
        uint64_t *boundResourceHandleId;
      };

      struct PipelineResourceSignatureCreateParams
      {
        Context *context;
        PipelineResourceDescription *resourceDescriptions;
        uint32_t resourceDescriptionCount;
        uint8_t binding;
      };

      struct PipelineResourceSignature
      {
        union
        {
          Vulkan::PipelineResourceSignature vk;
        };
        PipelineResourceBinding *resources;
        uint32_t resourceCount;
        uint8_t binding;
        Context *context;
      };

      struct ImageResource
      {
        union
        {
          Vulkan::ImageResource vk;
        };
        Context *context;
        bool swapchainImage;
        uint8_t format;
        uint8_t sampleFilter;
        bool depth;
        Math::UVector2 dimensions;
        uint64_t handleId;
      };

      struct ImageResourceCreateParams
      {
        Context *context;
        Math::UVector2 dimensions;
        uint8_t format;
        uint8_t sampleFilter;
        bool writable;
        bool depth;
        bool createImage;
        void *imageData;
        size_t imageDataSize;
      };

#define LOW_RENDERER_BUFFER_USAGE_RESOURCE_CONSTANT 1
#define LOW_RENDERER_BUFFER_USAGE_RESOURCE_BUFFER 2
#define LOW_RENDERER_BUFFER_USAGE_VERTEX 4
#define LOW_RENDERER_BUFFER_USAGE_INDEX 8

      namespace BufferMemoryType {
        enum Enum
        {
          DEFAULT,
          HOST_VISIBLE
        };
      }

      namespace IndexBufferType {
        enum Enum
        {
          UINT8,
          UINT16,
          UINT32
        };
      }

      struct BufferCreateParams
      {
        Context *context;
        uint32_t usageFlags;
        uint32_t bufferSize;
        void *data;
        uint8_t memoryType = BufferMemoryType::DEFAULT;
      };

      struct Buffer
      {
        union
        {
          Vulkan::Buffer vk;
        };
        Context *context;
        uint32_t bufferSize;
        uint32_t usageFlags;
      };

      namespace ContextState {
        enum Enum
        {
          SUCCESS,
          FAILED,
          OUT_OF_DATE
        };
      }

      namespace PipelineRasterizerCullMode {
        enum Enum
        {
          BACK,
          FRONT
        };
      }

      namespace PipelineRasterizerFrontFace {
        enum Enum
        {
          CLOCKWISE,
          COUNTER_CLOCKWISE
        };
      }

      namespace PipelineRasterizerPolygonMode {
        enum Enum
        {
          FILL,
          LINE
        };
      }

      namespace PipelineType {
        enum Enum
        {
          GRAPHICS,
          COMPUTE
        };
      }

#define LOW_RENDERER_COLOR_WRITE_BIT_RED 1
#define LOW_RENDERER_COLOR_WRITE_BIT_GREEN 2
#define LOW_RENDERER_COLOR_WRITE_BIT_BLUE 4
#define LOW_RENDERER_COLOR_WRITE_BIT_ALPHA 8

      struct GraphicsPipelineColorTarget
      {
        bool blendEnable;
        uint32_t wirteMask;
      };

      namespace VertexAttributeType {
        enum Enum
        {
          VECTOR2,
          VECTOR3
        };
      }

      namespace CompareOperation {
        enum Enum
        {
          LESS,
          EQUAL
        };
      }

      struct Pipeline
      {
        union
        {
          Vulkan::Pipeline vk;
        };
        uint8_t type;
        Context *context;
      };

      struct PipelineComputeCreateParams
      {
        Context *context;
        const char *shaderPath;
        PipelineResourceSignature *signatures;
        uint8_t signatureCount;
      };

      struct PipelineGraphicsCreateParams
      {
        Context *context;
        const char *vertexShaderPath;
        const char *fragmentShaderPath;
        Math::UVector2 dimensions;
        PipelineResourceSignature *signatures;
        uint8_t signatureCount;
        uint8_t cullMode;
        uint8_t frontFace;
        uint8_t polygonMode;
        GraphicsPipelineColorTarget *colorTargets;
        uint8_t colorTargetCount;
        Renderpass *renderpass;
        bool depthWrite;
        bool depthTest;
        uint8_t depthCompareOperation;

        uint8_t *vertexDataAttributesType;
        uint32_t vertexDataAttributeCount;
      };

      struct DrawParams
      {
        Context *context;
        uint32_t vertexCount;
        uint32_t firstVertex;
      };

      struct DrawIndexedParams
      {
        Context *context;
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t instanceCount;
        uint32_t firstInstance;
      };

      struct DrawIndexedBatchEntry
      {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t instanceCount;
        uint32_t firstInstance;
      };

      struct DrawIndexedBatchParams
      {};

      struct ImGuiImage
      {
        union
        {
          Vulkan::ImGuiImage vk;
        };
        Context *context;
      };

      struct ApiBackendCallback
      {
        void (*context_create)(Context &, ContextCreateParams &);
        void (*context_cleanup)(Context &);
        void (*context_wait_idle)(Context &);
        void (*context_update_dimensions)(Context &);

        uint8_t (*frame_prepare)(Context &);
        void (*frame_render)(Context &);
        void (*imgui_prepare_frame)(Context &);
        void (*imgui_render)(Context &);

        void (*renderpass_create)(Renderpass &, RenderpassCreateParams &);
        void (*renderpass_cleanup)(Renderpass &);
        void (*renderpass_begin)(Renderpass &);
        void (*renderpass_end)(Renderpass &);

        void (*pipeline_resource_signature_create)(
            PipelineResourceSignature &,
            PipelineResourceSignatureCreateParams &);
        void (*pipeline_resource_signature_set_constant_buffer)(
            PipelineResourceSignature &, Util::Name, uint32_t,
            Resource::Buffer);
        void (*pipeline_resource_signature_set_buffer)(
            PipelineResourceSignature &, Util::Name, uint32_t,
            Resource::Buffer);
        void (*pipeline_resource_signature_set_image)(
            PipelineResourceSignature &, Util::Name, uint32_t, Resource::Image);
        void (*pipeline_resource_signature_set_sampler)(
            PipelineResourceSignature &, Util::Name, uint32_t, Resource::Image);
        void (*pipeline_resource_signature_set_unbound_sampler)(
            PipelineResourceSignature &, Util::Name, uint32_t, Resource::Image);
        void (*pipeline_resource_signature_set_texture2d)(
            PipelineResourceSignature &, Util::Name, uint32_t, Resource::Image);
        void (*pipeline_resource_signature_commit)(PipelineResourceSignature &);
        void (*pipeline_resource_signature_commit_clear)(Context &);

        void (*pipeline_compute_create)(Pipeline &,
                                        PipelineComputeCreateParams &);
        void (*pipeline_graphics_create)(Pipeline &,
                                         PipelineGraphicsCreateParams &);
        void (*pipeline_cleanup)(Pipeline &);
        void (*pipeline_bind)(Pipeline &);

        void (*compute_dispatch)(Context &, Math::UVector3);
        void (*draw)(DrawParams &);
        void (*draw_indexed)(DrawIndexedParams &);

        void (*imageresource_create)(ImageResource &,
                                     ImageResourceCreateParams &);
        void (*imageresource_cleanup)(ImageResource &);

        void (*buffer_create)(Buffer &, BufferCreateParams &);
        void (*buffer_read)(Buffer &, void *, uint32_t, uint32_t);
        void (*buffer_write)(Buffer &, void *, uint32_t, uint32_t);
        void (*buffer_set)(Buffer &, void *);
        void (*buffer_cleanup)(Buffer &);
        void (*buffer_bind_vertex)(Buffer &);
        void (*buffer_bind_index)(Buffer &, uint8_t);

        Util::String (*compile)(Util::String);
        Util::String (*get_shader_source_path)(Util::String);

        void (*renderdoc_section_begin)(Context &, Util::String,
                                        Math::Color p_Color);
        void (*renderdoc_section_end)(Context &);

        void (*imgui_image_create)(Backend::ImGuiImage &,
                                   Backend::ImageResource &);
        void (*imgui_image_cleanup)(Backend::ImGuiImage &);
        void (*imgui_image_render)(Backend::ImGuiImage &, Math::UVector2 &);
      };

      ApiBackendCallback &callbacks();

      void initialize();
    } // namespace Backend
  }   // namespace Renderer
} // namespace Low
