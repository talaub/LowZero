#include "LowRendererVulkanRenderStepsBasic.h"

#include "LowRendererRenderStep.h"
#include "LowRendererRenderView.h"
#include "LowRendererUiCanvas.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "LowUtil.h"

#include "LowRenderer.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererVulkan.h"
#include "LowRendererVkImage.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVkPipeline.h"
#include "LowRendererVkViewInfo.h"
#include "LowRendererDrawCommand.h"
#include "LowRendererMeshInfo.h"
#include "LowRendererVulkanBase.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanBuffer.h"

#include <EASTL/sort.h>

#define GET_STEP_DATA(renderview, renderstep)                        \
  renderview.get_step_data()[renderstep.get_index()]

namespace Low {
  namespace Renderer {
    namespace Vulkan {

      static bool initialize_solid_material_renderstep()
      {

        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_SOLID_MATERIAL_NAME);

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Draw solids",
                                     SINGLE_ARG({0.5f, 0.5f, 0.5f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          VkClearValue l_ClearColorValueDepth = {};
          l_ClearColorValueDepth.depthStencil.depth =
              1.0f; // Depth clear value (1.0f is the farthest)
          l_ClearColorValueDepth.depthStencil.stencil =
              0; // Stencil clear value

          Image l_AlbedoImage = p_RenderView.get_gbuffer_albedo()
                                    .get_gpu()
                                    .get_data_handle();
          Image l_NormalsImage = p_RenderView.get_gbuffer_normals()
                                     .get_gpu()
                                     .get_data_handle();
          Image l_DepthImage = p_RenderView.get_gbuffer_depth()
                                   .get_gpu()
                                   .get_data_handle();
          Image l_ViewPositionImage =
              p_RenderView.get_gbuffer_viewposition()
                  .get_gpu()
                  .get_data_handle();

          {
            VkViewport l_Viewport = {};
            l_Viewport.x = 0;
            l_Viewport.y = 0;
            l_Viewport.width =
                static_cast<float>(p_RenderView.get_dimensions().x);
            l_Viewport.height =
                static_cast<float>(p_RenderView.get_dimensions().y);
            l_Viewport.minDepth = 0.f;
            l_Viewport.maxDepth = 1.f;

            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            VkRect2D l_Scissor = {};
            l_Scissor.offset.x = 0;
            l_Scissor.offset.y = 0;
            l_Scissor.extent.width = p_RenderView.get_dimensions().x;
            l_Scissor.extent.height = p_RenderView.get_dimensions().y;

            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);
          }

          {
            // Transfer the gbuffer images
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_albedo()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_normals()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_viewposition()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_depth()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
          }

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(3);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_AlbedoImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);
          l_ColorAttachments[1] = InitUtil::attachment_info(
              l_NormalsImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);
          l_ColorAttachments[2] = InitUtil::attachment_info(
              l_ViewPositionImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingAttachmentInfo l_DepthAttachment =
              InitUtil::attachment_info(
                  l_DepthImage.get_allocated_image().imageView,
                  &l_ClearColorValueDepth,
                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x,
               p_RenderView.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              &l_DepthAttachment);

          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindIndexBuffer(
              l_Cmd, Global::get_mesh_index_buffer().m_Buffer.buffer,
              0, VK_INDEX_TYPE_UINT32);

          MaterialType l_CurrentMaterialType =
              Low::Util::Handle::DEAD;

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

            if (it->get_render_object().get_mesh().get_state() !=
                MeshState::LOADED) {
              it++;
              continue;
            }

            GpuSubmesh i_GpuSubmesh = it->get_submesh();

            LOW_ASSERT(i_GpuSubmesh.is_alive(),
                       "Submesh of mesh entry not alive anymore. "
                       "This should not happen if the corresponding "
                       "RenderObject is still alive.");
            LOW_ASSERT(
                i_GpuSubmesh.get_state() == MeshState::LOADED,
                "Submesh is not loaded to GPU. Cannot render.");

            Material i_Material = it->get_material();
            /*
            if (!i_Material.is_alive()) {
              i_Material = i_MeshInfo.get_material();
            }
            */
            if (!i_Material.is_alive()) {
              i_Material = it->get_render_object().get_material();
            }

            LOW_ASSERT(i_Material.is_alive(),
                       "Material of draw command is not alive "
                       "anymore. This should not happen.");

            if (i_Material.get_material_type() !=
                l_CurrentMaterialType) {
              l_CurrentMaterialType = i_Material.get_material_type();

              Pipeline i_Pipeline =
                  l_CurrentMaterialType.get_draw_pipeline_handle();

              vkCmdBindPipeline(l_Cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                i_Pipeline.get_pipeline());

              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();

              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  i_Pipeline.get_layout(), 0, 1, &l_Set, 0, nullptr);

              {
                VkDescriptorSet l_TextureSet =
                    Global::get_current_texture_descriptor_set();
                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout(), 1, 1, &l_TextureSet, 0,
                    nullptr);
              }

