#include "LowRendererVulkanRenderer.h"

#include "LowMath.h"
#include "LowRendererEditorImageGpu.h"
#include "LowRendererGlobals.h"
#include "LowRendererGpuTexture.h"
#include "LowRendererSS2DCanvas.h"
#include "LowRendererSS2DDrawCommand.h"
#include "LowRendererTextureState.h"
#include "LowRendererVkBufferHolder.h"
#include "LowRendererVulkan.h"
#include "LowRendererVulkanBase.h"
#include "LowRendererVkSS2DCanvasBackend.h"

#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanPipelineManager.h"
#include "LowRendererVulkanBuffer.h"
#include "LowRendererRenderView.h"
#include "LowRendererVkViewInfo.h"
#include "LowRendererDrawCommand.h"
#include "LowRendererTexture.h"
#include "LowRendererTextureExport.h"
#include "LowRendererVkTexExport.h"
#include "LowRendererEditorImage.h"
#include "LowRendererVkPipelineLayout.h"
#include "LowRendererVkTextureSlots.h"

#include "LowRenderer.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "LowMathVectorUtil.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "glm/ext/quaternion_geometric.hpp"
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
#define SS2DDRAWCOMMAND_COUNT 128u

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"
#include "../../LowDependencies/stb/stb_image_write.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      Context g_Context;

      static Vulkan::Pipeline create_pipeline_from_config(
          const GraphicsPipelineConfig &p_Config,
          PipelineLayout p_PipelineLayout);

      struct ComputePushConstants
      {
        Math::Vector4 data1;
        Math::Vector4 data2;
        Math::Vector4 data3;
        Math::Vector4 data4;
      };

      struct
      {
        PipelineLayout solidBasePipelineLayout;
        Pipeline solidBasePipeline;

        Pipeline lightingPipeline;

        PipelineLayout solidHighlightPipelineLayout;

        Pipeline ss2d;
        PipelineLayout ss2dLayout;
      } g_Pipelines;

      VkDescriptorSet g_Ss2dGlobalSet;
      VkDescriptorSetLayout g_Ss2dCanvasDSLayout;
      VkDescriptorSetLayout g_Ss2dGlobalDSLayout;

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
          case ImageFormat::RGB16_SFLOAT:
            return VK_FORMAT_R16G16B16_SFLOAT;
          case ImageFormat::R32_UINT:
            return VK_FORMAT_R32_UINT;
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

      static Vulkan::Pipeline create_pipeline_from_config(
          const GraphicsPipelineConfig &p_Config,
          PipelineLayout p_PipelineLayout)
      {
        Vulkan::PipelineUtil::GraphicsPipelineBuilder l_Builder;

        l_Builder.pipelineLayout = p_PipelineLayout;
        l_Builder.set_shaders(p_Config.vertexShader,
                              p_Config.fragmentShader);
        l_Builder.set_input_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        l_Builder.set_cull_mode(
            Convert::cull_mode(p_Config.cullMode),
            Convert::front_face(p_Config.frontFace));
        l_Builder.set_multismapling_none();

        if (p_Config.alphaBlending) {
          l_Builder.set_blending_alpha();
        } else {
          l_Builder.disable_blending();
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

        if (!p_Config.depthTest) {
          l_Builder.disable_depth_test();
        }

        if (p_Config.wireframe) {
          l_Builder.set_polygon_mode(VK_POLYGON_MODE_LINE,
                                     p_Config.lineStrength);
        }

        return l_Builder.register_pipeline();
      }

      static bool ss2d_pipeline_cleanup()
      {
        vkDestroyDescriptorSetLayout(Global::get_device(),
                                     g_Ss2dCanvasDSLayout, nullptr);
        vkDestroyDescriptorSetLayout(Global::get_device(),
                                     g_Ss2dGlobalDSLayout, nullptr);
        return true;
      }

      static bool ss2d_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "fullscreen_triangle.vert";
        Util::String l_FragmentShaderPath = "ss2d.frag";

        Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;

        {
          DescriptorUtil::DescriptorLayoutBuilder l_Builder;
          l_Builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

          g_Ss2dGlobalDSLayout = l_Builder.build(
              Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);
        }

        {
          DescriptorUtil::DescriptorLayoutBuilder l_Builder;
          l_Builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

          g_Ss2dCanvasDSLayout = l_Builder.build(
              Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);
        }

        l_DescriptorSetLayouts.push_back(g_Ss2dGlobalDSLayout);
        l_DescriptorSetLayouts.push_back(g_Ss2dCanvasDSLayout);

        VkPipelineLayoutCreateInfo l_Layout{};
        l_Layout.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_Layout.pNext = nullptr;
        l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
        l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

        g_Pipelines.ss2dLayout =
            PipelineUtil::create_layout(N(SS2D), l_Layout);

        // Create pipeline
        PipelineUtil::GraphicsPipelineBuilder l_Builder;
        l_Builder.pipelineLayout = g_Pipelines.ss2dLayout;
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

        g_Pipelines.ss2d = l_Builder.register_pipeline();

        g_Ss2dGlobalSet =
            Global::get_global_descriptor_allocator().allocate(
                Global::get_device(), g_Ss2dGlobalDSLayout);

        DescriptorUtil::DescriptorWriter l_Writer;
        l_Writer.write_buffer(
            0, Global::get_ss2d_drawcommand_buffer().buffer,
            Global::get_ss2d_drawcommand_buffer().info.size, 0,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        l_Writer.update_set(Global::get_device(), g_Ss2dGlobalSet);

        return true;
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

      bool solid_base_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "solid_base.vert";
        Util::String l_FragmentShaderPath = "solid_base.frag";

        {
          VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
              PipelineUtil::layout_create_info();

          Util::List<VkDescriptorSetLayout> l_Layouts;
          l_Layouts.resize(3);
          l_Layouts[0] = Global::get_global_descriptor_set_layout();
          l_Layouts[1] = Global::get_texture_descriptor_set_layout();
          l_Layouts[2] =
              Global::get_view_info_descriptor_set_layout();

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

          g_Pipelines.solidBasePipelineLayout =
              PipelineUtil::create_layout(N(solid_base),
                                          l_PipelineLayoutInfo);
        }
        {
          VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
              PipelineUtil::layout_create_info();

          Util::List<VkDescriptorSetLayout> l_Layouts;
          l_Layouts.resize(3);
          l_Layouts[0] = Global::get_global_descriptor_set_layout();
          l_Layouts[1] = Global::get_texture_descriptor_set_layout();
          l_Layouts[2] =
              Global::get_view_info_descriptor_set_layout();

          l_PipelineLayoutInfo.sType =
              VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
          l_PipelineLayoutInfo.pNext = nullptr;
          l_PipelineLayoutInfo.pSetLayouts = l_Layouts.data();
          l_PipelineLayoutInfo.setLayoutCount = l_Layouts.size();

          VkPushConstantRange l_PushConstant{};
          l_PushConstant.offset = 0;
          l_PushConstant.size =
              sizeof(RenderEntryHighlightPushConstant);
          l_PushConstant.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

          l_PipelineLayoutInfo.pPushConstantRanges = &l_PushConstant;
          l_PipelineLayoutInfo.pushConstantRangeCount = 1;

          g_Pipelines.solidHighlightPipelineLayout =
              PipelineUtil::create_layout(N(solid_highlight),
                                          l_PipelineLayoutInfo);
        }

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

      bool setup_renderflow(Context &p_Context)
      {
        lighting_pipeline_init(p_Context);
        ss2d_pipeline_init(p_Context);
        solid_base_pipeline_init(p_Context);
        return true;
      }

      static void initialize_types()
      {
        PipelineLayout::initialize();
        Pipeline::initialize();
        Image::initialize();
        TexExport::initialize();
        Scene::initialize();
        ViewInfo::initialize();
        BufferHolder::initialize();
        SS2DCanvasBackend::initialize();
      }

      static void cleanup_types()
      {
        SS2DCanvasBackend::cleanup();
        BufferHolder::cleanup();
        ViewInfo::cleanup();
        Scene::cleanup();
        TexExport::cleanup();
        Pipeline::cleanup();
        Image::cleanup();
        PipelineLayout::cleanup();
      }

      size_t request_resource_staging_buffer_space(
          const size_t p_RequestedSize, size_t *p_OutOffset)
      {
        StagingBuffer &l_ResourceStagingBuffer =
            Global::get_current_resource_staging_buffer();

        return l_ResourceStagingBuffer.request_space(p_RequestedSize,
                                                     p_OutOffset);
      }

      bool resource_staging_buffer_write(void *p_Data,
                                         const size_t p_DataSize,
                                         const size_t p_Offset)
      {
        StagingBuffer &l_ResourceStagingBuffer =
            Global::get_current_resource_staging_buffer();

        return l_ResourceStagingBuffer.write(p_Data, p_DataSize,
                                             p_Offset);
      }

      static bool initialize_default_texture()
      {
        Vulkan::Image l_Image = Vulkan::Image::make(N(Default));
        get_default_texture().get_gpu().set_data_handle(
            l_Image.get_id());

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
          for (u32 j = 0; j < GpuTexture::get_capacity(); ++j) {
            if (!GpuTexture::find_by_index(j).is_alive()) {
              Global::get_texture_update_queue(i).push(
                  {get_default_texture().get_gpu(), j});
            }
          }
        }

        return true;
      }

      bool initialize()
      {
        initialize_types();

        int l_Width, l_Height;
        Util::Window::get_main_window().get_size(&l_Width, &l_Height);

        LOWR_VK_ASSERT_RETURN(Base::initialize(),
                              "Failed to initialize vulkan globals");

        LOWR_VK_ASSERT_RETURN(
            Base::context_initialize(
                g_Context, Math::UVector2(l_Width, l_Height)),
            "Failed to initialize global vulkan context");

        LOWR_VK_ASSERT_RETURN(setup_renderflow(g_Context),
                              "Failed to setup renderflow");

        LOWR_VK_ASSERT_RETURN(initialize_basic_rendersteps(),
                              "Failed to create basic rendersteps");

        return true;
      }

      bool cleanup_renderflow(Context &p_Context)
      {
        ss2d_pipeline_cleanup();
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

        cleanup_basic_rendersteps();

        cleanup_types();

        LOWR_VK_ASSERT_RETURN(
            Base::context_cleanup(g_Context),
            "Failed to cleanup vulkan global context");

        LOWR_VK_ASSERT_RETURN(Base::cleanup(),
                              "Failed to cleanup vulkan globals");

        return true;
      }

      static bool draw_render_entries(Context &p_Context,
                                      float p_Delta)
      {

        for (u32 i = 0u; i < RenderView::living_count(); ++i) {
          RenderView i_RenderView = RenderView::living_instances()[i];
          RenderScene i_RenderScene = i_RenderView.get_render_scene();
          if (!i_RenderScene.is_alive()) {
            continue;
          }
          ViewInfo i_ViewInfo = i_RenderView.get_view_info_handle();
          if (!i_ViewInfo.is_alive() ||
              !i_ViewInfo.is_initialized()) {
            continue;
          }
          Util::String i_Title = Util::String("RenderView: ") +
                                 i_RenderView.get_name().c_str();
          VK_RENDERDOC_SECTION_BEGIN(i_Title.c_str(),
                                     SINGLE_ARG({0.9f, 0.2f, 0.4f}));

          for (u32 j = 0; j < i_RenderView.get_steps().size(); ++j) {
            RenderStep i_RenderStep = i_RenderView.get_steps()[j];

            if (i_RenderStep.is_alive()) {
              LOW_ASSERT(i_RenderStep.execute(p_Delta, i_RenderView),
                         "Failed to execute renderstep");
            }
          }

          i_RenderView.get_debug_geometry().clear();
          i_RenderView.get_highlight_draws_solid().clear();
          i_RenderView.get_highlight_draws_debug_geometry().clear();

          VK_RENDERDOC_SECTION_END();
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

        VK_RENDERDOC_SECTION_BEGIN("Blit to swapchain",
                                   SINGLE_ARG({0.8f, 0.8f, 0.8f}));
        // TODO: Change this - this is just for testing
        if (false) {
          RenderView l_RenderView = RenderView::living_instances()[0];
          ViewInfo l_ViewInfo = l_RenderView.get_view_info_handle();

          ImageUtil::cmd_transition(
              l_Cmd,
              l_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

          Image l_LitImage = l_RenderView.get_lit_image()
                                 .get_gpu()
                                 .get_data_handle();

          VkExtent2D l_SourceExtent;
          l_SourceExtent.width = l_RenderView.get_dimensions().x;
          l_SourceExtent.height = l_RenderView.get_dimensions().y;

          ImageUtil::Internal::cmd_copy2D(
              l_Cmd, l_LitImage.get_allocated_image().image,
              g_Context.swapchain.drawImage.image, l_SourceExtent,
              p_Context.swapchain.drawExtent);

          ImageUtil::cmd_transition(
              l_Cmd,
              l_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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

        VK_RENDERDOC_SECTION_END();

        return true;
      }

      static bool prepare_render_view_directional_light(
          float p_Delta, RenderView p_RenderView, ViewInfo p_ViewInfo)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        RenderScene l_RenderScene = p_RenderView.get_render_scene();
        if (!l_RenderScene.is_alive()) {
          return true;
        }

        const Math::Matrix4x4 l_ViewMatrix =
            glm::lookAt(p_RenderView.get_camera_position(),
                        p_RenderView.get_camera_position() +
                            p_RenderView.get_camera_direction(),
                        LOW_VECTOR3_UP);

        Math::Vector4 l_Direction =
            l_ViewMatrix *
            Math::Vector4(
                l_RenderScene.get_directional_light_direction().x,
                l_RenderScene.get_directional_light_direction().y,
                l_RenderScene.get_directional_light_direction().z,
                0.0f);

        l_Direction = glm::normalize(l_Direction);

        DirectionalLightInfo l_LightInfo;
        l_LightInfo.direction = l_Direction;
        l_LightInfo.color.r =
            l_RenderScene.get_directional_light_color().r;
        l_LightInfo.color.g =
            l_RenderScene.get_directional_light_color().g;
        l_LightInfo.color.b =
            l_RenderScene.get_directional_light_color().b;
        l_LightInfo.color.a =
            l_RenderScene.get_directional_light_intensity();

        size_t l_StagingOffset = 0;
        const u64 l_FrameUploadSpace =
            p_ViewInfo.request_current_staging_buffer_space(
                sizeof(DirectionalLightInfo), &l_StagingOffset);

        LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >=
                                  sizeof(DirectionalLightInfo),
                              "Did not have enough staging "
                              "buffer space to upload "
                              "directional light info frame data.");
        {
          p_ViewInfo.write_current_staging_buffer(
              &l_LightInfo, l_FrameUploadSpace, l_StagingOffset);

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = l_StagingOffset;
          l_CopyRegion.dstOffset = 0;
          l_CopyRegion.size = l_FrameUploadSpace;

          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              p_ViewInfo.get_current_staging_buffer().buffer.buffer,
              p_ViewInfo.get_directional_light_buffer().buffer, 1,
              &l_CopyRegion);
        }

        return true;
      }

      static bool prepare_ss2dcanvas_dimensions(float p_Delta,
                                                SS2DCanvas p_Canvas)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        p_Canvas.set_actual_dimensions(
            p_Canvas.get_desired_dimensions());

        if (p_Canvas.get_out_image().is_alive()) {
          Image l_Image =
              p_Canvas.get_out_image().get_gpu().get_data_handle();

          ImGui_ImplVulkan_RemoveTexture(
              (VkDescriptorSet)p_Canvas.get_out_image()
                  .get_gpu()
                  .get_imgui_texture_id());

          ImageUtil::destroy(l_Image);
          l_Image.destroy();
        }

        VkExtent3D l_Extent{p_Canvas.get_dimensions().x,
                            p_Canvas.get_dimensions().y, 1};

        const VkFormat l_ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        {
          if (!p_Canvas.get_out_image().is_alive()) {
            p_Canvas.set_out_image(Texture::make_gpu_ready(
                N(Out), TextureFormatCategory::Float));
          }

          Image l_Image =
              p_Canvas.get_out_image().get_gpu().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Out));
            p_Canvas.get_out_image().get_gpu().set_data_handle(
                l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent, l_ImageFormat,
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

        return true;
      }

      static bool prepare_render_view_dimensions(
          float p_Delta, RenderView p_RenderView, ViewInfo p_ViewInfo)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        p_RenderView.set_actual_dimensions(
            p_RenderView.get_desired_dimensions());

        if (p_ViewInfo.is_initialized()) {
          wait_idle();
          {
            Image l_Image = p_RenderView.get_gbuffer_albedo()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_albedo()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_gbuffer_normals()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_normals()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_gbuffer_viewposition()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView
                    .get_gbuffer_viewposition()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_highlight_map()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_highlight_map()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_object_map()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_object_map()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_gbuffer_depth()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_gbuffer_depth()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_lit_image()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_lit_image()
                    .get_gpu()
                    .get_imgui_texture_id());

            ImageUtil::destroy(l_Image);
            l_Image.destroy();
          }
          {
            Image l_Image = p_RenderView.get_blurred_image()
                                .get_gpu()
                                .get_data_handle();

            ImGui_ImplVulkan_RemoveTexture(
                (VkDescriptorSet)p_RenderView.get_blurred_image()
                    .get_gpu()
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

        // Update UI draw command buffer
        {
          if (!p_ViewInfo.is_initialized()) {
            AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
                sizeof(u32) * UI_DRAWCOMMAND_COUNT,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            p_ViewInfo.set_ui_drawcommand_buffer(l_Buffer);

            DescriptorUtil::DescriptorWriter l_Writer;

            l_Writer.write_buffer(
                4, p_ViewInfo.get_ui_drawcommand_buffer().buffer,
                sizeof(u32) * UI_DRAWCOMMAND_COUNT, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.update_set(
                Global::get_device(),
                p_ViewInfo.get_view_data_descriptor_set());
          }
        }

        VkExtent3D l_Extent{p_RenderView.get_dimensions().x,
                            p_RenderView.get_dimensions().y, 1};

        const VkFormat l_ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        {
          if (!p_RenderView.get_gbuffer_albedo().is_alive()) {
            p_RenderView.set_gbuffer_albedo(Texture::make_gpu_ready(
                N(Albedo), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_gbuffer_albedo()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Albedo));
            p_RenderView.get_gbuffer_albedo()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent, l_ImageFormat,
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
            p_RenderView.set_gbuffer_normals(Texture::make_gpu_ready(
                N(Normals), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_gbuffer_normals()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Normals));
            p_RenderView.get_gbuffer_normals()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent, l_ImageFormat,
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
          if (!p_RenderView.get_gbuffer_viewposition().is_alive()) {
            p_RenderView.set_gbuffer_viewposition(
                Texture::make_gpu_ready(
                    N(ViewPosition), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_gbuffer_viewposition()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(ViewPosition));
            p_RenderView.get_gbuffer_viewposition()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent, l_ImageFormat,
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
          if (!p_RenderView.get_highlight_map().is_alive()) {
            p_RenderView.set_highlight_map(Texture::make_gpu_ready(
                N(Highlight), TextureFormatCategory::Uint));
          }

          Image l_Image = p_RenderView.get_highlight_map()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(HighlightMap));
            p_RenderView.get_highlight_map()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
          }

          ImageUtil::create(l_Image, l_Extent, VK_FORMAT_R32_UINT,
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
          if (!p_RenderView.get_object_map().is_alive()) {
            p_RenderView.set_object_map(Texture::make_gpu_ready(
                N(ObjectMap), TextureFormatCategory::Uint));
          }

          Image l_Image = p_RenderView.get_object_map()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(ObjectMap));
            p_RenderView.get_object_map().get_gpu().set_data_handle(
                l_Image.get_id());
          }
          {
            AllocatedBuffer l_OldObjectBuffer =
                p_ViewInfo.get_object_id_buffer();
            if (l_OldObjectBuffer.buffer != VK_NULL_HANDLE) {
              BufferUtil::destroy_buffer(l_OldObjectBuffer);
              p_ViewInfo.set_object_id_buffer({});
            }
          }

          ImageUtil::create(l_Image, l_Extent, VK_FORMAT_R32_UINT,
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                            false);

          AllocatedBuffer l_ObjectBuffer = BufferUtil::create_buffer(
              sizeof(u32) * l_Extent.width * l_Extent.height,
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
              VMA_MEMORY_USAGE_GPU_TO_CPU);

          p_ViewInfo.set_object_id_buffer(l_ObjectBuffer);

          ImageUtil::cmd_transition(
              l_Cmd, l_Image, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        {
          if (!p_RenderView.get_gbuffer_depth().is_alive()) {
            p_RenderView.set_gbuffer_depth(Texture::make_gpu_ready(
                N(Depth), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_gbuffer_depth()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Depth));
            l_Image.set_depth(true);
            p_RenderView.get_gbuffer_depth()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
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
            p_RenderView.set_lit_image(Texture::make_gpu_ready(
                N(Lit), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_lit_image()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Lit));
            p_RenderView.get_lit_image().get_gpu().set_data_handle(
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
          if (!p_RenderView.get_blurred_image().is_alive()) {
            p_RenderView.set_blurred_image(Texture::make_gpu_ready(
                N(Blurred), TextureFormatCategory::Float));
          }

          Image l_Image = p_RenderView.get_blurred_image()
                              .get_gpu()
                              .get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Image::make(N(Blurred));
            p_RenderView.get_blurred_image()
                .get_gpu()
                .set_data_handle(l_Image.get_id());
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
              0,
              p_RenderView.get_gbuffer_albedo()
                  .get_gpu()
                  .get_data_handle(),
              l_Samplers.no_lod_nearest_repeat_black,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

          l_Writer.write_image(
              1,
              p_RenderView.get_gbuffer_normals()
                  .get_gpu()
                  .get_data_handle(),
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
        size_t l_StagingOffset = 0;

        const u64 l_FrameInfoSize = sizeof(ViewInfoFrameData);

        const u64 l_FrameUploadSpace =
            p_ViewInfo.request_current_staging_buffer_space(
                l_FrameInfoSize, &l_StagingOffset);

        LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >= l_FrameInfoSize,
                              "Did not have enough staging "
                              "buffer space to upload "
                              "view info frame data.");
        {
          ViewInfoFrameData l_FrameData;

          {

            const float l_Zoom = p_RenderView.get_ui_camera_zoom();
            const Math::Vector2 l_Panning =
                p_RenderView.get_ui_camera_position();
            const Math::UVector2 l_Dimensions =
                p_RenderView.get_dimensions();

            Math::Matrix4x4 l_ViewMatrix(1.0f);
            l_ViewMatrix = glm::scale(
                l_ViewMatrix, Math::Vector3(l_Zoom, l_Zoom, 1.0f));
            l_ViewMatrix = glm::translate(
                l_ViewMatrix, Math::Vector3(-l_Panning, 0.0f));

            Math::Matrix4x4 l_ProjectionMatrix =
                glm::ortho(0.0f, (float)l_Dimensions.x,
                           (float)l_Dimensions.y, 0.0f, -1.0f, 1.0f);

            const Math::Matrix4x4 l_ViewProjection =
                l_ProjectionMatrix * l_ViewMatrix;

            l_FrameData.uiViewProjectionMatrix = l_ViewProjection;

            p_RenderView.set_ui_projection_matrix(l_ProjectionMatrix);
            p_RenderView.set_ui_view_matrix(l_ViewMatrix);
          }

          l_FrameData.dimensions.x = p_RenderView.get_dimensions().x;
          l_FrameData.dimensions.y = p_RenderView.get_dimensions().y;
          l_FrameData.dimensions.z = 1.0f / l_FrameData.dimensions.x;
          l_FrameData.dimensions.w = 1.0f / l_FrameData.dimensions.y;

          l_FrameData.projectionMatrix = glm::perspective(
              glm::radians(p_RenderView.get_camera_fov()),
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

          l_FrameData.gbufferIndices.w =
              p_RenderView.get_gbuffer_viewposition()
                  .get_gpu()
                  .get_bindless_index();
          l_FrameData.gbufferIndices.z =
              p_RenderView.get_gbuffer_depth()
                  .get_gpu()
                  .get_bindless_index();
          l_FrameData.gbufferIndices.x =
              p_RenderView.get_gbuffer_albedo()
                  .get_gpu()
                  .get_bindless_index();
          l_FrameData.gbufferIndices.y =
              p_RenderView.get_gbuffer_normals()
                  .get_gpu()
                  .get_bindless_index();

          l_FrameData.lightClusters.x =
              p_ViewInfo.get_light_clusters().x;
          l_FrameData.lightClusters.y =
              p_ViewInfo.get_light_clusters().y;
          l_FrameData.lightClusters.z =
              p_ViewInfo.get_light_clusters().z;
          l_FrameData.lightClusters.w =
              p_ViewInfo.get_light_cluster_count();

          l_FrameData.textureIndices.x = 0;
          if (p_RenderView.get_ssao_image().is_alive() &&
              p_RenderView.get_ssao_image().get_state() ==
                  TextureState::LOADED) {
            l_FrameData.textureIndices.x =
                p_RenderView.get_ssao_image()
                    .get_gpu()
                    .get_bindless_index();
          }

          l_FrameData.textureIndices.y = 0;
          if (p_RenderView.get_cavities_image().is_alive() &&
              p_RenderView.get_cavities_image().get_state() ==
                  TextureState::LOADED) {
            l_FrameData.textureIndices.y =
                p_RenderView.get_cavities_image()
                    .get_gpu()
                    .get_bindless_index();
          }

          p_ViewInfo.write_current_staging_buffer(
              &l_FrameData, l_FrameUploadSpace, l_StagingOffset);

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = l_StagingOffset;
          l_CopyRegion.dstOffset = 0;
          l_CopyRegion.size = l_FrameUploadSpace;

          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              p_ViewInfo.get_current_staging_buffer().buffer.buffer,
              p_ViewInfo.get_view_data_buffer().buffer, 1,
              &l_CopyRegion);
        }

        return true;
      }

      static bool render_ss2d_canvases(const float p_Delta)
      {
        VkCommandBuffer l_Cmd = Global::get_current_command_buffer();

        VK_RENDERDOC_SECTION_BEGIN("SS2D Drawing",
                                   SINGLE_ARG({0.9f, 0.5f, 0.0f}));

        for (u32 i = 0u; i < SS2DCanvas::living_count(); ++i) {
          SS2DCanvas i_Canvas = SS2DCanvas::living_instances()[i];

          SS2DCanvasBackend i_Backend = i_Canvas.get_backend_handle();

          if (!i_Backend.is_alive()) {
            i_Backend = SS2DCanvasBackend::make(i_Canvas.get_name());

            i_Backend.set_canvas_data(
                Global::get_global_descriptor_allocator().allocate(
                    Global::get_device(), g_Ss2dCanvasDSLayout));

            i_Canvas.set_backend_handle(i_Backend.get_id());
          }

          if (i_Canvas.is_dimensions_dirty()) {

            if (!prepare_ss2dcanvas_dimensions(p_Delta, i_Canvas)) {
              return false;
            }

            i_Canvas.set_dimensions_dirty(false);
          }

          if (!i_Canvas.get_drawcommand_index_buffer().is_alive()) {
            i_Canvas.set_drawcommand_index_buffer(
                Buffer::make(N(DC_Index)));
          }

          BufferHolder l_Holder =
              i_Canvas.get_drawcommand_index_buffer()
                  .get_data_handle();

          if (!l_Holder.is_alive()) {
            l_Holder = BufferUtil::create_buffer_holder(
                sizeof(u32) * SS2DDRAWCOMMAND_COUNT,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            i_Canvas.get_drawcommand_index_buffer().set_data_handle(
                l_Holder.get_id());

            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(0, l_Holder.get().buffer,
                                  l_Holder.get().info.size, 0,
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

            l_Writer.update_set(Global::get_device(),
                                i_Backend.get_canvas_data());
          }

          VkDescriptorSet i_CanvasDS = i_Backend.get_canvas_data();

          {

            u32 i_DrawCommandIndices[SS2DDRAWCOMMAND_COUNT];

            for (u32 j = 0; j < SS2DDRAWCOMMAND_COUNT; ++j) {
              i_DrawCommandIndices[j] =
                  SS2DDrawCommand::get_capacity() + 10;
            }

            u32 j = 0;
            for (auto it = i_Canvas.get_draw_commands().begin();
                 it != i_Canvas.get_draw_commands().end();) {
              SS2DDrawCommand i_DrawCommand = *it;
              if (!i_DrawCommand.is_alive()) {
                it = i_Canvas.get_draw_commands().erase(it);
                continue;
              }
              i_DrawCommandIndices[j] = i_DrawCommand.get_index();

              j++;
              it++;
            }

            size_t l_StagingOffset = 0;
            const u64 l_FrameUploadSpace =
                Vulkan::Global::get_current_frame_staging_buffer()
                    .request_space(sizeof(u32) *
                                       SS2DDRAWCOMMAND_COUNT,
                                   &l_StagingOffset);

            if (l_FrameUploadSpace <
                sizeof(u32) * SS2DDRAWCOMMAND_COUNT) {
              // We don't have enough space on the staging buffer to
              // upload the indices
              return false;
            }

            LOW_ASSERT(
                Vulkan::Global::get_current_frame_staging_buffer()
                    .write(i_DrawCommandIndices,
                           sizeof(u32) * SS2DDRAWCOMMAND_COUNT,
                           l_StagingOffset),
                "Failed to write ss2d canvas DC indices to staging "
                "buffer");

            VkBufferCopy i_CopyRegion;
            i_CopyRegion.srcOffset = l_StagingOffset;
            i_CopyRegion.dstOffset = 0;
            i_CopyRegion.size = l_FrameUploadSpace;

            // TODO: Change to transfer queue command buffer - or
            // leave this specifically on the graphics queue not sure
            // tbh
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_frame_staging_buffer()
                    .buffer.buffer,
                l_Holder.get().buffer, 1, &i_CopyRegion);
          }

          ImageUtil::cmd_transition(
              l_Cmd,
              i_Canvas.get_out_image().get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.2f, 0.2f, 0.2f, 1.0f}};

          Image l_OutImage =
              i_Canvas.get_out_image().get_gpu().get_data_handle();

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_OutImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {i_Canvas.get_dimensions().x,
               i_Canvas.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            g_Pipelines.ss2d.get());

          vkCmdBindDescriptorSets(l_Cmd,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  g_Pipelines.ss2dLayout.get(), 0, 1,
                                  &g_Ss2dGlobalSet, 0, nullptr);

          vkCmdBindDescriptorSets(l_Cmd,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  g_Pipelines.ss2dLayout.get(), 1, 1,
                                  &i_CanvasDS, 0, nullptr);

          VkViewport l_Viewport = {};
          l_Viewport.x = 0;
          l_Viewport.y = 0;
          l_Viewport.width =
              static_cast<float>(i_Canvas.get_dimensions().x);
          l_Viewport.height =
              static_cast<float>(i_Canvas.get_dimensions().y);
          l_Viewport.minDepth = 0.f;
          l_Viewport.maxDepth = 1.f;

          vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

          VkRect2D l_Scissor = {};
          l_Scissor.offset.x = 0;
          l_Scissor.offset.y = 0;
          l_Scissor.extent.width = i_Canvas.get_dimensions().x;
          l_Scissor.extent.height = i_Canvas.get_dimensions().y;

          vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

          vkCmdDraw(l_Cmd, 3, 1, 0, 0);

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd,
              i_Canvas.get_out_image().get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        VK_RENDERDOC_SECTION_END();

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
            l_Writer.write_buffer(
                5, i_ViewInfo.get_debug_geometry_buffer().buffer,
                sizeof(DebugGeometryUpload) * DEBUG_GEOMETRY_COUNT, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Writer.write_buffer(
                6, i_ViewInfo.get_directional_light_buffer().buffer,
                sizeof(DirectionalLightInfo), 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

            l_Writer.update_set(
                Global::get_device(),
                i_ViewInfo.get_view_data_descriptor_set());
          }

          // Reset the current staging buffer
          i_ViewInfo.get_current_staging_buffer().occupied = 0;

          LOWR_VK_ASSERT_RETURN(
              prepare_render_view_directional_light(
                  p_Delta, i_RenderView, i_ViewInfo),
              "Failed to update directional light info for render "
              "view");

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

      bool update_dirty_editor_images(float p_Delta)
      {
        Samplers &l_Samplers = Global::get_samplers();

        for (auto it = EditorImageGpu::ms_Dirty.begin();
             it != EditorImageGpu::ms_Dirty.end();) {

          EditorImageGpu i_Gpu = *it;
          if (!i_Gpu.is_alive()) {
            it = EditorImageGpu::ms_Dirty.erase(it);
            continue;
          }

          EditorImage i_EditorImage = i_Gpu.get_editor_image_handle();

          if (!i_EditorImage.is_alive()) {
            it = EditorImageGpu::ms_Dirty.erase(it);
            continue;
          }

          if (i_EditorImage.get_state() != TextureState::LOADED) {
            ++it;
            continue;
          }

          Image i_Image = i_Gpu.get_data_handle();

          if (!i_Image.is_alive()) {
            ++it;
            continue;
          }

          i_Gpu.set_imgui_texture_id(
              (ImTextureID)ImGui_ImplVulkan_AddTexture(
                  l_Samplers.no_lod_nearest_repeat_black,
                  i_Image.get_allocated_image().imageView,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

          i_Gpu.set_imgui_texture_initialized(true);

          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
            EditorImageUpdate i_Update;
            i_Update.gpuEditorImage = i_Gpu;
            i_Update.editorImageIndex = i_Gpu.get_index();
            Global::get_editor_image_update_queue(i).push(i_Update);
          }

          it = EditorImageGpu::ms_Dirty.erase(it);
        }

        return true;
      }

      bool update_dirty_textures(float p_Delta)
      {
        Samplers &l_Samplers = Global::get_samplers();

        for (auto it = GpuTexture::ms_Dirty.begin();
             it != GpuTexture::ms_Dirty.end(); ++it) {

          GpuTexture i_GpuTexture = *it;
          if (!i_GpuTexture.is_alive()) {
            continue;
          }

          Image i_Image = i_GpuTexture.get_data_handle();

          if (!i_Image.is_alive()) {
            continue;
          }

          i_GpuTexture.set_imgui_texture_id(
              (ImTextureID)ImGui_ImplVulkan_AddTexture(
                  l_Samplers.no_lod_nearest_repeat_black,
                  i_Image.get_allocated_image().imageView,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

          for (u32 i = 0; i < Global::get_frame_overlap(); ++i) {
            TextureUpdate i_Update;
            i_Update.gpuTexture = i_GpuTexture;
            i_Update.textureIndex = i_GpuTexture.get_bindless_index();
            Global::get_texture_update_queue(i).push(i_Update);
          }
        }

        GpuTexture::ms_Dirty.clear();

        return true;
      }

      static bool update_point_lights(float p_Delta)
      {
        bool l_Result = true;
        for (u32 i = 0; i < RenderScene::living_count(); ++i) {
          RenderScene i_RenderScene =
              RenderScene::living_instances()[i];
          Vulkan::Scene i_Scene = i_RenderScene.get_data_handle();

          if (!i_Scene.is_initialized()) {
            i_Scene.set_initialized(true);

            size_t i_StagingOffset = 0;
            const u64 i_FrameUploadSpace =
                Vulkan::Global::get_current_frame_staging_buffer()
                    .request_space(sizeof(PointLightInfo) *
                                       POINTLIGHT_COUNT,
                                   &i_StagingOffset);

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

            Util::List<PointLightInfo> i_Infos;
            i_Infos.resize(POINTLIGHT_COUNT);
            for (u32 k = 0; k < POINTLIGHT_COUNT; ++k) {
              i_Infos[k] = i_Info;
            }

            LOW_ASSERT(
                Vulkan::Global::get_current_frame_staging_buffer()
                    .write(i_Infos.data(),
                           sizeof(PointLightInfo) * POINTLIGHT_COUNT,
                           i_StagingOffset),
                "Failed to write pointlight data to staging "
                "buffer");

            VkBufferCopy i_CopyRegion;
            i_CopyRegion.srcOffset = i_StagingOffset;
            i_CopyRegion.dstOffset = 0;
            i_CopyRegion.size = i_FrameUploadSpace;

            // TODO: Change to transfer queue command buffer - or
            // leave this specifically on the graphics queue not sure
            // tbh
            // PERF: First call probably very inefficient, maybe we
            // can fix that at some point
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_frame_staging_buffer()
                    .buffer.buffer,
                i_Scene.get_point_light_buffer().buffer, 1,
                &i_CopyRegion);
          }

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
            const u64 l_FrameUploadSpace =
                Vulkan::Global::get_current_frame_staging_buffer()
                    .request_space(sizeof(PointLightInfo),
                                   &l_StagingOffset);

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

            LOW_ASSERT(
                Vulkan::Global::get_current_frame_staging_buffer()
                    .write(&i_Info, sizeof(PointLightInfo),
                           l_StagingOffset),
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
            // TODO: Check if it's possible to batch all these
            // together per renderscene
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_frame_staging_buffer()
                    .buffer.buffer,
                i_Scene.get_point_light_buffer().buffer, 1,
                &i_CopyRegion);
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
            const u64 l_FrameUploadSpace =
                Vulkan::Global::get_current_frame_staging_buffer()
                    .request_space(sizeof(PointLightInfo),
                                   &l_StagingOffset);

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

            LOW_ASSERT(
                Vulkan::Global::get_current_frame_staging_buffer()
                    .write(&i_Info, sizeof(PointLightInfo),
                           l_StagingOffset),
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
            // TODO: Check if it's possible to batch all these
            // together per renderscene
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_frame_staging_buffer()
                    .buffer.buffer,
                i_Scene.get_point_light_buffer().buffer,
                i_Copies.size(), i_Copies.data());
          }

          i_RenderScene.get_pointlight_deleted_slots().clear();
        }

        return l_Result;
      }

      static float half_to_float(uint16_t half)
      {
        uint16_t h = half;
        uint32_t sign = (h >> 15) & 0x00000001;
        uint32_t exp = (h >> 10) & 0x0000001f;
        uint32_t mant = h & 0x000003ff;

        uint32_t f;
        if (exp == 0) {
          if (mant == 0) {
            // Zero
            f = sign << 31;
          } else {
            // Subnormal
            while ((mant & 0x00000400) == 0) {
              mant <<= 1;
              exp -= 1;
            }
            exp += 1;
            mant &= ~0x00000400;
            f = (sign << 31) | ((exp + (127 - 15)) << 23) |
                (mant << 13);
          }
        } else if (exp == 31) {
          // Inf/NaN
          f = (sign << 31) | 0x7f800000 | (mant << 13);
        } else {
          // Normalized
          f = (sign << 31) | ((exp + (127 - 15)) << 23) |
              (mant << 13);
        }

        float result;
        memcpy(&result, &f, sizeof(result));
        return result;
      }

      static uint8_t float_to_unorm8(float f)
      {
        return static_cast<uint8_t>(glm::clamp(f, 0.0f, 1.0f) *
                                    255.0f);
      }

      static bool handle_texture_exports(float p_Delta)
      {
        Util::List<TextureExport> l_ExportsToDelete;

        for (u32 i = 0; i < TextureExport::living_count(); ++i) {
          TextureExport i_Export =
              TextureExport::living_instances()[i];

          if (i_Export.get_state() == TextureExportState::COPIED) {

            TexExport i_TexExport = i_Export.get_data_handle();

            if (i_TexExport.get_frame_index() !=
                Global::get_current_frame_index()) {
              continue;
            }

            Image i_Image =
                i_Export.get_texture().get_gpu().get_data_handle();

            // Map buffer and write PNG
            {
              const u64 l_ImageSize =
                  i_TexExport.get_staging_buffer().info.size;

              const u32 l_Width =
                  i_Image.get_allocated_image().extent.width;
              const u32 l_Height =
                  i_Image.get_allocated_image().extent.height;

              // TODO: Check channel count
              u32 l_Channels = 4;
              u32 l_PixelSize = sizeof(u8);

              if (i_Image.get_allocated_image().format ==
                  VK_FORMAT_R16G16B16A16_SFLOAT) {
                l_PixelSize = sizeof(u16);
              }

              _LOW_ASSERT(l_ImageSize == l_Width * l_Height *
                                             l_Channels *
                                             l_PixelSize);

              // Flip image vertically and write
              Util::List<u8> l_Pixels;
              l_Pixels.resize(l_Width * l_Height * l_Channels);

              if (i_Image.get_allocated_image().format ==
                  VK_FORMAT_R16G16B16A16_SFLOAT) {

                u16 *l_Data = reinterpret_cast<u16 *>(
                    i_TexExport.get_staging_buffer()
                        .info.pMappedData);

                for (u32 j = 0; j < l_Width * l_Height; ++j) {

                  float r = half_to_float(l_Data[j * 4 + 0]);
                  float g = half_to_float(l_Data[j * 4 + 1]);
                  float b = half_to_float(l_Data[j * 4 + 2]);
                  float a = half_to_float(l_Data[j * 4 + 3]);

                  l_Pixels[j * 4 + 0] = float_to_unorm8(r);
                  l_Pixels[j * 4 + 1] = float_to_unorm8(g);
                  l_Pixels[j * 4 + 2] = float_to_unorm8(b);
                  l_Pixels[j * 4 + 3] = float_to_unorm8(a);
                }

              } else {
                uint8_t *l_Data = reinterpret_cast<uint8_t *>(
                    i_TexExport.get_staging_buffer()
                        .info.pMappedData);

                for (u64 j = 0; j < l_ImageSize; ++j) {
                  l_Pixels[j] = l_Data[j];
                }
              }

              /*
              Util::List<uint8_t> l_FlippedPixels;
              l_FlippedPixels.resize(l_Width * l_Height *
                                     l_Channels);

              for (uint32_t i_Row = 0; i_Row < l_Height; ++i_Row) {
                memcpy(
                    &l_FlippedPixels[i_Row * l_Width * l_Channels],
                    &l_Pixels[(l_Height - i_Row - 1) * l_Width *
                              l_Channels],
                    l_Width * l_Channels);
              }
              */

              stbi_write_png(i_Export.get_path().c_str(), l_Width,
                             l_Height, l_Channels, l_Pixels.data(),
                             l_Width * l_Channels);
            }

            LOWR_VK_ASSERT_RETURN(
                i_Export.finish(),
                "Failed to call finish callback on TextureExport");

            l_ExportsToDelete.push_back(i_Export);
          }
        }

        for (auto it = l_ExportsToDelete.begin();
             it != l_ExportsToDelete.end(); ++it) {
          it->destroy();
        }
        return true;
      }

      bool prepare_tick(float p_Delta)
      {
        static bool l_Initialized = false;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
          for (u32 i = 0; i < MaterialType::living_count(); ++i) {
            MaterialType i_MaterialType =
                MaterialType::living_instances()[i];

            Pipeline i_DrawPipeline =
                i_MaterialType.get_draw_pipeline_handle();
            Pipeline i_PickPipeline =
                i_MaterialType.get_pick_pipeline_handle();
            Pipeline i_HighlightPipeline =
                i_MaterialType.get_highlight_pipeline_handle();

            // TODO: Choose different layout based on family

            if (!i_DrawPipeline.is_alive()) {
              i_MaterialType.set_draw_pipeline_handle(
                  create_pipeline_from_config(
                      i_MaterialType.get_draw_pipeline_config(),
                      g_Pipelines.solidBasePipelineLayout));
            }
            if (i_MaterialType.allows_picking()) {
              if (!i_PickPipeline.is_alive()) {
                i_MaterialType.set_pick_pipeline_handle(
                    create_pipeline_from_config(
                        i_MaterialType.get_pick_pipeline_config(),
                        g_Pipelines.solidBasePipelineLayout));
              }
            }
            if (i_MaterialType.allows_highlighting()) {
              if (!i_HighlightPipeline.is_alive()) {
                i_MaterialType.set_highlight_pipeline_handle(
                    create_pipeline_from_config(
                        i_MaterialType
                            .get_highlight_pipeline_config(),
                        g_Pipelines.solidHighlightPipelineLayout));
              }
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

        // render_ss2d_canvases(p_Delta);

        update_dirty_textures(p_Delta);
        update_dirty_editor_images(p_Delta);

        LOWR_VK_ASSERT_RETURN(handle_texture_exports(p_Delta),
                              "Failed to export textures.");

        {
          DescriptorUtil::DescriptorWriter l_Writer;

          Samplers &l_Samplers = Global::get_samplers();

          bool l_AddedEntry = false;

          while (
              !Global::get_current_texture_update_queue().empty()) {
            TextureUpdate i_Update =
                Global::get_current_texture_update_queue().front();
            Global::get_current_texture_update_queue().pop();

            if (!i_Update.gpuTexture.is_alive()) {
              continue;
            }

            Image i_Image = i_Update.gpuTexture.get_data_handle();

            if (!i_Image.is_alive()) {
              continue;
            }

            l_AddedEntry = true;

            {
              TextureFormatCategory l_Category =
                  i_Update.gpuTexture.get_format_category();
              u32 l_Binding =
                  (l_Category == TextureFormatCategory::Float)  ? 0
                  : (l_Category == TextureFormatCategory::Uint) ? 1
                                                                : 2;
              l_Writer.write_image(
                  l_Binding, i_Image.get_allocated_image().imageView,
                  VK_NULL_HANDLE,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                  i_Update.textureIndex);
            }
          }

          if (l_AddedEntry) {
            l_Writer.update_set(
                Global::get_device(),
                Global::get_current_texture_descriptor_set());
          }
        }

        {
          DescriptorUtil::DescriptorWriter l_Writer;

          Samplers &l_Samplers = Global::get_samplers();

          bool l_AddedEntry = false;

          while (!Global::get_current_editor_image_update_queue()
                      .empty()) {
            EditorImageUpdate i_Update =
                Global::get_current_editor_image_update_queue()
                    .front();
            Global::get_current_editor_image_update_queue().pop();

            if (!i_Update.gpuEditorImage.is_alive()) {
              continue;
            }

            Image i_Image = i_Update.gpuEditorImage.get_data_handle();

            if (!i_Image.is_alive()) {
              continue;
            }

            l_AddedEntry = true;

            l_Writer.write_image(
                4, i_Image.get_allocated_image().imageView,
                l_Samplers.no_lod_nearest_repeat_black,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                i_Update.editorImageIndex);
          }

          if (l_AddedEntry) {
            l_Writer.update_set(
                Global::get_device(),
                Global::get_current_texture_descriptor_set());
          }
        }

        return l_Result;
      }

      bool copy_image_to_staging_buffer()
      {
        return true;
      }

      bool tick(float p_Delta)
      {
        if (!g_Context.requireResize) {
          LOWR_VK_ASSERT_RETURN(draw(g_Context, p_Delta),
                                "Failed to draw vulkan renderer");

          // Handle Texture exports
          {
            VkCommandBuffer l_Cmd =
                Global::get_current_command_buffer();

            for (u32 i = 0; i < TextureExport::living_count(); ++i) {
              TextureExport i_Export =
                  TextureExport::living_instances()[i];

              if (!i_Export.get_texture().is_alive() ||
                  i_Export.get_texture().get_state() !=
                      TextureState::LOADED) {
                continue;
              }

              if (i_Export.get_state() ==
                  TextureExportState::SCHEDULED) {

                TexExport i_TexExport =
                    TexExport::make(i_Export.get_name());
                i_Export.set_data_handle(i_TexExport.get_id());

                i_TexExport.set_frame_index(
                    Global::get_current_frame_index());

                Texture i_Texture = i_Export.get_texture();
                Image i_Image = i_Texture.get_gpu().get_data_handle();

                const u32 l_Width =
                    i_Image.get_allocated_image().extent.width;
                const u32 l_Height =
                    i_Image.get_allocated_image().extent.height;
                // TODO: Check channel count
                u32 l_Channels = 4;
                u32 l_PixelSize = sizeof(u8);

                if (i_Image.get_allocated_image().format ==
                    VK_FORMAT_R16G16B16A16_SFLOAT) {
                  l_PixelSize = sizeof(u16);
                }

                const u64 l_ImageSize =
                    l_Width * l_Height * l_Channels * l_PixelSize;

                LOW_LOG_DEBUG << "IMAGESIZE: " << l_ImageSize
                              << LOW_LOG_END;

                AllocatedBuffer l_Buffer = BufferUtil::create_buffer(
                    l_ImageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VMA_MEMORY_USAGE_GPU_TO_CPU);

                i_TexExport.set_staging_buffer(l_Buffer);

                i_Export.observe(OBSERVABLE_DESTROY,
                                 i_TexExport.get_id());

                ImageUtil::cmd_transition(
                    l_Cmd, i_Image,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                // Copy image to buffer
                {
                  VkBufferImageCopy l_Region = {};
                  l_Region.bufferOffset = 0;
                  l_Region.bufferRowLength = 0;
                  l_Region.bufferImageHeight = 0;
                  l_Region.imageSubresource.aspectMask =
                      VK_IMAGE_ASPECT_COLOR_BIT;
                  l_Region.imageSubresource.mipLevel = 0;
                  l_Region.imageSubresource.baseArrayLayer = 0;
                  l_Region.imageSubresource.layerCount = 1;
                  l_Region.imageExtent = {l_Width, l_Height, 1};

                  vkCmdCopyImageToBuffer(
                      l_Cmd, i_Image.get_allocated_image().image,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      i_TexExport.get_staging_buffer().buffer, 1,
                      &l_Region);
                }

                ImageUtil::cmd_transition(
                    l_Cmd, i_Image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                i_Export.set_state(TextureExportState::COPIED);
              }
            }
          }

          Base::context_present(g_Context);
        }

        if (!RenderView::ms_ScheduledForDeletion.empty()) {
          vkDeviceWaitIdle(Global::get_device());
          RenderView::ms_FullDestroy = true;
          for (auto it = RenderView::ms_ScheduledForDeletion.begin();
               it != RenderView::ms_ScheduledForDeletion.end();
               ++it) {
            RenderView i_RenderView = *it;
            if (i_RenderView.is_alive()) {
              i_RenderView.destroy();
            }
          }
          RenderView::ms_ScheduledForDeletion.clear();
          RenderView::ms_FullDestroy = false;
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

        /*
        if (l_Dimensions.x > 0 && l_Dimensions.y > 0) {
          RenderView i_RenderView = RenderView::living_instances()[0];
          i_RenderView.set_dimensions(l_Dimensions);
        }
        */

        return true;
      }
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
