#include "LowRenderer.h"

#include "LowRendererVulkanRenderer.h"
#include "LowRendererBase.h"
#include "LowRendererResourceManager.h"
#include "LowRendererMeshResource.h"
#include "LowRendererImageResource.h"
#include "LowRendererVulkan.h"
#include "LowRendererVkImage.h"

#include "LowUtilAssert.h"

#include "imgui_impl_vulkan.h"

namespace Low {
  namespace Renderer {

    VkSampler g_TestSampler;
    Low::Renderer::ImageResource g_Img;
    VkDescriptorSet g_ImgDS;

    static void initialize_types()
    {
      MeshResource::initialize();
      ImageResource::initialize();
    }

    static void cleanup_types()
    {
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
      LOW_ASSERT(Vulkan::cleanup(),
                 "Failed to cleanup Vulkan renderer");

      cleanup_types();
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
      if (!l_ImageInit) {
        if (g_Img.get_state() == ImageResourceState::LOADED) {
          l_ImageInit = true;
          Vulkan::Image l_Img = g_Img.get_data_handle();
          g_ImgDS = ImGui_ImplVulkan_AddTexture(
              g_TestSampler, l_Img.get_allocated_image().imageView,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
      }

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
