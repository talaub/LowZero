#include "LowGfxVulkanBackend.h"
#include "LowGfxVulkanState.h"

#include "LowGfxLogInternal.h"
#include "LowUtilAssert.h"

namespace Low {
  namespace Gfx {
    namespace Vulkan {
      static VkShaderStageFlags to_vulkan_shader_stages(
          ShaderStage p_Stages)
      {
        VkShaderStageFlags l_Stages = 0;

        if ((p_Stages & ShaderStage::Vertex) != ShaderStage::None) {
          l_Stages |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if ((p_Stages & ShaderStage::Fragment) != ShaderStage::None) {
          l_Stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if ((p_Stages & ShaderStage::Compute) != ShaderStage::None) {
          l_Stages |= VK_SHADER_STAGE_COMPUTE_BIT;
        }

        return l_Stages;
      }

      static VkShaderStageFlagBits to_vulkan_shader_stage(
          ShaderStage p_Stage)
      {
        switch (p_Stage) {
        case ShaderStage::Vertex:
          return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
          return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Compute:
          return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
          break;
        }

        LOW_ASSERT(false, "Unsupported shader stage");
        return VK_SHADER_STAGE_VERTEX_BIT;
      }

      static VkDescriptorType to_vulkan_descriptor_type(
          DescriptorType p_Type)
      {
        switch (p_Type) {
        case DescriptorType::UniformBuffer:
          return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::StorageBuffer:
          return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::SampledImage:
          return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::StorageImage:
          return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::Sampler:
          return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::CombinedImageSampler:
          return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }

        LOW_ASSERT(false, "Unsupported descriptor type");
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

        LOW_ASSERT(false, "Unsupported image format");
        return VK_FORMAT_UNDEFINED;
      }

      static VkFormat to_vulkan_vertex_format(VertexFormat p_Format)
      {
        switch (p_Format) {
        case VertexFormat::Float:
          return VK_FORMAT_R32_SFLOAT;
        case VertexFormat::Float2:
          return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormat::Float3:
          return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormat::Float4:
          return VK_FORMAT_R32G32B32A32_SFLOAT;
        case VertexFormat::UInt:
          return VK_FORMAT_R32_UINT;
        case VertexFormat::UInt2:
          return VK_FORMAT_R32G32_UINT;
        case VertexFormat::UInt3:
          return VK_FORMAT_R32G32B32_UINT;
        case VertexFormat::UInt4:
          return VK_FORMAT_R32G32B32A32_UINT;
        }

        LOW_ASSERT(false, "Unsupported vertex format");
        return VK_FORMAT_R32G32B32_SFLOAT;
      }

      static VkVertexInputRate to_vulkan_vertex_input_rate(
          VertexInputRate p_Rate)
      {
        switch (p_Rate) {
        case VertexInputRate::Vertex:
          return VK_VERTEX_INPUT_RATE_VERTEX;
        case VertexInputRate::Instance:
          return VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        LOW_ASSERT(false, "Unsupported vertex input rate");
        return VK_VERTEX_INPUT_RATE_VERTEX;
      }

      static VkPrimitiveTopology to_vulkan_topology(
          PrimitiveTopology p_Topology)
      {
        switch (p_Topology) {
        case PrimitiveTopology::TriangleList:
          return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
          return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList:
          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:
          return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::PointList:
          return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        LOW_ASSERT(false, "Unsupported primitive topology");
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      }

      static VkPolygonMode to_vulkan_polygon_mode(
          PolygonMode p_Mode)
      {
        switch (p_Mode) {
        case PolygonMode::Fill:
          return VK_POLYGON_MODE_FILL;
        case PolygonMode::Line:
          return VK_POLYGON_MODE_LINE;
        }

        LOW_ASSERT(false, "Unsupported polygon mode");
        return VK_POLYGON_MODE_FILL;
      }

      static VkCullModeFlags to_vulkan_cull_mode(CullMode p_Mode)
      {
        switch (p_Mode) {
        case CullMode::None:
          return VK_CULL_MODE_NONE;
        case CullMode::Front:
          return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:
          return VK_CULL_MODE_BACK_BIT;
        case CullMode::FrontAndBack:
          return VK_CULL_MODE_FRONT_AND_BACK;
        }

        LOW_ASSERT(false, "Unsupported cull mode");
        return VK_CULL_MODE_BACK_BIT;
      }

      static VkFrontFace to_vulkan_front_face(FrontFace p_FrontFace)
      {
        switch (p_FrontFace) {
        case FrontFace::CounterClockwise:
          return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case FrontFace::Clockwise:
          return VK_FRONT_FACE_CLOCKWISE;
        }

        LOW_ASSERT(false, "Unsupported front face");
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
      }

      static VkCompareOp to_vulkan_compare_op(CompareOp p_Compare)
      {
        switch (p_Compare) {
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

        LOW_ASSERT(false, "Unsupported compare op");
        return VK_COMPARE_OP_LESS_OR_EQUAL;
      }

      static VkBlendFactor to_vulkan_blend_factor(
          BlendFactor p_Factor)
      {
        switch (p_Factor) {
        case BlendFactor::Zero:
          return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
          return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcAlpha:
          return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
          return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:
          return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
          return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor::SrcColor:
          return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
          return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:
          return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
          return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        }

        LOW_ASSERT(false, "Unsupported blend factor");
        return VK_BLEND_FACTOR_ONE;
      }

      static VkBlendOp to_vulkan_blend_op(BlendOp p_Op)
      {
        switch (p_Op) {
        case BlendOp::Add:
          return VK_BLEND_OP_ADD;
        case BlendOp::Subtract:
          return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
          return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:
          return VK_BLEND_OP_MIN;
        case BlendOp::Max:
          return VK_BLEND_OP_MAX;
        }

        LOW_ASSERT(false, "Unsupported blend op");
        return VK_BLEND_OP_ADD;
      }

      static VkColorComponentFlags to_vulkan_color_write_mask(
          ColorWriteMask p_Mask)
      {
        VkColorComponentFlags l_Mask = 0;
        if ((static_cast<u8>(p_Mask) &
             static_cast<u8>(ColorWriteMask::R)) != 0) {
          l_Mask |= VK_COLOR_COMPONENT_R_BIT;
        }
        if ((static_cast<u8>(p_Mask) &
             static_cast<u8>(ColorWriteMask::G)) != 0) {
          l_Mask |= VK_COLOR_COMPONENT_G_BIT;
        }
        if ((static_cast<u8>(p_Mask) &
             static_cast<u8>(ColorWriteMask::B)) != 0) {
          l_Mask |= VK_COLOR_COMPONENT_B_BIT;
        }
        if ((static_cast<u8>(p_Mask) &
             static_cast<u8>(ColorWriteMask::A)) != 0) {
          l_Mask |= VK_COLOR_COMPONENT_A_BIT;
        }
        return l_Mask;
      }

      static VkImageLayout to_vulkan_descriptor_image_layout(
          DescriptorType p_Type, ImageState p_State)
      {
        if (p_Type == DescriptorType::StorageImage) {
          return VK_IMAGE_LAYOUT_GENERAL;
        }

        switch (p_State) {
        case ImageState::ShaderRead:
          return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageState::ShaderWrite:
          return VK_IMAGE_LAYOUT_GENERAL;
        default:
          break;
        }

        LOW_ASSERT(false,
                   "Unsupported image state for descriptor binding");
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }

      static VulkanContextState &get_context_state(
          Detail::ContextImpl &p_Context)
      {
        VulkanContextState *l_State =
            static_cast<VulkanContextState *>(p_Context.backend_state);
        LOW_ASSERT(l_State, "Missing Vulkan context state");
        return *l_State;
      }

      Detail::BackendShaderModule create_shader_module(
          Detail::ContextImpl &p_Context,
          const ShaderModuleDesc &p_Desc)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanShaderModuleState *l_Shader =
            new VulkanShaderModuleState();

        VkShaderModuleCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        l_Info.codeSize = p_Desc.code.size() * sizeof(u32);
        l_Info.pCode = p_Desc.code.data();

        VkResult l_Result = vkCreateShaderModule(
            l_State.device, &l_Info, nullptr, &l_Shader->shader_module);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan shader module: {}",
                       static_cast<int>(l_Result));
          delete l_Shader;
          LOW_ASSERT(false, "Failed to create Vulkan shader module");
        }

        Detail::BackendShaderModule l_Backend;
        l_Backend.format = p_Desc.format;
        l_Backend.backend_state = l_Shader;
        return l_Backend;
      }

      void destroy_shader_module(
          Detail::ContextImpl &p_Context,
          Detail::BackendShaderModule &p_ShaderModule)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanShaderModuleState *l_Shader =
            static_cast<VulkanShaderModuleState *>(
                p_ShaderModule.backend_state);
        if (l_Shader) {
          if (l_Shader->shader_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(l_State.device,
                                  l_Shader->shader_module, nullptr);
          }
          delete l_Shader;
        }

        p_ShaderModule.backend_state = nullptr;
      }

      Detail::BackendBindGroupLayout create_bind_group_layout(
          Detail::ContextImpl &p_Context,
          const BindGroupLayoutDesc &p_Desc)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanBindGroupLayoutState *l_Layout =
            new VulkanBindGroupLayoutState();

        Util::List<VkDescriptorSetLayoutBinding> l_Bindings;
        l_Bindings.resize(p_Desc.entries.size());
        for (u32 i = 0; i < p_Desc.entries.size(); ++i) {
          const BindGroupLayoutEntry &i_Entry = p_Desc.entries[i];
          l_Bindings[i].binding = i_Entry.binding;
          l_Bindings[i].descriptorType =
              to_vulkan_descriptor_type(i_Entry.type);
          l_Bindings[i].descriptorCount = i_Entry.count;
          l_Bindings[i].stageFlags =
              to_vulkan_shader_stages(i_Entry.stages);
        }

        VkDescriptorSetLayoutCreateInfo l_Info{};
        l_Info.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        l_Info.bindingCount = l_Bindings.size();
        l_Info.pBindings = l_Bindings.data();

        VkResult l_Result = vkCreateDescriptorSetLayout(
            l_State.device, &l_Info, nullptr,
            &l_Layout->descriptor_set_layout);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(
              p_Context, LogLevel::Error,
              "Failed to create Vulkan descriptor set layout: {}",
              static_cast<int>(l_Result));
          delete l_Layout;
          LOW_ASSERT(false,
                     "Failed to create Vulkan descriptor set layout");
        }

