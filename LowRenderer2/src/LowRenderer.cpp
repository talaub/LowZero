#include "LowRenderer.h"

#include "LowRendererVulkanRenderer.h"
#include "LowRendererBase.h"
#include "LowRendererResourceManager.h"
#include "LowRendererMeshResource.h"
#include "LowRendererImageResource.h"
#include "LowRendererVulkan.h"
#include "LowRendererVkImage.h"
#include "LowRendererRenderObject.h"
#include "LowRendererGlobals.h"
#include "LowRendererRenderView.h"
#include "LowRendererMaterial.h"
#include "LowRendererTexture.h"
#include "LowRendererRenderScene.h"
#include "LowRendererDrawCommand.h"
#include "LowRendererRenderStep.h"
#include "LowRendererPointLight.h"
#include "LowRendererSubmeshGeometry.h"
#include "LowRendererMesh.h"
#include "LowRendererMeshGeometry.h"
#include "LowRendererGpuSubmesh.h"
#include "LowRendererGpuMesh.h"
#include "LowRendererUiDrawCommand.h"
#include "LowRendererUiCanvas.h"

#include "LowUtilAssert.h"

#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {

    VkSampler g_TestSampler;
    Low::Renderer::ImageResource g_Img;
    VkDescriptorSet g_ImgDS;

    static void initialize_types()
    {
      RenderStep::initialize();
      RenderScene::initialize();
      RenderView::initialize();
      RenderObject::initialize();
      Mesh::initialize();
      MeshGeometry::initialize();
      SubmeshGeometry::initialize();
      GpuMesh::initialize();
      GpuSubmesh::initialize();
      MeshResource::initialize();
      ImageResource::initialize();
      Material::initialize();
      Texture::initialize();
      DrawCommand::initialize();
      PointLight::initialize();
      UiCanvas::initialize();
      UiDrawCommand::initialize();
    }

    static void cleanup_types()
    {
      UiDrawCommand::cleanup();
      UiCanvas::cleanup();
      PointLight::cleanup();
      RenderView::cleanup();
      RenderObject::cleanup();
      ImageResource::cleanup();
      MeshResource::cleanup();
      GpuSubmesh::cleanup();
      GpuMesh::cleanup();
      SubmeshGeometry::cleanup();
      MeshGeometry::cleanup();
      Mesh::cleanup();
      Material::cleanup();
      Texture::cleanup();
      RenderScene::cleanup();
      DrawCommand::cleanup();
      RenderStep::cleanup();
    }

    void initialize()
    {
      initialize_types();
      LOW_ASSERT(Vulkan::initialize(),
                 "Failed to initialize Vulkan renderer");

      {
        Low::Util::String l_BasePath =
            Low::Util::get_project().dataPath;
        l_BasePath += "/resources/img2d/test.ktx";

        g_Img = Low::Renderer::load_image(l_BasePath);
      }

      VkSamplerCreateInfo sampl{};
      sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

      sampl.magFilter = VK_FILTER_NEAREST;
      sampl.minFilter = VK_FILTER_NEAREST;
      // sampl.minLod = 0.0f;
      // sampl.maxLod = 0.0f;
      sampl.mipmapMode =
          VK_SAMPLER_MIPMAP_MODE_LINEAR; // Enable mipmaps

      sampl.addressModeU =
          VK_SAMPLER_ADDRESS_MODE_REPEAT; // Addressing mode
      sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

      sampl.mipLodBias = 0.0f; // LOD bias for mip selection
      sampl.minLod = 0.0f;     // Minimum LOD level
      sampl.maxLod = static_cast<float>(4 - 1); // Max LOD level
      sampl.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

      vkCreateSampler(Low::Renderer::Vulkan::Global::get_device(),
                      &sampl, nullptr, &g_TestSampler);
    }

    void cleanup()
    {
      Vulkan::wait_idle();

      vkDestroySampler(Vulkan::Global::get_device(), g_TestSampler,
                       nullptr);

      cleanup_types();

      LOW_ASSERT(Vulkan::cleanup(),
                 "Failed to cleanup Vulkan renderer");
    }

    static bool update_dirty_drawcommands(float p_Delta)
    {
      bool l_Result = true;

      for (auto it = DrawCommand::ms_Dirty.begin();
           it != DrawCommand::ms_Dirty.end(); ++it) {
        DrawCommand i_DrawCommand = *it;

        if (!i_DrawCommand.is_alive() ||
            i_DrawCommand.get_render_object().is_dirty()) {
          // We will skip dead draw commands as well as ones that have
          // a dirty renderobject because then they will be updated
          // later together with their render object
          continue;
        }

        size_t l_StagingOffset = 0;
        // TODO: Reconsider if that needs to be on the resource
        // staging buffer
        const u64 l_FrameUploadSpace =
            Vulkan::request_resource_staging_buffer_space(
                sizeof(DrawCommandUpload), &l_StagingOffset);

        if (l_FrameUploadSpace < sizeof(DrawCommandUpload)) {
          // We don't have enough space on the staging buffer to
          // upload this drawcommand
          l_Result = false;
          break;
        }

        bool i_IsFirstTimeUpload = false;
        if (!i_DrawCommand.is_uploaded()) {
          LOW_ASSERT(
              !i_DrawCommand.get_render_object().is_alive(),
              "Drawcommands that are not yet uploaded should exist "
              "on their own and not as part of a renderobject");

          u32 i_Slot = 0;
          if (!Vulkan::Global::get_drawcommand_buffer().reserve(
                  1, &i_Slot)) {
            // Could not reserve space in the draw command buffer for
            // another entry
            l_Result = false;
            break;
          }

          i_DrawCommand.set_slot(i_Slot);
          i_DrawCommand.set_uploaded(true);
          i_IsFirstTimeUpload = true;
        }

        _LOW_ASSERT(i_DrawCommand.is_uploaded());

        DrawCommandUpload i_Upload;
        i_Upload.world_transform =
            i_DrawCommand.get_world_transform();

        RenderScene i_RenderScene =
            i_DrawCommand.get_render_scene_handle();

        if (i_IsFirstTimeUpload) {
          LOW_ASSERT(i_RenderScene.insert_draw_command(i_DrawCommand),
                     "Failed so add draw command to render scene");
        }

        // TODO: Check if this has to be the resource staging buffer
        LOW_ASSERT(
            Vulkan::resource_staging_buffer_write(
                &i_Upload, sizeof(DrawCommandUpload),
                l_StagingOffset),
            "Failed to write draw command data to staging buffer");

        VkBufferCopy i_CopyRegion;
        i_CopyRegion.srcOffset = l_StagingOffset;
        i_CopyRegion.dstOffset =
            i_DrawCommand.get_slot() * sizeof(DrawCommandUpload);
        i_CopyRegion.size = l_FrameUploadSpace;

        // TODO: Change to transfer queue command buffer - or leave
        // this specifically on the graphics queue not sure tbh
        // TODO: Also adjust the staging buffer if necessary
        vkCmdCopyBuffer(
            Vulkan::Global::get_current_command_buffer(),
            Vulkan::Global::get_current_resource_staging_buffer()
                .buffer.buffer,
            Vulkan::Global::get_drawcommand_buffer().m_Buffer.buffer,
            1, &i_CopyRegion);
      }

      DrawCommand::ms_Dirty.clear();
      return l_Result;
    }

    static bool update_dirty_renderobjects(float p_Delta)
    {
      bool l_Result = true;

      Util::List<RenderObject> l_RescheduleRenderObjects;

      for (auto it = RenderObject::ms_Dirty.begin();
           it != RenderObject::ms_Dirty.end(); ++it) {
        RenderObject i_RenderObject = *it;
        if (!i_RenderObject.is_alive()) {
          continue;
        }

        if (i_RenderObject.get_mesh().get_state() !=
            MeshState::LOADED) {
          // If the renderobject's meshresource has not been loaded
          // yet we reschedule its update so that we can initialize
          // everything properly
          l_RescheduleRenderObjects.push_back(i_RenderObject);
          continue;
        }

        if (i_RenderObject.get_draw_commands().empty()) {
          Mesh i_Mesh = i_RenderObject.get_mesh();
          GpuMesh i_GpuMesh = i_Mesh.get_gpu();

          for (auto sit = i_GpuMesh.get_submeshes().begin();
               sit != i_GpuMesh.get_submeshes().end(); ++sit) {
            GpuSubmesh i_GpuSubmesh = *sit;

            DrawCommand i_DrawCommand = DrawCommand::make(
                i_RenderObject,
                i_RenderObject.get_render_scene_handle(),
                i_GpuSubmesh);

            i_RenderObject.get_draw_commands().push_back(
                i_DrawCommand);
          }
        }

        const Util::List<DrawCommand> &i_DrawCommands =
            i_RenderObject.get_draw_commands();

        size_t l_StagingOffset = 0;
        // TODO: Reconsider if that needs to be on the resource
        // staging buffer
        const u64 l_FrameUploadSpace =
            Vulkan::request_resource_staging_buffer_space(
                sizeof(DrawCommandUpload) * i_DrawCommands.size(),
                &l_StagingOffset);

        if (l_FrameUploadSpace <
            (sizeof(DrawCommandUpload) * i_DrawCommands.size())) {
          // We don't have enough space on the staging buffer to
          // upload this drawcommand
          l_Result = false;
          break;
        }

        bool i_IsFirstTimeUpload = false;
        if (!i_RenderObject.is_uploaded()) {
          u32 i_Slot = 0;
          if (!Vulkan::Global::get_drawcommand_buffer().reserve(
                  i_DrawCommands.size(), &i_Slot)) {
            // Could not reserve space in the draw command buffer
            // for another entry
            l_Result = false;
            break;
          }

          i_RenderObject.set_slot(i_Slot);
          i_RenderObject.set_uploaded(true);
          i_IsFirstTimeUpload = true;
        }

        Util::List<DrawCommandUpload> i_Uploads;

        for (u32 i = 0; i < i_DrawCommands.size(); ++i) {
          DrawCommand i_DrawCommand = i_DrawCommands[i];

          if (i_IsFirstTimeUpload) {
            i_DrawCommand.set_slot(i_RenderObject.get_slot() + i);
            i_DrawCommand.set_uploaded(true);

            RenderScene i_RenderScene =
                i_DrawCommand.get_render_scene_handle();
            i_RenderScene.insert_draw_command(i_DrawCommand);
          }

          // TODO: Take submesh transform into account
          // Submesh can be fetched from the meshinfo of the draw
          // command
          i_DrawCommand.set_world_transform(
              i_RenderObject.get_world_transform());

          DrawCommandUpload i_Upload;
          i_Upload.world_transform =
              i_DrawCommand.get_world_transform();

          i_Uploads.push_back(i_Upload);
        }

        LOW_ASSERT(
            Vulkan::resource_staging_buffer_write(
                i_Uploads.data(),
                sizeof(DrawCommandUpload) * i_Uploads.size(),
                l_StagingOffset),
            "Failed to write draw command data to staging buffer");

        VkBufferCopy i_CopyRegion;
        i_CopyRegion.srcOffset = l_StagingOffset;
        i_CopyRegion.dstOffset =
            i_RenderObject.get_slot() * sizeof(DrawCommandUpload);
        i_CopyRegion.size = l_FrameUploadSpace;

        // TODO: Change to transfer queue command buffer - or leave
        // this specifically on the graphics queue not sure tbh
        // TODO: Also adjust the staging buffer if necessary
        vkCmdCopyBuffer(
            Vulkan::Global::get_current_command_buffer(),
            Vulkan::Global::get_current_resource_staging_buffer()
                .buffer.buffer,
            Vulkan::Global::get_drawcommand_buffer().m_Buffer.buffer,
            1, &i_CopyRegion);

        i_RenderObject.set_dirty(false);
      }

      RenderObject::ms_Dirty.clear();

      for (auto it = l_RescheduleRenderObjects.begin();
           it != l_RescheduleRenderObjects.end(); ++it) {
        it->set_dirty(true);
      }

      return l_Result;
    }

    static bool update_drawcommand_buffer(float p_Delta)
    {
      update_dirty_drawcommands(p_Delta);
      update_dirty_renderobjects(p_Delta);
      return true;
    }

    static bool upload_material(float p_Delta, Material p_Material)
    {
      size_t l_StagingOffset = 0;

      const u64 l_FrameUploadSpace =
          Vulkan::request_resource_staging_buffer_space(
              MATERIAL_DATA_SIZE, &l_StagingOffset);

      if (l_FrameUploadSpace < MATERIAL_DATA_SIZE) {
        return false;
      }

      LOW_ASSERT(Vulkan::resource_staging_buffer_write(
                     p_Material.get_data(), MATERIAL_DATA_SIZE,
                     l_StagingOffset),
                 "Failed to write material data to staging buffer");

      VkBufferCopy l_CopyRegion{};
      l_CopyRegion.srcOffset = l_StagingOffset;
      l_CopyRegion.dstOffset =
          p_Material.get_index() * MATERIAL_DATA_SIZE;
      l_CopyRegion.size = MATERIAL_DATA_SIZE;
      // TODO: Change to transfer queue command buffer
      vkCmdCopyBuffer(
          Vulkan::Global::get_current_command_buffer(),
          Vulkan::Global::get_current_resource_staging_buffer()
              .buffer.buffer,
          Vulkan::Global::get_material_data_buffer().buffer, 1,
          &l_CopyRegion);

      p_Material.set_dirty(false);
    }

    static void tick_materials(float p_Delta)
    {
      for (u32 i = 0; i < Material::living_count(); ++i) {
        Material i_Material = Material::living_instances()[i];

        if (!i_Material.is_dirty()) {
          continue;
        }

        if (!upload_material(p_Delta, i_Material)) {
          // If we ever fail to upload a material (most likely
          // because there is not enough space left on the staging
          // buffer), we just stop trying for this frame
          break;
        }
      }
    }

    void prepare_tick(float p_Delta)
    {
      // Resetting the resource staging buffer for this frame
      Vulkan::Global::get_current_resource_staging_buffer().occupied =
          0;

      LOW_ASSERT(Vulkan::prepare_tick(p_Delta),
                 "Failed to prepare tick Vulkan renderer");

      {
        // Setup rendersteps
        static bool l_IsInitialized = false;
        if (!l_IsInitialized) {
          l_IsInitialized = true;
          for (u32 i = 0; i < RenderStep::living_count(); ++i) {
            LOW_ASSERT(RenderStep::living_instances()[i].setup(),
                       "Failed to setup renderstep.");
          }
        }
      }
    }

    void tick(float p_Delta)
    {

      static bool l_ImageInit = false;
      ResourceManager::tick(p_Delta);
      update_drawcommand_buffer(p_Delta);
      tick_materials(p_Delta);
      if (!l_ImageInit) {
        if (g_Img.get_state() == ImageResourceState::LOADED) {
          l_ImageInit = true;
          Vulkan::Image l_Img = g_Img.get_texture().get_data_handle();
          g_ImgDS = ImGui_ImplVulkan_AddTexture(
              g_TestSampler, l_Img.get_allocated_image().imageView,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
      }

      ImGui::Begin("Camera");
      RenderView l_RenderView = RenderView::living_instances()[0];
      Math::Vector3 l_CameraPosition =
          l_RenderView.get_camera_position();
      ImGui::DragFloat3("Position", (float *)&l_CameraPosition);
      l_RenderView.set_camera_position(l_CameraPosition);

      {
        static float value = 0;
        static float value2 = 0;

        static float split = 150.0f;
        float splitter_thickness = 4.0f;
        ImVec2 region = ImGui::GetContentRegionAvail();

        // Left panel: label column
        ImGui::BeginChild("LeftColumn", ImVec2(split, 0), false);
        ImGui::Text("Property Name");
        ImGui::EndChild();

        // Splitter: draw and interact
        ImGui::SameLine();
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Normal
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.7f, 0.7f, 0.7f, 1.0f)); // Hover
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Dragging
        ImGui::Button("##Splitter",
                      ImVec2(splitter_thickness, region.y));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemActive())
          split += ImGui::GetIO().MouseDelta.x;

        ImGui::SetCursorPosX(
            split + splitter_thickness); // Position right of splitter

        // Right panel: value column
        ImGui::SameLine();
        ImGui::BeginChild("RightColumn", ImVec2(0, 0), false);
        ImGui::InputFloat("##Value2", &value2);
        ImGui::EndChild();

        ImGui::BeginChild("LeftColumn", ImVec2(split, 0), false);
        ImGui::Text("Property2");
        ImGui::EndChild();

        // Splitter: draw and interact
        /*
        ImGui::SameLine();
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Normal
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.7f, 0.7f, 0.7f, 1.0f)); // Hover
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Dragging
        ImGui::Button("##Splitter",
                      ImVec2(splitter_thickness, region.y));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemActive())
          split += ImGui::GetIO().MouseDelta.x;

        ImGui::SetCursorPosX(
            split + splitter_thickness); // Position right of splitter
            */

        // Right panel: value column
        ImGui::SameLine();
        ImGui::BeginChild("RightColumn", ImVec2(0, 0), false);
        ImGui::InputFloat("##Value", &value);
        ImGui::EndChild();
      }
      ImGui::End();

      ImGui::Begin("Normals");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_normals()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("Depth");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_depth()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("View Position");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_viewposition()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("Albedo");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_albedo()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();

      if (l_ImageInit) {
        ImGui::Begin("Viewport");

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ImGui::Image(
            (ImTextureID)g_ImgDS,
            ImVec2{viewportPanelSize.x, viewportPanelSize.y});

        ImGui::End();
      }

      LOW_ASSERT(Vulkan::tick(p_Delta),
                 "Failed to tick Vulkan renderer");
    }

    void check_window_resize(float p_Delta)
    {
      LOW_ASSERT(
          Vulkan::check_window_resize(p_Delta),
          "Failed to check for window resize in Vulkan renderer");
    }

    Mesh load_mesh(Util::String p_MeshPath)
    {
      MeshResource l_MeshResource = MeshResource::make(p_MeshPath);
      Mesh l_Mesh = Mesh::make(l_MeshResource.get_name());
      l_Mesh.set_resource(l_MeshResource);

      _LOW_ASSERT(ResourceManager::load_mesh(l_Mesh));

      return l_Mesh;
    }

    ImageResource load_image(Util::String p_ImagePath)
    {
      ImageResource l_ImageResource =
          ImageResource::make(p_ImagePath);

      _LOW_ASSERT(
          ResourceManager::load_image_resource(l_ImageResource));

      return l_ImageResource;
    }
  } // namespace Renderer
} // namespace Low
