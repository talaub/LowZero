#include "LowRendererVulkanRenderStepsBasic.h"

#include "LowRendererRenderStep.h"
#include "LowRendererRenderView.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "LowUtil.h"

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

namespace Low {
  namespace Renderer {
    namespace Vulkan {

      struct RenderEntryPushConstant
      {
        u32 renderObjectSlot;
      };

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

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          VkClearValue l_ClearColorValueDepth = {};
          l_ClearColorValueDepth.depthStencil.depth =
              1.0f; // Depth clear value (1.0f is the farthest)
          l_ClearColorValueDepth.depthStencil.stencil =
              0; // Stencil clear value

          Image l_AlbedoImage =
              p_RenderView.get_gbuffer_albedo().get_data_handle();
          Image l_NormalsImage =
              p_RenderView.get_gbuffer_normals().get_data_handle();
          Image l_DepthImage =
              p_RenderView.get_gbuffer_depth().get_data_handle();

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
                p_RenderView.get_gbuffer_albedo().get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_normals().get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_depth().get_data_handle(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
          }

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(2);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_AlbedoImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);
          l_ColorAttachments[1] = InitUtil::attachment_info(
              l_NormalsImage.get_allocated_image().imageView,
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

            vkCmdDrawIndexed(l_Cmd, i_MeshInfo.get_index_count(), 1,
                             i_MeshInfo.get_index_start(), 0, 0);

            it++;
          }

          vkCmdEndRendering(l_Cmd);

          {
            // Transfer the gbuffer images
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_albedo().get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_normals().get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_gbuffer_depth().get_data_handle(),
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

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

          BaseLightingStepData *l_Data =
              (BaseLightingStepData *)p_RenderView
                  .get_step_data()[p_RenderStep.get_index()];

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          ImageUtil::cmd_transition(
              l_Cmd, p_RenderView.get_lit_image().get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          Image l_LitImage =
              p_RenderView.get_lit_image().get_data_handle();

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
            l_Scissor.extent.height = p_RenderView.get_dimensions().x;

            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);

            ImageUtil::cmd_transition(
                l_Cmd, p_RenderView.get_lit_image().get_data_handle(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
          }
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
        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low