        Detail::BackendBindGroupLayout l_Backend;
        l_Backend.entries.assign(p_Desc.entries.begin(),
                                 p_Desc.entries.end());
        l_Backend.backend_state = l_Layout;
        return l_Backend;
      }

      void destroy_bind_group_layout(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroupLayout &p_BindGroupLayout)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanBindGroupLayoutState *l_Layout =
            static_cast<VulkanBindGroupLayoutState *>(
                p_BindGroupLayout.backend_state);
        if (l_Layout) {
          if (l_Layout->descriptor_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(
                l_State.device, l_Layout->descriptor_set_layout,
                nullptr);
          }
          delete l_Layout;
        }

        p_BindGroupLayout.entries.clear();
        p_BindGroupLayout.backend_state = nullptr;
      }

      Detail::BackendPipelineLayout create_pipeline_layout(
          Detail::ContextImpl &p_Context,
          const PipelineLayoutDesc &p_Desc)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanPipelineLayoutState *l_Layout =
            new VulkanPipelineLayoutState();

        Util::List<VkDescriptorSetLayout> l_SetLayouts;
        l_SetLayouts.resize(p_Desc.bind_group_layouts.size());
        for (u32 i = 0; i < p_Desc.bind_group_layouts.size(); ++i) {
          Detail::BackendBindGroupLayout *i_Layout =
              p_Context.bind_group_layouts.get(
                  p_Desc.bind_group_layouts[i]);
          LOW_ASSERT(i_Layout,
                     "Invalid bind group layout in pipeline layout");
          VulkanBindGroupLayoutState *i_State =
              static_cast<VulkanBindGroupLayoutState *>(
                  i_Layout->backend_state);
          LOW_ASSERT(i_State && i_State->descriptor_set_layout !=
                                    VK_NULL_HANDLE,
                     "Missing Vulkan descriptor set layout");
          l_SetLayouts[i] = i_State->descriptor_set_layout;
        }

        VkPipelineLayoutCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_Info.setLayoutCount = l_SetLayouts.size();
        l_Info.pSetLayouts = l_SetLayouts.data();

        VkResult l_Result = vkCreatePipelineLayout(
            l_State.device, &l_Info, nullptr,
            &l_Layout->pipeline_layout);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(p_Context, LogLevel::Error,
                       "Failed to create Vulkan pipeline layout: {}",
                       static_cast<int>(l_Result));
          delete l_Layout;
          LOW_ASSERT(false, "Failed to create Vulkan pipeline layout");
        }

        Detail::BackendPipelineLayout l_Backend;
        l_Backend.bind_group_layouts.assign(
            p_Desc.bind_group_layouts.begin(),
            p_Desc.bind_group_layouts.end());
        l_Backend.backend_state = l_Layout;
        return l_Backend;
      }

      void destroy_pipeline_layout(
          Detail::ContextImpl &p_Context,
          Detail::BackendPipelineLayout &p_PipelineLayout)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanPipelineLayoutState *l_Layout =
            static_cast<VulkanPipelineLayoutState *>(
                p_PipelineLayout.backend_state);
        if (l_Layout) {
          if (l_Layout->pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(l_State.device,
                                    l_Layout->pipeline_layout,
                                    nullptr);
          }
          delete l_Layout;
        }

        p_PipelineLayout.bind_group_layouts.clear();
        p_PipelineLayout.backend_state = nullptr;
      }

      static void write_bind_group_entries(
          VulkanContextState &p_State, Detail::ContextImpl &p_Context,
          VkDescriptorSet p_DescriptorSet,
          Util::Span<const BindGroupEntry> p_Entries)
      {
        Util::List<VkDescriptorBufferInfo> l_BufferInfos;
        Util::List<VkDescriptorImageInfo> l_ImageInfos;
        Util::List<VkWriteDescriptorSet> l_Writes;
        l_BufferInfos.reserve(p_Entries.size());
        l_ImageInfos.reserve(p_Entries.size());
        l_Writes.reserve(p_Entries.size());

        for (const BindGroupEntry &i_Entry : p_Entries) {
          VkWriteDescriptorSet l_Write{};
          l_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          l_Write.dstSet = p_DescriptorSet;
          l_Write.dstBinding = i_Entry.binding;
          l_Write.dstArrayElement = i_Entry.array_element;
          l_Write.descriptorCount = 1;
          l_Write.descriptorType =
              to_vulkan_descriptor_type(i_Entry.type);

          if (i_Entry.type == DescriptorType::UniformBuffer ||
              i_Entry.type == DescriptorType::StorageBuffer) {
            Detail::BackendBuffer *i_Buffer =
                p_Context.buffers.get(i_Entry.buffer.buffer);
            VulkanBufferState *i_BufferState =
                static_cast<VulkanBufferState *>(
                    i_Buffer->backend_state);
            VkDescriptorBufferInfo l_BufferInfo{};
            l_BufferInfo.buffer = i_BufferState->buffer;
            l_BufferInfo.offset = i_Entry.buffer.offset;
            l_BufferInfo.range =
                i_Entry.buffer.range == LOW_UINT64_MAX
                    ? VK_WHOLE_SIZE
                    : i_Entry.buffer.range;
            l_BufferInfos.push_back(l_BufferInfo);
            l_Write.pBufferInfo = &l_BufferInfos.back();
          } else {
            VkDescriptorImageInfo l_ImageInfo{};
            if (i_Entry.type != DescriptorType::Sampler) {
              Detail::BackendImageView *i_ImageView =
                  p_Context.image_views.get(i_Entry.image.view);
              VulkanImageViewState *i_ImageViewState =
                  static_cast<VulkanImageViewState *>(
                      i_ImageView->backend_state);
              l_ImageInfo.imageView = i_ImageViewState->image_view;
              l_ImageInfo.imageLayout =
                  to_vulkan_descriptor_image_layout(
                      i_Entry.type, i_Entry.image.state);
            }
            if (i_Entry.type == DescriptorType::Sampler ||
                i_Entry.type ==
                    DescriptorType::CombinedImageSampler) {
              Detail::BackendSampler *i_Sampler =
                  p_Context.samplers.get(i_Entry.sampler);
              VulkanSamplerState *i_SamplerState =
                  static_cast<VulkanSamplerState *>(
                      i_Sampler->backend_state);
              l_ImageInfo.sampler = i_SamplerState->sampler;
            }
            l_ImageInfos.push_back(l_ImageInfo);
            l_Write.pImageInfo = &l_ImageInfos.back();
          }

          l_Writes.push_back(l_Write);
        }

        vkUpdateDescriptorSets(p_State.device, l_Writes.size(),
                               l_Writes.data(), 0, nullptr);
      }

      static VkDescriptorPool get_descriptor_pool(
          VulkanContextState &p_State)
      {
        VulkanDescriptorAllocatorGrowable &l_Allocator =
            p_State.descriptor_allocator;

        if (!l_Allocator.ready_pools.empty()) {
          VkDescriptorPool l_Pool = l_Allocator.ready_pools.back();
          l_Allocator.ready_pools.pop_back();
          return l_Pool;
        }

        Util::List<VkDescriptorPoolSize> l_PoolSizes;
        l_PoolSizes.reserve(l_Allocator.ratios.size());
        for (const VulkanDescriptorPoolSizeRatio &i_Ratio :
             l_Allocator.ratios) {
          VkDescriptorPoolSize l_Size{};
          l_Size.type = i_Ratio.type;
          l_Size.descriptorCount =
              static_cast<u32>(i_Ratio.ratio *
                                l_Allocator.sets_per_pool);
          if (l_Size.descriptorCount == 0) {
            l_Size.descriptorCount = 1;
          }
          l_PoolSizes.push_back(l_Size);
        }

        VkDescriptorPoolCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        l_Info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        l_Info.maxSets = l_Allocator.sets_per_pool;
        l_Info.poolSizeCount = l_PoolSizes.size();
        l_Info.pPoolSizes = l_PoolSizes.data();

        VkDescriptorPool l_Pool = VK_NULL_HANDLE;
        VkResult l_Result = vkCreateDescriptorPool(
            p_State.device, &l_Info, nullptr, &l_Pool);
        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to create Vulkan descriptor pool");

        l_Allocator.sets_per_pool =
            static_cast<u32>(l_Allocator.sets_per_pool * 1.5f);
        if (l_Allocator.sets_per_pool > 4092) {
          l_Allocator.sets_per_pool = 4092;
        }

        return l_Pool;
      }

      static void allocate_descriptor_set(
          VulkanContextState &p_State, VkDescriptorSetLayout p_Layout,
          VulkanBindGroupState &p_BindGroup)
      {
        VulkanDescriptorAllocatorGrowable &l_Allocator =
            p_State.descriptor_allocator;
        VkDescriptorPool l_Pool = get_descriptor_pool(p_State);

        VkDescriptorSetAllocateInfo l_Info{};
        l_Info.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        l_Info.descriptorPool = l_Pool;
        l_Info.descriptorSetCount = 1;
        l_Info.pSetLayouts = &p_Layout;

        VkResult l_Result = vkAllocateDescriptorSets(
            p_State.device, &l_Info, &p_BindGroup.descriptor_set);

        if (l_Result == VK_ERROR_OUT_OF_POOL_MEMORY ||
            l_Result == VK_ERROR_FRAGMENTED_POOL) {
          l_Allocator.full_pools.push_back(l_Pool);
          l_Pool = get_descriptor_pool(p_State);
          l_Info.descriptorPool = l_Pool;
          l_Result = vkAllocateDescriptorSets(p_State.device, &l_Info,
                                              &p_BindGroup
                                                   .descriptor_set);
        }

        LOW_ASSERT(l_Result == VK_SUCCESS,
                   "Failed to allocate Vulkan descriptor set");
        p_BindGroup.descriptor_pool = l_Pool;
        l_Allocator.ready_pools.push_back(l_Pool);
      }

      Detail::BackendBindGroup
      create_bind_group(Detail::ContextImpl &p_Context,
                        const BindGroupDesc &p_Desc)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanBindGroupState *l_BindGroup = new VulkanBindGroupState();

        Detail::BackendBindGroupLayout *l_Layout =
            p_Context.bind_group_layouts.get(p_Desc.layout);
        LOW_ASSERT(l_Layout, "Invalid bind group layout");
        VulkanBindGroupLayoutState *l_LayoutState =
            static_cast<VulkanBindGroupLayoutState *>(
                l_Layout->backend_state);
        LOW_ASSERT(l_LayoutState,
                   "Missing Vulkan bind group layout state");

        allocate_descriptor_set(l_State,
                                l_LayoutState->descriptor_set_layout,
                                *l_BindGroup);
        write_bind_group_entries(l_State, p_Context,
                                 l_BindGroup->descriptor_set,
                                 p_Desc.entries);

        Detail::BackendBindGroup l_Backend;
        l_Backend.layout = p_Desc.layout;
        l_Backend.backend_state = l_BindGroup;
        return l_Backend;
      }

      void update_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroup &p_BindGroup,
          Util::Span<const BindGroupEntry> p_Entries)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanBindGroupState *l_BindGroup =
            static_cast<VulkanBindGroupState *>(
                p_BindGroup.backend_state);
        LOW_ASSERT(l_BindGroup &&
                       l_BindGroup->descriptor_set != VK_NULL_HANDLE,
                   "Cannot update empty Vulkan bind group");

        write_bind_group_entries(l_State, p_Context,
                                 l_BindGroup->descriptor_set,
                                 p_Entries);
      }

      void destroy_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendBindGroup &p_BindGroup)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanBindGroupState *l_BindGroup =
            static_cast<VulkanBindGroupState *>(
                p_BindGroup.backend_state);
        if (l_BindGroup) {
          if (l_BindGroup->descriptor_pool != VK_NULL_HANDLE &&
              l_BindGroup->descriptor_set != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(l_State.device,
                                 l_BindGroup->descriptor_pool, 1,
                                 &l_BindGroup->descriptor_set);
          }
          delete l_BindGroup;
        }

        p_BindGroup.layout = BindGroupLayout{};
        p_BindGroup.backend_state = nullptr;
      }

      Detail::BackendGraphicsPipeline create_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          const GraphicsPipelineDesc &p_Desc)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanGraphicsPipelineState *l_Pipeline =
            new VulkanGraphicsPipelineState();

        Detail::BackendPipelineLayout *l_PipelineLayout =
            p_Context.pipeline_layouts.get(p_Desc.layout);
        VulkanPipelineLayoutState *l_PipelineLayoutState =
            static_cast<VulkanPipelineLayoutState *>(
                l_PipelineLayout->backend_state);

        Util::List<VkPipelineShaderStageCreateInfo> l_ShaderStages;
        l_ShaderStages.resize(p_Desc.shaders.size());
        for (u32 i = 0; i < p_Desc.shaders.size(); ++i) {
          const ShaderStageDesc &i_Shader = p_Desc.shaders[i];
          Detail::BackendShaderModule *i_Module =
              p_Context.shader_modules.get(i_Shader.module);
          VulkanShaderModuleState *i_ModuleState =
              static_cast<VulkanShaderModuleState *>(
                  i_Module->backend_state);

          l_ShaderStages[i].sType =
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          l_ShaderStages[i].stage =
              to_vulkan_shader_stage(i_Shader.stage);
          l_ShaderStages[i].module = i_ModuleState->shader_module;
          l_ShaderStages[i].pName = i_Shader.entry_point;
        }

        Util::List<VkVertexInputBindingDescription> l_VertexBindings;
        l_VertexBindings.resize(p_Desc.vertex_buffers.size());
        for (u32 i = 0; i < p_Desc.vertex_buffers.size(); ++i) {
          const VertexBufferLayoutDesc &i_Buffer =
              p_Desc.vertex_buffers[i];
          l_VertexBindings[i].binding = i_Buffer.binding;
          l_VertexBindings[i].stride = i_Buffer.stride;
          l_VertexBindings[i].inputRate =
              to_vulkan_vertex_input_rate(i_Buffer.input_rate);
        }

        Util::List<VkVertexInputAttributeDescription>
            l_VertexAttributes;
        l_VertexAttributes.resize(p_Desc.vertex_attributes.size());
        for (u32 i = 0; i < p_Desc.vertex_attributes.size(); ++i) {
          const VertexAttributeDesc &i_Attribute =
              p_Desc.vertex_attributes[i];
          l_VertexAttributes[i].location = i_Attribute.location;
          l_VertexAttributes[i].binding = i_Attribute.binding;
          l_VertexAttributes[i].format =
              to_vulkan_vertex_format(i_Attribute.format);
          l_VertexAttributes[i].offset = i_Attribute.offset;
        }

        VkPipelineVertexInputStateCreateInfo l_VertexInput{};
        l_VertexInput.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        l_VertexInput.vertexBindingDescriptionCount =
            l_VertexBindings.size();
        l_VertexInput.pVertexBindingDescriptions =
            l_VertexBindings.data();
        l_VertexInput.vertexAttributeDescriptionCount =
            l_VertexAttributes.size();
        l_VertexInput.pVertexAttributeDescriptions =
            l_VertexAttributes.data();

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology =
            to_vulkan_topology(p_Desc.topology);

        VkPipelineViewportStateCreateInfo l_Viewport{};
        l_Viewport.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_Viewport.viewportCount = 1;
        l_Viewport.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Raster{};
        l_Raster.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Raster.polygonMode =
            to_vulkan_polygon_mode(p_Desc.polygon_mode);
        l_Raster.cullMode = to_vulkan_cull_mode(p_Desc.cull_mode);
        l_Raster.frontFace = to_vulkan_front_face(p_Desc.front_face);
        l_Raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo l_Multisample{};
        l_Multisample.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo l_DepthStencil{};
        l_DepthStencil.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        l_DepthStencil.depthTestEnable =
            p_Desc.depth_test_enabled ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthWriteEnable =
            p_Desc.depth_write_enabled ? VK_TRUE : VK_FALSE;
        l_DepthStencil.depthCompareOp =
            to_vulkan_compare_op(p_Desc.depth_compare);

        Util::List<VkPipelineColorBlendAttachmentState>
            l_BlendAttachments;
        l_BlendAttachments.resize(p_Desc.color_targets.size());
        for (u32 i = 0; i < p_Desc.color_targets.size(); ++i) {
          const ColorTargetDesc &i_Target = p_Desc.color_targets[i];
          l_BlendAttachments[i].blendEnable =
              i_Target.blend_enabled ? VK_TRUE : VK_FALSE;
          l_BlendAttachments[i].srcColorBlendFactor =
              to_vulkan_blend_factor(i_Target.src_color_factor);
          l_BlendAttachments[i].dstColorBlendFactor =
              to_vulkan_blend_factor(i_Target.dst_color_factor);
          l_BlendAttachments[i].colorBlendOp =
              to_vulkan_blend_op(i_Target.color_op);
          l_BlendAttachments[i].srcAlphaBlendFactor =
              to_vulkan_blend_factor(i_Target.src_alpha_factor);
          l_BlendAttachments[i].dstAlphaBlendFactor =
              to_vulkan_blend_factor(i_Target.dst_alpha_factor);
          l_BlendAttachments[i].alphaBlendOp =
              to_vulkan_blend_op(i_Target.alpha_op);
          l_BlendAttachments[i].colorWriteMask =
              to_vulkan_color_write_mask(i_Target.write_mask);
        }

        VkPipelineColorBlendStateCreateInfo l_Blend{};
        l_Blend.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_Blend.attachmentCount = l_BlendAttachments.size();
        l_Blend.pAttachments = l_BlendAttachments.data();

        VkDynamicState l_DynamicStates[2] = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo l_Dynamic{};
        l_Dynamic.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_Dynamic.dynamicStateCount = 2;
        l_Dynamic.pDynamicStates = l_DynamicStates;

        Util::List<VkFormat> l_ColorFormats;
        l_ColorFormats.resize(p_Desc.color_targets.size());
        for (u32 i = 0; i < p_Desc.color_targets.size(); ++i) {
          l_ColorFormats[i] =
              to_vulkan_format(p_Desc.color_targets[i].format);
        }

        VkPipelineRenderingCreateInfo l_Rendering{};
        l_Rendering.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        l_Rendering.colorAttachmentCount = l_ColorFormats.size();
        l_Rendering.pColorAttachmentFormats = l_ColorFormats.data();
        l_Rendering.depthAttachmentFormat =
            to_vulkan_format(p_Desc.depth_format);

        VkGraphicsPipelineCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_Info.pNext = &l_Rendering;
        l_Info.stageCount = l_ShaderStages.size();
        l_Info.pStages = l_ShaderStages.data();
        l_Info.pVertexInputState = &l_VertexInput;
        l_Info.pInputAssemblyState = &l_InputAssembly;
        l_Info.pViewportState = &l_Viewport;
        l_Info.pRasterizationState = &l_Raster;
        l_Info.pMultisampleState = &l_Multisample;
        l_Info.pDepthStencilState = &l_DepthStencil;
        l_Info.pColorBlendState = &l_Blend;
        l_Info.pDynamicState = &l_Dynamic;
        l_Info.layout = l_PipelineLayoutState->pipeline_layout;
        l_Info.renderPass = VK_NULL_HANDLE;
        l_Info.subpass = 0;

        VkResult l_Result = vkCreateGraphicsPipelines(
            l_State.device, VK_NULL_HANDLE, 1, &l_Info, nullptr,
            &l_Pipeline->pipeline);
        if (l_Result != VK_SUCCESS) {
          Detail::logf(
              p_Context, LogLevel::Error,
              "Failed to create Vulkan graphics pipeline: {}",
              static_cast<int>(l_Result));
          delete l_Pipeline;
          LOW_ASSERT(false,
                     "Failed to create Vulkan graphics pipeline");
        }

        Detail::BackendGraphicsPipeline l_Backend;
        l_Backend.layout = p_Desc.layout;
        l_Backend.backend_state = l_Pipeline;
        return l_Backend;
      }

      void destroy_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendGraphicsPipeline &p_GraphicsPipeline)
      {
        VulkanContextState &l_State = get_context_state(p_Context);
        VulkanGraphicsPipelineState *l_Pipeline =
            static_cast<VulkanGraphicsPipelineState *>(
                p_GraphicsPipeline.backend_state);
        if (l_Pipeline) {
          if (l_Pipeline->pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(l_State.device, l_Pipeline->pipeline,
                              nullptr);
          }
          delete l_Pipeline;
        }

        p_GraphicsPipeline.layout = PipelineLayout{};
        p_GraphicsPipeline.backend_state = nullptr;
      }

      void bind_graphics_pipeline(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendGraphicsPipeline &p_GraphicsPipeline)
      {
        get_context_state(p_Context);
        VulkanCommandListState *l_CommandList =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandList &&
                       l_CommandList->command_buffer !=
                           VK_NULL_HANDLE,
                   "Cannot bind graphics pipeline without Vulkan "
                   "command buffer");
        VulkanGraphicsPipelineState *l_Pipeline =
            static_cast<VulkanGraphicsPipelineState *>(
                p_GraphicsPipeline.backend_state);
        LOW_ASSERT(l_Pipeline &&
                       l_Pipeline->pipeline != VK_NULL_HANDLE,
                   "Cannot bind empty Vulkan graphics pipeline");

        vkCmdBindPipeline(l_CommandList->command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          l_Pipeline->pipeline);
      }

      void bind_bind_group(
          Detail::ContextImpl &p_Context,
          Detail::BackendCommandList &p_CommandList,
          Detail::BackendPipelineLayout &p_PipelineLayout,
          u32 p_GroupIndex, Detail::BackendBindGroup &p_BindGroup)
      {
        get_context_state(p_Context);
        VulkanCommandListState *l_CommandList =
            static_cast<VulkanCommandListState *>(
                p_CommandList.backend_state);
        LOW_ASSERT(l_CommandList &&
                       l_CommandList->command_buffer !=
                           VK_NULL_HANDLE,
                   "Cannot bind bind group without Vulkan command "
                   "buffer");
        VulkanPipelineLayoutState *l_PipelineLayout =
            static_cast<VulkanPipelineLayoutState *>(
                p_PipelineLayout.backend_state);
        LOW_ASSERT(l_PipelineLayout &&
                       l_PipelineLayout->pipeline_layout !=
                           VK_NULL_HANDLE,
                   "Cannot bind bind group without Vulkan pipeline "
                   "layout");
        VulkanBindGroupState *l_BindGroup =
            static_cast<VulkanBindGroupState *>(
                p_BindGroup.backend_state);
        LOW_ASSERT(l_BindGroup &&
                       l_BindGroup->descriptor_set != VK_NULL_HANDLE,
                   "Cannot bind empty Vulkan descriptor set");

        vkCmdBindDescriptorSets(
            l_CommandList->command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            l_PipelineLayout->pipeline_layout, p_GroupIndex, 1,
            &l_BindGroup->descriptor_set, 0, nullptr);
      }
    } // namespace Vulkan
  } // namespace Gfx
} // namespace Low
