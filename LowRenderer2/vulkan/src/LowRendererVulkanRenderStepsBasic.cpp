#include "LowRendererVulkanRenderStepsBasic.h"

#include "LowMath.h"
#include "LowRendererRenderStep.h"
#include "LowRendererRenderView.h"
#include "LowRendererTextureState.h"
#include "LowRendererUiCanvas.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilHashing.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
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
#include "LowRendererVulkanBase.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanBuffer.h"
#include "LowRendererPointLight.h"

#include <EASTL/sort.h>

#define GET_STEP_DATA(renderview, renderstep)                        \
  renderview.get_step_data()[renderstep.get_index()]

namespace Low {
  namespace Renderer {
    namespace Vulkan {
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

      static bool draw_highlightmap_solid(VkCommandBuffer p_Cmd,
                                          RenderView p_RenderView,
                                          RenderStep p_RenderStep,
                                          float p_Delta,
                                          ViewInfo p_ViewInfo)
      {

        MaterialType l_CurrentMaterialType = Low::Util::Handle::DEAD;

        for (const HighlightDrawSolid &i_Draw :
             p_RenderView.get_highlight_draws_solid()) {
          if (!i_Draw.drawCommand.is_alive()) {
            continue;
          }
          if (!i_Draw.drawCommand.has_any_render_object()) {
            continue;
          }

          if (i_Draw.drawCommand.get_any_render_object_mesh()
                  .get_state() != MeshState::LOADED) {
            continue;
          }

          GpuSubmesh i_GpuSubmesh = i_Draw.drawCommand.get_submesh();

          LOW_ASSERT(i_GpuSubmesh.is_alive(),
                     "Submesh of mesh entry not alive anymore. "
                     "This should not happen if the corresponding "
                     "RenderObject is still alive.");
          LOW_ASSERT(i_GpuSubmesh.get_state() == MeshState::LOADED,
                     "Submesh is not loaded to GPU. Cannot render.");

          Material i_Material = i_Draw.drawCommand.get_material();
          if (!i_Material.is_alive()) {
            i_Material =
                i_Draw.drawCommand.get_any_render_object_material();
          }

          LOW_ASSERT(i_Material.is_alive(),
                     "Material of draw command is not alive "
                     "anymore. This should not happen.");

          if (!i_Material.get_material_type().is_alive()) {
            continue;
          }

          if (i_Material.get_material_type() !=
              l_CurrentMaterialType) {
            l_CurrentMaterialType = i_Material.get_material_type();

            Pipeline i_Pipeline =
                l_CurrentMaterialType.get_highlight_pipeline_handle();

            vkCmdBindPipeline(p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              i_Pipeline.get());

            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(p_Cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    i_Pipeline.get_layout().get(), 0,
                                    1, &l_Set, 0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  i_Pipeline.get_layout().get(), 1, 1, &l_TextureSet,
                  0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                p_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(p_Cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    i_Pipeline.get_layout().get(), 2,
                                    1, &l_DescriptorSet, 0, nullptr);
          }

          Pipeline i_Pipeline =
              l_CurrentMaterialType.get_highlight_pipeline_handle();

          RenderEntryHighlightPushConstant i_PushConstants{};
          i_PushConstants.renderObjectSlot =
              i_Draw.drawCommand.get_slot();
          i_PushConstants.highlightType = (u32)i_Draw.highlightType;
          i_PushConstants.activeVertexBuffer =
              (u32)i_Draw.drawCommand.get_active_vertex_buffer();

          vkCmdPushConstants(p_Cmd, i_Pipeline.get_layout().get(),
                             VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                             sizeof(RenderEntryHighlightPushConstant),
                             &i_PushConstants);

          vkCmdDrawIndexed(
              p_Cmd, i_GpuSubmesh.get_index_count(), 1,
              i_GpuSubmesh.get_index_start(),
              static_cast<int32_t>(
                  i_Draw.drawCommand.get_active_vertex_offset()),
              0);
        }

        return true;
      }

      static bool initialize_highlightmap_draw_renderstep()
      {

        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_HIGHLIGHTMAP_DRAW);

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Highlightmap draw",
                                     SINGLE_ARG({0.1f, 0.8f, 0.3f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          VkClearValue l_ClearObjectValue = {};
          l_ClearObjectValue.color.uint32[0] = LOW_UINT32_MAX;

          VkClearValue l_ClearColorValueDepth = {};
          l_ClearColorValueDepth.depthStencil.depth =
              1.0f; // Depth clear value (1.0f is the farthest)
          l_ClearColorValueDepth.depthStencil.stencil =
              0; // Stencil clear value

          Image l_DepthImage = p_RenderView.get_gbuffer_depth()
                                   .get_gpu()
                                   .get_data_handle();
          Image l_HighlightMapImage = p_RenderView.get_highlight_map()
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
                p_RenderView.get_highlight_map()
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
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_HighlightMapImage.get_allocated_image().imageView,
              &l_ClearObjectValue, VK_IMAGE_LAYOUT_GENERAL);

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

          LOW_ASSERT_ERROR_RETURN_FALSE(
              draw_highlightmap_solid(l_Cmd, p_RenderView,
                                      p_RenderStep, p_Delta,
                                      l_ViewInfo),
              "Faled to draw solids to highlight.");

          vkCmdEndRendering(l_Cmd);

          {
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_highlight_map()
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

      struct HighlightEdgeStepData
      {
        Pipeline pipeline;
      };

      static bool initialize_highlight_edge_draw_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_HIGHLIGHT_EDGE_DRAW);

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              HighlightEdgeStepData *l_Data =
                  new HighlightEdgeStepData;

              l_Data->pipeline =
                  PipelineUtil::GraphicsPipelineBuilder::
                      prepare_fullscreen_effect(
                          Global::get_blur_pipeline_layout(),
                          "highlight_edge.frag",
                          VK_FORMAT_R8G8B8A8_UNORM)
                          .register_pipeline();

              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              return true;
            });

        l_RenderStep.set_teardown_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              // FIX: Delete data
              return true;
            });