              VkDescriptorSet l_DescriptorSet =
                  l_ViewInfo.get_view_data_descriptor_set();

              vkCmdBindDescriptorSets(l_Cmd,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      i_Pipeline.get_layout(), 2, 1,
                                      &l_DescriptorSet, 0, nullptr);
            }

            Pipeline i_Pipeline =
                l_CurrentMaterialType.get_draw_pipeline_handle();

            RenderEntryPushConstant i_PushConstants;
            i_PushConstants.renderObjectSlot = it->get_slot();

            vkCmdPushConstants(l_Cmd, i_Pipeline.get_layout(),
                               VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                               sizeof(RenderEntryPushConstant),
                               &i_PushConstants);

            vkCmdDrawIndexed(l_Cmd, i_GpuSubmesh.get_index_count(), 1,
                             i_GpuSubmesh.get_index_start(),
                             i_GpuSubmesh.get_vertex_start(), 0);

            it++;
          }

          vkCmdEndRendering(l_Cmd);

          {
            // Transfer the gbuffer images
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_albedo()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_normals()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_viewposition()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_depth()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          VK_RENDERDOC_SECTION_END();

          return true;
        });

        return true;
      }

      struct BaseLightingStepData
      {
        Pipeline pipeline;
      };

      static bool initialize_base_lighting_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_LIGHTING_NAME);

        l_RenderStep.set_prepare_callback(
            [&](RenderStep p_RenderStep,
                RenderView p_RenderView) -> bool {
              BaseLightingStepData *l_Data = new BaseLightingStepData;

              Util::String l_VertexShaderPath =
                  "fullscreen_triangle.vert";
              Util::String l_FragmentShaderPath = "lighting.frag";

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

              l_Data->pipeline = l_Builder.register_pipeline();

              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              return true;
            });
        l_RenderStep.set_teardown_callback(
            [&](RenderStep p_RenderStep,
                RenderView p_RenderView) -> bool {
              // TODO: Delete data
              return true;
            });

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Base lighting",
                                     SINGLE_ARG({0.0f, 1.0f, 1.0f}));

          BaseLightingStepData *l_Data =
              (BaseLightingStepData *)p_RenderView
                  .get_step_data()[p_RenderStep.get_index()];

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          Image l_LitImage = p_RenderView.get_lit_image()
                                 .get_gpu()
                                 .get_data_handle();

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_LitImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x,
               p_RenderView.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            l_Data->pipeline.get_pipeline());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout(), 0, 1, &l_Set,
                0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  Global::get_lighting_pipeline_layout(), 1, 1,
                  &l_TextureSet, 0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                l_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout(), 2, 1,
                &l_DescriptorSet, 0, nullptr);

            VkViewport l_Viewport = {};
            l_Viewport.x = 0;
            l_Viewport.y = 0;
            l_Viewport.width =
                static_cast<float>(p_RenderView.get_dimensions().x);
            l_Viewport.height =
                static_cast<float>(p_RenderView.get_dimensions().y);
            l_Viewport.minDepth = 0.f;
            l_Viewport.maxDepth = 1.f;

            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            VkRect2D l_Scissor = {};
            l_Scissor.offset.x = 0;
            l_Scissor.offset.y = 0;
            l_Scissor.extent.width = p_RenderView.get_dimensions().x;
            l_Scissor.extent.height = p_RenderView.get_dimensions().y;

            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);

            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_lit_image()
                    .get_gpu()
                    .get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          VK_RENDERDOC_SECTION_END();
          return true;
        });

        return true;
      }

      struct
      {
        Pipeline cullingPipeline;
      } g_LightCullingBaseGlobalData;

      static bool initialize_base_light_culling_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_LIGHTCULLING_NAME);

        {
          PipelineUtil::ComputePipelineBuilder l_Builder;
          l_Builder.set_shader("light_culling.comp");
          l_Builder.set_pipeline_layout(
              Global::get_lighting_pipeline_layout());
          g_LightCullingBaseGlobalData.cullingPipeline =
              l_Builder.register_pipeline();
        }

        l_RenderStep.set_execute_callback(
            [&](RenderStep p_RenderStep, float p_Delta,
                RenderView p_RenderView) -> bool {
              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              VK_RENDERDOC_SECTION_BEGIN(
                  "Pointlight culling",
                  SINGLE_ARG({0.0f, 1.0f, 0.8f}));

              ViewInfo l_ViewInfo =
                  p_RenderView.get_view_info_handle();

              vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                g_LightCullingBaseGlobalData
                                    .cullingPipeline.get_pipeline());

              {
                VkDescriptorSet l_Set =
                    Global::get_global_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                    Global::get_lighting_pipeline_layout(), 0, 1,
                    &l_Set, 0, nullptr);

                {
                  VkDescriptorSet l_TextureSet =
                      Global::get_current_texture_descriptor_set();
                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                      Global::get_lighting_pipeline_layout(), 1, 1,
                      &l_TextureSet, 0, nullptr);
                }

                VkDescriptorSet l_DescriptorSet =
                    l_ViewInfo.get_view_data_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                    Global::get_lighting_pipeline_layout(), 2, 1,
                    &l_DescriptorSet, 0, nullptr);
              }

              vkCmdDispatch(l_Cmd, l_ViewInfo.get_light_clusters().x,
                            l_ViewInfo.get_light_clusters().y,
                            l_ViewInfo.get_light_clusters().z);

              VK_RENDERDOC_SECTION_END();
              return true;
            });

        return true;
      }

      struct
      {
        Pipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        AllocatedBuffer kernelBuffer;
        Texture noise;
        Image noiseImage;
      } g_BaseSsaoStepData;

      struct BaseSsaoStepData
      {
        Texture texture;
        Texture tempBlurTexture;
      };

      struct BaseSsaoPushConstants
      {
        Math::Vector2 noiseScale;
        float radius;
        float bias;
        float power;
      };

