#pragma once

#include "LowRendererContext.h"
#include "LowRendererFramebuffer.h"
#include "LowRendererCommandBuffer.h"
#include "LowRendererCommandPool.h"
#include "LowRendererImage2D.h"
#include "LowRendererRenderpass.h"
#include "LowRendererSwapchain.h"
#include "LowRendererGraphicsPipeline.h"
#include "LowRendererPipelineInterface.h"
#include "LowRendererUniformPool.h"
#include "LowRendererUniformScopeInterface.h"
#include "LowRendererUniformScope.h"
#include "LowRendererUniform.h"
#include "LowRendererBuffer.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      struct Vertex
      {
        Math::Vector3 position;
        Math::Vector3 normal;
        Math::Vector2 textureCoordinates;
        Math::Vector3 tangent;
        Math::Vector3 bitangent;
      };

      struct ContextCreateParams
      {
        Window *window;
        bool validation_enabled;
      };

      struct CommandPoolCreateParams
      {
        Context context;
      };

      struct RenderpassCreateParams
      {
        Context context;
        Util::List<Backend::ImageFormat> formats;
        Util::List<bool> clearTargets;
        bool useDepth;
        bool clearDepth;
      };

      struct RenderpassStartParams
      {
        Framebuffer framebuffer;
        CommandBuffer commandbuffer;
        Util::List<Math::Color> clearColorValues;
        Math::Vector2 clearDepthValue;
      };

      struct RenderpassStopParams
      {
        CommandBuffer commandbuffer;
      };

      struct FramebufferCreateParams
      {
        Context context;
        Util::List<Image2D> renderTargets;
        uint8_t framesInFlight;
        Math::UVector2 dimensions;
        Renderpass renderpass;
      };

      struct Image2DCreateParams
      {
        Context context;
        CommandPool commandPool;
        Math::UVector2 dimensions;
        Backend::ImageFormat format;
        bool writeable;
        bool depth;
        void *imageData;
        size_t imageDataSize;
      };

      struct SwapchainCreateParams
      {
        Context context;
        CommandPool commandPool;
      };

      struct GraphicsPipelineCreateParams
      {
        Context context;
        PipelineInterface interface;
        Util::String vertexPath;
        Util::String fragmentPath;
        Renderpass renderpass;
        Math::UVector2 dimensions;
        uint8_t cullMode;
        uint8_t frontFace;
        uint8_t polygonMode;
        Util::List<Backend::GraphicsPipelineColorTarget> colorTargets;
        bool vertexInput;
      };

      struct PipelineInterfaceCreateParams
      {
        Context context;
        Util::List<Interface::UniformScopeInterface> uniformScopeInterfaces;
      };

      struct DrawParams
      {
        CommandBuffer commandBuffer;
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
      };

      struct DrawIndexedParams
      {
        CommandBuffer commandBuffer;
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t vertexOffset;
        uint32_t firstInstance;
      };

      struct DrawIndexedBindlessParams
      {
        CommandBuffer commandBuffer;
        Buffer drawInfo;
        size_t offset;
        uint32_t drawCount;
        uint32_t stride;
      };

      void draw(DrawParams &p_Params);
      void draw_indexed(DrawIndexedParams &p_Params);
      void draw_indexed_bindless(DrawIndexedBindlessParams &p_Params);

      struct UniformScopeInterfaceCreateParams
      {
        Context context;
        Util::List<Backend::UniformInterface> uniformInterfaces;
      };

      struct UniformPoolCreateParams
      {
        Context context;
        uint32_t uniformBufferCount;
        uint32_t storageBufferCount;
        uint32_t samplerCount;
        uint32_t rendertargetCount;
        uint32_t scopeCount;
      };

      struct UniformBufferCreateParams
      {
        Context context;
        Swapchain swapchain;
        uint8_t bufferType;
        size_t bufferSize;
        uint32_t binding;
        uint32_t arrayIndex;
      };

      struct UniformBufferSetParams
      {
        Context context;
        Swapchain swapchain;
        void *value;
      };

      struct UniformImageCreateParams
      {
        Context context;
        Swapchain swapchain;
        Image2D image;
        uint8_t imageType;
        uint32_t binding;
        uint32_t arrayIndex;
      };

      struct UniformScopeCreateParams
      {
        Context context;
        Swapchain swapchain;
        UniformScopeInterface interface;
        UniformPool pool;
        Util::List<Uniform> uniforms;
      };

      struct UniformScopeBindGraphicsParams
      {
        Context context;
        Swapchain swapchain;
        Util::List<UniformScope> scopes;
        GraphicsPipeline pipeline;
        uint32_t startIndex;
      };

      struct BufferCreateParams
      {
        Context context;
        CommandPool commandPool;
        size_t bufferSize;
        void *data;
        uint8_t bufferUsageType;
      };

      namespace ShaderProgramUtils {
        Util::String compile(Util::String p_Path);

        void register_graphics_pipeline(GraphicsPipeline p_Pipeline,
                                        GraphicsPipelineCreateParams &p_Params);

        void delist_graphics_pipeline(GraphicsPipeline p_Pipeline);

        void tick(float p_Delta);
      }; // namespace ShaderProgramUtils

      namespace UniformPoolUtils {
        UniformPool get_uniform_pool(UniformPoolCreateParams &p_Params);
      }
    } // namespace Interface
  }   // namespace Renderer
} // namespace Low
