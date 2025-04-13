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
      RenderView::initialize();
      RenderObject::initialize();
      MeshResource::initialize();
      ImageResource::initialize();
    }

    static void cleanup_types()
    {
      RenderView::cleanup();
      RenderObject::cleanup();
      ImageResource::cleanup();
      MeshResource::cleanup();
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

      VkSamplerCreateInfo sampl = {
          .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

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

    static bool upload_render_object(RenderObject p_RenderObject)
    {
      MeshResource l_MeshResource =
          p_RenderObject.get_mesh_resource();
      const u32 l_FullMeshInfoCount =
          l_MeshResource.get_full_meshinfo_count();

      size_t l_StagingOffset = 0;
      const u64 l_FrameUploadSpace =
          Vulkan::request_resource_staging_buffer_space(
              sizeof(RenderObjectUpload) * l_FullMeshInfoCount,
              &l_StagingOffset);

      if (l_FrameUploadSpace <
          (sizeof(RenderObjectUpload) * l_FullMeshInfoCount)) {
        return false;
      }

      bool l_IsFirstTimeUpload = false;

      if (!p_RenderObject.is_uploaded()) {
        u32 l_Slot = 0;
        if (!Vulkan::Global::get_renderobject_buffer().reserve(
                l_FullMeshInfoCount, &l_Slot)) {
          return false;
        }

        LOW_LOG_INFO << "Renderobject upload" << LOW_LOG_END;
        p_RenderObject.set_slot(l_Slot);
        p_RenderObject.set_uploaded(true);
        l_IsFirstTimeUpload = true;
      }

      _LOW_ASSERT(p_RenderObject.is_uploaded());

      Util::List<RenderObjectUpload> l_Uploads;
      l_Uploads.resize(l_FullMeshInfoCount);

      RenderView l_RenderView =
          p_RenderObject.get_render_view_handle();

      u32 l_Index = 0;

      for (auto sit = l_MeshResource.get_submeshes().begin();
           sit != l_MeshResource.get_submeshes().end(); ++sit) {
        for (auto mit = sit->get_meshinfos().begin();
             mit != sit->get_meshinfos().end(); ++mit) {
          // TODO: Take submesh transform into account
          l_Uploads[l_Index].world_transform =
              p_RenderObject.get_world_transform();

          if (l_IsFirstTimeUpload) {

            LOW_ASSERT(l_RenderView.add_render_entry(p_RenderObject,
                                                     l_Index, *mit),
                       "Failed to add render entry to renderview");
          }

          l_Index++;
        }
      }

      LOW_ASSERT(
          Vulkan::resource_staging_buffer_write(
              l_Uploads.data(),
              sizeof(RenderObjectUpload) * l_Uploads.size(),
              l_StagingOffset),
          "Failed to write render object data to staging buffer");

      VkBufferCopy l_CopyRegion{};
      l_CopyRegion.srcOffset = l_StagingOffset;
      l_CopyRegion.dstOffset =
          p_RenderObject.get_slot() * sizeof(RenderObjectUpload);
      l_CopyRegion.size = l_FrameUploadSpace;
      // TODO: Change to transfer queue command buffer - or leave
      // this specifically on the graphics queue not sure tbh
      vkCmdCopyBuffer(
          Vulkan::Global::get_current_command_buffer(),
          Vulkan::Global::get_current_resource_staging_buffer()
              .buffer.buffer,
          Vulkan::Global::get_renderobject_buffer().m_Buffer.buffer,
          1, &l_CopyRegion);

      LOW_LOG_INFO << "updated renderobject" << LOW_LOG_END;

      p_RenderObject.set_dirty(false);
    }

    static void tick_render_objects(float p_Delta)
    {
      for (u32 i = 0; i < RenderObject::living_count(); ++i) {
        RenderObject i_RenderObject =
            RenderObject::living_instances()[i];

        if (i_RenderObject.is_dirty()) {
          upload_render_object(i_RenderObject);
        }
      }
    }

    void tick(float p_Delta)
    {
      static bool l_ImageInit = false;

      // Resetting the resource staging buffer for this frame
      Vulkan::Global::get_current_resource_staging_buffer().occupied =
          0;

      LOW_ASSERT(Vulkan::prepare_tick(p_Delta),
                 "Failed to prepare tick Vulkan renderer");
      ResourceManager::tick(p_Delta);
      tick_render_objects(p_Delta);
      if (!l_ImageInit) {
        if (g_Img.get_state() == ImageResourceState::LOADED) {
          l_ImageInit = true;
          Vulkan::Image l_Img = g_Img.get_data_handle();
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

    MeshResource load_mesh(Util::String p_MeshPath)
    {
      MeshResource l_MeshResource = MeshResource::make(p_MeshPath);

      _LOW_ASSERT(
          ResourceManager::load_mesh_resource(l_MeshResource));

      return l_MeshResource;
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
