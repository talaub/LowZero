#include "LowGfxBackend.h"
#include "LowGfxImage.h"
#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"

#include "LowGfxLogInternal.h"
#include "LowUtilAssert.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Gfx {
    namespace Vulkan {

      static bool has_usage(ImageUsage p_Value, ImageUsage p_Flag)
      {
        return (static_cast<u32>(p_Value) &
                static_cast<u32>(p_Flag)) != 0;
      }

      static VkFormat to_vulkan_format(ImageFormat p_Format)
      {
        switch (p_Format) {
        case ImageFormat::Undefined:
          return VK_FORMAT_UNDEFINED;

        case ImageFormat::R8_UNorm:
          return VK_FORMAT_R8_UNORM;
        case ImageFormat::R8_SNorm:
          return VK_FORMAT_R8_SNORM;
        case ImageFormat::R8_UInt:
          return VK_FORMAT_R8_UINT;
        case ImageFormat::R8_SInt:
          return VK_FORMAT_R8_SINT;

        case ImageFormat::R8G8_UNorm:
          return VK_FORMAT_R8G8_UNORM;
        case ImageFormat::R8G8_SNorm:
          return VK_FORMAT_R8G8_SNORM;
        case ImageFormat::R8G8_UInt:
          return VK_FORMAT_R8G8_UINT;
        case ImageFormat::R8G8_SInt:
          return VK_FORMAT_R8G8_SINT;

        case ImageFormat::R8G8B8A8_UNorm:
          return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::R8G8B8A8_SNorm:
          return VK_FORMAT_R8G8B8A8_SNORM;
        case ImageFormat::R8G8B8A8_UInt:
          return VK_FORMAT_R8G8B8A8_UINT;
        case ImageFormat::R8G8B8A8_SInt:
          return VK_FORMAT_R8G8B8A8_SINT;
        case ImageFormat::R8G8B8A8_SRGB:
          return VK_FORMAT_R8G8B8A8_SRGB;

        case ImageFormat::B8G8R8A8_UNorm:
          return VK_FORMAT_B8G8R8A8_UNORM;
        case ImageFormat::B8G8R8A8_SRGB:
          return VK_FORMAT_B8G8R8A8_SRGB;

        case ImageFormat::R16_UNorm:
          return VK_FORMAT_R16_UNORM;
        case ImageFormat::R16_SNorm:
          return VK_FORMAT_R16_SNORM;
        case ImageFormat::R16_UInt:
          return VK_FORMAT_R16_UINT;
        case ImageFormat::R16_SInt:
          return VK_FORMAT_R16_SINT;
        case ImageFormat::R16_Float:
          return VK_FORMAT_R16_SFLOAT;

        case ImageFormat::R16G16_UNorm:
          return VK_FORMAT_R16G16_UNORM;
        case ImageFormat::R16G16_SNorm:
          return VK_FORMAT_R16G16_SNORM;
        case ImageFormat::R16G16_UInt:
          return VK_FORMAT_R16G16_UINT;
        case ImageFormat::R16G16_SInt:
          return VK_FORMAT_R16G16_SINT;
        case ImageFormat::R16G16_Float:
          return VK_FORMAT_R16G16_SFLOAT;

        case ImageFormat::R16G16B16A16_UNorm:
          return VK_FORMAT_R16G16B16A16_UNORM;
        case ImageFormat::R16G16B16A16_SNorm:
          return VK_FORMAT_R16G16B16A16_SNORM;
        case ImageFormat::R16G16B16A16_UInt:
          return VK_FORMAT_R16G16B16A16_UINT;
        case ImageFormat::R16G16B16A16_SInt:
          return VK_FORMAT_R16G16B16A16_SINT;
        case ImageFormat::R16G16B16A16_Float:
          return VK_FORMAT_R16G16B16A16_SFLOAT;

        case ImageFormat::R32_UInt:
          return VK_FORMAT_R32_UINT;
        case ImageFormat::R32_SInt:
          return VK_FORMAT_R32_SINT;
        case ImageFormat::R32_Float:
          return VK_FORMAT_R32_SFLOAT;

        case ImageFormat::R32G32_UInt:
          return VK_FORMAT_R32G32_UINT;
        case ImageFormat::R32G32_SInt:
          return VK_FORMAT_R32G32_SINT;
        case ImageFormat::R32G32_Float:
          return VK_FORMAT_R32G32_SFLOAT;

        case ImageFormat::R32G32B32A32_UInt:
          return VK_FORMAT_R32G32B32A32_UINT;
        case ImageFormat::R32G32B32A32_SInt:
          return VK_FORMAT_R32G32B32A32_SINT;
        case ImageFormat::R32G32B32A32_Float:
          return VK_FORMAT_R32G32B32A32_SFLOAT;

        case ImageFormat::D16_UNorm:
          return VK_FORMAT_D16_UNORM;
        case ImageFormat::D32_Float:
          return VK_FORMAT_D32_SFLOAT;
        case ImageFormat::D24_UNorm_S8_UInt:
          return VK_FORMAT_D24_UNORM_S8_UINT;
        case ImageFormat::D32_Float_S8_UInt:
          return VK_FORMAT_D32_SFLOAT_S8_UINT;
        }

        LOW_ASSERT(false, "Unsupported LowGfx image format");
        return VK_FORMAT_UNDEFINED;
      }

      static VkImageUsageFlags to_vulkan_usage(ImageUsage p_Usage)
      {
        VkImageUsageFlags l_Usage = 0;

        if (has_usage(p_Usage, ImageUsage::Sampled)) {
          l_Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (has_usage(p_Usage, ImageUsage::Storage)) {
          l_Usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (has_usage(p_Usage, ImageUsage::ColorAttachment)) {
          l_Usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (has_usage(p_Usage,
                      ImageUsage::DepthStencilAttachment)) {
          l_Usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (has_usage(p_Usage, ImageUsage::TransferSrc)) {
          l_Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (has_usage(p_Usage, ImageUsage::TransferDst)) {
          l_Usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        return l_Usage;
      }

      static VkImageType
      to_vulkan_image_type(ImageDimension p_Dimension)
      {
        switch (p_Dimension) {
        case ImageDimension::Image2D:
          return VK_IMAGE_TYPE_2D;
        case ImageDimension::Image3D:
          return VK_IMAGE_TYPE_3D;
        }

        LOW_ASSERT(false, "Unsupported LowGfx image dimension");
        return VK_IMAGE_TYPE_2D;
      }

      static VkImageViewType
      to_vulkan_image_view_type(ImageViewType p_Type)
      {
        switch (p_Type) {
        case ImageViewType::Image2D:
          return VK_IMAGE_VIEW_TYPE_2D;
        case ImageViewType::Image2DArray:
          return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case ImageViewType::Image3D:
          return VK_IMAGE_VIEW_TYPE_3D;
        case ImageViewType::Cube:
          return VK_IMAGE_VIEW_TYPE_CUBE;
        case ImageViewType::CubeArray:
          return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        }

        LOW_ASSERT(false, "Unsupported LowGfx image view type");
        return VK_IMAGE_VIEW_TYPE_2D;
      }

      static VkImageAspectFlags
      to_vulkan_aspect(ImageAspect p_Aspect)
      {
        switch (p_Aspect) {
        case ImageAspect::Color:
          return VK_IMAGE_ASPECT_COLOR_BIT;
        case ImageAspect::Depth:
          return VK_IMAGE_ASPECT_DEPTH_BIT;
        case ImageAspect::Stencil:
          return VK_IMAGE_ASPECT_STENCIL_BIT;
        case ImageAspect::DepthStencil:
          return VK_IMAGE_ASPECT_DEPTH_BIT |
                 VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        LOW_ASSERT(false, "Unsupported LowGfx image aspect");
        return VK_IMAGE_ASPECT_COLOR_BIT;
      }

      static VkFilter to_vulkan_filter(FilterMode p_Mode)
      {
        switch (p_Mode) {
        case FilterMode::Nearest:
          return VK_FILTER_NEAREST;
        case FilterMode::Linear:
          return VK_FILTER_LINEAR;
        }

        LOW_ASSERT(false, "Unsupported LowGfx filter mode");
        return VK_FILTER_LINEAR;
      }

      static VkSamplerMipmapMode
      to_vulkan_mipmap_mode(MipmapMode p_Mode)
      {
        switch (p_Mode) {
        case MipmapMode::Nearest:
          return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case MipmapMode::Linear:
          return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }

        LOW_ASSERT(false, "Unsupported LowGfx mipmap mode");
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
      }

      static VkSamplerAddressMode
      to_vulkan_address_mode(AddressMode p_Mode)
      {
        switch (p_Mode) {
        case AddressMode::Repeat:
          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case AddressMode::MirroredRepeat:
          return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case AddressMode::ClampToEdge:
          return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::ClampToBorder:
          return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        }

        LOW_ASSERT(false, "Unsupported LowGfx address mode");
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
      }

      static VkBorderColor to_vulkan_border_color(BorderColor p_Color)
      {
        switch (p_Color) {
        case BorderColor::TransparentBlack:
          return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case BorderColor::OpaqueBlack:
          return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case BorderColor::OpaqueWhite:
          return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        }

        LOW_ASSERT(false, "Unsupported LowGfx border color");
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
      }

      static VkCompareOp to_vulkan_compare_op(CompareOp p_Op)
      {
        switch (p_Op) {
        case CompareOp::Never:
          return VK_COMPARE_OP_NEVER;
        case CompareOp::Less:
          return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:
          return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessOrEqual:
          return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:
          return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:
          return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterOrEqual:
          return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::Always:
          return VK_COMPARE_OP_ALWAYS;
        }

        LOW_ASSERT(false, "Unsupported LowGfx compare op");
        return VK_COMPARE_OP_LESS_OR_EQUAL;
      }

      static void set_debug_name(const VulkanContextState &p_State,
                                 const VulkanImageState &p_Image,
                                 const char *p_Name)
      {
        if (!p_Name || p_Image.image == VK_NULL_HANDLE) {
          return;
        }

        vmaSetAllocationName(p_State.allocator, p_Image.allocation,
                             p_Name);

        PFN_vkSetDebugUtilsObjectNameEXT l_SetObjectName =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(p_State.device,
                                    "vkSetDebugUtilsObjectNameEXT"));
        if (!l_SetObjectName) {
          return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        l_NameInfo.objectHandle =
            reinterpret_cast<uint64_t>(p_Image.image);
        l_NameInfo.pObjectName = p_Name;

        l_SetObjectName(p_State.device, &l_NameInfo);
      }

      static void
      set_debug_name(const VulkanContextState &p_State,
                     const VulkanImageViewState &p_ImageView,
                     const char *p_Name)
      {
        if (!p_Name ||
            p_ImageView.image_view == VK_NULL_HANDLE) {
          return;
        }

        PFN_vkSetDebugUtilsObjectNameEXT l_SetObjectName =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(p_State.device,
                                    "vkSetDebugUtilsObjectNameEXT"));
        if (!l_SetObjectName) {
          return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        l_NameInfo.objectHandle = reinterpret_cast<uint64_t>(
            p_ImageView.image_view);
        l_NameInfo.pObjectName = p_Name;

        l_SetObjectName(p_State.device, &l_NameInfo);
      }

      static void
      set_debug_name(const VulkanContextState &p_State,
                     const VulkanSamplerState &p_Sampler,
                     const char *p_Name)
      {
        if (!p_Name || p_Sampler.sampler == VK_NULL_HANDLE) {
          return;
        }

        PFN_vkSetDebugUtilsObjectNameEXT l_SetObjectName =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(p_State.device,
                                    "vkSetDebugUtilsObjectNameEXT"));
        if (!l_SetObjectName) {
          return;
        }

        VkDebugUtilsObjectNameInfoEXT l_NameInfo{};
        l_NameInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        l_NameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
        l_NameInfo.objectHandle =
            reinterpret_cast<uint64_t>(p_Sampler.sampler);
        l_NameInfo.pObjectName = p_Name;

        l_SetObjectName(p_State.device, &l_NameInfo);
      }

      Detail::BackendImage
      create_image(Detail::ContextImpl &p_Context,
                   const ImageDesc &p_Desc)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot create Vulkan image without context state");
        LOW_ASSERT(l_State->allocator,
                   "Cannot create Vulkan image without allocator");
        LOW_ASSERT(!has_usage(p_Desc.usage, ImageUsage::Present),
                   "Cannot create normal Vulkan image with present "
                   "usage");

        VulkanImageState *l_ImageState = new VulkanImageState();

        VkImageCreateInfo l_Info = {};
        l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        l_Info.pNext = nullptr;

        l_Info.imageType = to_vulkan_image_type(p_Desc.dimension);
        if (p_Desc.dimension == ImageDimension::Image2D &&
            p_Desc.array_layers >= 6 &&
            p_Desc.extent.x == p_Desc.extent.y) {
          l_Info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VkExtent3D l_LocalExtent{};
        l_LocalExtent.width = p_Desc.extent.x;
        l_LocalExtent.height = p_Desc.extent.y;
        l_LocalExtent.depth = p_Desc.extent.z;

        l_Info.format = to_vulkan_format(p_Desc.format);
        l_Info.extent = l_LocalExtent;

        l_Info.mipLevels = p_Desc.mip_levels;
        l_Info.arrayLayers = p_Desc.array_layers;

        // MSAA
        l_Info.samples = VK_SAMPLE_COUNT_1_BIT;

        // CHECK: Check that again and learn what it does
        // May have to be dynamic (change based on input params)
        l_Info.tiling = VK_IMAGE_TILING_OPTIMAL;
        l_Info.usage = to_vulkan_usage(p_Desc.usage);
        l_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        l_Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo l_AllocationInfo{};
        l_AllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        l_AllocationInfo.requiredFlags =
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkResult l_Result =
            vmaCreateImage(l_State->allocator, &l_Info,
                           &l_AllocationInfo, &l_ImageState->image,
                           &l_ImageState->allocation,
                           &l_ImageState->info);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan image: {}",
                       static_cast<int>(l_Result));
          delete l_ImageState;
        }
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan image");

        l_ImageState->format = l_Info.format;
        l_ImageState->extent = l_Info.extent;
        l_ImageState->mip_levels = p_Desc.mip_levels;
        l_ImageState->array_layers = p_Desc.array_layers;

        set_debug_name(*l_State, *l_ImageState, p_Desc.debug_name);

        Detail::BackendImage l_Image;
        l_Image.format = p_Desc.format;
        l_Image.dimension = p_Desc.dimension;
        l_Image.extent = p_Desc.extent;
        l_Image.mip_levels = p_Desc.mip_levels;
        l_Image.array_layers = p_Desc.array_layers;
        l_Image.usage = p_Desc.usage;
        l_Image.backend_state = l_ImageState;
        return l_Image;
      }

      Detail::BackendImageView
      create_image_view(Detail::ContextImpl &p_Context,
                        const ImageViewDesc &p_Desc)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot create Vulkan image view without context state");

        Detail::BackendImage *l_Image =
            p_Context.images.get(p_Desc.image);
        LOW_ASSERT(l_Image,
                   "Cannot create Vulkan image view for invalid image");

        VulkanImageState *l_ImageState =
            static_cast<VulkanImageState *>(l_Image->backend_state);
        LOW_ASSERT(l_ImageState && l_ImageState->image != VK_NULL_HANDLE,
                   "Cannot create Vulkan image view without image");

        VulkanImageViewState *l_ImageViewState =
            new VulkanImageViewState();

        const ImageFormat l_ViewFormat =
            p_Desc.format == ImageFormat::Undefined ? l_Image->format
                                                    : p_Desc.format;

        VkImageViewCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        l_Info.image = l_ImageState->image;
        l_Info.viewType = to_vulkan_image_view_type(p_Desc.type);
        l_Info.format = to_vulkan_format(l_ViewFormat);
        l_Info.subresourceRange.aspectMask =
            to_vulkan_aspect(p_Desc.aspect);
        l_Info.subresourceRange.baseMipLevel = p_Desc.base_mip;
        l_Info.subresourceRange.levelCount = p_Desc.mip_count;
        l_Info.subresourceRange.baseArrayLayer = p_Desc.base_layer;
        l_Info.subresourceRange.layerCount = p_Desc.layer_count;

        VkResult l_Result = vkCreateImageView(
            l_State->device, &l_Info, nullptr,
            &l_ImageViewState->image_view);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan image view: {}",
                       static_cast<int>(l_Result));
          delete l_ImageViewState;
        }
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan image view");

        set_debug_name(*l_State, *l_ImageViewState,
                       p_Desc.debug_name);

        Detail::BackendImageView l_ImageView;
        l_ImageView.image = p_Desc.image;
        l_ImageView.format = l_ViewFormat;
        l_ImageView.aspect = p_Desc.aspect;
        l_ImageView.base_mip = p_Desc.base_mip;
        l_ImageView.mip_count = p_Desc.mip_count;
        l_ImageView.base_layer = p_Desc.base_layer;
        l_ImageView.layer_count = p_Desc.layer_count;
        l_ImageView.backend_state = l_ImageViewState;
        return l_ImageView;
      }

      void
      destroy_image_view(Detail::ContextImpl &p_Context,
                         Detail::BackendImageView &p_ImageView)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot destroy Vulkan image view without context state");

        VulkanImageViewState *l_ImageViewState =
            static_cast<VulkanImageViewState *>(
                p_ImageView.backend_state);
        if (l_ImageViewState) {
          if (l_State->device != VK_NULL_HANDLE &&
              l_ImageViewState->image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(l_State->device,
                               l_ImageViewState->image_view,
                               nullptr);
          }

          delete l_ImageViewState;
        }

        p_ImageView.image = Image{};
        p_ImageView.format = ImageFormat::Undefined;
        p_ImageView.backend_state = nullptr;
      }

      Detail::BackendSampler
      create_sampler(Detail::ContextImpl &p_Context,
                     const SamplerDesc &p_Desc)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot create Vulkan sampler without context state");

        VulkanSamplerState *l_SamplerState =
            new VulkanSamplerState();

        VkSamplerCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        l_Info.magFilter = to_vulkan_filter(p_Desc.mag_filter);
        l_Info.minFilter = to_vulkan_filter(p_Desc.min_filter);
        l_Info.mipmapMode =
            to_vulkan_mipmap_mode(p_Desc.mipmap_mode);
        l_Info.addressModeU =
            to_vulkan_address_mode(p_Desc.address_u);
        l_Info.addressModeV =
            to_vulkan_address_mode(p_Desc.address_v);
        l_Info.addressModeW =
            to_vulkan_address_mode(p_Desc.address_w);
        l_Info.mipLodBias = p_Desc.mip_lod_bias;
        l_Info.anisotropyEnable =
            p_Desc.anisotropy_enabled ? VK_TRUE : VK_FALSE;
        l_Info.maxAnisotropy = p_Desc.max_anisotropy;
        l_Info.compareEnable =
            p_Desc.compare_enabled ? VK_TRUE : VK_FALSE;
        l_Info.compareOp = to_vulkan_compare_op(p_Desc.compare_op);
        l_Info.minLod = p_Desc.min_lod;
        l_Info.maxLod = p_Desc.max_lod;
        l_Info.borderColor =
            to_vulkan_border_color(p_Desc.border_color);
        l_Info.unnormalizedCoordinates = VK_FALSE;

        VkResult l_Result =
            vkCreateSampler(l_State->device, &l_Info, nullptr,
                            &l_SamplerState->sampler);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan sampler: {}",
                       static_cast<int>(l_Result));
          delete l_SamplerState;
        }
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan sampler");

        set_debug_name(*l_State, *l_SamplerState,
                       p_Desc.debug_name);

        Detail::BackendSampler l_Sampler;
        l_Sampler.backend_state = l_SamplerState;
        return l_Sampler;
      }

      void destroy_sampler(Detail::ContextImpl &p_Context,
                           Detail::BackendSampler &p_Sampler)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot destroy Vulkan sampler without context state");

        VulkanSamplerState *l_SamplerState =
            static_cast<VulkanSamplerState *>(
                p_Sampler.backend_state);
        if (l_SamplerState) {
          if (l_State->device != VK_NULL_HANDLE &&
              l_SamplerState->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(l_State->device,
                             l_SamplerState->sampler, nullptr);
          }

          delete l_SamplerState;
        }

        p_Sampler.backend_state = nullptr;
      }

      void destroy_image(Detail::ContextImpl &p_Context,
                         Detail::BackendImage &p_Image)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(
                p_Context.backend_state);
        LOW_ASSERT(
            l_State,
            "Cannot destroy Vulkan buffer without context state");

        VulkanImageState *l_ImageState =
            static_cast<VulkanImageState *>(p_Image.backend_state);
        if (l_ImageState) {
          if (l_State->allocator &&
              l_ImageState->image != VK_NULL_HANDLE &&
              l_ImageState->allocation) {
            vmaDestroyImage(l_State->allocator, l_ImageState->image,
                            l_ImageState->allocation);
          }

          delete l_ImageState;
        }

        p_Image.usage = ImageUsage::None;
        p_Image.backend_state = nullptr;
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
