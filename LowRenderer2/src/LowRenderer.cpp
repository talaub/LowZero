#include "LowRenderer.h"

#include "LowRendererEditorImageGpu.h"
#include "LowRendererEditorImageStaging.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererBase.h"
#include "LowRendererResourceManager.h"
#include "LowRendererMeshResource.h"
#include "LowRendererVulkan.h"
#include "LowRendererVkImage.h"
#include "LowRendererRenderObject.h"
#include "LowRendererGlobals.h"
#include "LowRendererRenderView.h"
#include "LowRendererMaterial.h"
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
#include "LowRendererUiRenderObject.h"
#include "LowRendererTexture.h"
#include "LowRendererTextureResource.h"
#include "LowRendererGpuTexture.h"
#include "LowRendererTexturePixels.h"
#include "LowRendererTextureStaging.h"
#include "LowRendererMaterialTypeFamily.h"
#include "LowRendererPrimitives.h"
#include "LowRendererFont.h"
#include "LowRendererFontResource.h"
#include "LowRendererTextureExport.h"
#include "LowRendererEditorImage.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilFileSystem.h"
#include "LowUtilHashing.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {

    VkSampler g_TestSampler;
    Low::Renderer::Texture g_Texture;

    MaterialTypes g_MaterialTypes;

    Util::List<ThumbnailCreationSchedule>
        g_ThumbnailCreationSchedules;

    Texture g_DefaultTexture;
    Material g_DefaultMaterial;
    Material g_DefaultMaterialTexture;

    Texture get_default_texture()
    {
      return g_DefaultTexture;
    }
    Material get_default_material()
    {
      return g_DefaultMaterial;
    }
    Material get_default_material_texture()
    {
      return g_DefaultMaterialTexture;
    }

    MaterialTypes &get_material_types()
    {
      return g_MaterialTypes;
    }

    static void initialize_enums()
    {
      MaterialTypeFamilyEnumHelper::initialize();
    }

    static void cleanup_enums()
    {
      MaterialTypeFamilyEnumHelper::cleanup();
    }

    static void initialize_types()
    {
      RenderScene::initialize();
      RenderView::initialize();
      RenderObject::initialize();
      Mesh::initialize();
      MeshGeometry::initialize();
      SubmeshGeometry::initialize();
      GpuMesh::initialize();
      GpuSubmesh::initialize();
      MeshResource::initialize();
      Material::initialize();
      DrawCommand::initialize();
      PointLight::initialize();
      UiCanvas::initialize();
      UiRenderObject::initialize();
      UiDrawCommand::initialize();
      Texture::initialize();
      TextureResource::initialize();
      TextureStaging::initialize();
      TexturePixels::initialize();
      GpuTexture::initialize();
      RenderStep::initialize();
      Font::initialize();
      FontResource::initialize();
      TextureExport::initialize();
      EditorImage::initialize();
      EditorImageStaging::initialize();
      EditorImageGpu::initialize();
    }

    static void cleanup_types()
    {
      EditorImageGpu::cleanup();
      EditorImageStaging::cleanup();
      EditorImage::cleanup();
      TextureExport::cleanup();
      FontResource::cleanup();
      Font::cleanup();
      UiDrawCommand::cleanup();
      UiRenderObject::cleanup();
      UiCanvas::cleanup();
      PointLight::cleanup();
      RenderView::cleanup();
      RenderObject::cleanup();
      MeshResource::cleanup();
      GpuSubmesh::cleanup();
      GpuMesh::cleanup();
      SubmeshGeometry::cleanup();
      MeshGeometry::cleanup();
      Mesh::cleanup();
      Material::cleanup();
      RenderScene::cleanup();
      DrawCommand::cleanup();
      RenderStep::cleanup();
      TextureResource::cleanup();
      TexturePixels::cleanup();
      TextureStaging::cleanup();
      GpuTexture::cleanup();
      Texture::cleanup();
    }

    static bool initialize_default_materials()
    {
      {
        g_DefaultMaterial =
            Material::make(N(Default), g_MaterialTypes.solidBase);

        g_DefaultMaterial.set_property_vector3(
            N(base_color), Math::ColorRGB(0.8f, 0.8f, 0.8f));
      }

      {
        g_DefaultMaterialTexture = Material::make(
            N(DefaultTexture), g_MaterialTypes.solidBase);

        g_DefaultMaterialTexture.set_property_vector3(
            N(base_color), Math::ColorRGB(0.8f, 0.8f, 0.8f));
        g_DefaultMaterialTexture.set_property_texture(
            N(albedo), get_default_texture());
      }
      return true;
    }

    static bool initialize_material_types()
    {
      const float l_DebugGeometryLineWidth = 3.0f;

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(solid_base), Low::Renderer::MaterialTypeFamily::SOLID);
        l_MT.add_input(N(base_color),
                       Low::Renderer::MaterialTypeInputType::VECTOR3);
        l_MT.add_input(N(albedo),
                       Low::Renderer::MaterialTypeInputType::TEXTURE);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("solid_base.vert");
        l_MT.set_draw_fragment_shader_path("solid_base.frag");

        g_MaterialTypes.solidBase = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(debug_geometry),
            Low::Renderer::MaterialTypeFamily::DEBUGGEOMETRY);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("debug_geometry.vert");
        l_MT.set_draw_fragment_shader_path("debug_geometry.frag");

        g_MaterialTypes.debugGeometry = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(debug_geometry_no_depth),
            Low::Renderer::MaterialTypeFamily::DEBUGGEOMETRY);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("debug_geometry.vert");
        l_MT.set_draw_fragment_shader_path("debug_geometry.frag");

        l_MT.get_draw_pipeline_config().depthTest = false;
        l_MT.get_draw_pipeline_config().depthFormat =
            ImageFormat::DEPTH;

        g_MaterialTypes.debugGeometryNoDepth = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(debug_geometry_wireframe),
            Low::Renderer::MaterialTypeFamily::DEBUGGEOMETRY);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("debug_geometry.vert");
        l_MT.set_draw_fragment_shader_path("debug_geometry.frag");

        l_MT.get_draw_pipeline_config().wireframe = true;
        l_MT.get_draw_pipeline_config().lineStrength =
            l_DebugGeometryLineWidth;

        g_MaterialTypes.debugGeometryWireframe = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(debug_geometry_no_depth_wireframe),
            Low::Renderer::MaterialTypeFamily::DEBUGGEOMETRY);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("debug_geometry.vert");
        l_MT.set_draw_fragment_shader_path("debug_geometry.frag");

        l_MT.get_draw_pipeline_config().depthTest = false;
        l_MT.get_draw_pipeline_config().depthFormat =
            ImageFormat::DEPTH;
        l_MT.get_draw_pipeline_config().wireframe = true;
        l_MT.get_draw_pipeline_config().lineStrength =
            l_DebugGeometryLineWidth;

        g_MaterialTypes.debugGeometryNoDepthWireframe = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(ui_base), Low::Renderer::MaterialTypeFamily::UI);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("base_ui.vert");
        l_MT.set_draw_fragment_shader_path("base_ui.frag");

        g_MaterialTypes.uiBase = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(ui_text), Low::Renderer::MaterialTypeFamily::UI);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("base_ui.vert");
        l_MT.set_draw_fragment_shader_path("ui_text.frag");

        g_MaterialTypes.uiText = l_MT;
      }

      return true;
    }

    static bool
    parse_font_resource_config(Util::String p_Path,
                               Util::Yaml::Node &p_Node,
                               FontResourceConfig &p_Config)
    {
      LOWR_ASSERT_RETURN(p_Node["version"], "Could not find version");
      const u32 l_Version = p_Node["version"].as<u32>();

      if (l_Version == 1) {
        LOWR_ASSERT_RETURN(p_Node["name"],
                           "Could not find font name");
        p_Config.name = LOW_YAML_AS_NAME(p_Node["name"]);

        LOWR_ASSERT_RETURN(p_Node["font_id"],
                           "Could not find font id");
        p_Config.fontId = Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["font_id"]));

        LOWR_ASSERT_RETURN(p_Node["asset_hash"],
                           "Could not find asset hash");
        p_Config.assetHash = Util::string_to_hash(
            LOW_YAML_AS_STRING(p_Node["asset_hash"]));

        LOWR_ASSERT_RETURN(p_Node["source_file"],
                           "Could not find source file");
        p_Config.sourceFile =
            LOW_YAML_AS_STRING(p_Node["source_file"]);

        p_Config.sidecarPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(p_Config.fontId) + ".font.yaml";
        p_Config.fontPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(p_Config.fontId) + ".msdf.ktx";

        p_Config.path = p_Path;
        return true;
      }

      LOWR_ASSERT_RETURN(
          false, "Version of font resource config not supported");
      return false;
    }

    static bool preload_resources()
    {
      {
        Util::List<Util::String> l_FilePaths;

        Util::FileIO::list_directory(
            Util::get_project().editorImagesPath.c_str(),
            l_FilePaths);
        Util::String l_Ending = ".png";

        for (Util::String &i_Path : l_FilePaths) {
          if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
            EditorImage i_EditorImage =
                EditorImage::make(N(EditorImage));
            i_EditorImage.set_path(i_Path);
          }
        }
      }

      {
        Util::List<Util::String> l_MeshResources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(),
            ".meshresource.yaml", l_MeshResources);

        for (auto it = l_MeshResources.begin();
             it != l_MeshResources.end(); ++it) {
          Util::Yaml::Node i_ResourceNode =
              Util::Yaml::load_file(it->c_str());
          MeshResourceConfig i_ResourceConfig;

          LOWR_ASSERT_RETURN(
              ResourceManager::parse_mesh_resource_config(
                  *it, i_ResourceNode, i_ResourceConfig),
              "Failed to pass mesh resource config.");

          Mesh::make_from_resource_config(i_ResourceConfig);
        }
      }

      {
        Util::List<Util::String> l_Resources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(),
            ".fontresource.yaml", l_Resources);

        for (auto it = l_Resources.begin(); it != l_Resources.end();
             ++it) {
          Util::Yaml::Node i_ResourceNode =
              Util::Yaml::load_file(it->c_str());
          FontResourceConfig i_ResourceConfig;

          LOWR_ASSERT_RETURN(
              parse_font_resource_config(*it, i_ResourceNode,
                                         i_ResourceConfig),
              "Failed to pass font resource config.");

          Font::make_from_resource_config(i_ResourceConfig);
        }
      }

      return true;
    }

    void initialize()
    {
      initialize_enums();
      initialize_types();
      LOW_ASSERT(Vulkan::initialize(),
                 "Failed to initialize Vulkan renderer");

      {
        /*
        Low::Util::String l_BasePath =
            Low::Util::get_project().dataPath;
        l_BasePath += "/resources/img2d/test.ktx";

        g_Texture = Low::Renderer::load_texture(l_BasePath);
        */
      }

      {
        g_DefaultTexture = Texture::make_gpu_ready(N(Default));
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

      LOW_ASSERT(preload_resources(), "Failed to preload resources.");

      initialize_primitives();

      LOW_ASSERT(initialize_material_types(),
                 "Failed to initialize material types.");

      LOW_ASSERT(initialize_default_materials(),
                 "Failed to initialize default materials");
    }

    void cleanup()
    {
      Vulkan::wait_idle();

      vkDestroySampler(Vulkan::Global::get_device(), g_TestSampler,
                       nullptr);

      cleanup_types();
      cleanup_enums();

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
        i_Upload.worldTransform = i_DrawCommand.get_world_transform();

        i_Upload.materialIndex = get_default_material().get_index();
        if (i_DrawCommand.get_material().is_alive()) {
          i_Upload.materialIndex =
              i_DrawCommand.get_material().get_index();
        }

        RenderScene i_RenderScene =
            i_DrawCommand.get_render_scene_handle();

        if (i_IsFirstTimeUpload) {
          LOW_ASSERT(i_RenderScene.insert_draw_command(i_DrawCommand),
                     "Failed so add draw command to render scene");
        }

        LOW_ASSERT(
            Vulkan::Global::get_current_frame_staging_buffer().write(
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
        vkCmdCopyBuffer(
            Vulkan::Global::get_current_command_buffer(),
            Vulkan::Global::get_current_frame_staging_buffer()
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

          DrawCommandUpload i_Upload;
          i_Upload.worldTransform =
              i_DrawCommand.get_world_transform();

          i_Upload.materialIndex = get_default_material().get_index();
          if (i_DrawCommand.get_material().is_alive()) {
            i_Upload.materialIndex =
                i_DrawCommand.get_material().get_index();
          }

          i_Uploads.push_back(i_Upload);
        }

        LOW_ASSERT(
            Vulkan::Global::get_current_frame_staging_buffer().write(
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
            Vulkan::Global::get_current_frame_staging_buffer()
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

    static bool initialize_ui_renderobjects(float p_Delta)
    {
      for (auto it = UiRenderObject::ms_NeedInitialization.begin();
           it != UiRenderObject::ms_NeedInitialization.end();) {
        if (it->get_mesh().get_state() != MeshState::LOADED) {
          ++it;
          continue;
        }

        // Create ui draw commands and assign to canvas
        {
          GpuMesh i_GpuMesh = it->get_mesh().get_gpu();

          for (auto sit = i_GpuMesh.get_submeshes().begin();
               sit != i_GpuMesh.get_submeshes().end(); ++sit) {
            GpuSubmesh i_GpuSubmesh = *sit;

            UiDrawCommand i_DrawCommand = UiDrawCommand::make(
                it->get_id(), it->get_canvas_handle(), i_GpuSubmesh);

            it->get_draw_commands().push_back(i_DrawCommand);
          }
        }
        it = UiRenderObject::ms_NeedInitialization.erase(it);
      }

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
      for (auto it = Material::ms_PendingTextureBindings.begin();
           it != Material::ms_PendingTextureBindings.end();) {
        if (!it->texture.is_alive() || !it->material.is_alive()) {
          it = Material::ms_PendingTextureBindings.erase(it);
          continue;
        }

        if (it->texture.get_state() == TextureState::LOADED) {
          it->material.set_property_texture(it->propertyName,
                                            it->texture);

          it = Material::ms_PendingTextureBindings.erase(it);
          continue;
        }

        ++it;
      }

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

      static bool l_Initialized = false;
      if (!l_Initialized) {

        for (u32 i = 0; i < RenderStep::living_count(); ++i) {
          LOW_ASSERT(RenderStep::living_instances()[i].setup(),
                     "Failed to setup renderstep.");
        }
        l_Initialized = true;
      }
    }

    static bool tick_thumbnail_creation(float p_Delta)
    {
      for (auto it = g_ThumbnailCreationSchedules.begin();
           it != g_ThumbnailCreationSchedules.end();) {
        if (it->state == ThumbnailCreationState::SCHEDULED) {
          if (it->mesh.get_state() != MeshState::LOADED) {
            ++it;
            continue;
          }
          // TODO: test if material is loaded
          if (it->material.is_alive() /* &&
              it->material.get_state() != MaterialState::LOADED */) {
            ++it;
            continue;
          }

          if (!it->view.get_lit_image().is_alive()) {
            ++it;
            continue;
          }

          Math::Sphere i_Sphere =
              it->mesh.get_gpu().get_bounding_sphere();

          Math::Vector3 i_Direction =
              glm::normalize(it->viewDirection);

          it->view.set_camera_direction(i_Direction);
          it->view.set_camera_position(
              i_Sphere.position -
              (i_Direction * (i_Sphere.radius + 8.0f)));

          TextureExport i_Export = TextureExport::make(N(Thumbnail));
          i_Export.set_path(it->path);
          i_Export.set_texture(it->view.get_lit_image());
          it->textureExport = i_Export;

          it->state = ThumbnailCreationState::SUBMITTED;
          ++it;
        } else if (it->state == ThumbnailCreationState::SUBMITTED) {
          if (it->textureExport.is_alive()) {
            ++it;
            continue;
          }

          it->object.destroy();
          it->view.destroy();
          it->scene.destroy();

          it = g_ThumbnailCreationSchedules.erase(it);
        }
      }
      return true;
    }

    void tick(float p_Delta)
    {

      static bool l_ImageInit = false;
      ResourceManager::tick(p_Delta);
      update_drawcommand_buffer(p_Delta);
      initialize_ui_renderobjects(p_Delta);
      tick_materials(p_Delta);
      if (!l_ImageInit) {
        if (g_Texture.is_alive() &&
            g_Texture.get_state() == TextureState::LOADED) {
          l_ImageInit = true;
        }
      }

      ImGui::Begin("Camera");
      RenderView l_RenderView = RenderView::living_instances()[0];
      Math::Vector3 l_CameraPosition =
          l_RenderView.get_camera_position();
      ImGui::DragFloat3("Position", (float *)&l_CameraPosition);
      l_RenderView.set_camera_position(l_CameraPosition);

      {
        RenderObject l_RenderObject =
            RenderObject::living_instances()[0];

        static bool l_Test = true;

        if (!l_Test &&
            l_RenderObject.get_mesh().get_gpu().is_alive()) {
          Math::Sphere l_Sphere = l_RenderObject.get_mesh()
                                      .get_gpu()
                                      .get_bounding_sphere();

          l_Test = true;

          Math::Vector3 l_Direction(0.2f, -0.1f, 1.0f);
          l_Direction = glm::normalize(l_Direction);

          LOW_LOG_DEBUG << l_Sphere.position << LOW_LOG_END;
          LOW_LOG_DEBUG << l_Sphere.radius << LOW_LOG_END;

          l_RenderView.set_camera_direction(l_Direction);
          l_RenderView.set_camera_position(
              l_Sphere.position -
              (l_Direction * (l_Sphere.radius + 8.0f)));
        }
        // MARK
      }

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
            split + splitter_thickness); // Position right of
        splitter
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
                       .get_gpu()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("Depth");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_depth()
                       .get_gpu()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("View Position");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_viewposition()
                       .get_gpu()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();
      ImGui::Begin("Albedo");
      ImGui::Image(RenderView::living_instances()[0]
                       .get_gbuffer_albedo()
                       .get_gpu()
                       .get_imgui_texture_id(),
                   ImVec2(256, 256));
      ImGui::End();

      if (l_ImageInit) {
        ImGui::Begin("Mipmapped");

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ImGui::Image(
            (ImTextureID)g_Texture.get_gpu().get_imgui_texture_id(),
            ImVec2{viewportPanelSize.x, viewportPanelSize.y});

        ImGui::End();
      }

      LOW_ASSERT(Vulkan::tick(p_Delta),
                 "Failed to tick Vulkan renderer");

      LOW_ASSERT(tick_thumbnail_creation(p_Delta),
                 "Failed to tick thumbnail creation");
    }

    void check_window_resize(float p_Delta)
    {
      LOW_ASSERT(
          Vulkan::check_window_resize(p_Delta),
          "Failed to check for window resize in Vulkan renderer");
    }

    void load_mesh(Mesh p_Mesh)
    {
      _LOW_ASSERT(ResourceManager::load_mesh(p_Mesh));
    }

    Texture load_texture(Util::String p_ImagePath)
    {
      TextureResource l_Resource = TextureResource::make(p_ImagePath);
      Texture l_Texture = Texture::make(l_Resource.get_name());
      l_Texture.set_resource(l_Resource);

      _LOW_ASSERT(ResourceManager::load_texture(l_Texture));

      return l_Texture;
    }

    void
    submit_thumbnail_creation(ThumbnailCreationSchedule p_Schedule)
    {
      p_Schedule.state = ThumbnailCreationState::SCHEDULED;
      g_ThumbnailCreationSchedules.push_back(p_Schedule);
    }
  } // namespace Renderer
} // namespace Low