#define KERNEL_VECTOR Low::Math::Vector4

      bool initialize_base_ssao_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_SSAO_NAME);

        const u8 l_Scale = 2;

        const Math::UVector2 l_NoiseDimensions(4);

        l_RenderStep.set_setup_callback([l_NoiseDimensions](
                                            RenderStep p_RenderStep)
                                            -> bool {
          const u32 l_KernelSize = 64u;

          {
            g_BaseSsaoStepData.kernelBuffer =
                BufferUtil::create_buffer(
                    sizeof(KERNEL_VECTOR) * l_KernelSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY);
          }

          {
            const size_t l_KernelDataSize =
                l_KernelSize * sizeof(KERNEL_VECTOR);

            Low::Util::List<KERNEL_VECTOR> l_SsaoKernel;
            for (int i = 0; i < l_KernelSize; ++i) {
#if 1
              KERNEL_VECTOR i_Sample(
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
                  rand() / float(RAND_MAX), 0.0f);
              i_Sample = glm::normalize(i_Sample);
              i_Sample *= rand() / float(RAND_MAX);
              float i_Scale = float(i) / float(l_KernelSize);
              i_Scale = glm::mix(0.1f, 1.0f, i_Scale * i_Scale);
              i_Sample *= i_Scale;
#else
              KERNEL_VECTOR i_Sample(1.0f, 2.0f, 3.0f, 4.0f);
#endif
              l_SsaoKernel.push_back(i_Sample);
            }

            size_t l_StagingOffset = 0;

            const u64 l_FrameUploadSpace =
                request_resource_staging_buffer_space(
                    l_KernelDataSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(
                l_FrameUploadSpace >= l_KernelDataSize,
                "Did not have enough staging buffer space to upload "
                "SSAO kernel data.");

            LOWR_VK_ASSERT_RETURN(
                resource_staging_buffer_write(l_SsaoKernel.data(),
                                              l_FrameUploadSpace,
                                              l_StagingOffset),
                "Failed to write SSAO kernel data to staging buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;
            // TODO: Change to transfer queue command buffer
            // TODO: probably change staging buffer it should not be
            // using the resource buffer i suppose
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                g_BaseSsaoStepData.kernelBuffer.buffer, 1,
                &l_CopyRegion);
          }

          {
            g_BaseSsaoStepData.noise =
                Texture::make_gpu_ready(N(SsaoKernel));
            Vulkan::Image l_Image =
                Vulkan::Image::make(N(SsaoKernel));
            g_BaseSsaoStepData.noise.get_gpu().set_data_handle(
                l_Image.get_id());

            g_BaseSsaoStepData.noiseImage = l_Image;

            Math::UVector2 l_Dimensions = l_NoiseDimensions;

            size_t l_StagingOffset = 0;
            const u64 l_UploadSpace =
                Vulkan::request_resource_staging_buffer_space(
                    l_Dimensions.x * l_Dimensions.y *
                        IMAGE_CHANNEL_COUNT,
                    &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(
                l_UploadSpace == (l_Dimensions.x * l_Dimensions.y *
                                  IMAGE_CHANNEL_COUNT),
                "Failed to request resource "
                "staging buffer space for "
                "ssao noise texture");

            VkExtent3D l_Extent;
            l_Extent.width = l_Dimensions.x;
            l_Extent.height = l_Dimensions.y;
            l_Extent.depth = 1;

            Vulkan::ImageUtil::create(
                l_Image, l_Extent, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT, false);

            Util::List<Math::Color> l_Pixels;
            for (int i = 0; i < (l_Dimensions.x * l_Dimensions.y);
                 ++i) {
              l_Pixels.push_back(Math::Color(
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f, 0.0f,
                  1.0f));
            }

            LOWR_VK_ASSERT_RETURN(
                Vulkan::resource_staging_buffer_write(
                    l_Pixels.data(), l_UploadSpace, l_StagingOffset),
                "Failed to upload ssao noice texture to resource "
                "staging "
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
          }

          {
            DescriptorUtil::DescriptorLayoutBuilder l_Builder;
            l_Builder.add_binding(0,
                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            l_Builder.add_binding(
                1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            g_BaseSsaoStepData.descriptorSetLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);

            g_BaseSsaoStepData.descriptorSet =
                Global::get_global_descriptor_allocator().allocate(
                    Global::get_device(),
                    g_BaseSsaoStepData.descriptorSetLayout);
          }

          {
            Samplers &l_Samplers = Global::get_samplers();

            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(
                0, g_BaseSsaoStepData.kernelBuffer.buffer,
                sizeof(KERNEL_VECTOR) * l_KernelSize, 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            l_Writer.write_image(
                1,
                g_BaseSsaoStepData.noiseImage.get_allocated_image()
                    .imageView,
                l_Samplers.no_lod_nearest_repeat_black,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            l_Writer.update_set(Global::get_device(),
                                g_BaseSsaoStepData.descriptorSet);
          }

          {
            Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
            l_DescriptorSetLayouts.push_back(
                Global::get_global_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                Global::get_texture_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                Global::get_view_info_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                g_BaseSsaoStepData.descriptorSetLayout);

            VkPipelineLayoutCreateInfo l_Layout{};
            l_Layout.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            l_Layout.pNext = nullptr;
            l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
            l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

            VkPushConstantRange l_PushConstant{};
            l_PushConstant.offset = 0;
            l_PushConstant.size = sizeof(BaseSsaoPushConstants);
            l_PushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            l_Layout.pPushConstantRanges = &l_PushConstant;
            l_Layout.pushConstantRangeCount = 1;

            LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
                Global::get_device(), &l_Layout, nullptr,
                &g_BaseSsaoStepData.pipelineLayout));
          }

          {
            const Util::String l_VertexShaderPath =
                "fullscreen_triangle.vert";
            const Util::String l_FragmentShaderPath =
                "base_ssao.frag";

            PipelineUtil::GraphicsPipelineBuilder l_Builder;
            l_Builder.set_shaders(l_VertexShaderPath,
                                  l_FragmentShaderPath);
            l_Builder.pipelineLayout =
                g_BaseSsaoStepData.pipelineLayout;
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
                VK_FORMAT_R8_UNORM);

            l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

            g_BaseSsaoStepData.pipeline =
                l_Builder.register_pipeline();
          }
          return true;
        });

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              BaseSsaoStepData *l_Data = new BaseSsaoStepData;
              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              l_Data->texture = Texture::make_gpu_ready(N(SsaoOut));
              l_Data->tempBlurTexture =
                  Texture::make_gpu_ready(N(SsaoBlurTemp));
              return true;
            });

        l_RenderStep.set_resolution_update_callback(
            [l_Scale](RenderStep p_RenderStep,
                      Math::UVector2 p_NewDimensions,
                      RenderView p_RenderView) -> bool {
              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              BaseSsaoStepData *l_Data =
                  (BaseSsaoStepData *)GET_STEP_DATA(p_RenderView,
                                                    p_RenderStep);

              {
                Texture l_Texture = l_Data->texture;
                LOWR_VK_ASSERT_RETURN(
                    l_Texture.is_alive(),
                    "Failed to execute basic ssao "
                    "renderstep because ssao "
                    "output texture was not alive.");

                Vulkan::Image l_Image =
                    l_Texture.get_gpu().get_data_handle();

                if (l_Image.is_alive()) {
                  ImGui_ImplVulkan_RemoveTexture(
                      (VkDescriptorSet)l_Texture.get_gpu()
                          .get_imgui_texture_id());

                  ImageUtil::destroy(l_Image);
                  l_Image.destroy();
                }

                l_Image = Vulkan::Image::make(N(SsaoOut));
                l_Texture.get_gpu().set_data_handle(l_Image.get_id());

                VkExtent3D l_Extent;
                l_Extent.width = p_NewDimensions.x / l_Scale;
                l_Extent.height = p_NewDimensions.y / l_Scale;
                l_Extent.depth = 1;

                LOWR_VK_ASSERT_RETURN(
                    Vulkan::ImageUtil::create(
                        l_Image, l_Extent, VK_FORMAT_R8_UNORM,
                        VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        false),
                    "Failed to create ssao out image.");

                ImageUtil::cmd_transition(
                    l_Cmd,
                    l_Data->texture.get_gpu().get_data_handle(),
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
              }
              {
                Texture l_Texture = l_Data->tempBlurTexture;
                LOWR_VK_ASSERT_RETURN(l_Texture.is_alive(),
                                      "Failed to execute basic ssao "
                                      "renderstep because ssao "
                                      "blur texture was not alive.");

                Vulkan::Image l_Image =
                    l_Texture.get_gpu().get_data_handle();

                if (l_Image.is_alive()) {
                  ImGui_ImplVulkan_RemoveTexture(
                      (VkDescriptorSet)l_Texture.get_gpu()
                          .get_imgui_texture_id());

                  ImageUtil::destroy(l_Image);
                  l_Image.destroy();
                }

                if (!l_Image.is_alive()) {
                  l_Data->tempBlurTexture.get_gpu().set_data_handle(
                      Vulkan::Image::make(N(SsaoBlur)));

                  VkExtent3D l_Extent;
                  l_Extent.width = p_NewDimensions.x / l_Scale;
                  l_Extent.height = p_NewDimensions.y / l_Scale;
                  l_Extent.depth = 1;

                  LOWR_VK_ASSERT_RETURN(
                      Vulkan::ImageUtil::create(
                          l_Data->tempBlurTexture.get_gpu()
                              .get_data_handle(),
                          l_Extent, VK_FORMAT_R8_UNORM,
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          false),
                      "Failed to create ssao blur image.");

                  ImageUtil::cmd_transition(
                      l_Cmd,
                      l_Data->tempBlurTexture.get_gpu()
                          .get_data_handle(),
                      VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
              }
              return true;
            });

        l_RenderStep.set_execute_callback([l_Scale,
                                           l_NoiseDimensions](
                                              RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Base SSAO",
                                     SINGLE_ARG({1.0f, 0.0f, 1.0f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          BaseSsaoStepData *l_Data =
              (BaseSsaoStepData *)GET_STEP_DATA(p_RenderView,
                                                p_RenderStep);

          Texture l_Texture = l_Data->texture;
          LOWR_VK_ASSERT_RETURN(
              l_Texture.is_alive(),
              "Failed to execute basic ssao renderstep "
              "because ssao "
              "output texture was not alive.");

          Math::UVector2 l_Dimensions = p_RenderView.get_dimensions();

          Vulkan::Image l_Image =
              l_Texture.get_gpu().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Vulkan::Image::make(N(SsaoOut));
            l_Texture.get_gpu().set_data_handle(l_Image.get_id());

            VkExtent3D l_Extent;
            l_Extent.width = l_Dimensions.x / l_Scale;
            l_Extent.height = l_Dimensions.y / l_Scale;
            l_Extent.depth = 1;

            LOWR_VK_ASSERT_RETURN(
                Vulkan::ImageUtil::create(
                    l_Image, l_Extent, VK_FORMAT_R8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    false),
                "Failed to create ssao out image.");

            ImageUtil::cmd_transition(
                l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          {
            Image l_TempImage =
                l_Data->tempBlurTexture.get_gpu().get_data_handle();
            if (!l_TempImage.is_alive()) {
              l_Data->tempBlurTexture.get_gpu().set_data_handle(
                  Vulkan::Image::make(N(SsaoBlur)));

              VkExtent3D l_Extent;
              l_Extent.width = l_Dimensions.x / l_Scale;
              l_Extent.height = l_Dimensions.y / l_Scale;
              l_Extent.depth = 1;

              LOWR_VK_ASSERT_RETURN(
                  Vulkan::ImageUtil::create(
                      l_Data->tempBlurTexture.get_gpu()
                          .get_data_handle(),
                      l_Extent, VK_FORMAT_R8_UNORM,
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      false),
                  "Failed to create ssao blur image.");

              ImageUtil::cmd_transition(
                  l_Cmd,
                  l_Data->tempBlurTexture.get_gpu().get_data_handle(),
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
          }

          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{1.0f, 1.0f, 1.0f, 1.0f}};

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_Image.get_allocated_image().imageView,
              &l_ClearColorValue,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x / l_Scale,
               p_RenderView.get_dimensions().y / l_Scale},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(
              l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
              g_BaseSsaoStepData.pipeline.get_pipeline());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(l_Cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    g_BaseSsaoStepData.pipelineLayout,
                                    0, 1, &l_Set, 0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_BaseSsaoStepData.pipelineLayout, 1, 1,
                  &l_TextureSet, 0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                l_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BaseSsaoStepData.pipelineLayout, 2, 1,
                &l_DescriptorSet, 0, nullptr);
          }
          {
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BaseSsaoStepData.pipelineLayout, 3, 1,
                &g_BaseSsaoStepData.descriptorSet, 0, nullptr);
          }

          BaseSsaoPushConstants l_PushConstants;

          Math::UVector2 d = p_RenderView.get_dimensions();
          Math::UVector2 nd = l_NoiseDimensions;

          l_PushConstants.noiseScale.x =
              1.0f / (float(p_RenderView.get_dimensions().x) /
                      float(l_NoiseDimensions.x));
          l_PushConstants.noiseScale.y =
              1.0f / (float(p_RenderView.get_dimensions().y) /
                      float(l_NoiseDimensions.y));
          l_PushConstants.radius = 0.5f;
          l_PushConstants.bias = 0.025f;
          l_PushConstants.power = 1.0f;

          vkCmdPushConstants(l_Cmd, g_BaseSsaoStepData.pipelineLayout,
                             VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                             sizeof(BaseSsaoPushConstants),
                             &l_PushConstants);

          VkViewport l_Viewport = {};
          l_Viewport.x = 0;
          l_Viewport.y = 0;
          l_Viewport.width = static_cast<float>(
              p_RenderView.get_dimensions().x / l_Scale);
          l_Viewport.height = static_cast<float>(
              p_RenderView.get_dimensions().y / l_Scale);
          l_Viewport.minDepth = 0.f;
          l_Viewport.maxDepth = 1.f;

          vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

          VkRect2D l_Scissor = {};
          l_Scissor.offset.x = 0;
          l_Scissor.offset.y = 0;
          l_Scissor.extent.width =
              p_RenderView.get_dimensions().x / l_Scale;
          l_Scissor.extent.height =
              p_RenderView.get_dimensions().y / l_Scale;

          vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

          vkCmdDraw(l_Cmd, 3, 1, 0, 0);

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_BEGIN("Blurring SSAO",
                                     SINGLE_ARG({1.0f, 0.0f, 0.0f}));

          Global::blur_image_1(
              l_Data->texture, l_Data->tempBlurTexture,
              l_Data->texture,
              {p_RenderView.get_dimensions().x / l_Scale,
               p_RenderView.get_dimensions().y / l_Scale});

          VK_RENDERDOC_SECTION_END();
          VK_RENDERDOC_SECTION_END();

          return true;
        });

        return true;
      }

      bool initialize_cavities_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_CAVITIES_NAME);

        l_RenderStep.set_setup_callback(
            [&](RenderStep p_RenderStep) -> bool { return true; });
        return true;
      }

      bool initialize_ui_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_UI_NAME);

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Draw UI",
                                     SINGLE_ARG({0.1f, 0.5f, 0.1f}));

          struct Entry
          {
            MaterialType materialType;
            GpuSubmesh submesh;
            u32 amount;
          };

          Util::List<Entry> l_Entries;

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          eastl::sort(
              p_RenderView.get_ui_canvases().begin(),
              p_RenderView.get_ui_canvases().end(),
              [](UiCanvas p_Canvas0, UiCanvas p_Canvas1) -> bool {
                return p_Canvas0.get_z_sorting() <
                       p_Canvas1.get_z_sorting();
              });

          Util::List<UiDrawCommandUpload> l_DrawCommandUploads;

          for (auto it = p_RenderView.get_ui_canvases().begin();
               it != p_RenderView.get_ui_canvases().end(); ++it) {
            UiCanvas i_Canvas = it->get_id();
            if (i_Canvas.is_z_dirty()) {
              eastl::sort(i_Canvas.get_draw_commands().begin(),
                          i_Canvas.get_draw_commands().end(),
                          [](UiDrawCommand p_DC0,
                             UiDrawCommand p_DC1) -> bool {
                            return p_DC0.get_z_sorting() <
                                   p_DC1.get_z_sorting();
                          });

              i_Canvas.set_z_dirty(false);
            }

            for (auto dit = i_Canvas.get_draw_commands().begin();
                 dit != i_Canvas.get_draw_commands().end();) {
              UiDrawCommand i_DrawCommand = dit->get_id();

              if (!i_DrawCommand.is_alive()) {
                dit = i_Canvas.get_draw_commands().erase(dit);
                continue;
              }

              if (l_Entries.empty()) {
                l_Entries.push_back(
                    {i_DrawCommand.get_material().get_material_type(),
                     i_DrawCommand.get_submesh(), 1});
              } else {
                if (l_Entries.end()->materialType ==
                        i_DrawCommand.get_material()
                            .get_material_type() &&
                    l_Entries.end()->submesh ==
                        i_DrawCommand.get_submesh()) {
                  l_Entries.end()->amount++;
                } else {
                  l_Entries.push_back({i_DrawCommand.get_material()
                                           .get_material_type(),
                                       i_DrawCommand.get_submesh(),
                                       1});
                }
              }

              UiDrawCommandUpload i_Upload;
              i_Upload.size = i_DrawCommand.get_size();
              i_Upload.position = i_DrawCommand.get_position();
              i_Upload.rotation2D =
                  glm::radians(-i_DrawCommand.get_rotation2D());
              i_Upload.textureIndex = 0;
              if (i_DrawCommand.get_texture().is_alive() &&
                  i_DrawCommand.get_texture().get_state() ==
                      TextureState::LOADED) {
                i_Upload.textureIndex =
                    i_DrawCommand.get_texture().get_gpu().get_index();
              }
              i_Upload.materialIndex = 0;
              if (i_DrawCommand.get_material().is_alive()) {
                i_Upload.materialIndex =
                    i_DrawCommand.get_material().get_index();
              }

              i_Upload.uvRect = i_DrawCommand.get_uv_rect();

              l_DrawCommandUploads.push_back(i_Upload);

              ++dit;
            }
          }

          // Upload the UI draw command data
          {
            size_t l_StagingOffset = 0;

            const u64 l_DrawCommandSize =
                sizeof(UiDrawCommandUpload) *
                l_DrawCommandUploads.size();

            if (l_DrawCommandSize == 0) {
              return true;
            }

            // TODO: This does not go on the resource staging
            // buffer but a frame staging buffer of some sort
            const u64 l_FrameUploadSpace =
                request_resource_staging_buffer_space(
                    l_DrawCommandSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >=
                                      l_DrawCommandSize,
                                  "Did not have enough staging "
                                  "buffer space to upload "
                                  "UI draw commands.");

            LOWR_VK_ASSERT_RETURN(resource_staging_buffer_write(
                                      l_DrawCommandUploads.data(),
                                      l_FrameUploadSpace,
                                      l_StagingOffset),
                                  "Failed to write ui draw command "
                                  "data to staging buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;
            // This probably has to be done on the graphics
            // queue so we can leave it as is
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                l_ViewInfo.get_ui_drawcommand_buffer().buffer, 1,
                &l_CopyRegion);
          }

          // Render UI
          if (!l_Entries.empty()) {
            GpuSubmesh l_CurrentGpuSubmesh = Util::Handle::DEAD;
            MaterialType l_CurrentMaterialType = Util::Handle::DEAD;

            u32 l_Offset = 0u;

            Image l_LitImage = p_RenderView.get_lit_image()
                                   .get_gpu()
                                   .get_data_handle();

            ImageUtil::cmd_transition(
                l_Cmd, l_LitImage,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
            l_ColorAttachments.resize(1);
            l_ColorAttachments[0] = InitUtil::attachment_info(
                l_LitImage.get_allocated_image().imageView, nullptr,
                VK_IMAGE_LAYOUT_GENERAL);

            VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
                {p_RenderView.get_dimensions().x,
                 p_RenderView.get_dimensions().y},
                l_ColorAttachments.data(), l_ColorAttachments.size(),
                nullptr);

            vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

            vkCmdBindIndexBuffer(
                l_Cmd,
                Global::get_mesh_index_buffer().m_Buffer.buffer, 0,
                VK_INDEX_TYPE_UINT32);

            for (auto it = l_Entries.begin(); it != l_Entries.end();
                 ++it) {

              if (it->materialType != l_CurrentMaterialType) {
                l_CurrentMaterialType = it->materialType;
                Pipeline i_Pipeline =
                    l_CurrentMaterialType.get_draw_pipeline_handle();

                // Switch pipeline
                vkCmdBindPipeline(l_Cmd,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  i_Pipeline.get_pipeline());

                // Bind descriptor sets
                {
                  VkDescriptorSet l_Set =
                      Global::get_global_descriptor_set();

                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout(), 0, 1, &l_Set, 0,
                      nullptr);

                  {
                    VkDescriptorSet l_TextureSet =
                        Global::get_current_texture_descriptor_set();
                    vkCmdBindDescriptorSets(
                        l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        i_Pipeline.get_layout(), 1, 1, &l_TextureSet,
                        0, nullptr);
                  }

                  VkDescriptorSet l_DescriptorSet =
                      l_ViewInfo.get_view_data_descriptor_set();

                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout(), 2, 1, &l_DescriptorSet,
                      0, nullptr);
                }
              }

              if (it->submesh != l_CurrentGpuSubmesh) {
                l_CurrentGpuSubmesh = it->submesh;
                // TODO: change offset?
              }

              vkCmdDrawIndexed(
                  l_Cmd, it->submesh.get_index_count(), it->amount,
                  it->submesh.get_index_start(),
                  it->submesh.get_vertex_start(), l_Offset);

              l_Offset += it->amount;
            }

            vkCmdEndRendering(l_Cmd);

            ImageUtil::cmd_transition(
                l_Cmd, l_LitImage,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          VK_RENDERDOC_SECTION_END();

          return true;
        });

        return true;
      }

      static MaterialType get_material_type(DebugGeometryDraw &p_Draw)
      {
        if (p_Draw.depthTest) {
          if (p_Draw.wireframe) {
            return get_material_types().debugGeometryWireframe;
          }
          return get_material_types().debugGeometry;
        } else {
          if (p_Draw.wireframe) {
            return get_material_types().debugGeometryNoDepthWireframe;
          }
          return get_material_types().debugGeometryNoDepth;
        }
      }

      bool initialize_debug_geometry_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_DEBUG_GEOMETRY_NAME);

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VK_RENDERDOC_SECTION_BEGIN(
              "Draw debug geometry",
              SINGLE_ARG({0.4f, 0.227f, 0.717f}));

          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          // TODO: Sort debug geometry entries

          Util::List<DebugGeometryUpload> l_Uploads;
          for (auto it = p_RenderView.get_debug_geometry().begin();
               it != p_RenderView.get_debug_geometry().end(); ++it) {
            l_Uploads.emplace_back(it->transform, it->color);
          }

          {
            size_t l_StagingOffset = 0;

            const u64 l_DrawCommandSize =
                sizeof(DebugGeometryUpload) * l_Uploads.size();

            if (l_DrawCommandSize == 0) {
              return true;
            }

            // TODO: This does not go on the resource staging
            // buffer but a frame staging buffer of some sort
            const u64 l_FrameUploadSpace =
                request_resource_staging_buffer_space(
                    l_DrawCommandSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >=
                                      l_DrawCommandSize,
                                  "Did not have enough staging "
                                  "buffer space to upload "
                                  "debug geometry draw commands.");

            LOWR_VK_ASSERT_RETURN(
                resource_staging_buffer_write(l_Uploads.data(),
                                              l_FrameUploadSpace,
                                              l_StagingOffset),
                "Failed to write debug geometry draw command "
                "data to staging buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;
            // This probably has to be done on the graphics
            // queue so we can leave it as is
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                l_ViewInfo.get_debug_geometry_buffer().buffer, 1,
                &l_CopyRegion);
          }

          Image l_LitImage = p_RenderView.get_lit_image()
                                 .get_gpu()
                                 .get_data_handle();

          Image l_DepthImage = p_RenderView.get_gbuffer_depth()
                                   .get_gpu()
                                   .get_data_handle();

          ImageUtil::cmd_transition(
              l_Cmd, l_LitImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
          ImageUtil::cmd_transition(
              l_Cmd, l_DepthImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_LitImage.get_allocated_image().imageView, nullptr,
              VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingAttachmentInfo l_DepthAttachment =
              InitUtil::attachment_info(
                  l_DepthImage.get_allocated_image().imageView,
                  nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x,
               p_RenderView.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              &l_DepthAttachment);

          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindIndexBuffer(
              l_Cmd, Global::get_mesh_index_buffer().m_Buffer.buffer,
              0, VK_INDEX_TYPE_UINT32);

          MaterialType l_CurrentMaterialType = Util::Handle::DEAD;

          for (u32 i = 0u;
               i < p_RenderView.get_debug_geometry().size(); ++i) {
            DebugGeometryDraw &i_Draw =
                p_RenderView.get_debug_geometry()[i];

            if (i_Draw.submesh.get_state() != MeshState::LOADED) {
              continue;
            }

            MaterialType i_MaterialType = get_material_type(i_Draw);

            if (!i_MaterialType.is_alive()) {
              continue;
            }

            Pipeline i_Pipeline =
                i_MaterialType.get_draw_pipeline_handle();

            if (l_CurrentMaterialType != i_MaterialType) {
              l_CurrentMaterialType = i_MaterialType;

              // Switch pipeline
              vkCmdBindPipeline(l_Cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                i_Pipeline.get_pipeline());

              // Bind descriptor sets
              {
                VkDescriptorSet l_Set =
                    Global::get_global_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout(), 0, 1, &l_Set, 0,
                    nullptr);

                {
                  VkDescriptorSet l_TextureSet =
                      Global::get_current_texture_descriptor_set();
                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout(), 1, 1, &l_TextureSet, 0,
                      nullptr);
                }

                VkDescriptorSet l_DescriptorSet =
                    l_ViewInfo.get_view_data_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout(), 2, 1, &l_DescriptorSet,
                    0, nullptr);
              }
            }

            // TODO: Change to custom Debug geometry push constant
            RenderEntryPushConstant i_PushConstants;
            i_PushConstants.renderObjectSlot = i;

            vkCmdPushConstants(l_Cmd, i_Pipeline.get_layout(),
                               VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                               sizeof(RenderEntryPushConstant),
                               &i_PushConstants);

            vkCmdDrawIndexed(l_Cmd, i_Draw.submesh.get_index_count(),
                             1, i_Draw.submesh.get_index_start(),
                             i_Draw.submesh.get_vertex_start(), 0);
          }

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd, l_LitImage,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          ImageUtil::cmd_transition(
              l_Cmd, l_DepthImage,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          p_RenderView.get_debug_geometry().clear();

          VK_RENDERDOC_SECTION_END();

          return true;
        });
        return true;
      }

      bool initialize_basic_rendersteps()
      {
        LOWR_VK_ASSERT_RETURN(
            initialize_solid_material_renderstep(),
            "Failed to initialize solid material renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_base_lighting_renderstep(),
            "Failed to initialize base lighting renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_base_light_culling_renderstep(),
            "Failed to initialize base light culling renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_cavities_renderstep(),
            "Failed to initialize cavities renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_base_ssao_renderstep(),
            "Failed to initialize base ssao renderstep");
        LOWR_VK_ASSERT_RETURN(initialize_ui_renderstep(),
                              "Failed to initialize UI renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_debug_geometry_renderstep(),
            "Failed to initialize debug geometry renderstep");
        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
