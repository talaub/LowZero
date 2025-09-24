#include "LowRendererRenderObjectSystem.h"

#include "LowRendererDrawCommand.h"
#include "LowRendererRenderObject.h"
#include "LowRendererAdaptiveRenderObject.h"
#include "LowRendererVulkan.h"
#include "LowRenderer.h"

namespace Low {
  namespace Renderer {
    namespace RenderObjectSystem {
      static bool update_dirty_drawcommands(float p_Delta)
      {
        bool l_Result = true;

        for (auto it = DrawCommand::ms_Dirty.begin();
             it != DrawCommand::ms_Dirty.end(); ++it) {
          DrawCommand i_DrawCommand = *it;

          if (!i_DrawCommand.is_alive() ||
              i_DrawCommand.get_render_object().is_dirty()) {
            // We will skip dead draw commands as well as ones that
            // have a dirty renderobject because then they will be
            // updated later together with their render object
            continue;
          }

          size_t l_StagingOffset = 0;
          const u64 l_FrameUploadSpace =
              Vulkan::Global::get_current_frame_staging_buffer()
                  .request_space(sizeof(DrawCommandUpload),
                                 &l_StagingOffset);

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
              // Could not reserve space in the draw command buffer
              // for another entry
              l_Result = false;
              break;
            }

            i_DrawCommand.set_slot(i_Slot);
            i_DrawCommand.set_uploaded(true);
            i_IsFirstTimeUpload = true;
          }

          _LOW_ASSERT(i_DrawCommand.is_uploaded());

          DrawCommandUpload i_Upload;
          i_Upload.worldTransform =
              i_DrawCommand.get_world_transform();

          i_Upload.objectId = i_DrawCommand.get_object_id();

          i_Upload.materialIndex = get_default_material().get_index();
          if (i_DrawCommand.get_material().is_alive() &&
              i_DrawCommand.get_material().get_state() ==
                  MaterialState::LOADED) {
            i_Upload.materialIndex =
                i_DrawCommand.get_material().get_gpu().get_index();
          }

          RenderScene i_RenderScene =
              i_DrawCommand.get_render_scene_handle();

          if (i_IsFirstTimeUpload) {
            LOW_ASSERT(
                i_RenderScene.insert_draw_command(i_DrawCommand),
                "Failed so add draw command to render scene");
          }

          LOW_ASSERT(
              Vulkan::Global::get_current_frame_staging_buffer()
                  .write(&i_Upload, sizeof(DrawCommandUpload),
                         l_StagingOffset),
              "Failed to write draw command data to staging buffer");

          VkBufferCopy i_CopyRegion;
          i_CopyRegion.srcOffset = l_StagingOffset;
          i_CopyRegion.dstOffset =
              i_DrawCommand.get_slot() * sizeof(DrawCommandUpload);
          i_CopyRegion.size = l_FrameUploadSpace;

          // TODO: Change to transfer queue command buffer - or leave
          // this specifically on the graphics queue not sure tbh
          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_current_frame_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_drawcommand_buffer()
                  .m_Buffer.buffer,
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

              if (!i_DrawCommand.get_material().is_alive()) {
                i_DrawCommand.set_material(
                    i_RenderObject.get_material());
              }

              i_RenderObject.get_draw_commands().push_back(
                  i_DrawCommand);
            }
          }

          const Util::List<DrawCommand> &i_DrawCommands =
              i_RenderObject.get_draw_commands();

          size_t l_StagingOffset = 0;
          const u64 l_FrameUploadSpace =
              Vulkan::Global::get_current_frame_staging_buffer()
                  .request_space(sizeof(DrawCommandUpload) *
                                     i_DrawCommands.size(),
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

            i_DrawCommand.set_object_id(
                i_RenderObject.get_object_id());

            DrawCommandUpload i_Upload;
            i_Upload.worldTransform =
                i_DrawCommand.get_world_transform();
            i_Upload.objectId = i_DrawCommand.get_object_id();

            i_Upload.materialIndex =
                get_default_material().get_index();
            if (i_DrawCommand.get_material().is_alive() &&
                i_DrawCommand.get_material().get_state() ==
                    MaterialState::LOADED) {
              i_Upload.materialIndex =
                  i_DrawCommand.get_material().get_gpu().get_index();
            }

            i_Uploads.push_back(i_Upload);
          }

          LOW_ASSERT(
              Vulkan::Global::get_current_frame_staging_buffer()
                  .write(i_Uploads.data(),
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
              Vulkan::Global::get_current_frame_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_drawcommand_buffer()
                  .m_Buffer.buffer,
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

      void tick(float p_Delta)
      {
        update_drawcommand_buffer(p_Delta);
      }
    } // namespace RenderObjectSystem
  } // namespace Renderer
} // namespace Low
