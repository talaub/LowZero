#include "LowRendererVulkanRenderer.h"

#include "LowRendererVulkanBase.h"
#include "LowRendererCompatibility.h"

#include "LowRendererVulkanPipeline.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVulkanInit.h"
#include "LowRendererVulkanPipelineManager.h"

#include "LowUtil.h"

#include <vulkan/vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "LowRendererVkPipeline.h"

namespace Low {
  namespace Renderer {
    namespace Vulkan {
      Context g_Context;

      struct ComputePushConstants
      {
        Math::Vector4 data1;
        Math::Vector4 data2;
        Math::Vector4 data3;
        Math::Vector4 data4;
      };

      struct
      {
        VkPipeline gradientPipeline;
        VkPipelineLayout gradientPipelineLayout;

        VkPipelineLayout trianglePipelineLayout;
        Pipeline trianglePipeline;
      } g_Pipelines;

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
            p_Context.device, &l_ComputeLayout, nullptr,
            &g_Pipelines.gradientPipelineLayout));

        Util::String l_ComputeShaderPath =
            Util::get_project().engineDataPath +
            "/lowr_shaders/gradient_color.comp.spv";

        VkShaderModule l_ComputeDrawShader;
        if (!PipelineUtil::load_shader_module(
                l_ComputeShaderPath.c_str(), p_Context.device,
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
            p_Context.device, VK_NULL_HANDLE, 1,
            &l_ComputePipelineCreateInfo, nullptr,
            &g_Pipelines.gradientPipeline));

        // Destroying the shader module
        vkDestroyShaderModule(p_Context.device, l_ComputeDrawShader,
                              nullptr);

        return true;
      }

      bool triangle_pipeline_init(Context &p_Context)
      {
        Util::String l_VertexShaderPath = "colored_triangle.vert";
        Util::String l_FragmentShaderPath = "colored_triangle.frag";

        VkPipelineLayoutCreateInfo l_PipelineLayoutInfo =
            PipelineUtil::layout_create_info();
        LOWR_VK_CHECK_RETURN(vkCreatePipelineLayout(
            p_Context.device, &l_PipelineLayoutInfo, nullptr,
            &g_Pipelines.trianglePipelineLayout));

        // Create pipeline
        PipelineUtil::GraphicsPipelineBuilder l_Builder;
        l_Builder.pipelineLayout = g_Pipelines.trianglePipelineLayout;
        l_Builder.set_shaders(p_Context, l_VertexShaderPath,
                              l_FragmentShaderPath);
        l_Builder.set_input_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        l_Builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        l_Builder.set_cull_mode(VK_CULL_MODE_NONE,
                                VK_FRONT_FACE_CLOCKWISE);
        l_Builder.set_multismapling_none();
        l_Builder.disable_blending();
        l_Builder.disable_depth_test();

        l_Builder.set_color_attachment_format(
            p_Context.swapchain.drawImage.format);
        l_Builder.set_depth_format(VK_FORMAT_UNDEFINED);

        g_Pipelines.trianglePipeline =
            l_Builder.register_pipeline(p_Context);

        return true;
      }

      bool setup_renderflow(Context &p_Context)
      {
        bg_pipelines_init(p_Context);
        triangle_pipeline_init(p_Context);
        return true;
      }

      static void initialize_types()
      {
        Pipeline::initialize();
      }

      static void cleanup_types()
      {
        Pipeline::cleanup();
      }

      bool initialize()
      {
        initialize_types();

        int l_Width, l_Height;
        SDL_GetWindowSize(Util::Window::get_main_window().sdlwindow,
                          &l_Width, &l_Height);

        LOWR_VK_ASSERT_RETURN(
            Base::initialize(g_Context,
                             Math::UVector2(l_Width, l_Height)),
            "Failed to initialize vulkan renderer");

        LOWR_VK_ASSERT_RETURN(setup_renderflow(g_Context),
                              "Failed to setup renderflow");

        return true;
      }

      bool bg_pipelines_cleanup(const Context &p_Context)
      {
        vkDestroyPipelineLayout(p_Context.device,
                                g_Pipelines.gradientPipelineLayout,
                                nullptr);
        vkDestroyPipeline(p_Context.device,
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

      bool cleanup()
      {
        vkDeviceWaitIdle(g_Context.device);

        cleanup_renderflow(g_Context);

        cleanup_types();

        LOWR_VK_ASSERT_RETURN(Base::cleanup(g_Context),
                              "Failed to cleanup vulkan renderer");

        return true;
      }

      bool geometry_draw(Context &p_Context)
      {
        VkCommandBuffer l_Cmd =
            p_Context.get_current_frame().mainCommandBuffer;

        // begin a render pass  connected to our draw image
        VkRenderingAttachmentInfo l_ColorAttachment =
            InitUtil::attachment_info(
                p_Context.swapchain.drawImage.imageView, nullptr,
                VK_IMAGE_LAYOUT_GENERAL);

        VkRenderingInfo l_RenderInfo =
            InitUtil::rendering_info(p_Context.swapchain.drawExtent,
                                     &l_ColorAttachment, nullptr);
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

      static bool draw(Context &p_Context, float p_Delta)
      {
        VkCommandBuffer l_Cmd =
            p_Context.get_current_frame().mainCommandBuffer;

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

#if 0
        float l_Flash = abs(sin(p_Context.frameNumber / 120.f));
        Math::Color l_ClearColor = {0.0f, 0.0f, l_Flash, 1.0f};

        ImageUtil::cmd_clear_color(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_GENERAL, l_ClearColor);
#else
        vkCmdBindPipeline(l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          g_Pipelines.gradientPipeline);

        vkCmdBindDescriptorSets(l_Cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                g_Pipelines.gradientPipelineLayout, 0,
                                1, &p_Context.drawImageDescriptors, 0,
                                nullptr);

        ComputePushConstants l_PushConstants;
        l_PushConstants.data1 = Math::Vector4(1, 0, 0, 1);
        l_PushConstants.data2 = Math::Vector4(0, 0, 1, 1);

        vkCmdPushConstants(l_Cmd, g_Pipelines.gradientPipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           sizeof(ComputePushConstants),
                           &l_PushConstants);

        vkCmdDispatch(
            l_Cmd,
            static_cast<u32>(std::ceil(
                p_Context.swapchain.drawExtent.width / 16.0)),
            static_cast<u32>(std::ceil(
                p_Context.swapchain.drawExtent.height / 16.0f)),
            1);
#endif

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        geometry_draw(p_Context);

        ImageUtil::cmd_transition(
            l_Cmd, p_Context.swapchain.drawImage.image,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        ImageUtil::cmd_transition(
            l_Cmd,
            p_Context.swapchain
                .images[p_Context.swapchain.imageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        return true;
      }

      bool tick(float p_Delta)
      {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();

        PipelineManager::tick(g_Context, p_Delta);

        bool l_Result = Base::context_prepare_draw(g_Context);

        if (l_Result) {
          LOWR_VK_ASSERT_RETURN(draw(g_Context, p_Delta),
                                "Failed to draw vulkan renderer");

          Base::context_present(g_Context);
        }
        return true;
      }

      bool check_window_resize(float p_Delta)
      {
        LOWR_VK_ASSERT_RETURN(
            Base::swapchain_resize(g_Context),
            "Failed to swapchain_resize vulkan renderer");
        return true;
      }
    } // namespace Vulkan
  }   // namespace Renderer
} // namespace Low