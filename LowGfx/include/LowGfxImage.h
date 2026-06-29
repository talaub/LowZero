#pragma once

#include "LowGfxToken.h"
#include "LowMath.h"

namespace Low {
  namespace Gfx {
    struct ImageTag;
    struct ImageViewTag;
    struct SamplerTag;

    using Image = Token<ImageTag>;
    using ImageView = Token<ImageViewTag>;
    using Sampler = Token<SamplerTag>;

    enum class ImageFormat : u16
    {
      Undefined,

      R8_UNorm,
      R8_SNorm,
      R8_UInt,
      R8_SInt,

      R8G8_UNorm,
      R8G8_SNorm,
      R8G8_UInt,
      R8G8_SInt,

      R8G8B8A8_UNorm,
      R8G8B8A8_SNorm,
      R8G8B8A8_UInt,
      R8G8B8A8_SInt,
      R8G8B8A8_SRGB,

      B8G8R8A8_UNorm,
      B8G8R8A8_SRGB,

      R16_UNorm,
      R16_SNorm,
      R16_UInt,
      R16_SInt,
      R16_Float,

      R16G16_UNorm,
      R16G16_SNorm,
      R16G16_UInt,
      R16G16_SInt,
      R16G16_Float,

      R16G16B16A16_UNorm,
      R16G16B16A16_SNorm,
      R16G16B16A16_UInt,
      R16G16B16A16_SInt,
      R16G16B16A16_Float,

      R32_UInt,
      R32_SInt,
      R32_Float,

      R32G32_UInt,
      R32G32_SInt,
      R32G32_Float,

      R32G32B32A32_UInt,
      R32G32B32A32_SInt,
      R32G32B32A32_Float,

      D16_UNorm,
      D32_Float,
      D24_UNorm_S8_UInt,
      D32_Float_S8_UInt
    };

    enum class ImageState
    {
      Undefined,
      ShaderRead,
      ShaderWrite,
      ColorAttachment,
      DepthWrite,
      DepthRead,
      TransferSrc,
      TransferDst,
      Present
    };

    enum class ImageDimension : u8
    {
      Image2D,
      Image3D
    };

    enum class ImageUsage : u32
    {
      None = 0,
      Sampled = 1 << 0,
      Storage = 1 << 1,
      ColorAttachment = 1 << 2,
      DepthStencilAttachment = 1 << 3,
      TransferSrc = 1 << 4,
      TransferDst = 1 << 5,
      Present = 1 << 6,
    };

    enum class ImageViewType : u8
    {
      Image2D,
      Image2DArray,
      Image3D,
      Cube,
      CubeArray
    };

    enum class ImageAspect : u8
    {
      Color,
      Depth,
      Stencil,
      DepthStencil
    };

    enum class LoadOp : u8
    {
      Load,
      Clear,
      DontCare
    };

    enum class StoreOp : u8
    {
      Store,
      DontCare
    };

    struct ClearValue
    {
      float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      float depth = 1.0f;
      u32 stencil = 0;
    };

    enum class FilterMode : u8
    {
      Nearest,
      Linear
    };

    enum class MipmapMode : u8
    {
      Nearest,
      Linear
    };

    enum class AddressMode : u8
    {
      Repeat,
      MirroredRepeat,
      ClampToEdge,
      ClampToBorder
    };

    enum class BorderColor : u8
    {
      TransparentBlack,
      OpaqueBlack,
      OpaqueWhite
    };

    enum class CompareOp : u8
    {
      Never,
      Less,
      Equal,
      LessOrEqual,
      Greater,
      NotEqual,
      GreaterOrEqual,
      Always
    };

    inline ImageUsage operator|(ImageUsage p_Left, ImageUsage p_Right)
    {
      return static_cast<ImageUsage>(static_cast<u32>(p_Left) |
                                     static_cast<u32>(p_Right));
    }

    inline ImageUsage operator&(ImageUsage p_Left, ImageUsage p_Right)
    {
      return static_cast<ImageUsage>(static_cast<u32>(p_Left) &
                                     static_cast<u32>(p_Right));
    }

