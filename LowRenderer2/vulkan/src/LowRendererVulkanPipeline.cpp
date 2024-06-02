#include "LowRendererVulkanPipeline.h"

#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanBase.h"

#include "LowUtilContainers.h"

#include <fstream>

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      namespace PipelineUtil {
        bool load_shader_module(const char *p_FilePath,
                                VkDevice p_Device,
                                VkShaderModule *p_OutShaderModule)
        {
          // Opens the file and has the cursor at the end of it
          std::ifstream l_File(p_FilePath,
                               std::ios::ate | std::ios::binary);

          if (!l_File.is_open()) {
            return false;
          }

          // Get the position of the cursor. Since the cursor is at
          // the end this gives us the size of the file in bytes
          size_t l_FileSize = (size_t)l_File.tellg();

          // SpirV uses U32
          Util::List<u32> l_Buffer(l_FileSize / sizeof(u32));

          // Position cursor back at the start of the file
          l_File.seekg(0);

          // Read the entire file into the buffer
          l_File.read((char *)l_Buffer.data(), l_FileSize);

          l_File.close();

          VkShaderModuleCreateInfo l_CreateInfo = {};
          l_CreateInfo.sType =
              VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
          l_CreateInfo.pNext = nullptr;

          // This needs to be in bytes (which is why we multiply it
          // buy the size of an int)
          l_CreateInfo.codeSize = l_Buffer.size() * sizeof(u32);
          l_CreateInfo.pCode = l_Buffer.data();

          VkShaderModule l_ShaderModule;
          LOWR_VK_CHECK_RETURN(vkCreateShaderModule(
              p_Device, &l_CreateInfo, nullptr, &l_ShaderModule));

          *p_OutShaderModule = l_ShaderModule;

          return true;
        }

        VkPipelineLayoutCreateInfo layout_create_info()
        {
          VkPipelineLayoutCreateInfo l_Info{};
          l_Info.sType =
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
          l_Info.pNext = nullptr;

          // empty defaults
          l_Info.flags = 0;
          l_Info.setLayoutCount = 0;
          l_Info.pSetLayouts = nullptr;
          l_Info.pushConstantRangeCount = 0;
          l_Info.pPushConstantRanges = nullptr;
          return l_Info;
        }

        VkPipelineShaderStageCreateInfo
        shader_stage_create_info(VkShaderStageFlagBits p_Stage,
                                 VkShaderModule p_ShaderModule,
                                 const char *p_Entry)
        {
          VkPipelineShaderStageCreateInfo l_Info{};
          l_Info.sType =
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          l_Info.pNext = nullptr;

          // shader stage
          l_Info.stage = p_Stage;
          // module containing the code for this shader stage
          l_Info.module = p_ShaderModule;
          // the entry point of the shader
          l_Info.pName = p_Entry;
          return l_Info;
        }

        void PipelineBuilder::clear()
        {
          inputAssembly = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

          rasterizer = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

          colorBlendAttachment = {};

          multisampling = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

          pipelineLayout = {};

          depthStencil = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

          renderInfo = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

          shaderStages.clear();
        }

        VkPipeline PipelineBuilder::build_pipeline(VkDevice p_Device)
        {
          VkPipelineViewportStateCreateInfo l_ViewportState = {};
          l_ViewportState.sType =
              VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
          l_ViewportState.pNext = nullptr;

          l_ViewportState.viewportCount = 1;
          l_ViewportState.scissorCount = 1;

          // TODO: Right now this is just a dummy
          VkPipelineColorBlendStateCreateInfo l_ColorBlending = {};
          l_ColorBlending.sType =
              VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
          l_ColorBlending.pNext = nullptr;

          l_ColorBlending.logicOpEnable = VK_FALSE;
          l_ColorBlending.logicOp = VK_LOGIC_OP_COPY;
          l_ColorBlending.attachmentCount = 1;
          l_ColorBlending.pAttachments = &colorBlendAttachment;

          // Clear for now
          VkPipelineVertexInputStateCreateInfo l_VertexInputInfo = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

          // Create the pipeline
          VkGraphicsPipelineCreateInfo l_PipelineInfo = {
              .sType =
                  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
          // Connect the renderinfo
          l_PipelineInfo.pNext = &renderInfo;

          l_PipelineInfo.stageCount = (u32)shaderStages.size();
          l_PipelineInfo.pStages = shaderStages.data();
          l_PipelineInfo.pVertexInputState = &l_VertexInputInfo;
          l_PipelineInfo.pInputAssemblyState = &inputAssembly;
          l_PipelineInfo.pViewportState = &l_ViewportState;
          l_PipelineInfo.pRasterizationState = &rasterizer;
          l_PipelineInfo.pMultisampleState = &multisampling;
          l_PipelineInfo.pColorBlendState = &l_ColorBlending;
          l_PipelineInfo.pDepthStencilState = &depthStencil;
          l_PipelineInfo.layout = pipelineLayout;

          VkDynamicState l_State[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};

          VkPipelineDynamicStateCreateInfo l_DynamicInfo = {
              .sType =
                  VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
          l_DynamicInfo.pDynamicStates = &l_State[0];
          l_DynamicInfo.dynamicStateCount = 2;

          l_PipelineInfo.pDynamicState = &l_DynamicInfo;

          VkPipeline l_NewPipeline;
          if (vkCreateGraphicsPipelines(
                  p_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo,
                  nullptr, &l_NewPipeline) != VK_SUCCESS) {
            LOW_LOG_ERROR << "Failed to create graphics pipeline"
                          << LOW_LOG_END;

            return VK_NULL_HANDLE;
          } else {
            return l_NewPipeline;
          }
        }

        void
        PipelineBuilder::set_shaders(VkShaderModule p_VertexShader,
                                     VkShaderModule p_FragmentShader)
        {
          shaderStages.clear();

          shaderStages.push_back(
              PipelineUtil::shader_stage_create_info(
                  VK_SHADER_STAGE_VERTEX_BIT, p_VertexShader));

          shaderStages.push_back(
              PipelineUtil::shader_stage_create_info(
                  VK_SHADER_STAGE_FRAGMENT_BIT, p_FragmentShader));
        }

        void PipelineBuilder::set_input_topology(
            VkPrimitiveTopology p_Topology)
        {
          inputAssembly.topology = p_Topology;
          // Only used for triangle strips and line strips
          inputAssembly.primitiveRestartEnable = VK_FALSE;
        }

        void PipelineBuilder::set_polygon_mode(VkPolygonMode p_Mode,
                                               float p_LineWidth)
        {
          rasterizer.polygonMode = p_Mode;
          rasterizer.lineWidth = p_LineWidth;
        }

        void
        PipelineBuilder::set_cull_mode(VkCullModeFlags p_CullMode,
                                       VkFrontFace p_FrontFace)
        {
          rasterizer.cullMode = p_CullMode;
          rasterizer.frontFace = p_FrontFace;
        }

        void PipelineBuilder::set_multismapling_none()
        {
          multisampling.sampleShadingEnable = false;
          multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
          multisampling.minSampleShading = 1.0f;
          multisampling.pSampleMask = nullptr;
          multisampling.alphaToCoverageEnable = VK_FALSE;
          multisampling.alphaToOneEnable = VK_FALSE;
        }

        void PipelineBuilder::disable_blending()
        {
          colorBlendAttachment.colorWriteMask =
              VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
          colorBlendAttachment.blendEnable = VK_FALSE;
        }

        void PipelineBuilder::set_color_attachment_format(
            VkFormat p_Format)
        {
          // TODO: Most likely revisit to allow drawing to multiple
          // attachments for GBuffer for instance
          colorAttachmentFormat = p_Format;
          renderInfo.colorAttachmentCount = 1;
          renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;
        }

        void PipelineBuilder::set_depth_format(VkFormat p_Format)
        {
          renderInfo.depthAttachmentFormat = p_Format;
        }

        void PipelineBuilder::disable_depth_test()
        {
          depthStencil.depthTestEnable = VK_FALSE;
          depthStencil.depthWriteEnable = VK_FALSE;
          depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
          depthStencil.depthBoundsTestEnable = VK_FALSE;
          depthStencil.stencilTestEnable = VK_FALSE;
          depthStencil.front = {};
          depthStencil.back = {};
          depthStencil.minDepthBounds = 0.0f;
          depthStencil.maxDepthBounds = 1.0f;
        }
      } // namespace PipelineUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
