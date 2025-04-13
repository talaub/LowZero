#include "LowRendererResourceManager.h"

#include "LowRendererMeshResourceState.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVkImage.h"
#include "LowRendererGlobals.h"
#include "LowRendererSubmesh.h"
#include "LowRendererMeshInfo.h"
#include "LowRendererVulkan.h"

#include "LowUtilContainers.h"
#include "LowUtilJobManager.h"
#include "LowUtilAssert.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

#define MESH_INDEX_SIZE sizeof(u32)

namespace Low {
  namespace Renderer {
    namespace ResourceManager {

      struct MeshEntry
      {
        union
        {
          u32 lodPriority;
          u32 resourcePriority;
        };
        u64 priority;

        MeshResource meshResource;

        u32 submeshCount;
      };

      auto g_MeshEntryComparator = [](const MeshEntry &p_A,
                                      const MeshEntry &p_B) {
        return p_A.priority > p_B.priority;
      };

      Util::List<MeshEntry> g_MeshEntriesContainer;

      Util::PriorityQueue<MeshEntry, decltype(g_MeshEntryComparator)>
          g_MeshEntries(g_MeshEntryComparator,
                        g_MeshEntriesContainer);

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
        u64 uploadedSize;
        struct
        {
          struct
          {
            ImageResource imageResource;
          } image;
          struct
          {
            MeshResource meshResource;
            Submesh submesh;
            MeshInfo meshInfo;
            u32 submeshIndex;
            u32 meshInfoIndex;
          } mesh;
        } data;

        bool is_image_upload()
        {
          return ImageResource::is_alive(data.image.imageResource);
        }

        bool is_mesh_upload()
        {
          return MeshResource::is_alive(data.mesh.meshResource);
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
        // TODO: Implement
        LOW_ASSERT(false, "Tried to unload mesh resource but the "
                          "method is not yet implemented");
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
          i_Entry.uploadedSize = 0;
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

              for (auto it = l_MeshResource.get_resource_mesh()
                                 .submeshes.begin();
                   it !=
                   l_MeshResource.get_resource_mesh().submeshes.end();
                   ++it) {
                Submesh i_Submesh = Submesh::make(it->name);
                i_Submesh.set_meshinfo_count(it->meshInfos.size());
                i_Submesh.set_state(MeshResourceState::MEMORYLOADED);
                i_Submesh.set_uploaded_meshinfo_count(0);

                for (auto mit = it->meshInfos.begin();
                     mit != it->meshInfos.end(); ++mit) {
                  MeshInfo i_MeshInfo = MeshInfo::make(it->name);
                  i_MeshInfo.set_vertex_count(mit->vertices.size());
                  i_MeshInfo.set_index_count(mit->indices.size());
                  i_MeshInfo.set_uploaded_vertex_count(0);
                  i_MeshInfo.set_uploaded_index_count(0);
                  i_MeshInfo.set_state(
                      MeshResourceState::MEMORYLOADED);
                  i_Submesh.get_meshinfos().push_back(i_MeshInfo);
                }
                l_MeshResource.get_submeshes().push_back(i_Submesh);
              }

              l_MeshResource.set_state(
                  MeshResourceState::MEMORYLOADED);
            });

        p_MeshResource.set_state(MeshResourceState::LOADINGTOMEMORY);
        return true;
      }