    inline ImageUsage &operator|=(ImageUsage &p_Left,
                                  ImageUsage p_Right)
    {
      p_Left = p_Left | p_Right;
      return p_Left;
    }

    struct ImageDesc
    {
      ImageFormat format = ImageFormat::Undefined;
      ImageDimension dimension = ImageDimension::Image2D;
      Math::UVector3 extent;
      u32 mip_levels = 1;
      u32 array_layers = 1;
      ImageUsage usage = ImageUsage::None;
      const char *debug_name = nullptr;
    };

    struct ImageViewDesc
    {
      Image image;
      ImageViewType type = ImageViewType::Image2D;
      ImageFormat format = ImageFormat::Undefined;
      ImageAspect aspect = ImageAspect::Color;
      u32 base_mip = 0;
      u32 mip_count = 1;
      u32 base_layer = 0;
      u32 layer_count = 1;
      const char *debug_name = nullptr;
    };

    struct SamplerDesc
    {
      FilterMode min_filter = FilterMode::Linear;
      FilterMode mag_filter = FilterMode::Linear;
      MipmapMode mipmap_mode = MipmapMode::Linear;

      AddressMode address_u = AddressMode::Repeat;
      AddressMode address_v = AddressMode::Repeat;
      AddressMode address_w = AddressMode::Repeat;

      float mip_lod_bias = 0.0f;
      float min_lod = 0.0f;
      float max_lod = 1000.0f;

      bool anisotropy_enabled = false;
      float max_anisotropy = 1.0f;

      bool compare_enabled = false;
      CompareOp compare_op = CompareOp::LessOrEqual;

      BorderColor border_color = BorderColor::OpaqueBlack;
      const char *debug_name = nullptr;
    };

    struct ImageBarrier
    {
      Image image;
      ImageState old_state = ImageState::Undefined;
      ImageState new_state = ImageState::Undefined;
      ImageAspect aspect = ImageAspect::Color;
      u32 base_mip = 0;
      u32 mip_count = 1;
      u32 base_layer = 0;
      u32 layer_count = 1;
    };

    struct BufferImageCopyRegion
    {
      u64 buffer_offset = 0;
      u32 buffer_row_length = 0;
      u32 buffer_image_height = 0;
      ImageAspect image_aspect = ImageAspect::Color;
      u32 image_mip = 0;
      u32 image_base_layer = 0;
      u32 image_layer_count = 1;
      Math::UVector3 image_offset = {0, 0, 0};
      Math::UVector3 image_extent = {1, 1, 1};
    };

    struct ImageCopyRegion
    {
      ImageAspect src_aspect = ImageAspect::Color;
      u32 src_mip = 0;
      u32 src_base_layer = 0;
      ImageAspect dst_aspect = ImageAspect::Color;
      u32 dst_mip = 0;
      u32 dst_base_layer = 0;
      u32 layer_count = 1;
      Math::UVector3 src_offset = {0, 0, 0};
      Math::UVector3 dst_offset = {0, 0, 0};
      Math::UVector3 extent = {1, 1, 1};
    };

    struct ImageBlitRegion
    {
      ImageAspect src_aspect = ImageAspect::Color;
      u32 src_mip = 0;
      u32 src_base_layer = 0;
      ImageAspect dst_aspect = ImageAspect::Color;
      u32 dst_mip = 0;
      u32 dst_base_layer = 0;
      u32 layer_count = 1;
      Math::UVector3 src_min = {0, 0, 0};
      Math::UVector3 src_max = {1, 1, 1};
      Math::UVector3 dst_min = {0, 0, 0};
      Math::UVector3 dst_max = {1, 1, 1};
    };

    struct ColorAttachmentDesc
    {
      ImageView view;
      ImageState state = ImageState::ColorAttachment;
      LoadOp load_op = LoadOp::Clear;
      StoreOp store_op = StoreOp::Store;
      ClearValue clear;
    };

    struct DepthAttachmentDesc
    {
      ImageView view;
      ImageState state = ImageState::Undefined;
      LoadOp load_op = LoadOp::Clear;
      StoreOp store_op = StoreOp::Store;
      ClearValue clear;
    };

  } // namespace Gfx
} // namespace Low
