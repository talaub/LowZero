#include "LowRendererVulkanPipeline.h"

#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanBase.h"
#include "LowRendererVulkanPipelineManager.h"

#include "LowUtilContainers.h"
#include "LowUtil.h"

#include <fstream>
#include <vulkan/vulkan_core.h>

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
          VkPipelineLayoutCreateInfo l_Info = {};
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
          VkPipelineShaderStageCreateInfo l_Info = {};
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

        GraphicsPipelineBuilder
        GraphicsPipelineBuilder::prepare_fullscreen_effect(
            VkPipelineLayout p_Layout, Util::String p_ShaderPath,
            VkFormat p_OutImageFormat)
        {
          PipelineUtil::GraphicsPipelineBuilder l_Builder;
          l_Builder.set_shaders("fullscreen_triangle.vert",
                                p_ShaderPath);
          l_Builder.pipelineLayout = p_Layout;
          l_Builder.set_input_topology(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
          l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
          l_Builder.set_cull_mode(VK_CULL_MODE_BACK_BIT,
                                  VK_FRONT_FACE_CLOCKWISE);
          l_Builder.set_multismapling_none();
          l_Builder.disable_blending();
          l_Builder.disable_depth_test();

          l_Builder.colorAttachmentFormats.clear();
          l_Builder.colorAttachmentFormats.push_back(
              p_OutImageFormat);

          l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

          return l_Builder;
        }

        void GraphicsPipelineBuilder::clear()
        {
          inputAssembly = {};
          inputAssembly.sType =
              VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

          rasterizer = {};
          rasterizer.sType =
              VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

          colorBlendAttachment = {};

          multisampling = {};
          multisampling.sType =
              VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

          pipelineLayout = {};

          depthStencil = {};
          depthStencil.sType =
              VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

          renderInfo = {};
          renderInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

          shaderStages.clear();
        }

        Pipeline GraphicsPipelineBuilder::register_pipeline()
        {
          Pipeline l_Pipeline = Pipeline::make(N(Pipeline));

          l_Pipeline.set_layout(pipelineLayout);

          PipelineManager::register_graphics_pipeline(l_Pipeline,
                                                      *this);

          return l_Pipeline;
        }

        VkPipeline
        GraphicsPipelineBuilder::build_pipeline(VkDevice p_Device)
        {
          renderInfo.colorAttachmentCount =
              colorAttachmentFormats.size();
          renderInfo.pColorAttachmentFormats =
              colorAttachmentFormats.data();

          VkPipelineViewportStateCreateInfo l_ViewportState = {};
          l_ViewportState.sType =
              VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
          l_ViewportState.pNext = nullptr;

          l_ViewportState.viewportCount = 1;
          l_ViewportState.scissorCount = 1;

          // TODO: Right now this is just a dummy
          VkPipelineColorBlendStateCreateInfo l_BlendStateCreateInfo;

          Util::List<VkPipelineColorBlendAttachmentState>
              l_BlendAttachments;

          for (u32 i = 0u; i < colorAttachmentFormats.size(); ++i) {
            l_BlendAttachments.push_back(colorBlendAttachment);
          }

          l_BlendStateCreateInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
          l_BlendStateCreateInfo.pNext = nullptr;

          l_BlendStateCreateInfo.logicOpEnable = VK_FALSE;
          l_BlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
          l_BlendStateCreateInfo.attachmentCount =
              l_BlendAttachments.size();
          l_BlendStateCreateInfo.pAttachments =
              l_BlendAttachments.data();

          // Clear for now
          VkPipelineVertexInputStateCreateInfo l_VertexInputInfo = {};
          l_VertexInputInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

          // Create the pipeline
          VkGraphicsPipelineCreateInfo l_PipelineInfo = {};
          l_PipelineInfo.sType =
              VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
          // Connect the renderinfo
          l_PipelineInfo.pNext = &renderInfo;

          inputAssembly.pNext = nullptr;

          l_PipelineInfo.stageCount = (u32)shaderStages.size();
          l_PipelineInfo.pStages = shaderStages.data();
          l_PipelineInfo.pVertexInputState = &l_VertexInputInfo;
          l_PipelineInfo.pInputAssemblyState = &inputAssembly;
          l_PipelineInfo.pViewportState = &l_ViewportState;
          l_PipelineInfo.pRasterizationState = &rasterizer;
          l_PipelineInfo.pMultisampleState = &multisampling;
          l_PipelineInfo.pColorBlendState = &l_BlendStateCreateInfo;
          l_PipelineInfo.pDepthStencilState = &depthStencil;
          l_PipelineInfo.layout = pipelineLayout;

          VkDynamicState l_State[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};

          VkPipelineDynamicStateCreateInfo l_DynamicInfo = {};
          l_DynamicInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
          l_DynamicInfo.pDynamicStates = &l_State[0];
          l_DynamicInfo.dynamicStateCount = 2;

          l_PipelineInfo.pDynamicState = &l_DynamicInfo;

          VkPipeline l_NewPipeline;
          if (vkCreateGraphicsPipelines(
                  p_Device, VK_NULL_HANDLE, 1, &l_PipelineInfo,
                  nullptr, &l_NewPipeline) != VK_SUCCESS) {
            LOW_LOG_ERROR << "Failed to create graphics pipeline"
                          << LOW_LOG_END;

            clear_shader_modules(p_Device);

            return VK_NULL_HANDLE;
          } else {
            clear_shader_modules(p_Device);

            return l_NewPipeline;
          }
        }

        void GraphicsPipelineBuilder::clear_shader_modules(
            VkDevice p_Device)
        {
          for (auto it = shaderStages.begin();
               it != shaderStages.end(); ++it) {
            vkDestroyShaderModule(p_Device, it->module, nullptr);
          }

          shaderStages.clear();
        }

        void GraphicsPipelineBuilder::set_shaders(
            Util::String p_VertexShader,
            Util::String p_FragmentShader, bool p_Project)
        {
          vertexShaderPath = Util::get_project().engineDataPath +
                             "/lowr_shaders/" + p_VertexShader;
          fragmentShaderPath = Util::get_project().engineDataPath +
                               "/lowr_shaders/" + p_FragmentShader;

          vertexSpirvPath = Util::get_project().engineDataPath +
                            "/lowr_spirv/" + p_VertexShader + ".spv";
          fragmentSpirvPath = Util::get_project().engineDataPath +
                              "/lowr_spirv/" + p_FragmentShader +
                              ".spv";
        }

        void GraphicsPipelineBuilder::update_shaders()
        {
          VkShaderModule l_FragShader;
          LOW_ASSERT(PipelineUtil::load_shader_module(
                         fragmentSpirvPath.c_str(),
                         Global::get_device(), &l_FragShader),
                     "Failed to load fragment shader");

          VkShaderModule l_VertShader;
          LOW_ASSERT(PipelineUtil::load_shader_module(
                         vertexSpirvPath.c_str(),
                         Global::get_device(), &l_VertShader),
                     "Failed to load vertex shader");

          set_shaders(l_VertShader, l_FragShader);
        }

        void GraphicsPipelineBuilder::set_shaders(
            VkShaderModule p_VertexShader,
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

        void GraphicsPipelineBuilder::set_input_topology(
            VkPrimitiveTopology p_Topology)
        {
          inputAssembly.topology = p_Topology;
          // Only used for triangle strips and line strips
          inputAssembly.primitiveRestartEnable = VK_FALSE;
        }

        void GraphicsPipelineBuilder::set_polygon_mode(
            VkPolygonMode p_Mode, float p_LineWidth)
        {
          rasterizer.polygonMode = p_Mode;
          rasterizer.lineWidth = p_LineWidth;
        }

        void GraphicsPipelineBuilder::set_cull_mode(
            VkCullModeFlags p_CullMode, VkFrontFace p_FrontFace)
        {
          rasterizer.cullMode = p_CullMode;
          rasterizer.frontFace = p_FrontFace;
        }

        void GraphicsPipelineBuilder::set_multismapling_none()
        {
          multisampling.sampleShadingEnable = false;
          multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
          multisampling.minSampleShading = 1.0f;
          multisampling.pSampleMask = nullptr;
          multisampling.alphaToCoverageEnable = VK_FALSE;
          multisampling.alphaToOneEnable = VK_FALSE;
        }

        void GraphicsPipelineBuilder::disable_blending()
        {
          colorBlendAttachment.colorWriteMask =
              VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
          colorBlendAttachment.blendEnable = VK_FALSE;
        }

        void
        GraphicsPipelineBuilder::set_depth_format(VkFormat p_Format)
        {
          renderInfo.depthAttachmentFormat = p_Format;

          if (p_Format != VK_FORMAT_UNDEFINED) {
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_TRUE;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = {};
            depthStencil.back = {};
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
          }
        }

        void GraphicsPipelineBuilder::disable_depth_test()
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

        void ComputePipelineBuilder::clear()
        {
          shaderStage = {};
          shaderStage.sType =
              VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
          shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
          shaderStage.pName =
              "main"; // Default entry point for compute shaders

          pipelineLayout = VK_NULL_HANDLE;
          computeShaderPath.clear();
          computeSpirvPath.clear();
        }

        VkPipeline
        ComputePipelineBuilder::build_pipeline(VkDevice p_Device)
        {
          VkComputePipelineCreateInfo pipelineCreateInfo = {};
          pipelineCreateInfo.sType =
              VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
          pipelineCreateInfo.stage = shaderStage;
          pipelineCreateInfo.layout = pipelineLayout;

          VkPipeline pipeline;
          if (vkCreateComputePipelines(p_Device, VK_NULL_HANDLE, 1,
                                       &pipelineCreateInfo, nullptr,
                                       &pipeline) != VK_SUCCESS) {

            LOW_LOG_ERROR << "Failed to create compute pipeline"
                          << LOW_LOG_END;
            vkDestroyShaderModule(p_Device, shaderStage.module,
                                  nullptr);
            return VK_NULL_HANDLE;
          }
          vkDestroyShaderModule(p_Device, shaderStage.module,
                                nullptr);
          return pipeline;
        }

        void ComputePipelineBuilder::set_pipeline_layout(
            VkPipelineLayout p_PipelineLayout)
        {
          pipelineLayout = p_PipelineLayout;
        }

        void ComputePipelineBuilder::set_shader(
            Util::String p_ComputeShader)
        {
          computeShaderPath = Util::get_project().engineDataPath +
                              "/lowr_shaders/" + p_ComputeShader;

          computeSpirvPath = Util::get_project().engineDataPath +
                             "/lowr_spirv/" + p_ComputeShader +
                             ".spv";
        }

        void ComputePipelineBuilder::set_shader(
            VkShaderModule p_ComputeShader)
        {
          shaderStage = PipelineUtil::shader_stage_create_info(
              VK_SHADER_STAGE_COMPUTE_BIT, p_ComputeShader);
        }

        void ComputePipelineBuilder::update_shader()
        {
          if (!computeSpirvPath.empty()) {
            VkShaderModule l_ComputeShader;
            LOW_ASSERT(PipelineUtil::load_shader_module(
                           computeSpirvPath.c_str(),
                           Global::get_device(), &l_ComputeShader),
                       "Failed to load compute shader");

            set_shader(l_ComputeShader);
          }
        }

        Pipeline ComputePipelineBuilder::register_pipeline()
        {
          Pipeline l_Pipeline = Pipeline::make(N(Pipeline));

          l_Pipeline.set_layout(pipelineLayout);

          PipelineManager::register_compute_pipeline(l_Pipeline,
                                                     *this);

          return l_Pipeline;
        }
      } // namespace PipelineUtil
    }   // namespace Vulkan
  }     // namespace Renderer
} // namespace Low