        l_RenderStep.set_execute_callback([](RenderStep p_RenderStep,
                                             float p_Delta,
                                             RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Highlight edge",
                                     SINGLE_ARG({1.0f, 0.8f, 0.0f}));

          HighlightEdgeStepData *l_Data =
              (HighlightEdgeStepData *)p_RenderView
                  .get_step_data()[p_RenderStep.get_index()];

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          Image l_OutImage = p_RenderView.get_tonemapped_image()
                                 .get_gpu()
                                 .get_data_handle();

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_OutImage.get_allocated_image().imageView, nullptr,
              VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x,
               p_RenderView.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            l_Data->pipeline.get());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_blur_pipeline_layout().get(), 0, 1,
                &l_Set, 0, nullptr);
          }
          {
            VkDescriptorSet l_TextureSet =
                Global::get_current_texture_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_blur_pipeline_layout().get(), 1, 1,
                &l_TextureSet, 0, nullptr);
          }

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

          struct
          {
            Math::Vector2 texelSize;
            u32 highlightMapIndex;
          } l_PushConstants;

          l_PushConstants.texelSize = {
              1.0f / p_RenderView.get_dimensions().x,
              1.0f / p_RenderView.get_dimensions().y};
          l_PushConstants.highlightMapIndex =
              p_RenderView.get_highlight_map()
                  .get_gpu()
                  .get_bindless_index();

          vkCmdPushConstants(
              l_Cmd, Global::get_blur_pipeline_layout().get(),
              VK_SHADER_STAGE_FRAGMENT_BIT, 0,
              sizeof(l_PushConstants), &l_PushConstants);

          vkCmdDraw(l_Cmd, 3, 1, 0, 0);

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();
          return true;
        });

        return true;
      }

      static bool draw_pickmap_debuggeometry(VkCommandBuffer p_Cmd,
                                             RenderView p_RenderView,
                                             RenderStep p_RenderStep,
                                             float p_Delta,
                                             ViewInfo p_ViewInfo)
      {
        MaterialType l_CurrentMaterialType = Low::Util::Handle::DEAD;

        for (u32 i = 0u; i < p_RenderView.get_debug_geometry().size();
             ++i) {
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
              i_MaterialType.get_pick_pipeline_handle();

          if (l_CurrentMaterialType != i_MaterialType) {
            l_CurrentMaterialType = i_MaterialType;

            // Switch pipeline
            vkCmdBindPipeline(p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              i_Pipeline.get());

            // Bind descriptor sets
            {
              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();

              vkCmdBindDescriptorSets(p_Cmd,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      i_Pipeline.get_layout().get(),
                                      0, 1, &l_Set, 0, nullptr);

              {
                VkDescriptorSet l_TextureSet =
                    Global::get_current_texture_descriptor_set();
                vkCmdBindDescriptorSets(
                    p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout().get(), 1, 1,
                    &l_TextureSet, 0, nullptr);
              }

              VkDescriptorSet l_DescriptorSet =
                  p_ViewInfo.get_view_data_descriptor_set();

              vkCmdBindDescriptorSets(
                  p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  i_Pipeline.get_layout().get(), 2, 1,
                  &l_DescriptorSet, 0, nullptr);
            }
          }

          // TODO: Change to custom Debug geometry push constant
          RenderEntryPushConstant i_PushConstants{};
          i_PushConstants.renderObjectSlot = i;
          i_PushConstants.activeVertexBuffer =
              (u32)VertexBuffer::Static;

          vkCmdPushConstants(p_Cmd, i_Pipeline.get_layout().get(),
                             VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                             sizeof(RenderEntryPushConstant),
                             &i_PushConstants);

          vkCmdDrawIndexed(p_Cmd, i_Draw.submesh.get_index_count(), 1,
                           i_Draw.submesh.get_index_start(),
                           i_Draw.submesh.get_vertex_start(), 0);
        }
        return true;
      }

      static bool draw_pickmap_solid(VkCommandBuffer p_Cmd,
                                     RenderView p_RenderView,
                                     RenderStep p_RenderStep,
                                     float p_Delta,
                                     ViewInfo p_ViewInfo)
      {
        MaterialType l_CurrentMaterialType = Low::Util::Handle::DEAD;

        for (auto it = p_RenderView.get_render_scene()
                           .get_draw_commands()
                           .begin();
             it != p_RenderView.get_render_scene()
                       .get_draw_commands()
                       .end();) {
          if (!it->is_alive()) {
            it = p_RenderView.get_render_scene()
                     .get_draw_commands()
                     .erase(it);
            continue;
          }
          if (!it->has_any_render_object()) {
            it = p_RenderView.get_render_scene()
                     .get_draw_commands()
                     .erase(it);
            continue;
          }

          if (it->get_any_render_object_mesh().get_state() !=
              MeshState::LOADED) {
            it++;
            continue;
          }

          // Mesh was refreshed since last time we rendered this
          if (it->is_any_render_object_uploaded() &&
              it->get_any_render_object_mesh().get_gpu().get_id() !=
                  it->get_any_render_object_last_uploaded_mesh_gpu_id()) {
            it->mark_any_render_object_dirty();
            it++;
            continue;
          }

          GpuSubmesh i_GpuSubmesh = it->get_submesh();

          LOW_ASSERT(i_GpuSubmesh.is_alive(),
                     "Submesh of mesh entry not alive anymore. "
                     "This should not happen if the corresponding "
                     "RenderObject is still alive.");
          LOW_ASSERT(i_GpuSubmesh.get_state() == MeshState::LOADED,
                     "Submesh is not loaded to GPU. Cannot render.");

          Material i_Material = it->get_material();
          if (!i_Material.is_alive()) {
            i_Material = it->get_any_render_object_material();
          }

          LOW_ASSERT(i_Material.is_alive(),
                     "Material of draw command is not alive "
                     "anymore. This should not happen.");

          if (i_Material.get_state() != MaterialState::LOADED) {
            i_Material = get_default_material_texture();
          }

          if (i_Material.get_material_type() !=
              l_CurrentMaterialType) {
            l_CurrentMaterialType = i_Material.get_material_type();

            Pipeline i_Pipeline =
                l_CurrentMaterialType.get_pick_pipeline_handle();

            vkCmdBindPipeline(p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              i_Pipeline.get());

            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(p_Cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    i_Pipeline.get_layout().get(), 0,
                                    1, &l_Set, 0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  p_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  i_Pipeline.get_layout().get(), 1, 1, &l_TextureSet,
                  0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                p_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(p_Cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    i_Pipeline.get_layout().get(), 2,
                                    1, &l_DescriptorSet, 0, nullptr);
          }

          Pipeline i_Pipeline =
              l_CurrentMaterialType.get_pick_pipeline_handle();

          RenderEntryPushConstant i_PushConstants{};
          i_PushConstants.renderObjectSlot = it->get_slot();
          i_PushConstants.activeVertexBuffer =
              (u32)it->get_active_vertex_buffer();

          vkCmdPushConstants(p_Cmd, i_Pipeline.get_layout().get(),
                             VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                             sizeof(RenderEntryPushConstant),
                             &i_PushConstants);

          vkCmdDrawIndexed(
              p_Cmd, i_GpuSubmesh.get_index_count(), 1,
              i_GpuSubmesh.get_index_start(),
              static_cast<int32_t>(it->get_active_vertex_offset()),
              0);

          it++;
        }
        return true;
      }

      static bool initialize_pickingmap_draw_renderstep()
      {

        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_PICKINGMAP_DRAW);

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Pickingmap draw",
                                     SINGLE_ARG({0.2f, 0.5f, 0.7f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          VkClearValue l_ClearObjectValue = {};
          l_ClearObjectValue.color.uint32[0] = LOW_UINT32_MAX;

          VkClearValue l_ClearColorValueDepth = {};
          l_ClearColorValueDepth.depthStencil.depth =
              1.0f; // Depth clear value (1.0f is the farthest)
          l_ClearColorValueDepth.depthStencil.stencil =
              0; // Stencil clear value

          Image l_DepthImage = p_RenderView.get_gbuffer_depth()
                                   .get_gpu()
                                   .get_data_handle();
          Image l_ObjectMapImage = p_RenderView.get_object_map()
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
                p_RenderView.get_object_map()
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
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_ObjectMapImage.get_allocated_image().imageView,
              &l_ClearObjectValue, VK_IMAGE_LAYOUT_GENERAL);

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

          LOW_ASSERT_ERROR_RETURN_FALSE(
              draw_pickmap_solid(l_Cmd, p_RenderView, p_RenderStep,
                                 p_Delta, l_ViewInfo),
              "Faled to draw solids to pickmap.");
          LOW_ASSERT_ERROR_RETURN_FALSE(
              draw_pickmap_debuggeometry(l_Cmd, p_RenderView,
                                         p_RenderStep, p_Delta,
                                         l_ViewInfo),
              "Faled to draw debuggeometry to pickmap.");

          vkCmdEndRendering(l_Cmd);

          {
            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_object_map()
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

          VkClearValue l_ClearObjectValue = {};
          l_ClearObjectValue.color.uint32[0] = LOW_UINT32_MAX;

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
          Image l_ObjectMapImage = p_RenderView.get_object_map()
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
                p_RenderView.get_object_map()
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
          /*
          l_ColorAttachments[3] = InitUtil::attachment_info(
              l_ObjectMapImage.get_allocated_image().imageView,
              &l_ClearObjectValue, VK_IMAGE_LAYOUT_GENERAL);
              */

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
            if (!it->is_alive()) {
              it = p_RenderView.get_render_scene()
                       .get_draw_commands()
                       .erase(it);
              continue;
            }
            if (!it->has_any_render_object()) {
              it = p_RenderView.get_render_scene()
                       .get_draw_commands()
                       .erase(it);
              continue;
            }

            if (it->get_any_render_object_mesh().get_state() !=
                MeshState::LOADED) {
              it++;
              continue;
            }

            // Mesh was refreshed since last time we rendered this
            if (it->is_any_render_object_uploaded() &&
                it->get_any_render_object_mesh().get_gpu().get_id() !=
                    it->get_any_render_object_last_uploaded_mesh_gpu_id()) {
              it->mark_any_render_object_dirty();
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
              i_Material = it->get_any_render_object_material();
            }

            LOW_ASSERT(i_Material.is_alive(),
                       "Material of draw command is not alive "
                       "anymore. This should not happen.");

            if (i_Material.get_state() != MaterialState::LOADED) {
              i_Material = get_default_material_texture();
            }

            if (i_Material.get_material_type() !=
                l_CurrentMaterialType) {
              l_CurrentMaterialType = i_Material.get_material_type();

              Pipeline i_Pipeline =
                  l_CurrentMaterialType.get_draw_pipeline_handle();

              vkCmdBindPipeline(l_Cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                i_Pipeline.get());

              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();

              vkCmdBindDescriptorSets(l_Cmd,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      i_Pipeline.get_layout().get(),
                                      0, 1, &l_Set, 0, nullptr);

              {
                VkDescriptorSet l_TextureSet =
                    Global::get_current_texture_descriptor_set();
                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout().get(), 1, 1,
                    &l_TextureSet, 0, nullptr);
              }

              VkDescriptorSet l_DescriptorSet =
                  l_ViewInfo.get_view_data_descriptor_set();

              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  i_Pipeline.get_layout().get(), 2, 1,
                  &l_DescriptorSet, 0, nullptr);
            }

            Pipeline i_Pipeline =
                l_CurrentMaterialType.get_draw_pipeline_handle();

            RenderEntryPushConstant i_PushConstants{};
            i_PushConstants.renderObjectSlot = it->get_slot();
            i_PushConstants.activeVertexBuffer =
                (u32)it->get_active_vertex_buffer();

            vkCmdPushConstants(l_Cmd, i_Pipeline.get_layout().get(),
                               VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                               sizeof(RenderEntryPushConstant),
                               &i_PushConstants);

            vkCmdDrawIndexed(
                l_Cmd, i_GpuSubmesh.get_index_count(), 1,
                i_GpuSubmesh.get_index_start(),
                static_cast<int32_t>(it->get_active_vertex_offset()),
                0);

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
                p_RenderView.get_object_map()
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
              // FIX: Delete data
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
              p_RenderView.get_tonemapped_image()
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
                            l_Data->pipeline.get());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout().get(), 0, 1,
                &l_Set, 0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  Global::get_lighting_pipeline_layout().get(), 1, 1,
                  &l_TextureSet, 0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                l_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout().get(), 2, 1,
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
                p_RenderView.get_tonemapped_image()
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

              vkCmdBindPipeline(
                  l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                  g_LightCullingBaseGlobalData.cullingPipeline.get());

              {
                VkDescriptorSet l_Set =
                    Global::get_global_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                    Global::get_lighting_pipeline_layout().get(), 0,
                    1, &l_Set, 0, nullptr);

                {
                  VkDescriptorSet l_TextureSet =
                      Global::get_current_texture_descriptor_set();
                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                      Global::get_lighting_pipeline_layout().get(), 1,
                      1, &l_TextureSet, 0, nullptr);
                }

                VkDescriptorSet l_DescriptorSet =
                    l_ViewInfo.get_view_data_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                    Global::get_lighting_pipeline_layout().get(), 2,
                    1, &l_DescriptorSet, 0, nullptr);
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
        PipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        AllocatedBuffer kernelBuffer;
        Texture noise;
        Image noiseImage;
        bool initialized = false;
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
            g_BaseSsaoStepData.noise = Texture::make_gpu_ready(
                N(SsaoKernel), TextureFormatCategory::Float);
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

            g_BaseSsaoStepData.pipelineLayout =
                PipelineUtil::create_layout(N(SSAO), l_Layout);
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
          g_BaseSsaoStepData.initialized = true;
          return true;
        });

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              BaseSsaoStepData *l_Data = new BaseSsaoStepData;
              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              l_Data->texture = Texture::make_gpu_ready(
                  N(SsaoOut), TextureFormatCategory::Float);
              l_Data->tempBlurTexture = Texture::make_gpu_ready(
                  N(SsaoBlurTemp), TextureFormatCategory::Float);

              p_RenderView.set_ssao_image(l_Data->texture);
              return true;
            });
        l_RenderStep.set_teardown_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              BaseSsaoStepData *l_Data =
                  (BaseSsaoStepData *)GET_STEP_DATA(p_RenderView,
                                                    p_RenderStep);
              if (l_Data) {
                if (l_Data->texture.is_alive()) {
                  l_Data->texture.destroy();
                }
                if (l_Data->tempBlurTexture.is_alive()) {
                  l_Data->tempBlurTexture.destroy();
                }
              }
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

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            g_BaseSsaoStepData.pipeline.get());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BaseSsaoStepData.pipelineLayout.get(), 0, 1, &l_Set,
                0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_BaseSsaoStepData.pipelineLayout.get(), 1, 1,
                  &l_TextureSet, 0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                l_ViewInfo.get_view_data_descriptor_set();

            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BaseSsaoStepData.pipelineLayout.get(), 2, 1,
                &l_DescriptorSet, 0, nullptr);
          }
          {
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_BaseSsaoStepData.pipelineLayout.get(), 3, 1,
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

          vkCmdPushConstants(
              l_Cmd, g_BaseSsaoStepData.pipelineLayout.get(),
              VK_SHADER_STAGE_FRAGMENT_BIT, 0,
              sizeof(BaseSsaoPushConstants), &l_PushConstants);

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

      struct
      {
        Pipeline pipeline;
        PipelineLayout pipelineLayout;
        bool initialized = false;
      } g_CavitiesStepData;

      struct CavitiesStepData
      {
        Texture texture;
      };

      struct CavitiesPushConstants
      {
        float radius;
        float ridgeStrength;
        float valleyStrength;
      };

      bool initialize_cavities_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_CAVITIES_NAME);

        l_RenderStep.set_setup_callback([](RenderStep p_RenderStep)
                                            -> bool {
          {
            Util::List<VkDescriptorSetLayout> l_DescriptorSetLayouts;
            l_DescriptorSetLayouts.push_back(
                Global::get_global_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                Global::get_texture_descriptor_set_layout());
            l_DescriptorSetLayouts.push_back(
                Global::get_view_info_descriptor_set_layout());

            VkPipelineLayoutCreateInfo l_Layout{};
            l_Layout.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            l_Layout.pNext = nullptr;
            l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
            l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

            VkPushConstantRange l_PushConstant{};
            l_PushConstant.offset = 0;
            l_PushConstant.size = sizeof(CavitiesPushConstants);
            l_PushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            l_Layout.pPushConstantRanges = &l_PushConstant;
            l_Layout.pushConstantRangeCount = 1;

            g_CavitiesStepData.pipelineLayout =
                PipelineUtil::create_layout(N(Cavities), l_Layout);
          }

          {
            const Util::String l_VertexShaderPath =
                "fullscreen_triangle.vert";
            const Util::String l_FragmentShaderPath = "cavities.frag";

            PipelineUtil::GraphicsPipelineBuilder l_Builder;
            l_Builder.set_shaders(l_VertexShaderPath,
                                  l_FragmentShaderPath);
            l_Builder.pipelineLayout =
                g_CavitiesStepData.pipelineLayout;
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
                VK_FORMAT_R8G8_UNORM);

            l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

            g_CavitiesStepData.pipeline =
                l_Builder.register_pipeline();
          }

          g_CavitiesStepData.initialized = true;
          return true;
        });

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              CavitiesStepData *l_Data = new CavitiesStepData;
              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              l_Data->texture = Texture::make_gpu_ready(
                  N(CavitiesOut), TextureFormatCategory::Float);
              p_RenderView.set_cavities_image(l_Data->texture);
              return true;
            });

        l_RenderStep.set_teardown_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              CavitiesStepData *l_Data =
                  (CavitiesStepData *)GET_STEP_DATA(p_RenderView,
                                                    p_RenderStep);
              if (l_Data) {
                if (l_Data->texture.is_alive()) {
                  l_Data->texture.destroy();
                }
              }
              return true;
            });

        l_RenderStep.set_resolution_update_callback(
            [](RenderStep p_RenderStep,
               Math::UVector2 p_NewDimensions,
               RenderView p_RenderView) -> bool {
              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              CavitiesStepData *l_Data =
                  (CavitiesStepData *)GET_STEP_DATA(p_RenderView,
                                                    p_RenderStep);

              Texture l_Texture = l_Data->texture;
              LOWR_VK_ASSERT_RETURN(
                  l_Texture.is_alive(),
                  "Failed to execute cavities renderstep because "
                  "cavities output texture was not alive.");

              Vulkan::Image l_Image =
                  l_Texture.get_gpu().get_data_handle();

              if (l_Image.is_alive()) {
                ImGui_ImplVulkan_RemoveTexture(
                    (VkDescriptorSet)l_Texture.get_gpu()
                        .get_imgui_texture_id());

                ImageUtil::destroy(l_Image);
                l_Image.destroy();
              }

              l_Image = Vulkan::Image::make(N(CavitiesOut));
              l_Texture.get_gpu().set_data_handle(l_Image.get_id());

              VkExtent3D l_Extent;
              l_Extent.width = p_NewDimensions.x;
              l_Extent.height = p_NewDimensions.y;
              l_Extent.depth = 1;

              LOWR_VK_ASSERT_RETURN(
                  Vulkan::ImageUtil::create(
                      l_Image, l_Extent, VK_FORMAT_R8G8_UNORM,
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      false),
                  "Failed to create cavities output image.");

              ImageUtil::cmd_transition(
                  l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

              return true;
            });

        l_RenderStep.set_execute_callback([](RenderStep p_RenderStep,
                                             float p_Delta,
                                             RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Cavities",
                                     SINGLE_ARG({0.5f, 0.8f, 0.2f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          CavitiesStepData *l_Data =
              (CavitiesStepData *)GET_STEP_DATA(p_RenderView,
                                                p_RenderStep);

          Texture l_Texture = l_Data->texture;
          LOWR_VK_ASSERT_RETURN(
              l_Texture.is_alive(),
              "Failed to execute cavities renderstep because "
              "cavities output texture was not alive.");

          Math::UVector2 l_Dimensions = p_RenderView.get_dimensions();

          Vulkan::Image l_Image =
              l_Texture.get_gpu().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Vulkan::Image::make(N(CavitiesOut));
            l_Texture.get_gpu().set_data_handle(l_Image.get_id());

            VkExtent3D l_Extent;
            l_Extent.width = l_Dimensions.x;
            l_Extent.height = l_Dimensions.y;
            l_Extent.depth = 1;

            LOWR_VK_ASSERT_RETURN(
                Vulkan::ImageUtil::create(
                    l_Image, l_Extent, VK_FORMAT_R8G8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    false),
                "Failed to create cavities output image.");

            ImageUtil::cmd_transition(
                l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }

          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_Image.get_allocated_image().imageView,
              &l_ClearColorValue,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {l_Dimensions.x, l_Dimensions.y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            g_CavitiesStepData.pipeline.get());

          {
            VkDescriptorSet l_Set =
                Global::get_global_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_CavitiesStepData.pipelineLayout.get(), 0, 1, &l_Set,
                0, nullptr);

            {
              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_CavitiesStepData.pipelineLayout.get(), 1, 1,
                  &l_TextureSet, 0, nullptr);
            }

            VkDescriptorSet l_DescriptorSet =
                l_ViewInfo.get_view_data_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_CavitiesStepData.pipelineLayout.get(), 2, 1,
                &l_DescriptorSet, 0, nullptr);
          }

          CavitiesPushConstants l_PushConstants;
          l_PushConstants.radius = 2.0f;
          l_PushConstants.ridgeStrength = 2.5f;
          l_PushConstants.valleyStrength = 1.5f;

          vkCmdPushConstants(
              l_Cmd, g_CavitiesStepData.pipelineLayout.get(),
              VK_SHADER_STAGE_FRAGMENT_BIT, 0,
              sizeof(CavitiesPushConstants), &l_PushConstants);

          VkViewport l_Viewport = {};
          l_Viewport.x = 0;
          l_Viewport.y = 0;
          l_Viewport.width = static_cast<float>(l_Dimensions.x);
          l_Viewport.height = static_cast<float>(l_Dimensions.y);
          l_Viewport.minDepth = 0.f;
          l_Viewport.maxDepth = 1.f;

          vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

          VkRect2D l_Scissor = {};
          l_Scissor.offset.x = 0;
          l_Scissor.offset.y = 0;
          l_Scissor.extent.width = l_Dimensions.x;
          l_Scissor.extent.height = l_Dimensions.y;

          vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

          vkCmdDraw(l_Cmd, 3, 1, 0, 0);

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();

          return true;
        });

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
          Util::List<u32> l_DrawCommandUploads;

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          eastl::sort(
              p_RenderView.get_ui_canvases().begin(),
              p_RenderView.get_ui_canvases().end(),
              [](UiCanvas p_Canvas0, UiCanvas p_Canvas1) -> bool {
                return p_Canvas0.get_z_sorting() <
                       p_Canvas1.get_z_sorting();
              });

          for (auto it = p_RenderView.get_ui_canvases().begin();
               it != p_RenderView.get_ui_canvases().end(); ++it) {
            UiCanvas i_Canvas = it->get_id();
            if (i_Canvas.is_z_dirty()) {
              eastl::sort(i_Canvas.get_draw_commands().begin(),
                          i_Canvas.get_draw_commands().end(),
                          [](UiDrawCommand p_DC0,
                             UiDrawCommand p_DC1) -> bool {
                            if (!p_DC0.is_alive() ||
                                !p_DC1.is_alive()) {
                              return false;
                            }
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
                l_Entries.emplace_back(
                    i_DrawCommand.get_material().get_material_type(),
                    i_DrawCommand.get_submesh(), 1);
              } else {
                if (l_Entries.back().materialType ==
                        i_DrawCommand.get_material()
                            .get_material_type() &&
                    l_Entries.back().submesh ==
                        i_DrawCommand.get_submesh()) {
                  l_Entries.back().amount++;
                } else {
                  l_Entries.emplace_back(i_DrawCommand.get_material()
                                             .get_material_type(),
                                         i_DrawCommand.get_submesh(),
                                         1);
                }
              }

              l_DrawCommandUploads.push_back(
                  i_DrawCommand.get_slot());

              ++dit;
            }
          }

          // Upload the UI draw command data
          {
            size_t l_StagingOffset = 0;

            const u64 l_DrawCommandSize =
                sizeof(u32) * l_DrawCommandUploads.size();

            if (l_DrawCommandSize == 0) {
              VK_RENDERDOC_SECTION_END();
              return true;
            }

            // TODO: This does not go on the resource staging
            // buffer but a frame staging buffer of some sort
            const u64 l_FrameUploadSpace =
                l_ViewInfo.request_current_staging_buffer_space(
                    l_DrawCommandSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >=
                                      l_DrawCommandSize,
                                  "Did not have enough staging "
                                  "buffer space to upload "
                                  "UI draw command indices.");

            LOWR_VK_ASSERT_RETURN(
                l_ViewInfo.write_current_staging_buffer(
                    l_DrawCommandUploads.data(), l_FrameUploadSpace,
                    l_StagingOffset),
                "Failed to write ui draw command indices"
                " to staging buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;

            // This probably has to be done on the graphics
            // queue so we can leave it as is
            BufferUtil::cmd_buffer_barrier(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_ui_drawcommand_buffer(), l_CopyRegion,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT |
                    VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT);
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_current_staging_buffer().buffer.buffer,
                l_ViewInfo.get_ui_drawcommand_buffer().buffer, 1,
                &l_CopyRegion);
            BufferUtil::cmd_buffer_barrier(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_ui_drawcommand_buffer(), l_CopyRegion,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
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
                                  i_Pipeline.get());

                // Bind descriptor sets
                {
                  VkDescriptorSet l_Set =
                      Global::get_global_descriptor_set();

                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout().get(), 0, 1, &l_Set, 0,
                      nullptr);

                  {
                    VkDescriptorSet l_TextureSet =
                        Global::get_current_texture_descriptor_set();
                    vkCmdBindDescriptorSets(
                        l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        i_Pipeline.get_layout().get(), 1, 1,
                        &l_TextureSet, 0, nullptr);
                  }

                  VkDescriptorSet l_DescriptorSet =
                      l_ViewInfo.get_view_data_descriptor_set();

                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout().get(), 2, 1,
                      &l_DescriptorSet, 0, nullptr);
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
            u32 l_EditorImageIndex = LOW_UINT32_MAX;
            if (it->editorImage.is_alive() &&
                it->editorImage.get_state() == TextureState::LOADED) {
              l_EditorImageIndex =
                  it->editorImage.get_gpu().get_index();
            }
            l_Uploads.emplace_back(it->transform, it->color,
                                   l_EditorImageIndex, it->pickId);
          }

          {
            size_t l_StagingOffset = 0;

            const u64 l_DrawCommandSize =
                sizeof(DebugGeometryUpload) * l_Uploads.size();

            if (l_DrawCommandSize == 0) {
              VK_RENDERDOC_SECTION_END();
              return true;
            }

            const u64 l_FrameUploadSpace =
                l_ViewInfo.request_current_staging_buffer_space(
                    l_DrawCommandSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(l_FrameUploadSpace >=
                                      l_DrawCommandSize,
                                  "Did not have enough staging "
                                  "buffer space to upload "
                                  "debug geometry draw commands.");

            LOWR_VK_ASSERT_RETURN(
                l_ViewInfo.write_current_staging_buffer(
                    l_Uploads.data(), l_FrameUploadSpace,
                    l_StagingOffset),
                "Failed to write debug geometry draw command "
                "data to staging buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;

            Vulkan::BufferUtil::cmd_buffer_barrier(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_debug_geometry_buffer(), l_CopyRegion,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT |
                    VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT);

            // This probably has to be done on the graphics
            // queue so we can leave it as is
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_current_staging_buffer().buffer.buffer,
                l_ViewInfo.get_debug_geometry_buffer().buffer, 1,
                &l_CopyRegion);

            Vulkan::BufferUtil::cmd_buffer_barrier(
                Vulkan::Global::get_current_command_buffer(),
                l_ViewInfo.get_debug_geometry_buffer(), l_CopyRegion,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
          }

          Image l_DepthImage = p_RenderView.get_gbuffer_depth()
                                   .get_gpu()
                                   .get_data_handle();

          Image l_TonemappedImage =
              p_RenderView.get_tonemapped_image()
                  .get_gpu()
                  .get_data_handle();

          ImageUtil::cmd_transition(
              l_Cmd, l_TonemappedImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
          ImageUtil::cmd_transition(
              l_Cmd, l_DepthImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_TonemappedImage.get_allocated_image().imageView,
              nullptr, VK_IMAGE_LAYOUT_GENERAL);

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
                                i_Pipeline.get());

              // Bind descriptor sets
              {
                VkDescriptorSet l_Set =
                    Global::get_global_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout().get(), 0, 1, &l_Set, 0,
                    nullptr);

                {
                  VkDescriptorSet l_TextureSet =
                      Global::get_current_texture_descriptor_set();
                  vkCmdBindDescriptorSets(
                      l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      i_Pipeline.get_layout().get(), 1, 1,
                      &l_TextureSet, 0, nullptr);
                }

                VkDescriptorSet l_DescriptorSet =
                    l_ViewInfo.get_view_data_descriptor_set();

                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    i_Pipeline.get_layout().get(), 2, 1,
                    &l_DescriptorSet, 0, nullptr);
              }
            }

            // TODO: Change to custom Debug geometry push constant
            RenderEntryPushConstant i_PushConstants{};
            i_PushConstants.renderObjectSlot = i;
            i_PushConstants.activeVertexBuffer =
                (u32)VertexBuffer::Static;

            vkCmdPushConstants(l_Cmd, i_Pipeline.get_layout().get(),
                               VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                               sizeof(RenderEntryPushConstant),
                               &i_PushConstants);

            vkCmdDrawIndexed(l_Cmd, i_Draw.submesh.get_index_count(),
                             1, i_Draw.submesh.get_index_start(),
                             i_Draw.submesh.get_vertex_start(), 0);
          }

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd, l_TonemappedImage,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          ImageUtil::cmd_transition(
              l_Cmd, l_DepthImage,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();

          return true;
        });
        return true;
      }

      bool initialize_object_id_copy_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_OBJECT_ID_COPY);

        l_RenderStep.set_execute_callback(
            [&](RenderStep p_RenderStep, float p_Delta,
                RenderView p_RenderView) -> bool {
              VK_RENDERDOC_SECTION_BEGIN(
                  "Object ID copy",
                  SINGLE_ARG({0.2f, 0.427f, 0.217f}));

              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              ViewInfo l_ViewInfo =
                  p_RenderView.get_view_info_handle();

              Image l_Image = p_RenderView.get_object_map()
                                  .get_gpu()
                                  .get_data_handle();

              const u32 l_Width =
                  l_Image.get_allocated_image().extent.width;
              const u32 l_Height =
                  l_Image.get_allocated_image().extent.height;

              ImageUtil::cmd_transition(
                  l_Cmd, l_Image,
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

                BufferUtil::cmd_buffer_barrier(
                    l_Cmd, l_ViewInfo.get_object_id_buffer(),
                    l_Region.bufferOffset,
                    sizeof(u32) * l_Width * l_Height,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);

                vkCmdCopyImageToBuffer(
                    l_Cmd, l_Image.get_allocated_image().image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    l_ViewInfo.get_object_id_buffer().buffer, 1,
                    &l_Region);

                BufferUtil::cmd_buffer_barrier(
                    l_Cmd, l_ViewInfo.get_object_id_buffer(),
                    l_Region.bufferOffset,
                    sizeof(u32) * l_Width * l_Height,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_HOST_BIT,
                    VK_ACCESS_2_HOST_READ_BIT);
              }

              ImageUtil::cmd_transition(
                  l_Cmd, l_Image,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

              VK_RENDERDOC_SECTION_END();
              return true;
            });
        return true;
      }

      struct BlurStepData
      {
        Texture tempBlurTexture;
      };

      static Global::BlurSettings
      calculate_blur_settings(Math::UVector2 p_Dimensions, u8 p_Scale)
      {
        Global::BlurSettings l_Settings;

        const u32 l_MinDimension = p_Dimensions.x < p_Dimensions.y
                                       ? p_Dimensions.x
                                       : p_Dimensions.y;
        const float l_TargetScreenRadius = Math::Util::clamp(
            float(l_MinDimension) * (16.0f / 1080.0f), 8.0f, 32.0f);
        const float l_TargetTextureRadius =
            l_TargetScreenRadius / float(p_Scale);
        const float l_Radius = Math::Util::floor(
            (l_TargetTextureRadius / l_Settings.stepScale) + 0.999f);

        l_Settings.radius = Math::Util::clamp((u32)l_Radius, 1, 32);
        l_Settings.sigma = Math::Util::clamp(
            float(l_Settings.radius) * 0.5f, 1.0f, 16.0f);

        return l_Settings;
      }

      bool initialize_blur_renderstep()
      {
        RenderStep l_RenderStep = RenderStep::make(RENDERSTEP_BLUR);

        const u8 l_Scale = 1;

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              BlurStepData *l_Data = new BlurStepData;
              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              l_Data->tempBlurTexture = Texture::make_gpu_ready(
                  N(BlurTemp), TextureFormatCategory::Float);
              return true;
            });

        l_RenderStep.set_resolution_update_callback(
            [l_Scale](RenderStep p_RenderStep,
                      Math::UVector2 p_NewDimensions,
                      RenderView p_RenderView) -> bool {
              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              BlurStepData *l_Data = (BlurStepData *)GET_STEP_DATA(
                  p_RenderView, p_RenderStep);

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
                      Vulkan::Image::make(N(BlurTemp)));

                  VkExtent3D l_Extent;
                  l_Extent.width = p_NewDimensions.x / l_Scale;
                  l_Extent.height = p_NewDimensions.y / l_Scale;
                  l_Extent.depth = 1;

                  LOWR_VK_ASSERT_RETURN(
                      Vulkan::ImageUtil::create(
                          l_Data->tempBlurTexture.get_gpu()
                              .get_data_handle(),
                          l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          false),
                      "Failed to create temp blur image.");

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

        l_RenderStep.set_execute_callback([l_Scale](
                                              RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Blur",
                                     SINGLE_ARG({1.0f, 1.0f, 0.2f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          BlurStepData *l_Data = (BlurStepData *)GET_STEP_DATA(
              p_RenderView, p_RenderStep);

          Texture l_Texture = p_RenderView.get_blurred_image();
          LOWR_VK_ASSERT_RETURN(l_Texture.is_alive(),
                                "Failed to execute blur renderstep "
                                "because blur "
                                "output texture was not alive.");

          Math::UVector2 l_Dimensions = p_RenderView.get_dimensions();

          Vulkan::Image l_Image =
              l_Texture.get_gpu().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Vulkan::Image::make(N(BlurOut));
            l_Texture.get_gpu().set_data_handle(l_Image.get_id());

            VkExtent3D l_Extent;
            l_Extent.width = l_Dimensions.x / l_Scale;
            l_Extent.height = l_Dimensions.y / l_Scale;
            l_Extent.depth = 1;

            LOWR_VK_ASSERT_RETURN(
                Vulkan::ImageUtil::create(
                    l_Image, l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    false),
                "Failed to create blur out image.");

            ImageUtil::cmd_transition(
                l_Cmd,
                p_RenderView.get_blurred_image()
                    .get_gpu()
                    .get_data_handle(),
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
                      l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      false),
                  "Failed to create blur image.");

              ImageUtil::cmd_transition(
                  l_Cmd,
                  l_Data->tempBlurTexture.get_gpu().get_data_handle(),
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            {
              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();

              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_BaseSsaoStepData.pipelineLayout.get(), 0, 1,
                  &l_Set, 0, nullptr);

              {
                VkDescriptorSet l_TextureSet =
                    Global::get_current_texture_descriptor_set();
                vkCmdBindDescriptorSets(
                    l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    g_BaseSsaoStepData.pipelineLayout.get(), 1, 1,
                    &l_TextureSet, 0, nullptr);
              }

              VkDescriptorSet l_DescriptorSet =
                  l_ViewInfo.get_view_data_descriptor_set();

              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_BaseSsaoStepData.pipelineLayout.get(), 2, 1,
                  &l_DescriptorSet, 0, nullptr);
            }
            {
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_BaseSsaoStepData.pipelineLayout.get(), 3, 1,
                  &g_BaseSsaoStepData.descriptorSet, 0, nullptr);
            }
          }

          Global::blur_image_4(
              p_RenderView.get_tonemapped_image(),
              l_Data->tempBlurTexture,
              p_RenderView.get_blurred_image(),
              {p_RenderView.get_dimensions().x / l_Scale,
               p_RenderView.get_dimensions().y / l_Scale},
              calculate_blur_settings(l_Dimensions, l_Scale));

          VK_RENDERDOC_SECTION_END();

          return true;
        });

        return true;
      }

      struct ShadowPassGlobalData
      {
        bool initialized = false;
      } g_ShadowStepData;

      static bool initialize_shadow_pass_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_SHADOW_PASS_NAME);

        l_RenderStep.set_setup_callback(
            [](RenderStep p_RenderStep) -> bool {
              g_ShadowStepData.initialized = true;
              return true;
            });

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              ViewInfo l_ViewInfo =
                  p_RenderView.get_view_info_handle();
              ShadowPassViewData &l_Data =
                  l_ViewInfo.get_shadow_pass_data();
              l_Data.directional_shadow_buffer =
                  BufferUtil::create_buffer(
                      sizeof(DirectionalLightShadowInfo),
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VMA_MEMORY_USAGE_GPU_ONLY);
              {
                Util::StringBuilder l_Builder;
                l_Builder.append(p_RenderView.get_name());
                l_Builder.append(" directional shadow buffer");
                Util::String l_Name = l_Builder.get();
                BufferUtil::set_name(l_Data.directional_shadow_buffer,
                                     l_Name.c_str());
              }

              l_Data.point_light_shadow_buffer =
                  BufferUtil::create_buffer(
                      sizeof(PointLightShadowInfo) * POINTLIGHT_COUNT,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VMA_MEMORY_USAGE_GPU_ONLY);
              {
                Util::StringBuilder l_Builder;
                l_Builder.append(p_RenderView.get_name());
                l_Builder.append(" point light shadow buffer");
                Util::String l_Name = l_Builder.get();
                BufferUtil::set_name(l_Data.point_light_shadow_buffer,
                                     l_Name.c_str());
              }

              return true;
            });

        l_RenderStep.set_teardown_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              ViewInfo l_ViewInfo =
                  p_RenderView.get_view_info_handle();
              ShadowPassViewData &l_Data =
                  l_ViewInfo.get_shadow_pass_data();
              if (l_Data.directional_shadow_buffer.buffer !=
                  VK_NULL_HANDLE) {
                BufferUtil::destroy_buffer(
                    l_Data.directional_shadow_buffer);
              }
              if (l_Data.point_light_shadow_buffer.buffer !=
                  VK_NULL_HANDLE) {
                BufferUtil::destroy_buffer(
                    l_Data.point_light_shadow_buffer);
              }
              return true;
            });

        l_RenderStep.set_execute_callback([](RenderStep p_RenderStep,
                                             float p_Delta,
                                             RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();
          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();
          RenderScene l_RenderScene = p_RenderView.get_render_scene();

          ShadowPassViewData &l_Data =
              l_ViewInfo.get_shadow_pass_data();

          if (!l_Data.descriptors_written) {
            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(
                7, l_Data.directional_shadow_buffer.buffer,
                sizeof(DirectionalLightShadowInfo), 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Writer.write_buffer(
                8, l_Data.point_light_shadow_buffer.buffer,
                sizeof(PointLightShadowInfo) * POINTLIGHT_COUNT, 0,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
            l_Writer.update_set(
                Global::get_device(),
                l_ViewInfo.get_view_data_descriptor_set());
            l_Data.descriptors_written = true;
          }

          VK_RENDERDOC_SECTION_BEGIN("Shadow pass",
                                     SINGLE_ARG({0.2f, 0.2f, 0.8f}));

          Image l_AtlasImage = p_RenderView.get_shadow_atlas()
                                   .get_gpu()
                                   .get_data_handle();

          ImageUtil::cmd_transition(
              l_Cmd, l_AtlasImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          VkClearValue l_DepthClear{};
          l_DepthClear.depthStencil.depth = 1.0f;
          l_DepthClear.depthStencil.stencil = 0;

          VkRenderingAttachmentInfo l_DepthAttachment =
              InitUtil::attachment_info(
                  l_AtlasImage.get_allocated_image().imageView,
                  nullptr, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE}, nullptr, 0,
              &l_DepthAttachment);

          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindIndexBuffer(
              l_Cmd, Global::get_mesh_index_buffer().m_Buffer.buffer,
              0, VK_INDEX_TYPE_UINT32);

          auto draw_scene_depth =
              [&](const Math::Matrix4x4 &p_LightSpace, u32 p_TileX,
                  u32 p_TileY, u32 p_TileW, u32 p_TileH) {
                VkClearAttachment l_Clear{};
                l_Clear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                l_Clear.clearValue = l_DepthClear;

                VkClearRect l_ClearRect{};
                l_ClearRect.rect.offset = {(int32_t)p_TileX,
                                           (int32_t)p_TileY};
                l_ClearRect.rect.extent = {p_TileW, p_TileH};
                l_ClearRect.baseArrayLayer = 0;
                l_ClearRect.layerCount = 1;

                vkCmdClearAttachments(l_Cmd, 1, &l_Clear, 1,
                                      &l_ClearRect);

                VkViewport l_Viewport{};
                l_Viewport.x = (float)p_TileX;
                l_Viewport.y = (float)p_TileY;
                l_Viewport.width = (float)p_TileW;
                l_Viewport.height = (float)p_TileH;
                l_Viewport.minDepth = 0.0f;
                l_Viewport.maxDepth = 1.0f;
                vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

                VkRect2D l_Scissor{};
                l_Scissor.offset = {(int32_t)p_TileX,
                                    (int32_t)p_TileY};
                l_Scissor.extent = {p_TileW, p_TileH};
                vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

                MaterialType l_CurrentMaterialType =
                    Low::Util::Handle::DEAD;

                for (auto it = p_RenderView.get_render_scene()
                                   .get_draw_commands()
                                   .begin();
                     it != p_RenderView.get_render_scene()
                               .get_draw_commands()
                               .end();
                     ++it) {
                  if (!it->is_alive() ||
                      !it->has_any_render_object()) {
                    continue;
                  }
                  if (it->get_any_render_object_mesh().get_state() !=
                      MeshState::LOADED) {
                    continue;
                  }

                  Material i_Material = it->get_material();
                  if (!i_Material.is_alive()) {
                    i_Material = it->get_any_render_object_material();
                  }
                  if (!i_Material.is_alive()) {
                    continue;
                  }

                  MaterialType i_MaterialType =
                      i_Material.get_material_type();
                  if (!i_MaterialType.is_alive() ||
                      !i_MaterialType.casts_shadows()) {
                    continue;
                  }

                  Pipeline i_ShadowPipeline =
                      i_MaterialType.get_shadow_pipeline_handle();
                  if (!i_ShadowPipeline.is_alive()) {
                    continue;
                  }

                  GpuSubmesh i_GpuSubmesh = it->get_submesh();
                  if (!i_GpuSubmesh.is_alive()) {
                    continue;
                  }

                  if (i_MaterialType != l_CurrentMaterialType) {
                    l_CurrentMaterialType = i_MaterialType;

                    vkCmdBindPipeline(l_Cmd,
                                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      i_ShadowPipeline.get());

                    VkDescriptorSet l_GlobalSet =
                        Global::get_global_descriptor_set();
                    vkCmdBindDescriptorSets(
                        l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        i_ShadowPipeline.get_layout().get(), 0, 1,
                        &l_GlobalSet, 0, nullptr);
                  }

                  ShadowPassPushConstants l_Push{};
                  l_Push.renderObjectSlot = it->get_slot();
                  l_Push.activeVertexBuffer =
                      (u32)it->get_active_vertex_buffer();
                  l_Push.lightSpaceMatrix = p_LightSpace;

                  vkCmdPushConstants(
                      l_Cmd, i_ShadowPipeline.get_layout().get(),
                      VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(ShadowPassPushConstants), &l_Push);

                  vkCmdDrawIndexed(
                      l_Cmd, i_GpuSubmesh.get_index_count(), 1,
                      i_GpuSubmesh.get_index_start(),
                      static_cast<int32_t>(
                          it->get_active_vertex_offset()),
                      0);
                }
              };

          for (int i = 0; i < SHADOW_CSM_CASCADE_COUNT; ++i) {
            u32 l_TileX = i * SHADOW_TILE_SIZE_CSM;
            draw_scene_depth(
                l_ViewInfo.get_directional_light_shadow_info()
                    .light_space[i],
                l_TileX, 0, SHADOW_TILE_SIZE_CSM,
                SHADOW_TILE_SIZE_CSM);
          }

          int l_PLRenderCount = 0;

          for (u32 li = 0; li < PointLight::living_count() &&
                           l_PLRenderCount < MAX_SHADOW_POINTLIGHTS;
               ++li) {
            PointLight i_PL = PointLight::living_instances()[li];
            if (!i_PL.is_alive()) {
              continue;
            }
            if (i_PL.get_render_scene_handle() !=
                l_RenderScene.get_id()) {
              continue;
            }

            const u32 l_PointLightSlot = i_PL.get_slot();
            if (l_PointLightSlot >= POINTLIGHT_COUNT) {
              continue;
            }

            auto l_Pos =
                l_Data.point_light_slot_mapping.find(i_PL.get_id());

            if (l_Pos == l_Data.point_light_slot_mapping.end()) {
              continue;
            }

            const u32 l_ShadowSlot = l_Pos->second;

            if (l_ShadowSlot >= MAX_SHADOW_POINTLIGHTS) {
              continue;
            }

            l_PLRenderCount++;

            const u32 l_TilesPerRow =
                SHADOW_ATLAS_SIZE / SHADOW_TILE_SIZE_POINT;

            for (int f = 0; f < 6; ++f) {
              u32 l_TileIdx = l_ShadowSlot * 6 + f;
              u32 l_TileX = (l_TileIdx % l_TilesPerRow) *
                            SHADOW_TILE_SIZE_POINT;
              u32 l_TileY =
                  SHADOW_TILE_SIZE_CSM + (l_TileIdx / l_TilesPerRow) *
                                             SHADOW_TILE_SIZE_POINT;

              draw_scene_depth(
                  l_Data.point_light_shadow_data[l_ShadowSlot]
                      .light_space[f],
                  l_TileX, l_TileY, SHADOW_TILE_SIZE_POINT,
                  SHADOW_TILE_SIZE_POINT);
            }
          }

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd, l_AtlasImage,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();

          return true;
        });

        return true;
      }

      struct
      {
        Pipeline generationPipeline;
        Pipeline compositePipeline;
        PipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        AllocatedBuffer kernelBuffer;
        Image noiseImage;
        Texture noiseTex;
        bool initialized = false;
      } g_SsgiStepData;

      struct SsgiStepData
      {
        Texture texture;
        Texture tempBlurTexture;
      };

      struct SsgiPushConstants
      {
        Math::Vector2 noiseScale;
        float radius;
        float strength;
      };

      bool initialize_ssgi_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_SSGI_NAME);

        const u8 l_Scale = 2;
        const Math::UVector2 l_NoiseDimensions(4);
        const u32 l_KernelSize = 16u;

        l_RenderStep.set_setup_callback([l_NoiseDimensions,
                                         l_KernelSize](
                                            RenderStep p_RenderStep)
                                            -> bool {
          {
            g_SsgiStepData.kernelBuffer = BufferUtil::create_buffer(
                sizeof(KERNEL_VECTOR) * l_KernelSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
          }

          {
            const size_t l_KernelDataSize =
                l_KernelSize * sizeof(KERNEL_VECTOR);

            Low::Util::List<KERNEL_VECTOR> l_Kernel;
            for (u32 i = 0; i < l_KernelSize; ++i) {
              KERNEL_VECTOR i_Sample(
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
                  (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
                  rand() / float(RAND_MAX), 0.0f);
              i_Sample = glm::normalize(i_Sample);
              i_Sample *= rand() / float(RAND_MAX);
              float i_Scale = float(i) / float(l_KernelSize);
              i_Scale = glm::mix(0.1f, 1.0f, i_Scale * i_Scale);
              i_Sample *= i_Scale;
              l_Kernel.push_back(i_Sample);
            }

            size_t l_StagingOffset = 0;
            const u64 l_FrameUploadSpace =
                request_resource_staging_buffer_space(
                    l_KernelDataSize, &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(
                l_FrameUploadSpace >= l_KernelDataSize,
                "Did not have enough staging buffer space to upload "
                "SSGI kernel data.");

            LOWR_VK_ASSERT_RETURN(
                resource_staging_buffer_write(l_Kernel.data(),
                                              l_FrameUploadSpace,
                                              l_StagingOffset),
                "Failed to write SSGI kernel data to staging "
                "buffer");

            VkBufferCopy l_CopyRegion{};
            l_CopyRegion.srcOffset = l_StagingOffset;
            l_CopyRegion.dstOffset = 0;
            l_CopyRegion.size = l_FrameUploadSpace;
            vkCmdCopyBuffer(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                g_SsgiStepData.kernelBuffer.buffer, 1, &l_CopyRegion);
          }

          {
            g_SsgiStepData.noiseTex = Texture::make_gpu_ready(
                N(SsgiNoise), TextureFormatCategory::Float);
            Vulkan::Image l_Image = Vulkan::Image::make(N(SsgiNoise));
            g_SsgiStepData.noiseTex.get_gpu().set_data_handle(
                l_Image.get_id());
            g_SsgiStepData.noiseImage = l_Image;

            size_t l_StagingOffset = 0;
            const u64 l_UploadSpace =
                Vulkan::request_resource_staging_buffer_space(
                    l_NoiseDimensions.x * l_NoiseDimensions.y *
                        IMAGE_CHANNEL_COUNT,
                    &l_StagingOffset);

            LOWR_VK_ASSERT_RETURN(
                l_UploadSpace ==
                    (l_NoiseDimensions.x * l_NoiseDimensions.y *
                     IMAGE_CHANNEL_COUNT),
                "Failed to request resource staging buffer space "
                "for SSGI noise texture");

            VkExtent3D l_Extent;
            l_Extent.width = l_NoiseDimensions.x;
            l_Extent.height = l_NoiseDimensions.y;
            l_Extent.depth = 1;

            Vulkan::ImageUtil::create(
                l_Image, l_Extent, VK_FORMAT_R8G8B8A8_SNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                false);

            struct NoisePixel
            {
              int8_t r, g, b, a;
            };
            Util::List<NoisePixel> l_Pixels;
            for (u32 i = 0;
                 i < (l_NoiseDimensions.x * l_NoiseDimensions.y);
                 ++i) {
              l_Pixels.push_back(
                  {(int8_t)(((rand() / float(RAND_MAX)) * 2.0f -
                             1.0f) *
                            127.0f),
                   (int8_t)(((rand() / float(RAND_MAX)) * 2.0f -
                             1.0f) *
                            127.0f),
                   0, 127});
            }

            LOWR_VK_ASSERT_RETURN(
                Vulkan::resource_staging_buffer_write(
                    l_Pixels.data(), l_UploadSpace, l_StagingOffset),
                "Failed to upload SSGI noise texture to resource "
                "staging buffer");

            ImageUtil::cmd_transition(
                Global::get_current_command_buffer(), l_Image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy l_Region = {};
            l_Region.bufferOffset = l_StagingOffset;
            l_Region.bufferRowLength = 0;
            l_Region.bufferImageHeight = 0;
            l_Region.imageSubresource.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
            l_Region.imageSubresource.mipLevel = 0;
            l_Region.imageSubresource.baseArrayLayer = 0;
            l_Region.imageSubresource.layerCount = 1;
            l_Region.imageOffset = {0, 0, 0};
            l_Region.imageExtent = {l_NoiseDimensions.x,
                                    l_NoiseDimensions.y, 1};

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
            g_SsgiStepData.descriptorSetLayout = l_Builder.build(
                Global::get_device(), VK_SHADER_STAGE_ALL_GRAPHICS);

            g_SsgiStepData.descriptorSet =
                Global::get_global_descriptor_allocator().allocate(
                    Global::get_device(),
                    g_SsgiStepData.descriptorSetLayout);
          }

          {
            Samplers &l_Samplers = Global::get_samplers();

            DescriptorUtil::DescriptorWriter l_Writer;
            l_Writer.write_buffer(
                0, g_SsgiStepData.kernelBuffer.buffer,
                sizeof(KERNEL_VECTOR) * l_KernelSize, 0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            l_Writer.write_image(
                1,
                g_SsgiStepData.noiseImage.get_allocated_image()
                    .imageView,
                l_Samplers.no_lod_nearest_repeat_black,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            l_Writer.update_set(Global::get_device(),
                                g_SsgiStepData.descriptorSet);
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
                g_SsgiStepData.descriptorSetLayout);

            VkPipelineLayoutCreateInfo l_Layout{};
            l_Layout.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            l_Layout.pNext = nullptr;
            l_Layout.pSetLayouts = l_DescriptorSetLayouts.data();
            l_Layout.setLayoutCount = l_DescriptorSetLayouts.size();

            VkPushConstantRange l_PushConstant{};
            l_PushConstant.offset = 0;
            l_PushConstant.size = sizeof(SsgiPushConstants);
            l_PushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            l_Layout.pPushConstantRanges = &l_PushConstant;
            l_Layout.pushConstantRangeCount = 1;

            g_SsgiStepData.pipelineLayout =
                PipelineUtil::create_layout(N(SSGI), l_Layout);
          }

          {
            PipelineUtil::GraphicsPipelineBuilder l_Builder;
            l_Builder.set_shaders("fullscreen_triangle.vert",
                                  "ssgi.frag");
            l_Builder.pipelineLayout = g_SsgiStepData.pipelineLayout;
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

            g_SsgiStepData.generationPipeline =
                l_Builder.register_pipeline();
          }

          {
            PipelineUtil::GraphicsPipelineBuilder l_Builder;
            l_Builder.set_shaders("fullscreen_triangle.vert",
                                  "ssgi_composite.frag");
            l_Builder.pipelineLayout =
                Global::get_lighting_pipeline_layout();
            l_Builder.set_input_topology(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
            l_Builder.set_cull_mode(VK_CULL_MODE_BACK_BIT,
                                    VK_FRONT_FACE_CLOCKWISE);
            l_Builder.set_multismapling_none();
            l_Builder.colorBlendAttachment.blendEnable = VK_TRUE;
            l_Builder.colorBlendAttachment.srcColorBlendFactor =
                VK_BLEND_FACTOR_ONE;
            l_Builder.colorBlendAttachment.dstColorBlendFactor =
                VK_BLEND_FACTOR_ONE;
            l_Builder.colorBlendAttachment.colorBlendOp =
                VK_BLEND_OP_ADD;
            l_Builder.colorBlendAttachment.srcAlphaBlendFactor =
                VK_BLEND_FACTOR_ZERO;
            l_Builder.colorBlendAttachment.dstAlphaBlendFactor =
                VK_BLEND_FACTOR_ONE;
            l_Builder.colorBlendAttachment.alphaBlendOp =
                VK_BLEND_OP_ADD;
            l_Builder.colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            l_Builder.disable_depth_test();
            l_Builder.colorAttachmentFormats.clear();
            l_Builder.colorAttachmentFormats.push_back(
                VK_FORMAT_R16G16B16A16_SFLOAT);
            l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

            g_SsgiStepData.compositePipeline =
                l_Builder.register_pipeline();
          }

          g_SsgiStepData.initialized = true;
          return true;
        });

        l_RenderStep.set_prepare_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              SsgiStepData *l_Data = new SsgiStepData;
              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              l_Data->texture = Texture::make_gpu_ready(
                  N(SsgiOut), TextureFormatCategory::Float);
              l_Data->tempBlurTexture = Texture::make_gpu_ready(
                  N(SsgiBlurTemp), TextureFormatCategory::Float);
              p_RenderView.set_ssgi_image(l_Data->texture);
              return true;
            });

        l_RenderStep.set_teardown_callback(
            [](RenderStep p_RenderStep,
               RenderView p_RenderView) -> bool {
              SsgiStepData *l_Data = (SsgiStepData *)GET_STEP_DATA(
                  p_RenderView, p_RenderStep);
              if (l_Data) {
                if (l_Data->texture.is_alive()) {
                  l_Data->texture.destroy();
                }
                if (l_Data->tempBlurTexture.is_alive()) {
                  l_Data->tempBlurTexture.destroy();
                }
              }
              return true;
            });

        l_RenderStep.set_resolution_update_callback(
            [l_Scale](RenderStep p_RenderStep,
                      Math::UVector2 p_NewDimensions,
                      RenderView p_RenderView) -> bool {
              VkCommandBuffer l_Cmd =
                  Global::get_current_command_buffer();

              SsgiStepData *l_Data = (SsgiStepData *)GET_STEP_DATA(
                  p_RenderView, p_RenderStep);

              {
                Texture l_Texture = l_Data->texture;
                LOWR_VK_ASSERT_RETURN(
                    l_Texture.is_alive(),
                    "Failed to execute SSGI renderstep because SSGI "
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

                l_Image = Vulkan::Image::make(N(SsgiOut));
                l_Texture.get_gpu().set_data_handle(l_Image.get_id());

                VkExtent3D l_Extent;
                l_Extent.width = p_NewDimensions.x / l_Scale;
                l_Extent.height = p_NewDimensions.y / l_Scale;
                l_Extent.depth = 1;

                LOWR_VK_ASSERT_RETURN(
                    Vulkan::ImageUtil::create(
                        l_Image, l_Extent,
                        VK_FORMAT_R16G16B16A16_SFLOAT,
                        VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        false),
                    "Failed to create SSGI out image.");

                ImageUtil::cmd_transition(
                    l_Cmd,
                    l_Data->texture.get_gpu().get_data_handle(),
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
              }

              {
                Texture l_Texture = l_Data->tempBlurTexture;
                LOWR_VK_ASSERT_RETURN(
                    l_Texture.is_alive(),
                    "Failed to execute SSGI renderstep because SSGI "
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
                      Vulkan::Image::make(N(SsgiBlur)));

                  VkExtent3D l_Extent;
                  l_Extent.width = p_NewDimensions.x / l_Scale;
                  l_Extent.height = p_NewDimensions.y / l_Scale;
                  l_Extent.depth = 1;

                  LOWR_VK_ASSERT_RETURN(
                      Vulkan::ImageUtil::create(
                          l_Data->tempBlurTexture.get_gpu()
                              .get_data_handle(),
                          l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          false),
                      "Failed to create SSGI blur image.");

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

          VK_RENDERDOC_SECTION_BEGIN("SSGI",
                                     SINGLE_ARG({0.3f, 0.8f, 0.3f}));

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          SsgiStepData *l_Data = (SsgiStepData *)GET_STEP_DATA(
              p_RenderView, p_RenderStep);

          Texture l_Texture = l_Data->texture;
          LOWR_VK_ASSERT_RETURN(
              l_Texture.is_alive(),
              "Failed to execute SSGI renderstep because SSGI output "
              "texture was not alive.");

          Math::UVector2 l_Dimensions = p_RenderView.get_dimensions();

          Vulkan::Image l_Image =
              l_Texture.get_gpu().get_data_handle();

          if (!l_Image.is_alive()) {
            l_Image = Vulkan::Image::make(N(SsgiOut));
            l_Texture.get_gpu().set_data_handle(l_Image.get_id());

            VkExtent3D l_Extent;
            l_Extent.width = l_Dimensions.x / l_Scale;
            l_Extent.height = l_Dimensions.y / l_Scale;
            l_Extent.depth = 1;

            LOWR_VK_ASSERT_RETURN(
                Vulkan::ImageUtil::create(
                    l_Image, l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    false),
                "Failed to create SSGI out image.");

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
                  Vulkan::Image::make(N(SsgiBlur)));

              VkExtent3D l_Extent;
              l_Extent.width = l_Dimensions.x / l_Scale;
              l_Extent.height = l_Dimensions.y / l_Scale;
              l_Extent.depth = 1;

              LOWR_VK_ASSERT_RETURN(
                  Vulkan::ImageUtil::create(
                      l_Data->tempBlurTexture.get_gpu()
                          .get_data_handle(),
                      l_Extent, VK_FORMAT_R16G16B16A16_SFLOAT,
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      false),
                  "Failed to create SSGI blur image.");

              ImageUtil::cmd_transition(
                  l_Cmd,
                  l_Data->tempBlurTexture.get_gpu().get_data_handle(),
                  VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
          }

          // Generation pass
          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          {
            VkClearValue l_ClearValue = {};
            l_ClearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

            Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
            l_ColorAttachments.resize(1);
            l_ColorAttachments[0] = InitUtil::attachment_info(
                l_Image.get_allocated_image().imageView,
                &l_ClearValue,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
                {l_Dimensions.x / l_Scale, l_Dimensions.y / l_Scale},
                l_ColorAttachments.data(), l_ColorAttachments.size(),
                nullptr);
            vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

            vkCmdBindPipeline(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                g_SsgiStepData.generationPipeline.get());

            {
              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_SsgiStepData.pipelineLayout.get(), 0, 1, &l_Set,
                  0, nullptr);

              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_SsgiStepData.pipelineLayout.get(), 1, 1,
                  &l_TextureSet, 0, nullptr);

              VkDescriptorSet l_ViewDescSet =
                  l_ViewInfo.get_view_data_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_SsgiStepData.pipelineLayout.get(), 2, 1,
                  &l_ViewDescSet, 0, nullptr);

              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  g_SsgiStepData.pipelineLayout.get(), 3, 1,
                  &g_SsgiStepData.descriptorSet, 0, nullptr);
            }

            SsgiPushConstants l_PushConstants;
            l_PushConstants.noiseScale.x =
                1.0f /
                (float(l_Dimensions.x) / float(l_NoiseDimensions.x));
            l_PushConstants.noiseScale.y =
                1.0f /
                (float(l_Dimensions.y) / float(l_NoiseDimensions.y));
            l_PushConstants.radius = 1.5f;
            l_PushConstants.strength = 0.5f;

            vkCmdPushConstants(
                l_Cmd, g_SsgiStepData.pipelineLayout.get(),
                VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                sizeof(SsgiPushConstants), &l_PushConstants);

            VkViewport l_Viewport = {};
            l_Viewport.x = 0;
            l_Viewport.y = 0;
            l_Viewport.width =
                static_cast<float>(l_Dimensions.x / l_Scale);
            l_Viewport.height =
                static_cast<float>(l_Dimensions.y / l_Scale);
            l_Viewport.minDepth = 0.f;
            l_Viewport.maxDepth = 1.f;
            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            VkRect2D l_Scissor = {};
            l_Scissor.offset.x = 0;
            l_Scissor.offset.y = 0;
            l_Scissor.extent.width = l_Dimensions.x / l_Scale;
            l_Scissor.extent.height = l_Dimensions.y / l_Scale;
            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);
          }

          ImageUtil::cmd_transition(
              l_Cmd, l_Data->texture.get_gpu().get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          // Blur the SSGI output
          Global::blur_image_4(
              l_Data->texture, l_Data->tempBlurTexture,
              l_Data->texture,
              {l_Dimensions.x / l_Scale, l_Dimensions.y / l_Scale});

          // Composite pass: additively blend SSGI onto lit_image
          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          {
            Image l_LitImage = p_RenderView.get_lit_image()
                                   .get_gpu()
                                   .get_data_handle();

            Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
            l_ColorAttachments.resize(1);
            l_ColorAttachments[0] = InitUtil::attachment_info(
                l_LitImage.get_allocated_image().imageView, nullptr,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
                {l_Dimensions.x, l_Dimensions.y},
                l_ColorAttachments.data(), l_ColorAttachments.size(),
                nullptr);
            vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

            vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              g_SsgiStepData.compositePipeline.get());

            {
              VkDescriptorSet l_Set =
                  Global::get_global_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  Global::get_lighting_pipeline_layout().get(), 0, 1,
                  &l_Set, 0, nullptr);

              VkDescriptorSet l_TextureSet =
                  Global::get_current_texture_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  Global::get_lighting_pipeline_layout().get(), 1, 1,
                  &l_TextureSet, 0, nullptr);

              VkDescriptorSet l_ViewDescSet =
                  l_ViewInfo.get_view_data_descriptor_set();
              vkCmdBindDescriptorSets(
                  l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                  Global::get_lighting_pipeline_layout().get(), 2, 1,
                  &l_ViewDescSet, 0, nullptr);
            }

            VkViewport l_Viewport = {};
            l_Viewport.x = 0;
            l_Viewport.y = 0;
            l_Viewport.width = static_cast<float>(l_Dimensions.x);
            l_Viewport.height = static_cast<float>(l_Dimensions.y);
            l_Viewport.minDepth = 0.f;
            l_Viewport.maxDepth = 1.f;
            vkCmdSetViewport(l_Cmd, 0, 1, &l_Viewport);

            VkRect2D l_Scissor = {};
            l_Scissor.offset.x = 0;
            l_Scissor.offset.y = 0;
            l_Scissor.extent.width = l_Dimensions.x;
            l_Scissor.extent.height = l_Dimensions.y;
            vkCmdSetScissor(l_Cmd, 0, 1, &l_Scissor);

            vkCmdDraw(l_Cmd, 3, 1, 0, 0);

            vkCmdEndRendering(l_Cmd);
          }

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();
          return true;
        });

        return true;
      }

      struct SkyGradientStepData
      {
        Pipeline pipeline;
      };

      struct TonemappingStepData
      {
        Pipeline pipeline;
      };

      static bool initialize_tonemapping_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_TONEMAPPING_NAME);

        l_RenderStep.set_prepare_callback(
            [&](RenderStep p_RenderStep,
                RenderView p_RenderView) -> bool {
              TonemappingStepData *l_Data = new TonemappingStepData;

              PipelineUtil::GraphicsPipelineBuilder l_Builder;
              l_Builder.pipelineLayout =
                  Global::get_lighting_pipeline_layout();
              l_Builder.set_shaders("fullscreen_triangle.vert",
                                    "tonemapping.frag");
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
                  VK_FORMAT_R8G8B8A8_UNORM);

              l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

              l_Data->pipeline = l_Builder.register_pipeline();

              p_RenderView.get_step_data()[p_RenderStep.get_index()] =
                  l_Data;
              return true;
            });

        l_RenderStep.set_teardown_callback(
            [&](RenderStep p_RenderStep,
                RenderView p_RenderView) -> bool { return true; });

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Tonemapping",
                                     SINGLE_ARG({1.0f, 0.75f, 0.2f}));

          TonemappingStepData *l_Data =
              (TonemappingStepData *)p_RenderView
                  .get_step_data()[p_RenderStep.get_index()];

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();
          Image l_TonemappedImage =
              p_RenderView.get_tonemapped_image()
                  .get_gpu()
                  .get_data_handle();

          ImageUtil::cmd_transition(
              l_Cmd, l_TonemappedImage,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          VkClearValue l_ClearColorValue = {};
          l_ClearColorValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

          Util::List<VkRenderingAttachmentInfo> l_ColorAttachments;
          l_ColorAttachments.resize(1);
          l_ColorAttachments[0] = InitUtil::attachment_info(
              l_TonemappedImage.get_allocated_image().imageView,
              &l_ClearColorValue, VK_IMAGE_LAYOUT_GENERAL);

          VkRenderingInfo l_RenderInfo = InitUtil::rendering_info(
              {p_RenderView.get_dimensions().x,
               p_RenderView.get_dimensions().y},
              l_ColorAttachments.data(), l_ColorAttachments.size(),
              nullptr);
          vkCmdBeginRendering(l_Cmd, &l_RenderInfo);

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            l_Data->pipeline.get());

          VkDescriptorSet l_GlobalSet =
              Global::get_global_descriptor_set();
          vkCmdBindDescriptorSets(
              l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
              Global::get_lighting_pipeline_layout().get(), 0, 1,
              &l_GlobalSet, 0, nullptr);

          VkDescriptorSet l_TextureSet =
              Global::get_current_texture_descriptor_set();
          vkCmdBindDescriptorSets(
              l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
              Global::get_lighting_pipeline_layout().get(), 1, 1,
              &l_TextureSet, 0, nullptr);

          VkDescriptorSet l_ViewSet =
              l_ViewInfo.get_view_data_descriptor_set();
          vkCmdBindDescriptorSets(
              l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
              Global::get_lighting_pipeline_layout().get(), 2, 1,
              &l_ViewSet, 0, nullptr);

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
              l_Cmd, l_TonemappedImage,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();
          return true;
        });

        return true;
      }

      static bool initialize_sky_gradient_renderstep()
      {
        RenderStep l_RenderStep =
            RenderStep::make(RENDERSTEP_SKY_GRADIENT_NAME);

        l_RenderStep.set_prepare_callback(
            [&](RenderStep p_RenderStep,
                RenderView p_RenderView) -> bool {
              SkyGradientStepData *l_Data = new SkyGradientStepData;

              PipelineUtil::GraphicsPipelineBuilder l_Builder;
              l_Builder.pipelineLayout =
                  Global::get_lighting_pipeline_layout();
              l_Builder.set_shaders("fullscreen_triangle.vert",
                                    "sky_gradient.frag");
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
                RenderView p_RenderView) -> bool { return true; });

        l_RenderStep.set_execute_callback([&](RenderStep p_RenderStep,
                                              float p_Delta,
                                              RenderView p_RenderView)
                                              -> bool {
          VkCommandBuffer l_Cmd =
              Global::get_current_command_buffer();

          VK_RENDERDOC_SECTION_BEGIN("Sky gradient",
                                     SINGLE_ARG({0.3f, 0.6f, 1.0f}));

          SkyGradientStepData *l_Data =
              (SkyGradientStepData *)p_RenderView
                  .get_step_data()[p_RenderStep.get_index()];

          ViewInfo l_ViewInfo = p_RenderView.get_view_info_handle();

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

          Image l_LitImage = p_RenderView.get_lit_image()
                                 .get_gpu()
                                 .get_data_handle();

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

          vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            l_Data->pipeline.get());

          {
            VkDescriptorSet l_GlobalSet =
                Global::get_global_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout().get(), 0, 1,
                &l_GlobalSet, 0, nullptr);

            VkDescriptorSet l_TextureSet =
                Global::get_current_texture_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout().get(), 1, 1,
                &l_TextureSet, 0, nullptr);

            VkDescriptorSet l_ViewSet =
                l_ViewInfo.get_view_data_descriptor_set();
            vkCmdBindDescriptorSets(
                l_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Global::get_lighting_pipeline_layout().get(), 2, 1,
                &l_ViewSet, 0, nullptr);

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
          }

          vkCmdEndRendering(l_Cmd);

          ImageUtil::cmd_transition(
              l_Cmd,
              p_RenderView.get_lit_image()
                  .get_gpu()
                  .get_data_handle(),
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          VK_RENDERDOC_SECTION_END();
          return true;
        });

        return true;
      }

      bool initialize_basic_rendersteps()
      {
        LOWR_VK_ASSERT_RETURN(
            initialize_shadow_pass_renderstep(),
            "Failed to initialize shadow pass renderstep");
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
        LOWR_VK_ASSERT_RETURN(initialize_ssgi_renderstep(),
                              "Failed to initialize SSGI renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_sky_gradient_renderstep(),
            "Failed to initialize sky gradient renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_tonemapping_renderstep(),
            "Failed to initialize tonemapping renderstep");
        LOWR_VK_ASSERT_RETURN(initialize_ui_renderstep(),
                              "Failed to initialize UI renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_debug_geometry_renderstep(),
            "Failed to initialize debug geometry renderstep");

        LOWR_VK_ASSERT_RETURN(
            initialize_object_id_copy_renderstep(),
            "Failed to initialize object copy renderstep");

        LOWR_VK_ASSERT_RETURN(initialize_blur_renderstep(),
                              "Failed to initialize blur renderstep");

        LOWR_VK_ASSERT_RETURN(
            initialize_pickingmap_draw_renderstep(),
            "Failed to initialize picking map draw renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_highlightmap_draw_renderstep(),
            "Failed to initialize highlight map draw renderstep");
        LOWR_VK_ASSERT_RETURN(
            initialize_highlight_edge_draw_renderstep(),
            "Failed to initialize highlight edge draw renderstep");
        return true;
      }

      bool cleanup_basic_rendersteps()
      {
        if (g_SsgiStepData.initialized) {
          if (g_SsgiStepData.noiseTex.is_alive()) {
            g_SsgiStepData.noiseTex.destroy();
          } else if (g_SsgiStepData.noiseImage.is_alive()) {
            ImageUtil::destroy(g_SsgiStepData.noiseImage);
            g_SsgiStepData.noiseImage.destroy();
          }

          BufferUtil::destroy_buffer(g_SsgiStepData.kernelBuffer);

          if (g_SsgiStepData.generationPipeline.is_alive()) {
            g_SsgiStepData.generationPipeline.destroy();
          }
          if (g_SsgiStepData.compositePipeline.is_alive()) {
            g_SsgiStepData.compositePipeline.destroy();
          }
          if (g_SsgiStepData.pipelineLayout.is_alive()) {
            g_SsgiStepData.pipelineLayout.destroy();
          }

          vkDestroyDescriptorSetLayout(
              Global::get_device(),
              g_SsgiStepData.descriptorSetLayout, nullptr);

          g_SsgiStepData.initialized = false;
        }

        // TODO: Move that into a lambda on the ssaobase renderstep
        if (!g_BaseSsaoStepData.initialized) {
          return true;
        }

        if (g_BaseSsaoStepData.noise.is_alive()) {
          g_BaseSsaoStepData.noise.destroy();
        } else if (g_BaseSsaoStepData.noiseImage.is_alive()) {
          ImageUtil::destroy(g_BaseSsaoStepData.noiseImage);
          g_BaseSsaoStepData.noiseImage.destroy();
        }

        BufferUtil::destroy_buffer(g_BaseSsaoStepData.kernelBuffer);

        if (g_BaseSsaoStepData.pipeline.is_alive()) {
          g_BaseSsaoStepData.pipeline.destroy();
        }
        if (g_BaseSsaoStepData.pipelineLayout.is_alive()) {
          g_BaseSsaoStepData.pipelineLayout.destroy();
        }

        vkDestroyDescriptorSetLayout(
            Global::get_device(),
            g_BaseSsaoStepData.descriptorSetLayout, nullptr);

        g_BaseSsaoStepData.initialized = false;
        return true;
      }
    } // namespace Vulkan
  } // namespace Renderer
} // namespace Low
