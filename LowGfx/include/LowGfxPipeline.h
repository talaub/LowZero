#pragma once

#include "LowGfxBuffer.h"
#include "LowGfxImage.h"
#include "LowGfxToken.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Gfx {
    struct ShaderModuleTag;
    struct BindGroupLayoutTag;
    struct BindGroupTag;
    struct PipelineLayoutTag;
    struct GraphicsPipelineTag;

    using ShaderModule = Token<ShaderModuleTag>;
    using BindGroupLayout = Token<BindGroupLayoutTag>;
    using BindGroup = Token<BindGroupTag>;
    using PipelineLayout = Token<PipelineLayoutTag>;
    using GraphicsPipeline = Token<GraphicsPipelineTag>;

    enum class ShaderStage : u32
    {
      None = 0,
      Vertex = 1 << 0,
      Fragment = 1 << 1,
      Compute = 1 << 2
    };

    enum class ShaderSourceFormat : u8
    {
      Spirv
    };

    enum class DescriptorType : u8
    {
      UniformBuffer,
      StorageBuffer,
      SampledImage,
      StorageImage,
      Sampler,
      CombinedImageSampler
    };

    enum class VertexFormat : u8
    {
      Float,
      Float2,
      Float3,
      Float4,
      UInt,
      UInt2,
      UInt3,
      UInt4
    };

    enum class VertexInputRate : u8
    {
      Vertex,
      Instance
    };

    enum class PrimitiveTopology : u8
    {
      TriangleList,
      TriangleStrip,
      LineList,
      LineStrip,
      PointList
    };

    enum class PolygonMode : u8
    {
      Fill,
      Line
    };

    enum class CullMode : u8
    {
      None,
      Front,
      Back,
      FrontAndBack
    };

    enum class FrontFace : u8
    {
      CounterClockwise,
      Clockwise
    };

    enum class BlendFactor : u8
    {
      Zero,
      One,
      SrcAlpha,
      OneMinusSrcAlpha,
      DstAlpha,
      OneMinusDstAlpha,
      SrcColor,
      OneMinusSrcColor,
      DstColor,
      OneMinusDstColor
    };

    enum class BlendOp : u8
    {
      Add,
      Subtract,
      ReverseSubtract,
      Min,
      Max
    };

    enum class ColorWriteMask : u8
    {
      None = 0,
      R = 1 << 0,
      G = 1 << 1,
      B = 1 << 2,
      A = 1 << 3,
      All = R | G | B | A
    };

    inline ShaderStage operator|(ShaderStage p_Left,
                                 ShaderStage p_Right)
    {
      return static_cast<ShaderStage>(static_cast<u32>(p_Left) |
                                      static_cast<u32>(p_Right));
    }

    inline ShaderStage operator&(ShaderStage p_Left,
                                 ShaderStage p_Right)
    {
      return static_cast<ShaderStage>(static_cast<u32>(p_Left) &
                                      static_cast<u32>(p_Right));
    }

    inline ShaderStage &operator|=(ShaderStage &p_Left,
                                   ShaderStage p_Right)
    {
      p_Left = p_Left | p_Right;
      return p_Left;
    }

    inline ColorWriteMask operator|(ColorWriteMask p_Left,
                                    ColorWriteMask p_Right)
    {
      return static_cast<ColorWriteMask>(
          static_cast<u8>(p_Left) | static_cast<u8>(p_Right));
    }

    struct ShaderModuleDesc
    {
      ShaderSourceFormat format = ShaderSourceFormat::Spirv;
      Util::Span<const u32> code;
      const char *debug_name = nullptr;
    };

    struct BindGroupLayoutEntry
    {
      u32 binding = 0;
      DescriptorType type = DescriptorType::UniformBuffer;
      u32 count = 1;
      ShaderStage stages = ShaderStage::None;
    };

    struct BindGroupLayoutDesc
    {
      Util::Span<const BindGroupLayoutEntry> entries;
      const char *debug_name = nullptr;
    };

    struct PipelineLayoutDesc
    {
      Util::Span<const BindGroupLayout> bind_group_layouts;
      const char *debug_name = nullptr;
    };

    struct BufferBinding
    {
      Buffer buffer;
      u64 offset = 0;
      u64 range = LOW_UINT64_MAX;
    };

    struct ImageBinding
    {
      ImageView view;
      ImageState state = ImageState::ShaderRead;
    };

    struct BindGroupEntry
    {
      u32 binding = 0;
      DescriptorType type = DescriptorType::UniformBuffer;
      u32 array_element = 0;
      BufferBinding buffer;
      ImageBinding image;
      Sampler sampler;
    };

    struct BindGroupDesc
    {
      BindGroupLayout layout;
      Util::Span<const BindGroupEntry> entries;
      const char *debug_name = nullptr;
    };

    struct ShaderStageDesc
    {
      ShaderStage stage = ShaderStage::None;
      ShaderModule module;
      const char *entry_point = "main";
    };

    struct VertexBufferLayoutDesc
    {
      u32 binding = 0;
      u32 stride = 0;
      VertexInputRate input_rate = VertexInputRate::Vertex;
    };

    struct VertexAttributeDesc
    {
      u32 location = 0;
      u32 binding = 0;
      VertexFormat format = VertexFormat::Float3;
      u32 offset = 0;
    };

    struct ColorTargetDesc
    {
      ImageFormat format = ImageFormat::Undefined;
      bool blend_enabled = false;
      BlendFactor src_color_factor = BlendFactor::One;
      BlendFactor dst_color_factor = BlendFactor::Zero;
      BlendOp color_op = BlendOp::Add;
      BlendFactor src_alpha_factor = BlendFactor::One;
      BlendFactor dst_alpha_factor = BlendFactor::Zero;
      BlendOp alpha_op = BlendOp::Add;
      ColorWriteMask write_mask = ColorWriteMask::All;
    };

    struct GraphicsPipelineDesc
    {
      PipelineLayout layout;
      Util::Span<const ShaderStageDesc> shaders;
      Util::Span<const VertexBufferLayoutDesc> vertex_buffers;
      Util::Span<const VertexAttributeDesc> vertex_attributes;

      PrimitiveTopology topology = PrimitiveTopology::TriangleList;
      PolygonMode polygon_mode = PolygonMode::Fill;
      CullMode cull_mode = CullMode::Back;
      FrontFace front_face = FrontFace::CounterClockwise;

      Util::Span<const ColorTargetDesc> color_targets;
      ImageFormat depth_format = ImageFormat::Undefined;
      bool depth_test_enabled = false;
      bool depth_write_enabled = false;
      CompareOp depth_compare = CompareOp::LessOrEqual;

      const char *debug_name = nullptr;
    };
  } // namespace Gfx
} // namespace Low
