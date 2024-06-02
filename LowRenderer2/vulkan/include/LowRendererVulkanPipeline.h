#pragma once

#include <vulkan/vulkan.h>

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineUtil {
        bool load_shader_module(const char *p_FilePath,
                                VkDevice p_Device,
                                VkShaderModule *p_OutShaderModule);

        VkPipelineLayoutCreateInfo layout_create_info();

        VkPipelineShaderStageCreateInfo
        shader_stage_create_info(VkShaderStageFlagBits p_Stage,
                                 VkShaderModule p_ShaderModule,
                                 const char *p_Entry = "main");

        struct PipelineBuilder
        {
          Util::List<VkPipelineShaderStageCreateInfo> shaderStages;

          VkPipelineInputAssemblyStateCreateInfo inputAssembly;
          VkPipelineRasterizationStateCreateInfo rasterizer;
          VkPipelineColorBlendAttachmentState colorBlendAttachment;
          VkPipelineMultisampleStateCreateInfo multisampling;
          VkPipelineLayout pipelineLayout;
          VkPipelineDepthStencilStateCreateInfo depthStencil;
          VkPipelineRenderingCreateInfo renderInfo;
          VkFormat colorAttachmentFormat;

          PipelineBuilder()
          {
            clear();
          }

          void clear();

          VkPipeline build_pipeline(VkDevice p_Device);

          void set_shaders(VkShaderModule p_VertexShader,
                           VkShaderModule p_FragmentShader);
          void set_input_topology(VkPrimitiveTopology p_Topology);
          void set_polygon_mode(VkPolygonMode p_Mode,
                                float p_LineWidth = 1.0f);
          void set_cull_mode(VkCullModeFlags p_CullMode,
                             VkFrontFace p_FrontFace);
          void set_multismapling_none();
          void disable_blending();
          void set_color_attachment_format(VkFormat p_Format);
          void set_depth_format(VkFormat p_Format);
          void disable_depth_test();
        };
      } // namespace PipelineUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
