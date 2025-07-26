#include "LowRendererVulkanRenderer.h"

#include "LowRendererVulkanBase.h"
#include "LowRendererCompatibility.h"

#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanPipelineManager.h"
#include "LowRendererVulkanBuffer.h"
#include "LowRendererRenderView.h"
#include "LowRendererVkViewInfo.h"
#include "LowRendererDrawCommand.h"
#include "LowRendererTexture.h"

#include "LowUtil.h"
#include "LowUtilResource.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "LowRendererVkPipeline.h"
#include "LowRendererVkImage.h"

#include "LowRendererHelper.h"
#include "LowRendererVulkanRenderStepsBasic.h"
#include "LowRendererPointLight.h"
#include "LowRendererVkScene.h"

#define POINTLIGHTS_PER_CLUSTER 32u
#define LIGHT_CLUSTER_DEPTH 24u

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      Context g_Context;
      Texture g_DefaultTexture;

      Vulkan::Pipeline create_pipeline_from_config(
          const GraphicsPipelineConfig &p_Config,
          VkPipelineLayout p_PipelineLayout);

      struct ComputePushConstants
      {
        Math::Vector4 data1;
        Math::Vector4 data2;
        Math::Vector4 data3;
        Math::Vector4 data4;
      };

      struct RenderEntryPushConstant
      {
        u32 renderObjectSlot;
      };

      struct
      {
        VkPipeline gradientPipeline;
        VkPipelineLayout gradientPipelineLayout;

        VkPipelineLayout trianglePipelineLayout;
        Pipeline trianglePipeline;

        VkPipelineLayout solidBasePipelineLayout;
        Pipeline solidBasePipeline;

        Pipeline lightingPipeline;
      } g_Pipelines;

      namespace Convert {
        static VkFormat image_format(ImageFormat p_Format)
        {
          switch (p_Format) {
          case ImageFormat::UNDEFINED:
            return VK_FORMAT_UNDEFINED;
          case ImageFormat::RGBA16_SFLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
          case ImageFormat::R16_SFLOAT:
            return VK_FORMAT_R16_SFLOAT;
          case ImageFormat::DEPTH:
            return VK_FORMAT_D32_SFLOAT;
          }

          LOW_ASSERT(false, "Unsupported format");
          return VK_FORMAT_MAX_ENUM;
        }

        static VkCullModeFlags
        cull_mode(GraphicsPipelineCullMode p_CullMode)
        {
          switch (p_CullMode) {
          case GraphicsPipelineCullMode::NONE:
            return VK_CULL_MODE_NONE;
          case GraphicsPipelineCullMode::BACK:
            return VK_CULL_MODE_BACK_BIT;
          case GraphicsPipelineCullMode::FRONT:
            return VK_CULL_MODE_FRONT_BIT;
          }

          LOW_ASSERT(false, "Unsupported cull mode");
          return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        }

        static VkFrontFace
        front_face(GraphicsPipelineFrontFace p_FrontFace)
        {
          switch (p_FrontFace) {
          case GraphicsPipelineFrontFace::CLOCKWISE:
            return VK_FRONT_FACE_CLOCKWISE;
          case GraphicsPipelineFrontFace::COUNTER_CLOCKWISE:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
          }

          LOW_ASSERT(false, "Unsupported front face");
          return VK_FRONT_FACE_MAX_ENUM;
        }
      } // namespace Convert

      Vulkan::Pipeline create_pipeline_from_config(
          const GraphicsPipelineConfig &p_Config,
          VkPipelineLayout p_PipelineLayout)
      {
        Vulkan::PipelineUtil::GraphicsPipelineBuilder l_Builder;

        l_Builder.pipelineLayout = p_PipelineLayout;
        l_Builder.set_shaders(p_Config.vertexShaderPath,
                              p_Config.fragmentShaderPath);
        l_Builder.set_input_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        l_Builder.set_cull_mode(
            Convert::cull_mode(p_Config.cullMode),
            Convert::front_face(p_Config.frontFace));
        l_Builder.set_multismapling_none();

        l_Builder.disable_blending();

        if (!p_Config.depthTest) {
          l_Builder.disable_depth_test();
        }

        l_Builder.colorAttachmentFormats.clear();

        for (u32 i = 0; i < p_Config.colorAttachmentFormats.size();
             ++i) {
          l_Builder.colorAttachmentFormats.push_back(
              Convert::image_format(
                  p_Config.colorAttachmentFormats[i]));
        }

        l_Builder.set_depth_format(
            Convert::image_format(p_Config.depthFormat));

        return l_Builder.register_pipeline();
      }

      bool lighting_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "fullscreen_triangle.vert";
        Util::String l_FragmentShaderPath = "lighting.frag";

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
            PipelineUtil::layout_create_info();

        // Create pipeline
        PipelineUtil::GraphicsPipelineBuilder l_Builder;
        l_Builder.pipelineLayout =
            Global::get_lighting_pipeline_layout();
        l_Builder.set_shaders(l_VertexShaderPath,
                              l_FragmentShaderPath);
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
            VK_FORMAT_R16G16B16A16_SFLOAT);

        l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

        g_Pipelines.lightingPipeline = l_Builder.register_pipeline();

        return true;
      }

      bool bg_pipelines_init(Context &p_Context)
      {
        VkPipelineLayoutCreateInfo l_ComputeLayout{};
        l_ComputeLayout.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_ComputeLayout.pNext = nullptr;
        l_ComputeLayout.pSetLayouts =
            &p_Context.drawImageDescriptorLayout;
        l_ComputeLayout.setLayoutCount = 1;

        VkPushConstantRange l_PushConstant{};
        l_PushConstant.offset = 0;
        l_PushConstant.size = sizeof(ComputePushConstants);
        l_PushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        l_ComputeLayout.pPushConstantRanges = &l_PushConstant;
        l_ComputeLayout.pushConstantRangeCount = 1;

        LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
            Global::get_device(), &l_ComputeLayout, nullptr,
            &g_Pipelines.gradientPipelineLayout));

        Util::String l_ComputeShaderPath =
            Util::get_project().engineDataPath +
            "/lowr_shaders/gradient_color.comp.spv";

        VkShaderModule l_ComputeDrawShader;
        if (!PipelineUtil::load_shader_module(
                l_ComputeShaderPath.c_str(), Global::get_device(),
                &l_ComputeDrawShader)) {
          LOW_LOG_ERROR << "Could not find shader file"
                        << LOW_LOG_END;
          return false;
        }

        VkPipelineShaderStageCreateInfo l_StageInfo{};
        l_StageInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_StageInfo.pNext = nullptr;
        l_StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        l_StageInfo.module = l_ComputeDrawShader;
        l_StageInfo.pName = "main";

        VkComputePipelineCreateInfo l_ComputePipelineCreateInfo{};
        l_ComputePipelineCreateInfo.sType =
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        l_ComputePipelineCreateInfo.pNext = nullptr;
        l_ComputePipelineCreateInfo.layout =
            g_Pipelines.gradientPipelineLayout;
        l_ComputePipelineCreateInfo.stage = l_StageInfo;

        LOWR_VK_CHECK_RETURN(vkCreateComputePipelines(
            Global::get_device(), VK_NULL_HANDLE, 1,
            &l_ComputePipelineCreateInfo, nullptr,
            &g_Pipelines.gradientPipeline));

        // Destroying the shader module
        vkDestroyShaderModule(Global::get_device(),
                              l_ComputeDrawShader, nullptr);

        return true;
      }

      bool solid_base_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "solid_base.vert";
        Util::String l_FragmentShaderPath = "solid_base.frag";

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
            PipelineUtil::layout_create_info();

        Util::List<VkDescriptorSetLayout> l_Layouts;
        l_Layouts.resize(3);
        l_Layouts[0] = Global::get_global_descriptor_set_layout();
        l_Layouts[1] = Global::get_texture_descriptor_set_layout();
        l_Layouts[2] = Global::get_view_info_descriptor_set_layout();

        l_PipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_PipelineLayoutInfo.pNext = nullptr;
        l_PipelineLayoutInfo.pSetLayouts = l_Layouts.data();
        l_PipelineLayoutInfo.setLayoutCount = l_Layouts.size();

        VkPushConstantRange l_PushConstant{};
        l_PushConstant.offset = 0;
        l_PushConstant.size = sizeof(RenderEntryPushConstant);
        l_PushConstant.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        l_PipelineLayoutInfo.pPushConstantRanges = &l_PushConstant;
        l_PipelineLayoutInfo.pushConstantRangeCount = 1;

        LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
            Global::get_device(), &l_PipelineLayoutInfo, nullptr,
            &g_Pipelines.solidBasePipelineLayout));

        // Create pipeline
        PipelineUtil::GraphicsPipelineBuilder l_Builder;
        l_Builder.pipelineLayout =
            g_Pipelines.solidBasePipelineLayout;
        l_Builder.set_shaders(l_VertexShaderPath,
                              l_FragmentShaderPath);
        l_Builder.set_input_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        l_Builder.set_cull_mode(VK_CULL_MODE_BACK_BIT,
                                VK_FRONT_FACE_COUNTER_CLOCKWISE);
        l_Builder.set_multismapling_none();
        l_Builder.disable_blending();
        l_Builder.disable_depth_test();

        l_Builder.colorAttachmentFormats.clear();
        l_Builder.colorAttachmentFormats.push_back(
            VK_FORMAT_R16G16B16A16_SFLOAT);
        l_Builder.colorAttachmentFormats.push_back(
            VK_FORMAT_R16G16B16A16_SFLOAT);

        l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

        g_Pipelines.solidBasePipeline = l_Builder.register_pipeline();

        return true;
      }

      bool triangle_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "colored_triangle.vert";
        Util::String l_FragmentShaderPath = "colored_triangle.frag";

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
            PipelineUtil::layout_create_info();
        LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
            Global::get_device(), &l_PipelineLayoutInfo, nullptr,
            &g_Pipelines.trianglePipelineLayout));

        // Create pipeline
        PipelineUtil::GraphicsPipelineBuilder l_Builder;
        l_Builder.pipelineLayout = g_Pipelines.trianglePipelineLayout;
        l_Builder.set_shaders(l_VertexShaderPath,
                              l_FragmentShaderPath);
        l_Builder.set_input_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        l_Builder.set_cull_mode(VK_CULL_MODE_NONE,
                                VK_FRONT_FACE_CLOCKWISE);
        l_Builder.set_multismapling_none();
        l_Builder.disable_blending();
        l_Builder.disable_depth_test();

        l_Builder.colorAttachmentFormats.push_back(
            p_Context.swapchain.drawImage.format);
        l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

        g_Pipelines.trianglePipeline = l_Builder.register_pipeline();

        return true;
      }

      bool setup_renderflow(Context &p_Context)
      {
        bg_pipelines_init(p_Context);
        lighting_pipeline_init(p_Context);
        triangle_pipeline_init(p_Context);
        solid_base_pipeline_init(p_Context);
        return true;
      }

      static void initialize_types()
      {
        Pipeline::initialize();
        Image::initialize();
      }

      static void cleanup_types()
      {
        Pipeline::cleanup();
        Image::cleanup();
      }

      size_t request_resource_staging_buffer_space(
          const size_t p_RequestedSize, size_t *p_OutOffset)
      {
        StagingBuffer &l_ResourceStagingBuffer =
            Global::get_current_resource_staging_buffer();

        const size_t l_AvailableSpace =
            l_ResourceStagingBuffer.size -
            l_ResourceStagingBuffer.occupied;

        *p_OutOffset = l_ResourceStagingBuffer.occupied;

        if (l_AvailableSpace >= p_RequestedSize) {
          l_ResourceStagingBuffer.occupied += p_RequestedSize;
          return p_RequestedSize;
        }

        l_ResourceStagingBuffer.occupied += l_AvailableSpace;
        return l_AvailableSpace;
      }

      bool resource_staging_buffer_write(void *p_Data,
                                         const size_t p_DataSize,
                                         const size_t p_Offset)
      {
        StagingBuffer &l_ResourceStagingBuffer =
            Global::get_current_resource_staging_buffer();

        // Converting the buffer to u8 so that it is easier for us to
        // offset the upload point into the buffer by p_Offset
        u8 *l_Buffer =
            (u8 *)l_ResourceStagingBuffer.buffer.info.pMappedData;

        memcpy((void *)&(l_Buffer[p_Offset]), p_Data, p_DataSize);

        return true;
      }

      static bool initialize_default_texture()
      {
        g_DefaultTexture = Texture::make(N(Default));
        Vulkan::Image l_Image = Vulkan::Image::make(N(Default));
        g_DefaultTexture.set_data_handle(l_Image.get_id());

        Math::UVector2 l_Dimensions(128.0f);

        size_t l_StagingOffset = 0;
        const u64 l_UploadSpace =
            Vulkan::request_resource_staging_buffer_space(
                l_Dimensions.x * l_Dimensions.y * IMAGE_CHANNEL_COUNT,
                &l_StagingOffset);

        LOW_ASSERT(l_UploadSpace == (l_Dimensions.x * l_Dimensions.y *
                                     IMAGE_CHANNEL_COUNT),
                   "Failed to request resource "
                   "staging buffer space for "
                   "default texture");

        VkExtent3D l_Extent;
        l_Extent.width = l_Dimensions.x;
        l_Extent.height = l_Dimensions.y;
        l_Extent.depth = 1;

        Vulkan::ImageUtil::create(l_Image, l_Extent,
                                  VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_USAGE_SAMPLED_BIT, false);

        Util::List<uint8_t> l_Pixels(l_Dimensions.x * l_Dimensions.y *
                                     IMAGE_CHANNEL_COUNT); // RGBA8

        for (uint32_t y = 0; y < l_Dimensions.y; ++y) {
          for (uint32_t x = 0; x < l_Dimensions.x; ++x) {
            // Determine if this tile is white or black
            // Tile size is 64x64 (128 / 2)
            bool i_WhiteTile =
                (x < 64 && y < 64) ||
                (x >= 64 &&
                 y >= 64); // Top-left and bottom-right white

            size_t i_Index = (y * l_Dimensions.x + x) * 4;
            if (i_WhiteTile) {
              l_Pixels[i_Index + 0] = 228; // R
              l_Pixels[i_Index + 1] = 114; // G
              l_Pixels[i_Index + 2] = 184; // B
              l_Pixels[i_Index + 3] = 255; // A
            } else {
              l_Pixels[i_Index + 0] = 114; // R
              l_Pixels[i_Index + 1] = 184; // G
              l_Pixels[i_Index + 2] = 228; // B
              l_Pixels[i_Index + 3] = 255; // A
            }
          }
        }

        LOW_ASSERT(
            Vulkan::resource_staging_buffer_write(
                l_Pixels.data(), l_UploadSpace, l_StagingOffset),
            "Failed to upload default image to resource staging "
            "buffer");

        ImageUtil::cmd_transition(
            Global::get_current_command_buffer(), l_Image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy l_Region = {};
        l_Region.bufferOffset = l_StagingOffset;
        l_Region.bufferRowLength =
            0; // Tightly packed (matches image width)
        l_Region.bufferImageHeight =
            0; // Tightly packed (matches image height)

        l_Region.imageSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        l_Region.imageSubresource.mipLevel = 0;
        l_Region.imageSubresource.baseArrayLayer = 0;
        l_Region.imageSubresource.layerCount = 1;

        l_Region.imageOffset = {0, 0, 0};
        l_Region.imageExtent = {
            l_Dimensions.x, // width
            l_Dimensions.y, // height
            1               // depth
        };

        vkCmdCopyBufferToImage(
            Vulkan::Global::get_current_command_buffer(),
            Vulkan::Global::get_current_resource_staging_buffer()
                .buffer.buffer,
            l_Image.get_allocated_image().image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &l_Region);

        ImageUtil::cmd_transition(
            Global::get_current_command_buffer(), l_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
          for (u32 j = 0; j < Texture::get_capacity(); ++j) {
            Global::get_texture_update_queue(i).push(
                {g_DefaultTexture, j});
          }
        }

        return true;
      }

      bool initialize()
      {
        initialize_types();

        int l_Width, l_Height;
        SDL_GetWindowSize(Util::Window::get_main_window().sdlwindow,
                          &l_Width, &l_Height);

        LOWR_VK_ASSERT_RETURN(Base::initialize(),
                              "Failed to initialize vulkan globals");

        LOWR_VK_ASSERT_RETURN(
            Base::context_initialize(
                g_Context, Math::UVector2(l_Width, l_Height)),
            "Failed to initialize global vulkan context");

        {
          Util::Resource::Mesh l_Mesh;
          Util::String l_Path = Util::get_project().dataPath;
          l_Path += "/_internal/assets/meshes/";
          l_Path += "cube.glb";
          Util::Resource::load_mesh(l_Path, l_Mesh);
        }

        LOWR_VK_ASSERT_RETURN(setup_renderflow(g_Context),
                              "Failed to setup renderflow");

        LOWR_VK_ASSERT_RETURN(initialize_basic_rendersteps(),
                              "Failed to create basic rendersteps");

        return true;
      }

      bool bg_pipelines_cleanup(const Context &p_Context)
      {
        vkDestroyPipelineLayout(Global::get_device(),
                                g_Pipelines.gradientPipelineLayout,
                                nullptr);
        vkDestroyPipeline(Global::get_device(),
                          g_Pipelines.gradientPipeline, nullptr);

        return true;
      }

      bool triangle_pipeline_cleanup(const Context &p_Context)
      {
        g_Pipelines.trianglePipeline.destroy();
        /*
        vkDestroyPipelineLayout(p_Context.device,
                                g_Pipelines.trianglePipelineLayout,
                                nullptr);
        vkDestroyPipeline(p_Context.device,
                          g_Pipelines.trianglePipeline, nullptr);
                          */

        return true;
      }

      bool cleanup_renderflow(Context &p_Context)
      {
        triangle_pipeline_cleanup(p_Context);
        bg_pipelines_cleanup(p_Context);

        return true;
      }

      bool wait_idle()
      {
        vkDeviceWaitIdle(Global::get_device());
        return true;
      }

      bool cleanup()
      {
        vkDeviceWaitIdle(Global::get_device());

        cleanup_renderflow(g_Context);

        cleanup_types();

        LOWR_VK_ASSERT_RETURN(
            Base::context_cleanup(g_Context),
            "Failed to cleanup vulkan global context");

        LOWR_VK_ASSERT_RETURN(Base::cleanup(),
                              "Failed to cleanup vulkan globals");

        return true;
      }

      bool geometry_draw(Context &p_Context)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        // begin a render pass  connected to our draw image
        VkRenderingAttachmentInfo l_ColorAttachment =
            InitUtil::attachment_info(
                p_Context.swapchain.drawImage.imageView, nullptr,
                VK_IMAGE_LAYOUT_GENERAL);

        VkRenderingInfo l_RenderInfo =
            InitUtil::rendering_info(p_Context.swapchain.drawExtent,
                                     &l_ColorAttachment, 1, nullptr);
        vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

        vkCmdBindPipeline(
            l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            g_Pipelines.trianglePipeline.get_pipeline());

        // set dynamic viewport and scissor
        VkViewport l_Viewport = {};
        l_Viewport.x = 0;
        l_Viewport.y = 0;
        l_Viewport.width =
            static_cast<float>(p_Context.swapchain.drawExtent.width);
        l_Viewport.height =
            static_cast<float>(p_Context.swapchain.drawExtent.height);
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;

        vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

        VkRect2D l_Scissor = {};
        l_Scissor.offset.x = 0;
        l_Scissor.offset.y = 0;
        l_Scissor.extent.width = p_Context.swapchain.drawExtent.width;
        l_Scissor.extent.height =
            p_Context.swapchain.drawExtent.height;

        vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

        // launch a draw command to draw 3 vertices
        vkCmdDraw(l_Cmd, 3, 1, 0, 0);

        vkCmdEndRendering(l_Cmd);

        return true;
      }

      static bool draw_render_entries(Context &p_Context,
                                      float p_Delta,
                                      RenderView p_RenderView)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

        VkClearValue l_ClearColorValue = {};
        l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        Image l_AlbedoImage =
            p_RenderView.get_gbuffer_albedo().get_data_handle();
        Image l_NormalsImage =
            p_RenderView.get_gbuffer_normals().get_data_handle();

        Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
        l_ColorAttachments.resize(2);
        l_ColorAttachments[0] = InitUtil::attachment_info(
            l_AlbedoImage.get_allocated_image().imageView,
            &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);
        l_ColorAttachments[1] = InitUtil::attachment_info(
            l_NormalsImage.get_allocated_image().imageView,
            &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);

        VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
            {p_RenderView.get_dimensions().x,
             p_RenderView.get_dimensions().y},
            l_ColorAttachments.data(), l_ColorAttachments.size(),
            nullptr);
        vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

        vkCmdBindIndexBuffer(
            l_Cmd, Global::get_mesh_index_buffer().m_Buffer.buffer, 0,
            VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(
            l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            g_Pipelines.solidBasePipeline.get_pipeline());

        VkDescriptorSet l_Set = Global::get_global_descriptor_set();

        vkCmdBindDescriptorSets(l_Cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                g_Pipelines.solidBasePipelineLayout,
                                0, 1, &l_Set, 0, nullptr);

        VkDescriptorSet l_DescriptorSet =
            l_ViewInfo.get_view_data_descriptor_set();

        vkCmdBindDescriptorSets(l_Cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                g_Pipelines.solidBasePipelineLayout,
                                1, 1, &l_DescriptorSet, 0, nullptr);

        // set dynamic viewport and scissor
        VkViewport l_Viewport = {};
        l_Viewport.x = 0;
        l_Viewport.y = 0;
        l_Viewport.width =
            static_cast<float>(p_Context.swapchain.drawExtent.width);
        l_Viewport.height =
            static_cast<float>(p_Context.swapchain.drawExtent.height);
        l_Viewport.minDepth = 0.f;
        l_Viewport.maxDepth = 1.f;

        vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

        VkRect2D l_Scissor = {};
        l_Scissor.offset.x = 0;
        l_Scissor.offset.y = 0;
        l_Scissor.extent.width = p_Context.swapchain.drawExtent.width;
        l_Scissor.extent.height =
            p_Context.swapchain.drawExtent.height;

        vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

        for (auto it = p_RenderView.get_render_scene()
                           .get_draw_commands()
                           .begin();
             it != p_RenderView.get_render_scene()
                       .get_draw_commands()
                       .end();) {
          if (!it->get_render_object().is_alive()) {
            it = p_RenderView.get_render_scene()
                     .get_draw_commands()
                     .erase(it);
            continue;
          }

          if (it->get_render_object()
                  .get_mesh_resource()
                  .get_state() != MeshResourceState::LOADED) {
            it++;
            continue;
          }

          MeshInfo i_MeshInfo = it->get_mesh_info();

          LOW_ASSERT(i_MeshInfo.is_alive(),
                     "Mesh info of mesh entry not alive anymore. "
                     "This should not happen if the corresponding "
                     "RenderObject is still alive.");

          RenderEntryPushConstant i_PushConstants;
          i_PushConstants.renderObjectSlot = it->get_slot();

          vkCmdPushConstants(
              l_Cmd, g_Pipelines.solidBasePipelineLayout,
              VK_SHADER_STAGE_ALL_GRAPHICS, 0,
              sizeof(RenderEntryPushConstant), &i_PushConstants);

          vkCmdDrawIndexed(l_Cmd, i_MeshInfo.get_index_count(), 1,
                           i_MeshInfo.get_index_start(), 0, 0);

          it++;
        }

        vkCmdEndRendering(l_Cmd);

        return true;
      }

      static bool draw_render_entries(Context &p_Context,
                                      float p_Delta)
      {
        for (u32 i = 0u; i < RenderView::living_count(); ++i) {
          RenderView i_RenderView = RenderView::living_instances()[i];

          /*
          if (!draw_render_entries(p_Context, p_Delta,
                                   i_RenderView)) {
            return false;
          }
          */
          for (u32 j = 0; j < i_RenderView.get_steps().size(); ++j) {
            RenderStep i_RenderStep = i_RenderView.get_steps()[j];

            if (i_RenderStep.is_alive()) {
              LOW_ASSERT(i_RenderStep.execute(p_Delta, i_RenderView),
                         "Failed to execute renderstep");
            }
          }
        }

        return true;
      }

      static bool draw(Context &p_Context, float p_Delta)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        ImageUtil::Internal::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ImageUtil::Internal::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        draw_render_entries(p_Context, p_Delta);

        // TODO: Change this - this is just for testing
        {
          RenderView l_RenderView = RenderView::living_instances()[0];
          ViewInfo l_ViewInfo = l_RenderView.get_view_info_handle();

          /*
          ImageUtil::cmd_transition(
              l_Cmd, l_ViewInfo.get_gbuffer_albedo().image,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
              */

          Image l_LitImage =
              l_RenderView.get_lit_image().get_data_handle();

          ImageUtil::Internal::cmd_copy2D(
              l_Cmd, l_LitImage.get_allocated_image().image,
              g_Context.swapchain.drawImage.image,
              p_Context.swapchain.drawExtent,
              p_Context.swapchain.drawExtent);

          /*
          ImageUtil::cmd_transition(
              l_Cmd, l_ViewInfo.get_gbuffer_albedo().image,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
              */
        }

        ImageUtil::Internal::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        ImageUtil::Internal::cmd_transition(
            l_Cmd,
            p_Context.swapchain
                .images[p_Context.swapchain.imageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        return true;
      }

      static bool prepare_render_view_dimensions(
          float p_Delta, RenderView p_RenderView, ViewInfo p_ViewInfo)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        if (p_ViewInfo.is_initialized()) {
          wait_idle();
          {
            Image l_Image =
                p_RenderView.get_gbuffer_albedo().get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_albedo()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image =
                p_RenderView.get_gbuffer_normals().get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_normals()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image =
                p_RenderView.get_gbuffer_depth().get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_depth()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image =
                p_RenderView.get_lit_image().get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_lit_image()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }

          {
            for (auto it = p_RenderView.get_steps().begin();
                 it != p_RenderView.get_steps().end(); ++it) {
              if (it->is_alive()) {
                it->update_resolution(p_RenderView.get_dimensions(),
                                      p_RenderView);
              }
            }
          }
        }

        // Update light cluster data
        {
          if (p_ViewInfo.is_initialized()) {
            BufferUtil::destroy_buffer(
                p_ViewInfo.get_point_light_cluster_buffer());
          }

          {
            Low::Math::UVector3 l_Clusters;
            l_Clusters.x = p_RenderView.get_dimensions().x / 16.0f;
            l_Clusters.y = p_RenderView.get_dimensions().y / 16.0f;
            l_Clusters.z = LIGHT_CLUSTER_DEPTH;
            p_ViewInfo.set_light_clusters(l_Clusters);
            p_ViewInfo.set_light_cluster_count(
                l_Clusters.x * l_Clusters.y * l_Clusters.z);

            AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
                sizeof(u32) * POINTLIGHTS_PER_CLUSTER *
                    p_ViewInfo.get_light_cluster_count(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            p_ViewInfo.set_point_light_cluster_buffer(l_Buffer);

            DescriptorUtil::DescriptorWriter l_Writer;

            l_Writer.write_buffer(
                2, p_ViewInfo.get_point_light_cluster_buffer().buffer,
                sizeof(u32) * POINTLIGHTS_PER_CLUSTER *
                    p_ViewInfo.get_light_cluster_count(),
                0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.write_buffer(
                3, p_ViewInfo.get_point_light_buffer().buffer,
                sizeof(PointLightInfo) * POINTLIGHT_COUNT, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.update_set(
                Global::get_device(),
                p_ViewInfo.get_view_data_descriptor_set());
          }
        }

        VkExtent3D l_Extent{p_RenderView.get_dimensions().x,
                            p_RenderView.get_dimensions().y, 1.0f};

        {
          if (!p_RenderView.get_gbuffer_albedo().is_alive()) {
            p_RenderView.set_gbuffer_albedo(Texture::make(N(Albedo)));
          }

          Image l_Image =
              p_RenderView.get_gbuffer_albedo().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Albedo));
            p_RenderView.get_gbuffer_albedo().set_data_handle(
                l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent,
                            VK_FORMAT_R16G16B16A16_SFLOAT,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                            false);

          ImageUtil::cmd_transition(
              l_Cmd, l_Image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        {
          if (!p_RenderView.get_gbuffer_normals().is_alive()) {
            p_RenderView.set_gbuffer_normals(
                Texture::make(N(Normals)));
          }

          Image l_Image =
              p_RenderView.get_gbuffer_normals().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Normals));
            p_RenderView.get_gbuffer_normals().set_data_handle(
                l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent,
                            VK_FORMAT_R16G16B16A16_SFLOAT,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                            false);

          ImageUtil::cmd_transition(
              l_Cmd, l_Image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        {
          if (!p_RenderView.get_gbuffer_depth().is_alive()) {
            p_RenderView.set_gbuffer_depth(Texture::make(N(Depth)));
          }

          Image l_Image =
              p_RenderView.get_gbuffer_depth().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Depth));
            l_Image.set_depth(true);
            p_RenderView.get_gbuffer_depth().set_data_handle(
                l_Image.get_id());
          }

          // TODO: Change type over to be extracted from the texture
          ImageUtil::create(
              l_Image, l_Extent,
              Convert::image_format(ImageFormat::DEPTH),
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  VK_IMAGE_USAGE_STORAGE_BIT |
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                  VK_IMAGE_USAGE_SAMPLED_BIT,
              false);

          ImageUtil::cmd_transition(
              l_Cmd, l_Image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        {
          if (!p_RenderView.get_lit_image().is_alive()) {
            p_RenderView.set_lit_image(Texture::make(N(Lit)));
          }

          Image l_Image =
              p_RenderView.get_lit_image().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Lit));
            p_RenderView.get_lit_image().set_data_handle(
                l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent,
                            VK_FORMAT_R16G16B16A16_SFLOAT,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                            false);

          ImageUtil::cmd_transition(
              l_Cmd, l_Image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        {
          VkDescriptorSet l_Set =
              Global::get_global_descriptor_allocator().allocate(
                  Global::get_device(),
                  Global::get_gbuffer_descriptor_set_layout());
          p_ViewInfo.set_gbuffer_descriptor_set(l_Set);

          Samplers &l_Samplers = Global::get_samplers();

          DescriptorUtil::DescriptorWriter l_Writer;
          l_Writer.write_image(
              0, p_RenderView.get_gbuffer_albedo().get_data_handle(),
              l_Samplers.no_lod_nearest_repeat_black,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

          l_Writer.write_image(
              1, p_RenderView.get_gbuffer_normals().get_data_handle(),
              l_Samplers.no_lod_nearest_repeat_black,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

          l_Writer.update_set(Global::get_device(), l_Set);
        }

        p_ViewInfo.set_initialized(true);

        return true;
      }

      static bool prepare_render_view_camera(float p_Delta,
                                             RenderView p_RenderView,
                                             ViewInfo p_ViewInfo)
      {
        const int CURRENT_FRAME_INDEX =
            Global::get_current_frame_index();

        StagingBuffer &l_StagingBuffer =
            p_ViewInfo.get_staging_buffers()[CURRENT_FRAME_INDEX];

        l_StagingBuffer.occupied = 0u;

        u8 *l_Buffer = (u8 *)p_ViewInfo
                           .get_staging_buffers()[CURRENT_FRAME_INDEX]
                           .buffer.info.pMappedData;

        {
          ViewInfoFrameData l_FrameData;

          l_FrameData.dimensions.x = p_RenderView.get_dimensions().x;
          l_FrameData.dimensions.y = p_RenderView.get_dimensions().y;
          l_FrameData.dimensions.z = 1.0f / l_FrameData.dimensions.x;
          l_FrameData.dimensions.w = 1.0f / l_FrameData.dimensions.y;

          l_FrameData.projectionMatrix = glm::perspective(
              glm::radians(45.0f),
              ((float)p_RenderView.get_dimensions().x) /
                  ((float)p_RenderView.get_dimensions().y),
              1.0f, 100.0f);

          l_FrameData.projectionMatrix[1][1] *= -1.0f;

          l_FrameData.nearFarPlane.x = 1.0f;
          l_FrameData.nearFarPlane.y = 100.0f;

          l_FrameData.viewMatrix =
              glm::lookAt(p_RenderView.get_camera_position(),
                          p_RenderView.get_camera_position() +
                              p_RenderView.get_camera_direction(),
                          LOW_VECTOR3_UP);

          l_FrameData.inverseViewMatrix =
              glm::inverse(l_FrameData.viewMatrix);
          l_FrameData.inverseProjectionMatrix =
              glm::inverse(l_FrameData.projectionMatrix);

          l_FrameData.viewProjectionMatrix =
              l_FrameData.projectionMatrix * l_FrameData.viewMatrix;

          l_FrameData.gbufferIndices.z =
              p_RenderView.get_gbuffer_depth().get_index();
          l_FrameData.gbufferIndices.x =
              p_RenderView.get_gbuffer_albedo().get_index();
          l_FrameData.gbufferIndices.y =
              p_RenderView.get_gbuffer_normals().get_index();

          l_FrameData.lightClusters.x =
              p_ViewInfo.get_light_clusters().x;
          l_FrameData.lightClusters.y =
              p_ViewInfo.get_light_clusters().y;
          l_FrameData.lightClusters.z =
              p_ViewInfo.get_light_clusters().z;
          l_FrameData.lightClusters.w =
              p_ViewInfo.get_light_cluster_count();

          memcpy((void *)&(l_Buffer[l_StagingBuffer.occupied]),
                 &l_FrameData, sizeof(ViewInfoFrameData));

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = l_StagingBuffer.occupied;
          l_CopyRegion.dstOffset = l_StagingBuffer.occupied;
          l_CopyRegion.size = sizeof(ViewInfoFrameData);

          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              l_StagingBuffer.buffer.buffer,
              p_ViewInfo.get_view_data_buffer().buffer, 1,
              &l_CopyRegion);

          l_StagingBuffer.occupied += sizeof(ViewInfoFrameData);
        }

        return true;
      }

      static bool prepare_render_views(float p_Delta)
      {
        for (u32 i = 0u; i < RenderView::living_count(); ++i) {
          RenderView i_RenderView = RenderView::living_instances()[i];

          ViewInfo i_ViewInfo = i_RenderView.get_view_info_handle();

          if (!i_ViewInfo.is_alive()) {
            i_ViewInfo = ViewInfo::make(i_RenderView.get_name());
            i_RenderView.set_view_info_handle(i_ViewInfo);
            Scene i_Scene =
                i_RenderView.get_render_scene().get_data_handle();

            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(
                0, i_ViewInfo.get_view_data_buffer().buffer,
                sizeof(ViewInfoFrameData), 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            l_Writer.write_buffer(
                1, i_Scene.get_point_light_buffer().buffer,
                sizeof(PointLightInfo) * POINTLIGHT_COUNT, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.update_set(
                Global::get_device(),
                i_ViewInfo.get_view_data_descriptor_set());
          }

          if (i_RenderView.is_dimensions_dirty()) {
            i_RenderView.set_camera_dirty(true);

            if (!prepare_render_view_dimensions(p_Delta, i_RenderView,
                                                i_ViewInfo)) {
              return false;
            }

            i_RenderView.set_dimensions_dirty(false);
          }

          if (i_RenderView.is_camera_dirty()) {
            if (!prepare_render_view_camera(p_Delta, i_RenderView,
                                            i_ViewInfo)) {
              return false;
            }
            i_RenderView.set_camera_dirty(false);
          }
        }
        return true;
      }

      bool update_dirty_textures(float p_Delta)
      {
        Samplers &l_Samplers = Global::get_samplers();

        for (auto it = Texture::ms_Dirty.begin();
             it != Texture::ms_Dirty.end(); ++it) {

          Texture i_Texture = *it;
          if (!i_Texture.is_alive()) {
            continue;
          }

          Image i_Image = i_Texture.get_data_handle();

          if (!i_Image.is_alive()) {
            continue;
          }

          i_Texture.set_imgui_texture_id(
              (ImTextureID)ImGui_ImplVulkan_AddTexture(
                  l_Samplers.no_lod_nearest_repeat_black,
                  i_Image.get_allocated_image().imageView,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
            TextureUpdate i_Update;
            i_Update.texture = i_Texture;
            i_Update.textureIndex = i_Texture.get_index();
            Global::get_texture_update_queue(i).push(i_Update);
          }
        }

        Texture::ms_Dirty.clear();

        return true;
      }

      static bool update_point_lights(float p_Delta)
      {
        bool l_Result = true;
        for (u32 i = 0; i < RenderScene::living_count(); ++i) {
          RenderScene i_RenderScene =
              RenderScene::living_instances()[i];
          Vulkan::Scene i_Scene = i_RenderScene.get_data_handle();
          for (auto it = i_RenderScene.get_pointlight_deleted_slots()
                             .begin();
               it !=
               i_RenderScene.get_pointlight_deleted_slots().end();
               ++it) {
            u32 i_Slot = *it;
            i_Scene.get_point_light_slots()[i_Slot] = false;
          }
        }

        for (auto it = PointLight::ms_Dirty.begin();
             it != PointLight::ms_Dirty.end(); ++it) {
          PointLight i_PointLight = it->get_id();

          RenderScene i_RenderScene =
              i_PointLight.get_render_scene_handle();
          Vulkan::Scene i_Scene = i_RenderScene.get_data_handle();

          if (i_PointLight.get_slot() >= POINTLIGHT_COUNT) {
            // Assign slot

            bool *i_Slots = i_Scene.get_point_light_slots();

            for (u32 i = 0u; i < POINTLIGHT_COUNT; ++i) {
              // We look for an empty slot to assign to the point
              // light
              if (!i_Slots[i]) {
                i_Slots[i] = true;
                i_PointLight.set_slot(i);
                // In case this slot was cleared this frame we remove
                // it from the list of deleted slots now so we don't
                // update the slot on the GPU twice
                i_RenderScene.get_pointlight_deleted_slots().erase(i);
                break;
              }
            }
          }

          if (i_PointLight.get_slot() < POINTLIGHT_COUNT) {
            // Upload the pointlight to the buffer

            size_t l_StagingOffset = 0;
            // TODO: Reconsider if that needs to be on the resource
            // staging buffer
            const u64 l_FrameUploadSpace =
                Vulkan::request_resource_staging_buffer_space(
                    sizeof(DrawCommandUpload), &l_StagingOffset);

            if (l_FrameUploadSpace < sizeof(PointLightInfo)) {
              // We don't have enough space on the staging buffer to
              // upload this pointlight
              l_Result = false;
              break;
            }

            PointLightInfo i_Info;
            i_Info.transform.x = i_PointLight.get_world_position().x;
            i_Info.transform.y = i_PointLight.get_world_position().y;
            i_Info.transform.z = i_PointLight.get_world_position().z;
            i_Info.transform.w = i_PointLight.get_range();
            i_Info.color.r = i_PointLight.get_color().r;
            i_Info.color.g = i_PointLight.get_color().g;
            i_Info.color.b = i_PointLight.get_color().b;
            i_Info.color.a = i_PointLight.get_intensity();
            i_Info.active = true;

            // TODO: Check if this has to be the resource staging
            // buffer
            LOW_ASSERT(
                Vulkan::resource_staging_buffer_write(
                    &i_Info, sizeof(PointLightInfo), l_StagingOffset),
                "Failed to write pointlight data to staging "
                "buffer");

            VkBufferCopy i_CopyRegion;
            i_CopyRegion.srcOffset = l_StagingOffset;
            i_CopyRegion.dstOffset =
                i_PointLight.get_slot() * sizeof(PointLightInfo);
            i_CopyRegion.size = l_FrameUploadSpace;

            // TODO: Change to transfer queue command buffer - or
            // leave this specifically on the graphics queue not sure
            // tbh
            // TODO: Also adjust the staging buffer if necessary
            // TODO: Check if it's possible to batch all these
            // together per renderscene
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                i_Scene.get_point_light_buffer().buffer, 1,
                &i_CopyRegion);

            LOW_LOG_DEBUG << "Upload pointlight" << LOW_LOG_END;
          }
        }

        PointLight::ms_Dirty.clear();

        for (u32 i = 0; i < RenderScene::living_count(); ++i) {
          RenderScene i_RenderScene =
              RenderScene::living_instances()[i];
          Vulkan::Scene i_Scene = i_RenderScene.get_data_handle();

          Util::List<VkBufferCopy> i_Copies;

          for (auto it = i_RenderScene.get_pointlight_deleted_slots()
                             .begin();
               it !=
               i_RenderScene.get_pointlight_deleted_slots().end();
               ++it) {
            size_t l_StagingOffset = 0;
            // TODO: Reconsider if that needs to be on the resource
            // staging buffer
            const u64 l_FrameUploadSpace =
                Vulkan::request_resource_staging_buffer_space(
                    sizeof(DrawCommandUpload), &l_StagingOffset);

            if (l_FrameUploadSpace < sizeof(PointLightInfo)) {
              // We don't have enough space on the staging buffer to
              // upload this pointlight
              l_Result = false;
              break;
            }

            PointLightInfo i_Info;
            i_Info.transform.x = 0;
            i_Info.transform.y = 0;
            i_Info.transform.z = 0;
            i_Info.transform.w = 0;
            i_Info.color.r = 0;
            i_Info.color.g = 0;
            i_Info.color.b = 0;
            i_Info.color.a = 0;
            i_Info.active = false;

            // TODO: Check if this has to be the resource staging
            // buffer
            LOW_ASSERT(
                Vulkan::resource_staging_buffer_write(
                    &i_Info, sizeof(PointLightInfo), l_StagingOffset),
                "Failed to write pointlight data to staging "
                "buffer");

            VkBufferCopy i_CopyRegion;
            i_CopyRegion.srcOffset = l_StagingOffset;
            i_CopyRegion.dstOffset = (*it) * sizeof(PointLightInfo);
            i_CopyRegion.size = l_FrameUploadSpace;

            i_Copies.push_back(i_CopyRegion);
          }

          if (!i_Copies.empty()) {
            // TODO: Change to transfer queue command buffer - or
            // leave this specifically on the graphics queue not sure
            // tbh
            // TODO: Also adjust the staging buffer if necessary
            // TODO: Check if it's possible to batch all these
            // together per renderscene
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                i_Scene.get_point_light_buffer().buffer,
                i_Copies.size(), i_Copies.data());
          }

          i_RenderScene.get_pointlight_deleted_slots().clear();
        }

        return l_Result;
      }

      bool prepare_tick(float p_Delta)
      {
        static bool l_Initialized = false;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        {
          for (u32 i = 0; i < MaterialType::living_count(); ++i) {
            MaterialType i_MaterialType =
                MaterialType::living_instances()[i];

            Pipeline i_DrawPipeline =
                i_MaterialType.get_draw_pipeline_handle();

            if (!i_DrawPipeline.is_alive()) {
              i_MaterialType.set_draw_pipeline_handle(
                  create_pipeline_from_config(
                      i_MaterialType.get_draw_pipeline_config(),
                      g_Pipelines.solidBasePipelineLayout));
            }
          }
        }

        PipelineManager::tick(p_Delta);

        bool l_Result = Base::context_prepare_draw(g_Context);

        if (!l_Initialized) {
          initialize_default_texture();
          l_Initialized = true;
        }

        {
          LOWR_VK_ASSERT_RETURN(update_point_lights(p_Delta),
                                "Failed to update point lights");
        }

        prepare_render_views(p_Delta);

        update_dirty_textures(p_Delta);

        {
          DescriptorUtil::DescriptorWriter l_Writer;

          Samplers &l_Samplers = Global::get_samplers();

          bool l_AddedEntry = false;

          while (
              !Global::get_current_texture_update_queue().empty()) {
            TextureUpdate i_Update =
                Global::get_current_texture_update_queue().front();
            Global::get_current_texture_update_queue().pop();

            if (!i_Update.texture.is_alive()) {
              continue;
            }

            Image i_Image = i_Update.texture.get_data_handle();

            if (!i_Image.is_alive()) {
              continue;
            }

            l_AddedEntry = true;

            l_Writer.write_image(
                0, i_Image.get_allocated_image().imageView,
                l_Samplers.no_lod_nearest_repeat_black,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                i_Update.textureIndex);
          }

          if (l_AddedEntry) {
            l_Writer.update_set(
                Global::get_device(),
                Global::get_current_texture_descriptor_set());
          }
        }

        return l_Result;
      }

      bool tick(float p_Delta)
      {
        ImGui::Render();

        if (!g_Context.requireResize) {
          LOWR_VK_ASSERT_RETURN(draw(g_Context, p_Delta),
                                "Failed to draw vulkan renderer");

          Base::context_present(g_Context);
        }
        Global::advance_frame_count();

        return true;
      }

      bool check_window_resize(float p_Delta)
      {
        LOWR_VK_ASSERT_RETURN(
            Base::swapchain_resize(g_Context),
            "Failed to swapchain_resize vulkan renderer");

        // TODO: CHANGE THIS IS JUST FOR TESTING
        // RenderViews should not automatically scale with the
        // Swapchain
        Math::UVector2 l_Dimensions{
            g_Context.swapchain.drawExtent.width,
            g_Context.swapchain.drawExtent.height};

        if (l_Dimensions.x > 0 && l_Dimensions.y > 0) {
          for (u32 i = 0u; i < RenderView::living_count(); ++i) {
            RenderView i_RenderView =
                RenderView::living_instances()[i];
            i_RenderView.set_dimensions(l_Dimensions);
          }
        }

        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