      static bool
      mesh_schedule_gpu_upload(MeshResource p_MeshResource)
      {
        p_MeshResource.set_submesh_count(
            p_MeshResource.get_resource_mesh().submeshes.size());
        p_MeshResource.set_uploaded_submesh_count(0);

        u32 l_FullMeshInfoCount = 0;

        int l_Index = 0;
        for (auto it =
                 p_MeshResource.get_resource_mesh().submeshes.begin();
             it != p_MeshResource.get_resource_mesh().submeshes.end();
             ++it) {

          int i_MeshIndex = 0;

          for (auto mit = it->meshInfos.begin();
               mit != it->meshInfos.end(); ++mit) {

            UploadEntry i_Entry;
            i_Entry.data.mesh.meshResource = p_MeshResource;
            i_Entry.data.mesh.submeshIndex = l_Index;
            i_Entry.data.mesh.meshInfoIndex = i_MeshIndex;
            i_Entry.data.mesh.submesh =
                p_MeshResource.get_submeshes()[l_Index];
            i_Entry.data.mesh.meshInfo =
                i_Entry.data.mesh.submesh
                    .get_meshinfos()[i_MeshIndex];
            i_Entry.progressPriority = 0;
            i_Entry.lodPriority = 0;
            i_Entry.waitPriority = 0;
            i_Entry.resourcePriority = 0;
            i_Entry.uploadedSize = 0;
            g_UploadSchedules.emplace(i_Entry);

            i_MeshIndex++;
            l_FullMeshInfoCount++;
          }

          l_Index++;
        }

        p_MeshResource.set_full_meshinfo_count(l_FullMeshInfoCount);
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

      static bool request_mesh_buffer_space(
          Vulkan::DynamicBuffer &p_DynamicBuffer, u64 p_Priority,
          u32 p_EntryCount, u32 *p_OutStart)
      {
        if (!p_DynamicBuffer.reserve(p_EntryCount, p_OutStart)) {
          if (g_MeshEntries.empty()) {
            return false;
          }

          if (g_MeshEntries.top().priority >= p_Priority) {
            return false;
          }

          unload_mesh_resource(g_MeshEntries.top().meshResource);
        }

        return true;
      }

      static bool
      request_mesh_buffer_space(UploadEntry &p_UploadEntry,
                                u32 p_VertexCount, u32 p_IndexCount,
                                u32 *p_OutVertexStart,
                                u32 *p_OutIndexStart)
      {
        MeshEntry l_MeshEntry;
        l_MeshEntry.lodPriority = p_UploadEntry.lodPriority;
        l_MeshEntry.resourcePriority = p_UploadEntry.resourcePriority;

        if (!request_mesh_buffer_space(
                Vulkan::Global::get_mesh_vertex_buffer(),
                l_MeshEntry.priority, p_VertexCount,
                p_OutVertexStart)) {
          return false;
        }
        if (!request_mesh_buffer_space(
                Vulkan::Global::get_mesh_index_buffer(),
                l_MeshEntry.priority, p_IndexCount,
                p_OutIndexStart)) {
          return false;
        }

        return true;
      }

      static bool mesh_upload(UploadEntry &p_UploadEntry)
      {
        MeshInfo l_RendererMeshInfo =
            p_UploadEntry.data.mesh.meshInfo;
        Submesh l_RendererSubmesh = p_UploadEntry.data.mesh.submesh;
        {

          if (l_RendererMeshInfo.get_state() ==
              MeshResourceState::MEMORYLOADED) {
            l_RendererMeshInfo.set_state(
                MeshResourceState::UPLOADINGTOGPU);

            u32 l_VertexStart;
            u32 l_IndexStart;
            if (!request_mesh_buffer_space(
                    p_UploadEntry,
                    l_RendererMeshInfo.get_vertex_count(),
                    l_RendererMeshInfo.get_index_count(),
                    &l_VertexStart, &l_IndexStart)) {
              LOW_LOG_WARN << "Could not load mesh since mesh buffer "
                              "space could not be reserved"
                           << LOW_LOG_END;
              return true;
            }

            l_RendererMeshInfo.set_vertex_start(l_VertexStart);
            l_RendererMeshInfo.set_index_start(l_IndexStart);
          }

          if (l_RendererSubmesh.get_state() ==
              MeshResourceState::MEMORYLOADED) {
            l_RendererSubmesh.set_state(
                MeshResourceState::UPLOADINGTOGPU);
          }
        }

        MeshResource l_MeshResource =
            p_UploadEntry.data.mesh.meshResource;
        Util::Resource::Submesh &l_Submesh =
            l_MeshResource.get_resource_mesh()
                .submeshes[p_UploadEntry.data.mesh.submeshIndex];
        Util::Resource::MeshInfo &l_MeshInfo =
            l_Submesh
                .meshInfos[p_UploadEntry.data.mesh.meshInfoIndex];

        u64 l_VertexDataSize = l_MeshInfo.vertices.size() *
                               sizeof(Low::Util::Resource::Vertex);
        u64 l_IndexDataSize =
            l_MeshInfo.indices.size() * MESH_INDEX_SIZE;

        if (p_UploadEntry.data.mesh.meshInfo
                .get_uploaded_vertex_count() <
            l_MeshInfo.vertices.size()) {
          const u64 l_FullSize = l_VertexDataSize;
          const u64 l_UploadedSize = p_UploadEntry.uploadedSize;

          u8 *l_Data = (u8 *)l_MeshInfo.vertices.data();

          const u64 l_SpaceRequired = l_FullSize - l_UploadedSize;

          size_t l_StagingOffset = 0;

          // Request staging buffer space
          const u64 l_FrameUploadSpace =
              Vulkan::request_resource_staging_buffer_space(
                  l_SpaceRequired, &l_StagingOffset);

          const float l_UploadProgress =
              (((float)l_UploadedSize) +
               ((float)l_FrameUploadSpace)) /
              ((float)l_FullSize);
          // Calculate the progress as percent. We divide it by 2
          // because we upload the vertices first and we still need to
          // upload the indices after that.
          const u8 l_UploadProgressPercent =
              Math::Util::floor(l_UploadProgress * 100.0f) / 2.0f;

          // Upload data to staging buffer
          LOW_ASSERT(
              Vulkan::resource_staging_buffer_write(
                  &l_Data[l_UploadedSize], l_FrameUploadSpace,
                  l_StagingOffset),
              "Failed to write mesh vertex data to staging buffer");

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = l_StagingOffset;
          l_CopyRegion.dstOffset =
              l_RendererMeshInfo.get_vertex_start();
          l_CopyRegion.size = l_FrameUploadSpace;
          // TODO: Change to transfer queue command buffer
          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_current_resource_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_mesh_vertex_buffer()
                  .m_Buffer.buffer,
              1, &l_CopyRegion);

          p_UploadEntry.uploadedSize += l_FrameUploadSpace;
          p_UploadEntry.data.mesh.meshInfo.set_uploaded_vertex_count(
              p_UploadEntry.uploadedSize /
              sizeof(Util::Resource::Vertex));
          p_UploadEntry.progressPriority = l_UploadProgressPercent;

          return l_FrameUploadSpace > 0.0f;
        } else if (p_UploadEntry.data.mesh.meshInfo
                       .get_uploaded_index_count() <
                   l_MeshInfo.indices.size()) {
          const u64 l_FullSize = l_IndexDataSize;
          const u64 l_UploadedSize =
              l_RendererMeshInfo.get_uploaded_index_count() *
              MESH_INDEX_SIZE;

          u8 *l_Data = (u8 *)l_MeshInfo.indices.data();

          const u64 l_SpaceRequired = l_FullSize - l_UploadedSize;

          size_t l_StagingOffset = 0;

          // Request staging buffer space
          const u64 l_FrameUploadSpace =
              Vulkan::request_resource_staging_buffer_space(
                  l_SpaceRequired, &l_StagingOffset);

          const float l_UploadProgress =
              (((float)l_UploadedSize) +
               ((float)l_FrameUploadSpace)) /
              ((float)l_FullSize);
          // Calculate the progress as percent.
          // This calculation takes into account that the vertices
          // have already been uploaded at this point which is why we
          // divide it by 2 and add 50% to it to account for the
          // vertices.
          const u8 l_UploadProgressPercent =
              (Math::Util::floor(l_UploadProgress * 100.0f) / 2.0f) +
              50;

          // Upload data to staging buffer
          LOW_ASSERT(
              Vulkan::resource_staging_buffer_write(
                  &l_Data[l_UploadedSize], l_FrameUploadSpace,
                  l_StagingOffset),
              "Failed to write mesh index data to staging buffer");

          VkBufferCopy l_CopyRegion{};
          l_CopyRegion.srcOffset = l_StagingOffset;
          l_CopyRegion.dstOffset =
              l_RendererMeshInfo.get_index_start();
          l_CopyRegion.size = l_FrameUploadSpace;
          // TODO: Change to transfer queue command buffer
          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_current_resource_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_mesh_index_buffer().m_Buffer.buffer,
              1, &l_CopyRegion);

          p_UploadEntry.uploadedSize += l_FrameUploadSpace;
          p_UploadEntry.data.mesh.meshInfo.set_uploaded_index_count(
              p_UploadEntry.uploadedSize / MESH_INDEX_SIZE);
          p_UploadEntry.progressPriority = l_UploadProgressPercent;

          return l_FrameUploadSpace > 0.0f;
        }

