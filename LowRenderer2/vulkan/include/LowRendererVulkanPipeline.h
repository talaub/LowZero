#pragma once

#include <vulkan/vulkan.h>

#include "LowUtilContainers.h"

#include "LowRendererVkPipeline.h"

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

        struct GraphicsPipelineBuilder
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

          Util::String vertexShaderPath;
          Util::String fragmentShaderPath;

          Util::String vertexSpirvPath;
          Util::String fragmentSpirvPath;

          GraphicsPipelineBuilder()
          {
            clear();
          }

          void clear();

          VkPipeline build_pipeline(VkDevice p_Device);

          Pipeline register_pipeline();

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

          void set_shaders(
                           Util::String p_VertexShader,
                           Util::String p_FragmentShader,
                           bool p_Project = false);

          void update_shaders();

        private:
          void set_shaders(VkShaderModule p_VertexShader,
                           VkShaderModule p_FragmentShader);

          void clear_shader_modules(VkDevice p_Device);
        };
      } // namespace PipelineUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
