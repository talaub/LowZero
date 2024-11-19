#include "LowRendererResourceManager.h"

#include "LowRendererMeshResourceState.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVkImage.h"
#include "LowRendererGlobals.h"

#include "LowUtilContainers.h"
#include "LowUtilJobManager.h"
#include "LowUtilAssert.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {
    namespace ResourceManager {

      struct
      {
        Util::Set<MeshResource> meshes;
        Util::Set<ImageResource> images;
      } g_LoadSchedules;

      struct UploadEntry
      {
        union
        {
          struct
          {
            u8 progressPriority;
            u8 lodPriority;
            u16 waitPriority;
            u32 resourcePriority;
          };
          u64 priority;
        };

        struct
        {
          struct
          {
            ImageResource imageResource;
            u64 uploadedSize;
          } image;
          struct
          {
            MeshResource meshResource;
            u32 submeshIndex;
          } submesh;
        } data;

        bool is_image_upload()
        {
          return ImageResource::is_alive(data.image.imageResource);
        }

        bool is_submesh_upload()
        {
          return MeshResource::is_alive(data.submesh.meshResource);
        }
      };

      auto g_UploadEntryComparator = [](const UploadEntry &p_A,
                                        const UploadEntry &p_B) {
        return p_A.priority > p_B.priority;
      };

      Util::List<UploadEntry> g_UploadSchedulesContainer;

      Util::PriorityQueue<UploadEntry,
                          decltype(g_UploadEntryComparator)>
          g_UploadSchedules(g_UploadEntryComparator,
                            g_UploadSchedulesContainer);

      static bool image_can_load(ImageResource p_ImageResource)
      {
        return p_ImageResource.get_state() ==
               ImageResourceState::UNLOADED;
      }

      bool load_image_resource(ImageResource p_ImageResource)
      {
        // Skip out on scheduling the load for this image resource
        // because it is either already loaded or in the process of
        // being loaded/unloaded
        if (!image_can_load(p_ImageResource)) {
          return false;
        }

        g_LoadSchedules.images.insert(p_ImageResource);

        return true;
      }

      bool unload_image_resource(ImageResource p_ImageResource)
      {
        return true;
      }

      static bool mesh_can_load(MeshResource p_MeshResource)
      {
        return p_MeshResource.get_state() ==
               MeshResourceState::UNLOADED;
      }

      bool load_mesh_resource(MeshResource p_MeshResource)
      {
        // Skip out on scheduling the load for this mesh resource
        // because it is either already loaded or in the process of
        // being loaded/unloaded
        if (!mesh_can_load(p_MeshResource)) {
          return false;
        }

        g_LoadSchedules.meshes.insert(p_MeshResource);

        return true;
      }

      bool unload_mesh_resource(MeshResource p_MeshResource)
      {
        return true;
      }

      static bool
      image_schedule_gpu_upload(ImageResource p_ImageResource)
      {
        Util::Resource::ImageMipMaps &l_ImageMipmaps =
            p_ImageResource.get_resource_image();

        Math::UVector2 l_Dimension =
            p_ImageResource.get_resource_image().mip0.dimensions;

        for (int i = IMAGE_MIPMAP_COUNT - 1; i >= 0; --i) {
          UploadEntry i_Entry;
          i_Entry.data.image.uploadedSize = 0;
          i_Entry.data.image.imageResource = p_ImageResource;
          i_Entry.progressPriority = 0;
          i_Entry.lodPriority = i;
          i_Entry.waitPriority = 0;
          i_Entry.resourcePriority = 0;
          g_UploadSchedules.emplace(i_Entry);
        }

        Vulkan::Image l_Image =
            Vulkan::Image::make(p_ImageResource.get_name());

        VkExtent3D l_Extent;
        // Uses the mip0 extent since it's the full resolution
        l_Extent.width = l_ImageMipmaps.mip0.dimensions.x;
        l_Extent.height = l_ImageMipmaps.mip0.dimensions.y;
        l_Extent.depth = 1;

        Vulkan::AllocatedImage l_AllocatedImage =
            Vulkan::ImageUtil::create(
                l_Extent, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT, true);

        l_Image.set_allocated_image(l_AllocatedImage);

        p_ImageResource.set_data_handle(l_Image.get_id());

        // TODO: Should be changed to transfer command buffer
        Vulkan::ImageUtil::cmd_transition(
            Vulkan::Global::get_current_command_buffer(),
            l_AllocatedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        p_ImageResource.set_state(ImageResourceState::UPLOADINGTOGPU);
        return true;
      }

      static bool
      image_schedule_memory_load(ImageResource p_ImageResource)
      {
        // Small hack. Passing the ImageResource in there directly
        // does not work for some reason so we're passing in the ID
        // and converting it back to a ImageResource in the job
        u64 l_ImageResourceId = p_ImageResource.get_id();
        // Overall this schedules a job that will load the image data
        // into the Util::Resource::ImageMipMaps instance stored on
        // the MeshResource and we'll set the state of the
        // ImageResource to MEMORYLOADED once this is done.
        Util::JobManager::default_pool().enqueue(
            [l_ImageResourceId]() {
              ImageResource l_ImageResource = l_ImageResourceId;
              Util::Resource::load_image_mipmaps(
                  l_ImageResource.get_path(),
                  l_ImageResource.get_resource_image());
              l_ImageResource.set_state(
                  ImageResourceState::MEMORYLOADED);
            });

        p_ImageResource.set_state(
            ImageResourceState::LOADINGTOMEMORY);
        return true;
      }

      static bool
      mesh_schedule_memory_load(MeshResource p_MeshResource)
      {
        // Small hack. Passing the MeshResource in there directly does
        // not work for some reason so we're passing in the ID and
        // converting it back to a MeshResource in the job
        u64 l_MeshResourceId = p_MeshResource.get_id();
        // Overall this schedules a job that will load the mesh data
        // into the Util::Resource::Mesh instance stored on the
        // MeshResource and we'll set the state of the MeshResource to
        // MEMORYLOADED once this is done.
        Util::JobManager::default_pool().enqueue(
            [l_MeshResourceId]() {
              MeshResource l_MeshResource = l_MeshResourceId;
              Util::Resource::load_mesh(
                  l_MeshResource.get_path(),
                  l_MeshResource.get_resource_mesh());
              l_MeshResource.set_state(
                  MeshResourceState::MEMORYLOADED);
            });

        p_MeshResource.set_state(MeshResourceState::LOADINGTOMEMORY);
        return true;
      }

      static bool
      mesh_schedule_gpu_upload(MeshResource p_MeshResource)
      {
        int l_Index = 0;
        for (auto it =
                 p_MeshResource.get_resource_mesh().submeshes.begin();
             it != p_MeshResource.get_resource_mesh().submeshes.end();
             ++it) {

          UploadEntry i_Entry;
          i_Entry.data.submesh.meshResource = p_MeshResource;
          i_Entry.data.submesh.submeshIndex = l_Index;
          i_Entry.progressPriority = 0;
          i_Entry.lodPriority = 0;
          i_Entry.waitPriority = 0;
          i_Entry.resourcePriority = 0;
          g_UploadSchedules.emplace(i_Entry);

          l_Index++;
        }

        p_MeshResource.set_state(MeshResourceState::UPLOADINGTOGPU);
        return true;
      }

      static bool meshes_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.meshes.begin();
             it != g_LoadSchedules.meshes.end(); ++it) {
          if (it->get_state() == MeshResourceState::MEMORYLOADED) {
            // As soon as we see that the meshresource has
            // successfully been loaded to memory we'll start
            // uploading the data to the GPU
            if (!mesh_schedule_gpu_upload(*it)) {
              LOW_LOG_ERROR << "Failed to schedule MeshResource '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
            }
            continue;
          } else if (it->get_state() == MeshResourceState::UNLOADED) {
            // If the MeshResource is scheduled to be loaded but
            // currently still marked as unloaded we will try to
            // schedule the memory load.
            if (!mesh_schedule_memory_load(*it)) {
              LOW_LOG_ERROR << "Failed to schedule MeshResource '"
                            << it->get_name()
                            << "' for loading to memory."
                            << LOW_LOG_END;
            }
            continue;
          }
        }

        return true;
      }

      static bool images_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.images.begin();
             it != g_LoadSchedules.images.end();) {
          if (it->get_state() == ImageResourceState::MEMORYLOADED) {
            // As soon as we see that the imageresource has
            // successfully been loaded to memory we'll start
            // uploading the data to the GPU
            if (image_schedule_gpu_upload(*it)) {
              it = g_LoadSchedules.images.erase(it);
            } else {
              LOW_LOG_ERROR << "Failed to schedule image '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
              it++;
            }
            continue;
          } else if (it->get_state() ==
                     ImageResourceState::UNLOADED) {
            // If the ImageResource is scheduled to be loaded but
            // currently still marked as unloaded we will try to
            // schedule the memory load.
            if (!image_schedule_memory_load(*it)) {
              LOW_LOG_ERROR << "Failed to schedule ImageResource '"
                            << it->get_name()
                            << "' for loading to memory."
                            << LOW_LOG_END;
            }
            continue;
          }
        }

        return true;
      }

      static bool mesh_upload(UploadEntry &p_UploadEntry)
      {
        return true;
      }

      static bool image_upload(UploadEntry &p_UploadEntry)
      {
        ImageResource l_ImageResource =
            p_UploadEntry.data.image.imageResource;

        Util::Resource::Image2D &l_Mip =
            l_ImageResource.get_resource_image().mip0;

        // Fetch the correct Util::Resource::Image2D instance for the
        // mip level requested
        switch (p_UploadEntry.lodPriority) {
        case 1:
          l_Mip = l_ImageResource.get_resource_image().mip1;
          break;
        case 2:
          l_Mip = l_ImageResource.get_resource_image().mip2;
          break;
        case 3:
          l_Mip = l_ImageResource.get_resource_image().mip3;
          break;
        }

        const u64 l_FullSize = l_Mip.size;
        const u64 l_UploadedSize =
            p_UploadEntry.data.image.uploadedSize;

        u8 *l_ImageData = (u8 *)l_Mip.data.data();

        const u64 l_SpaceRequired = l_FullSize - l_UploadedSize;

        size_t l_StagingOffset = 0;

        // Request staging buffer space
        const u64 l_FrameUploadSpace =
            Vulkan::request_resource_staging_buffer_space(
                l_SpaceRequired, &l_StagingOffset);

        // Starting to calculate and set the progress percent of the
        // resource. This is important so that resources that are
        // almost done uploading get finished first.
        const float l_UploadProgress =
            (((float)l_UploadedSize) + ((float)l_FrameUploadSpace)) /
            ((float)l_FullSize);
        const u8 l_UploadProgressPercent =
            Math::Util::floor(l_UploadProgress * 100.0f);

        // Upload data to staging buffer
        LOW_ASSERT(Vulkan::resource_staging_buffer_write(
                       &l_ImageData[l_UploadedSize],
                       l_FrameUploadSpace, l_StagingOffset),
                   "Failed to write image data to staging buffer");

        {
          Vulkan::Image l_Image = l_ImageResource.get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          int32_t l_ImagePixelOffset =
              static_cast<int32_t>(l_UploadedSize) /
              IMAGE_CHANNEL_COUNT;
          u32 l_PixelUpload = static_cast<u32>(l_FrameUploadSpace) /
                              IMAGE_CHANNEL_COUNT;

          const u32 l_ImageWidth = l_Mip.dimensions.x;

          Util::List<VkBufferImageCopy> l_Regions;

          while (l_PixelUpload > 0) {
            VkBufferImageCopy &i_Region = l_Regions.emplace_back();

            i_Region.bufferOffset = l_StagingOffset;

            i_Region.imageSubresource.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
            i_Region.imageSubresource.mipLevel =
                p_UploadEntry.lodPriority;
            i_Region.imageSubresource.baseArrayLayer = 0;
            i_Region.imageSubresource.layerCount =
                1; // No cubemap etc.

            int32_t i_PixelsAlreadyPresentInRow =
                l_ImagePixelOffset % l_ImageWidth;
            int32_t i_RowsAlreadyCopied =
                l_ImagePixelOffset / l_ImageWidth;

            i_Region.imageOffset = {
                i_PixelsAlreadyPresentInRow, i_RowsAlreadyCopied,
                0}; // Start row-by-pixel offset here.

            u32 i_PixelsCopiedThisRegion =
                l_ImageWidth - i_PixelsAlreadyPresentInRow;
            i_Region.imageExtent = {
                i_PixelsCopiedThisRegion, 1,
                1}; // Exact only small portions uploaded

            l_ImagePixelOffset += i_PixelsCopiedThisRegion;
            l_StagingOffset +=
                i_PixelsCopiedThisRegion * IMAGE_CHANNEL_COUNT;
            l_PixelUpload -= i_PixelsCopiedThisRegion;
          }

          if (!l_Regions.empty()) {
            // TODO: Change to transfer queue command buffer

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(),
                l_Image.get_allocated_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            vkCmdCopyBufferToImage(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                l_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                l_Regions.size(), l_Regions.data());

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(),
                l_Image.get_allocated_image(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }
        }

        p_UploadEntry.data.image.uploadedSize += l_FrameUploadSpace;

        p_UploadEntry.progressPriority = l_UploadProgressPercent;

        return l_FrameUploadSpace > 0;
      }

      static bool conclude_resource_upload(UploadEntry &p_UploadEntry)
      {
        if (p_UploadEntry.is_image_upload()) {
          p_UploadEntry.data.image.imageResource.set_state(
              ImageResourceState::LOADED);

          Vulkan::Image l_Image =
              p_UploadEntry.data.image.imageResource
                  .get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          // TODO: Check if this can stay graphics queue command
          // buffer or needs changing. Maybe we also need additional
          // synchronization
          Vulkan::ImageUtil::cmd_transition(
              Vulkan::Global::get_current_command_buffer(),
              l_Image.get_allocated_image(),
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          // Add the loaded mip to the list of mips loaded and sort
          // said list so that it is easier later on to find the
          // lowest and highest mip loaded for this specific image
          p_UploadEntry.data.image.imageResource.loaded_mips()
              .push_back(p_UploadEntry.lodPriority);
          std::sort(
              p_UploadEntry.data.image.imageResource.loaded_mips()
                  .begin(),
              p_UploadEntry.data.image.imageResource.loaded_mips()
                  .end());

          LOW_LOG_DEBUG << "Image fully loaded" << LOW_LOG_END;
        }
        /*
        else if (p_UploadEntry.mesh.is_alive()){
          p_UploadEntry.mesh.set_state(MeshResourceState::LOADED);
        }
        */

        return true;
      }

      static bool upload_resource(UploadEntry &p_UploadEntry)
      {
        volatile int i = 0;
        if (p_UploadEntry.is_image_upload() &&
            p_UploadEntry.progressPriority < 100) {
          return image_upload(p_UploadEntry);
        } else if (p_UploadEntry.is_submesh_upload()) {
          return mesh_upload(p_UploadEntry);
        }

        LOW_ASSERT(false, "ResourceManager encountered unknown "
                          "resource type to upload");
      }

      static bool uploads_tick(float p_Delta)
      {
        bool l_ContinueUploading = true;
        while (!g_UploadSchedules.empty() && l_ContinueUploading) {
          l_ContinueUploading =
              upload_resource((UploadEntry &)g_UploadSchedules.top());

          // If the top most resource ahs been fully uploaded, pop it
          // from the list and move on to the next
          if (g_UploadSchedules.top().progressPriority >= 100) {
            conclude_resource_upload(
                (UploadEntry &)g_UploadSchedules.top());
            g_UploadSchedules.pop();
          }
        }

        for (auto &i_Entry : g_UploadSchedulesContainer) {
          i_Entry.waitPriority++;
        }
        return true;
      }

      void tick(float p_Delta)
      {
        _LOW_ASSERT(meshes_tick(p_Delta));
        _LOW_ASSERT(images_tick(p_Delta));
        _LOW_ASSERT(uploads_tick(p_Delta));
      }
    } // namespace ResourceManager
  }   // namespace Renderer
} // namespace Low