        LOW_ASSERT(false, "Unknown situation, mesh is most likely "
                          "already fully uploaded");
        return false;
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
        const u64 l_UploadedSize = p_UploadEntry.uploadedSize;

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

        p_UploadEntry.uploadedSize += l_FrameUploadSpace;

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
        } else if (p_UploadEntry.is_mesh_upload()) {
          MeshInfo l_MeshInfo = p_UploadEntry.data.mesh.meshInfo;
          Submesh l_Submesh = p_UploadEntry.data.mesh.submesh;
          MeshResource l_MeshResource =
              p_UploadEntry.data.mesh.meshResource;
          // At this point one mesh info has been fully
          // uploaded to we set it to LOADED before
          // checking if the assigned submesh and
          // meshresource have now been completely
          // uploaded.
          l_MeshInfo.set_state(MeshResourceState::LOADED);
          l_Submesh.set_uploaded_meshinfo_count(
              l_Submesh.get_uploaded_meshinfo_count() + 1);

          if (l_Submesh.get_uploaded_meshinfo_count() >=
              l_Submesh.get_meshinfo_count()) {
            l_Submesh.set_state(MeshResourceState::LOADED);

            l_MeshResource.set_uploaded_submesh_count(
                l_MeshResource.get_uploaded_submesh_count() + 1);

            if (l_MeshResource.get_uploaded_submesh_count() ==
                l_MeshResource.get_submesh_count()) {
              l_MeshResource.set_state(MeshResourceState::LOADED);
            }
          }
        }

        return true;
      }

      static bool upload_resource(UploadEntry &p_UploadEntry)
      {
        volatile int i = 0;
        if (p_UploadEntry.is_image_upload() &&
            p_UploadEntry.progressPriority < 100) {
          return image_upload(p_UploadEntry);
        } else if (p_UploadEntry.is_mesh_upload() &&
                   p_UploadEntry.progressPriority < 100) {
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
