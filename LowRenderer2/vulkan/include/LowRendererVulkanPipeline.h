#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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
          Util::List<VkFormat> colorAttachmentFormats;

          Util::String vertexShaderPath;
          Util::String fragmentShaderPath;

          Util::String vertexSpirvPath;
          Util::String fragmentSpirvPath;

          GraphicsPipelineBuilder()
          {
            clear();
          }

          void clear();

          void set_blending_alpha();

          VkPipeline build_pipeline(VkDevice p_Device);

          Pipeline register_pipeline();

          void set_input_topology(VkPrimitiveTopology p_Topology);
          void set_polygon_mode(VkPolygonMode p_Mode,
                                float p_LineWidth = 1.0f);
          void set_cull_mode(VkCullModeFlags p_CullMode,
                             VkFrontFace p_FrontFace);
          void set_multismapling_none();
          void disable_blending();
          void set_depth_format(VkFormat p_Format);
          void disable_depth_test();

          void set_shaders(Util::String p_VertexShader,
                           Util::String p_FragmentShader,
                           bool p_Project = false);

          void update_shaders();

          static GraphicsPipelineBuilder
          prepare_fullscreen_effect(VkPipelineLayout p_Layout,
                                    Util::String p_ShaderPath,
                                    VkFormat p_OutImageFormat);

        private:
          void set_shaders(VkShaderModule p_VertexShader,
                           VkShaderModule p_FragmentShader);

          void clear_shader_modules(VkDevice p_Device);
        };

        struct ComputePipelineBuilder
        {
          VkPipelineShaderStageCreateInfo shaderStage;
          VkPipelineLayout pipelineLayout;
          Util::String computeShaderPath;
          Util::String computeSpirvPath;

          ComputePipelineBuilder()
          {
            clear();
          }

          void clear();

          VkPipeline build_pipeline(VkDevice p_Device);

          Pipeline register_pipeline();

          void set_shader(Util::String p_ComputeShader);

          void set_pipeline_layout(VkPipelineLayout p_PipelineLayout);

          void update_shader();

        private:
          void set_shader(VkShaderModule p_ComputeShader);
        };
      } // namespace PipelineUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
