#include "LowRenderer.h"

#include "LowMath.h"
#include "LowRendererEditorImageGpu.h"
#include "LowRendererEditorImageStaging.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererBase.h"
#include "LowRendererResourceManager.h"
#include "LowRendererMeshResource.h"
#include "LowRendererVulkan.h"
#include "LowRendererVulkanBuffer.h"
#include "LowRendererVkImage.h"
#include "LowRendererRenderObject.h"
#include "LowRendererGlobals.h"
#include "LowRendererRenderView.h"
#include "LowRendererMaterial.h"
#include "LowRendererMaterialResource.h"
#include "LowRendererGpuMaterial.h"
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
#include "LowRendererMaterialState.h"
#include "LowRendererModel.h"
#include "LowRendererModelResource.h"
#include "LowRendererAdaptiveRenderObject.h"
#include "LowRendererRenderObjectSystem.h"
#include "LowRendererResourceImporter.h"
#include "LowRendererSS2DDrawCommand.h"
#include "LowRendererSS2DCanvas.h"
#include "LowRendererBuffer.h"
#include "LowRendererShaderSource.h"
#include "LowRendererShaderVariant.h"
#include "LowRendererSkeleton.h"
#include "LowRendererSkeletonResource.h"
#include "LowRendererAnimationClip.h"
#include "LowRendererAnimationClipResource.h"
#include "LowRendererSkinningCommand.h"
#include "LowRendererSkinningInstance.h"
#include "LowRendererSkinningPose.h"
#include "LowRendererSkinningSystem.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilFileSystem.h"
#include "LowUtilHandle.h"
#include "LowUtilHashing.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilSerialization.h"
#include "LowUtilString.h"
#include "LowUtilAssetManager.h"
#include "LowUtilJobManager.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <imgui.h>
#include <vulkan/vulkan_core.h>

namespace Low {
  namespace Renderer {

    MaterialTypes g_MaterialTypes;

    Util::List<ThumbnailCreationSchedule>
        g_ThumbnailCreationSchedules;

    Texture g_DefaultTexture;
    Material g_DefaultMaterial;
    Material g_DefaultMaterialTexture;
    Material g_DefaultMaterialUi;
    Material g_DefaultMaterialUiText;
    Material g_DefaultMaterialUiOutline;

    RenderView g_GameRenderView;
    RenderView g_EditorRenderView;

    RenderScene g_GlobalScene;

    SS2DCanvas g_SS2DCanvas;

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
    Material get_default_material_ui()
    {
      return g_DefaultMaterialUi;
    }
    Material get_default_material_ui_text()
    {
      return g_DefaultMaterialUiText;
    }
    Material get_default_material_ui_outline()
    {
      return g_DefaultMaterialUiOutline;
    }

    MaterialTypes &get_material_types()
    {
      return g_MaterialTypes;
    }

    static void initialize_enums()
    {
      MaterialTypeFamilyEnumHelper::initialize();
      MaterialStateEnumHelper::initialize();
    }

    static void cleanup_enums()
    {
      MaterialTypeFamilyEnumHelper::cleanup();
      MaterialStateEnumHelper::cleanup();
    }

    static void initialize_types()
    {
      Buffer::initialize();
      MaterialType::initialize();
      SkeletonResource::initialize();
      Skeleton::initialize();
      AnimationClipResource::initialize();
      AnimationClip::initialize();
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
      MaterialResource::initialize();
      GpuMaterial::initialize();
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
      Model::initialize();
      ModelResource::initialize();
      AdaptiveRenderObject::initialize();
      SS2DCanvas::initialize();
      SS2DDrawCommand::initialize();
      ShaderVariant::initialize();
      ShaderSource::initialize();
      SkinningInstance::initialize();
      SkinningCommand::initialize();
      SkinningPose::initialize();
    }

    static void cleanup_types()
    {
      SkinningPose::cleanup();
      SkinningCommand::cleanup();
      SkinningInstance::cleanup();
      SkeletonResource::cleanup();
      Skeleton::cleanup();
      AnimationClipResource::cleanup();
      AnimationClip::cleanup();
      ShaderVariant::cleanup();
      ShaderSource::cleanup();
      SS2DDrawCommand::cleanup();
      SS2DCanvas::cleanup();
      AdaptiveRenderObject::cleanup();
      ModelResource::cleanup();
      Model::cleanup();
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
      GpuMaterial::cleanup();
      MaterialResource::cleanup();
      Material::cleanup();
      RenderScene::cleanup();
      DrawCommand::cleanup();
      RenderStep::cleanup();
      TextureResource::cleanup();
      TexturePixels::cleanup();
      TextureStaging::cleanup();
      GpuTexture::cleanup();
      Texture::cleanup();
      MaterialType::cleanup();
      Buffer::cleanup();
    }

    static bool initialize_default_materials()
    {
      {
        g_DefaultMaterial = Material::make_gpu_ready_with_uid(
            N(Default), g_MaterialTypes.solidBase,
            Util::make_fixed_unique_id("renderer.material.Default"));

        g_DefaultMaterial.set_property_vector3(
            N(base_color), Math::ColorRGB(0.8f, 0.8f, 0.8f));
      }

      {
        g_DefaultMaterialTexture = Material::make_gpu_ready_with_uid(
            N(DefaultTexture), g_MaterialTypes.solidBase,
            Util::make_fixed_unique_id(
                "renderer.material.DefaultTexture"));

        g_DefaultMaterialTexture.set_property_vector3(
            N(base_color), Math::ColorRGB(0.8f, 0.8f, 0.8f));
        g_DefaultMaterialTexture.set_property_texture(
            N(albedo), get_default_texture());
      }

      {
        g_DefaultMaterialUi = Material::make_gpu_ready_with_uid(
            N(DefaultMaterialUi), g_MaterialTypes.uiBase,
            Util::make_fixed_unique_id(
                "renderer.material.DefaultMaterialUi"));
      }
      {
        g_DefaultMaterialUiText = Material::make_gpu_ready_with_uid(
            N(DefaultMaterialUiText), g_MaterialTypes.uiText,
            Util::make_fixed_unique_id(
                "renderer.material.DefaultMaterialUiText"));
      }
      {
        g_DefaultMaterialUiOutline =
            Material::make_gpu_ready_with_uid(
                N(DefaultMaterialUiOutline),
                g_MaterialTypes.uiOutline,
                Util::make_fixed_unique_id(
                    "renderer.material.DefaultMaterialUiOutline"));
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
        l_MT.casts_shadows(true);

        g_MaterialTypes.solidBase = l_MT;
      }

      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(water), Low::Renderer::MaterialTypeFamily::SOLID);
        l_MT.add_input(N(shallow_color),
                       Low::Renderer::MaterialTypeInputType::VECTOR3);
        l_MT.add_input(N(deep_color),
                       Low::Renderer::MaterialTypeInputType::VECTOR3);
        l_MT.add_input(N(fresnel_power),
                       Low::Renderer::MaterialTypeInputType::FLOAT);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("water.vert");
        l_MT.set_draw_fragment_shader_path("water.frag");
        l_MT.casts_shadows(false);

        g_MaterialTypes.water = l_MT;
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
      {
        MaterialType l_MT = Low::Renderer::MaterialType::make(
            N(ui_text), Low::Renderer::MaterialTypeFamily::UI);
        l_MT.finalize();

        l_MT.set_draw_vertex_shader_path("base_ui.vert");
        l_MT.set_draw_fragment_shader_path("ui_outline.frag");

        g_MaterialTypes.uiOutline = l_MT;
      }

      return true;
    }

    static bool
    parse_font_resource_config(Util::String p_Path,
                               Util::Serial::Node &p_Node,
                               FontResourceConfig &p_Config)
    {
      LOWR_ASSERT_RETURN(p_Node["version"], "Could not find version");
      const u32 l_Version = p_Node["version"].as<u32>();

      if (l_Version == 1) {
        LOWR_ASSERT_RETURN(p_Node["name"],
                           "Could not find font name");
        p_Config.name = p_Node["name"].as<Util::Name>();

        LOWR_ASSERT_RETURN(p_Node["font_id"],
                           "Could not find font id");
        p_Config.fontId = p_Node["font_id"].as<Util::U64Id>().val;

        LOWR_ASSERT_RETURN(p_Node["asset_hash"],
                           "Could not find asset hash");
        p_Config.assetHash = p_Config.fontId =
            p_Node["asset_hash"].as<Util::U64Id>().val;

        LOWR_ASSERT_RETURN(p_Node["source_file"],
                           "Could not find source file");
        p_Config.sourceFile =
            p_Node["source_file"].as<Util::String>();

        p_Config.sidecarPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(p_Config.fontId) + ".font.yaml";
        p_Config.fontPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(p_Config.fontId) + ".msdf.ktx";

        p_Config.path = Util::PathHelper::normalize(p_Path);
        return true;
      }

      LOWR_ASSERT_RETURN(
          false, "Version of font resource config not supported");
      return false;
    }

    static bool preload_resources()
    {
      {
#if 0
        Util::List<Util::String> l_FilePaths;

        Util::FileIO::list_directory(
            Util::get_project().editorImagesPath.c_str(),
            l_FilePaths);
        Util::String l_Ending = ".png";

        for (Util::String &i_Path : l_FilePaths) {
          if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
            Util::String i_NameString =
                Util::PathHelper::get_base_name_no_ext(i_Path);
            EditorImage i_EditorImage =
                EditorImage::make(LOW_NAME(i_NameString.c_str()));
            i_EditorImage.set_path(i_Path);
          }
        }
#else
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(EditorImage), Renderer::EditorImage::IDENTIFIER);
        l_Builder.auto_initialize(true)
            .initialize_on_startup(true)
            .add_asset_suffix(".png");
        l_Builder.add_initialize_directory(
            Util::get_project().editorImagesPath, true);
        l_Builder
            .initializer(
                [](const Util::String p_Path) -> Util::Handle {
                  Util::String i_NameString =
                      Util::PathHelper::get_base_name_no_ext(p_Path);
                  EditorImage i_EditorImage = EditorImage::make(
                      LOW_NAME(i_NameString.c_str()));
                  i_EditorImage.set_path(
                      Util::PathHelper::normalize(p_Path));

                  return i_EditorImage.get_id();
                })
            .no_saving();

        Util::AssetManager::register_asset_type(l_Builder.build());
#endif
      }

#if 0
      {
        Util::List<Util::String> l_MeshResources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(),
            ".meshresource.yaml", l_MeshResources);

        for (auto it = l_MeshResources.begin();
             it != l_MeshResources.end(); ++it) {
          Util::Serial::Node i_ResourceNode =
              Util::Serial::load_yaml_file(it->c_str());
          MeshResourceConfig i_ResourceConfig;

          LOWR_ASSERT_RETURN(
              ResourceManager::parse_mesh_resource_config(
                  *it, i_ResourceNode, i_ResourceConfig),
              "Failed to parse mesh resource config.");

          ResourceManager::register_asset(
              i_ResourceConfig.meshId,
              Mesh::make_from_resource_config(i_ResourceConfig));
        }
      }
#else
      // Mesh asset type
      Util::AssetManager::TypeRegistratorBuilder l_Builder(
          N(Mesh), Renderer::Mesh::IDENTIFIER);
      l_Builder.auto_initialize(true)
          .initialize_on_startup(true)
          .add_asset_suffix(".meshresource.yaml");
      l_Builder.add_initialize_directory(Util::get_project().dataPath,
                                         true);
      l_Builder.add_raw_suffix(".obj")
          .add_raw_suffix(".glb")
          .add_raw_suffix(".gltf");
      l_Builder
          .add_import_directory(Util::get_project().dataPath, true,
                                true)
          .import_on_startup(true);

      l_Builder
          .initializer([](const Util::String p_Path) -> Util::Handle {
            Util::Serial::Node i_ResourceNode =
                Util::Serial::load_yaml_file(p_Path.c_str());
            MeshResourceConfig i_ResourceConfig;

            LOWR_ASSERT_RETURN(
                ResourceManager::parse_mesh_resource_config(
                    p_Path, i_ResourceNode, i_ResourceConfig),
                "Failed to parse mesh resource config.");
            Mesh l_ExistingMesh = ResourceManager::find_asset<Mesh>(
                i_ResourceConfig.meshId);
            if (!l_ExistingMesh.is_alive()) {
              ResourceManager::register_asset(
                  i_ResourceConfig.meshId,
                  Mesh::make_from_resource_config(i_ResourceConfig));
            }
          })
          .saver([](Util::Handle p_Handle) {
            Mesh l_Mesh = p_Handle.get_id();
            MeshResource l_Resource = l_Mesh.get_resource();
            Util::Serial::Node l_Node = Util::Serial::load_yaml_file(
                l_Resource.get_sidecar_path().c_str());
            l_Node["skeleton"] = 0;
            if (l_Mesh.get_skeleton().is_alive()) {
              l_Node["skeleton"] =
                  Util::U64Id{l_Mesh.get_skeleton().get_unique_id()};
            }
            Util::Serial::write_yaml_file(
                l_Resource.get_sidecar_path().c_str(), l_Node);
          })
          .importer([](const Util::String p_Path) -> Util::String {
            std::filesystem::path l_FilePath(p_Path.c_str());
            const Util::String l_Output =
                l_FilePath.replace_extension("").string().c_str();
            if (!ResourceImporter::import_mesh(p_Path, l_Output)) {
              LOW_LOG_ERROR << "Failed to import mesh."
                            << LOW_LOG_END;
              return "";
            }

            return l_Output + ".meshresource.yaml";
          });

      l_Builder.raw_deleter([](const Util::String p_Path) {
        std::filesystem::path l_FilePath(p_Path.c_str());

        for (u32 i = 0; i < MeshResource::living_count(); ++i) {
          MeshResource i_Resource =
              MeshResource::living_instances()[i];

          if (i_Resource.get_source_file() == p_Path) {
            Util::FileIO::delete_sync(i_Resource.get_path().c_str());
            break;
          }
        }
      });

      l_Builder.deleter([](const Util::String p_Path) {
        std::filesystem::path l_FilePath(p_Path.c_str());
      });

      Util::AssetManager::register_asset_type(l_Builder.build());
#endif

      if (0) {
        Util::List<Util::String> l_TextureResources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(), ".texresource.yaml",
            l_TextureResources);

        for (auto it = l_TextureResources.begin();
             it != l_TextureResources.end(); ++it) {
          Util::Serial::Node i_ResourceNode =
              Util::Serial::load_yaml_file(it->c_str());
          TextureResourceConfig i_ResourceConfig;

          LOWR_ASSERT_RETURN(
              ResourceManager::parse_texture_resource_config(
                  *it, i_ResourceNode, i_ResourceConfig),
              "Failed to parse texture resource config.");

          ResourceManager::register_asset(
              i_ResourceConfig.textureId,
              Texture::make_from_resource_config(i_ResourceConfig));
        }
      } else {
        // Texture asset type
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(Texture), Renderer::Texture::IDENTIFIER);
        l_Builder.auto_initialize(true)
            .initialize_on_startup(true)
            .add_asset_suffix(".texresource.yaml");
        l_Builder.add_initialize_directory(
            Util::get_project().dataPath, true);
        l_Builder.add_raw_suffix(".png");
        l_Builder
            .add_import_directory(Util::get_project().dataPath, true,
                                  true)
            .import_on_startup(true);

        l_Builder
            .initializer(
                [](const Util::String p_Path) -> Util::Handle {
                  Util::Serial::Node i_ResourceNode =
                      Util::Serial::load_yaml_file(p_Path.c_str());
                  TextureResourceConfig i_ResourceConfig;

                  LOWR_ASSERT_RETURN(
                      ResourceManager::parse_texture_resource_config(
                          p_Path, i_ResourceNode, i_ResourceConfig),
                      "Failed to parse texture resource config.");
                  Texture l_ExistingTexture =
                      ResourceManager::find_asset<Texture>(
                          i_ResourceConfig.textureId);
                  if (!l_ExistingTexture.is_alive()) {
                    ResourceManager::register_asset(
                        i_ResourceConfig.textureId,
                        Texture::make_from_resource_config(
                            i_ResourceConfig));
                  }
                })
            .no_saving()
            .importer([](const Util::String p_Path) -> Util::String {
              std::filesystem::path l_FilePath(p_Path.c_str());

              if (Util::FileSystem::is_file_in_directory(
                      l_FilePath,
                      std::filesystem::path(
                          Util::get_project()
                              .editorImagesPath.c_str()),
                      true)) {
                return "";
              }
              const Util::String l_Output =
                  l_FilePath.replace_extension("").string().c_str();
              if (!ResourceImporter::import_texture(p_Path,
                                                    l_Output)) {
                LOW_LOG_ERROR << "Failed to import texture."
                              << LOW_LOG_END;
                return "";
              }

              return l_Output + ".texresource.yaml";
            });

        l_Builder.raw_deleter([](const Util::String p_Path) {
          std::filesystem::path l_FilePath(p_Path.c_str());

          if (Util::FileSystem::is_file_in_directory(
                  l_FilePath,
                  std::filesystem::path(
                      Util::get_project().editorImagesPath.c_str()),
                  true)) {
            return;
          }

          for (u32 i = 0; i < TextureResource::living_count(); ++i) {
            TextureResource i_Resource =
                TextureResource::living_instances()[i];

            if (i_Resource.get_source_file() == p_Path) {
              Util::FileIO::delete_sync(
                  i_Resource.get_path().c_str());
              break;
            }
          }
        });

        l_Builder.deleter([](const Util::String p_Path) {
          std::filesystem::path l_FilePath(p_Path.c_str());
        });

        Util::AssetManager::register_asset_type(l_Builder.build());
      }

      {
        Util::List<Util::String> l_Resources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(),
            ".fontresource.yaml", l_Resources);

        for (auto it = l_Resources.begin(); it != l_Resources.end();
             ++it) {
          Util::Serial::Node i_ResourceNode =
              Util::Serial::load_yaml_file(it->c_str());
          FontResourceConfig i_ResourceConfig;

          LOWR_ASSERT_RETURN(
              parse_font_resource_config(*it, i_ResourceNode,
                                         i_ResourceConfig),
              "Failed to pass font resource config.");

          ResourceManager::register_asset(
              i_ResourceConfig.fontId,
              Font::make_from_resource_config(i_ResourceConfig));
        }
      }

#if 0
      {
        Util::List<Util::String> l_Resources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(), ".material.yaml",
            l_Resources);

        for (auto it = l_Resources.begin(); it != l_Resources.end();
             ++it) {
          Util::Serial::Node i_Node =
              Util::Serial::load_yaml_file(it->c_str());

          if (!(i_Node["version"] &&
                i_Node["version"].as<u32>() >= 2)) {
            continue;
          }

          Material i_Material =
              Material::deserialize(i_Node, Util::Handle::DEAD);
          MaterialResource i_Resource = MaterialResource::make(*it);
          i_Material.set_resource(i_Resource);
          ResourceManager::register_asset(i_Material.get_unique_id(),
                                          i_Material);
        }
      }
#else
      // Material asset type
      {
        Util::AssetManager::TypeRegistratorBuilder l_Builder(
            N(Material), Renderer::Material::IDENTIFIER);
        l_Builder.auto_initialize(true)
            .initialize_on_startup(true)
            .load_path_property_name(N(path))
            .add_asset_suffix(".materialresource.yaml");

        l_Builder.add_initialize_directory(
            Util::get_project().dataPath, true);
        l_Builder.initializer(
            [](const Util::String p_Path) -> Util::Handle {
              Util::Serial::Node l_ResourceNode =
                  Util::Serial::load_yaml_file(p_Path.c_str());

              MaterialResourceConfig l_ResourceConfig;

              LOWR_ASSERT_RETURN(
                  ResourceManager::parse_material_resource_config(
                      p_Path, l_ResourceNode, l_ResourceConfig),
                  "Failed to parse material resource config.");
              Material l_ExistingMaterial =
                  ResourceManager::find_asset<Material>(
                      l_ResourceConfig.material_id);
              if (!l_ExistingMaterial.is_alive()) {
                ResourceManager::register_asset(
                    l_ResourceConfig.material_id,
                    Material::make_from_resource_config(
                        l_ResourceConfig));
              }
            });

        l_Builder.creatable().creator([](const Util::Name p_Name,
                                         const Util::String p_Path)
                                          -> Util::Handle {
          Material l_Material =
              Material::make(p_Name, get_material_types().solidBase);
          MaterialResourceConfig l_ResourceConfig;
          l_ResourceConfig.material_id = l_Material.get_unique_id();
          l_ResourceConfig.path = p_Path;
          l_ResourceConfig.data_path = Util::project_asset_cache_path(
              Util::hash_to_string(l_Material.get_id()) +
              "material.yaml");
          l_ResourceConfig.name = p_Name;

          MaterialResource l_Resource =
              MaterialResource::make_from_config(l_ResourceConfig);
          l_Material.set_resource(l_Resource);

          return l_Material.get_id();
        });

        l_Builder.saver([](Util::Handle p_Handle) {
          Material l_Material = p_Handle.get_id();
          MaterialResource l_Resource = l_Material.get_resource();

          Util::Serial::Node l_DataNode;
          l_Material.serialize(l_DataNode);

          Util::Serial::Node l_ResourceNode;
          l_ResourceNode["version"] = 1;
          l_ResourceNode["name"] = l_Material.get_name();
          l_ResourceNode["material_id"] =
              Util::U64Id{l_Material.get_unique_id()};

          Util::JobManager::IO::schedule_write_yaml(
              l_Resource.get_data_path().c_str(), l_DataNode);
          Util::JobManager::IO::schedule_write_yaml(
              l_Resource.get_path().c_str(), l_ResourceNode);
        });

        Util::AssetManager::register_asset_type(l_Builder.build());
      }
#endif

      {
        Util::List<Util::String> l_Resources;
        Util::FileSystem::collect_files_with_suffix(
            Util::get_project().dataPath.c_str(), ".model.yaml",
            l_Resources);

        for (auto it = l_Resources.begin(); it != l_Resources.end();
             ++it) {
          Util::Serial::Node i_Node =
              Util::Serial::load_yaml_file(it->c_str());

          Model i_Model =
              Model::deserialize(i_Node, Util::Handle::DEAD);
          ModelResource i_Resource = ModelResource::make(*it);
          i_Model.set_resource(i_Resource);
          ResourceManager::register_asset(i_Model.get_unique_id(),
                                          i_Model);
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
        g_DefaultTexture = Texture::make_gpu_ready(
            N(Default), TextureFormatCategory::Float);
      }

      initialize_primitives();

      LOW_ASSERT(initialize_material_types(),
                 "Failed to initialize material types.");

      LOW_ASSERT(preload_resources(), "Failed to preload resources.");

      LOW_ASSERT(initialize_default_materials(),
                 "Failed to initialize default materials");

      g_GlobalScene = RenderScene::make(N(Global));

      g_GameRenderView = RenderView::make_default(N(Game));
      g_GameRenderView.set_render_scene(g_GlobalScene);

      g_GlobalScene.set_directional_light_color(1.0f, 1.0f, 1.0f);
      g_GlobalScene.set_directional_light_intensity(0.75f);
      g_GlobalScene.set_directional_light_direction(-0.15f, -1.0f,
                                                    -1.5f);

      // TODO: Create separate editor renderview in editor builds
      g_EditorRenderView = g_GameRenderView;

      g_EditorRenderView.add_step_by_name(RENDERSTEP_PICKINGMAP_DRAW);
      g_EditorRenderView.add_step_by_name(
          RENDERSTEP_HIGHLIGHTMAP_DRAW);
      g_EditorRenderView.add_step_by_name(
          RENDERSTEP_HIGHLIGHT_EDGE_DRAW);
      g_EditorRenderView.add_step_by_name(RENDERSTEP_BLUR);
      // g_EditorRenderView = RenderView::make(N(Editor));

      Util::Window::get_main_window().eventCallbacks.push_back(
          &ImGui_ImplSDL2_ProcessEvent);
    }

    void cleanup()
    {
      Vulkan::wait_idle();

      RenderView::ms_FullDestroy = true;
      cleanup_types();
      cleanup_enums();

      LOW_ASSERT(Vulkan::cleanup(),
                 "Failed to cleanup Vulkan renderer");
    }

    void wait_idle()
    {
      Vulkan::wait_idle();
    }

    static bool upload_material(float p_Delta, GpuMaterial p_Gpu)
    {
      size_t l_StagingOffset = 0;

      const u64 l_FrameUploadSpace =
          Vulkan::request_resource_staging_buffer_space(
              MATERIAL_DATA_SIZE, &l_StagingOffset);

      if (l_FrameUploadSpace < MATERIAL_DATA_SIZE) {
        return false;
      }

      LOW_ASSERT(
          Vulkan::resource_staging_buffer_write(
              p_Gpu.get_data(), MATERIAL_DATA_SIZE, l_StagingOffset),
          "Failed to write material data to staging buffer");

      VkBufferCopy l_CopyRegion{};
      l_CopyRegion.srcOffset = l_StagingOffset;
      l_CopyRegion.dstOffset = p_Gpu.get_index() * MATERIAL_DATA_SIZE;
      l_CopyRegion.size = MATERIAL_DATA_SIZE;
      // TODO: Change to transfer queue command buffer
      Vulkan::BufferUtil::cmd_buffer_barrier(
          Vulkan::Global::get_current_command_buffer(),
          Vulkan::Global::get_material_data_buffer(), l_CopyRegion,
          VK_PIPELINE_STAGE_2_TRANSFER_BIT |
              VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
          VK_ACCESS_2_TRANSFER_WRITE_BIT |
              VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
          VK_ACCESS_2_TRANSFER_WRITE_BIT);
      vkCmdCopyBuffer(
          Vulkan::Global::get_current_command_buffer(),
          Vulkan::Global::get_current_resource_staging_buffer()
              .buffer.buffer,
          Vulkan::Global::get_material_data_buffer().buffer, 1,
          &l_CopyRegion);
      Vulkan::BufferUtil::cmd_buffer_barrier(
          Vulkan::Global::get_current_command_buffer(),
          Vulkan::Global::get_material_data_buffer(), l_CopyRegion,
          VK_PIPELINE_STAGE_2_TRANSFER_BIT,
          VK_ACCESS_2_TRANSFER_WRITE_BIT,
          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
              VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
          VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

      p_Gpu.set_dirty(false);
      return true;
    }

    static void tick_materials(float p_Delta)
    {
      Util::UniqueLock<Util::Mutex> l_PendingLock(
          Material::ms_PendingTextureBindingsMutex);
      for (auto it = Material::ms_PendingTextureBindings.begin();
           it != Material::ms_PendingTextureBindings.end();) {
        if (!it->texture.is_alive() || !it->material.is_alive()) {
          it = Material::ms_PendingTextureBindings.erase(it);
          continue;
        }

        if (it->texture.get_state() == TextureState::LOADED) {
          it->material.set_property_texture(it->propertyName,
                                            it->texture);
          break;
        }

        ++it;
      }

      for (u32 i = 0; i < GpuMaterial::living_count(); ++i) {
        GpuMaterial i_Gpu = GpuMaterial::living_instances()[i];
        Material i_Material = i_Gpu.get_material_handle();

        if (!i_Gpu.is_dirty()) {
          continue;
        }

        if (!upload_material(p_Delta, i_Gpu)) {
          // If we ever fail to upload a material (most likely
          // because there is not enough space left on the staging
          // buffer), we just stop trying for this frame
          break;
        }
        i_Material.set_state(MaterialState::LOADED);
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

        {
          SS2DCanvas l_Canvas = SS2DCanvas::make(N(Test));
          l_Canvas.set_dimensions(1024, 1024);

          {
            SS2DDrawCommand l_DC = SS2DDrawCommand::make(
                N(Rect1), l_Canvas, SS2DType::Rect);
            l_DC.set_position(100.0f, 100.0f);
            l_DC.set_half_extents(20, 30);
            l_DC.set_color(Math::Color(1.0f, 0.0f, 0.0f, 1.0f));
          }

          {
            SS2DDrawCommand l_DC = SS2DDrawCommand::make(
                N(Circle1), l_Canvas, SS2DType::Circle);
            l_DC.set_position(500.0f, 100.0f);
            l_DC.set_radius(20.0f);
            l_DC.set_color(Math::Color(0.0f, 1.0f, 0.0f, 1.0f));
          }

          {
            SS2DDrawCommand l_DC = SS2DDrawCommand::make(
                N(RoundRect1), l_Canvas, SS2DType::RoundedRect);
            l_DC.set_position(100.0f, 300.0f);
            l_DC.set_half_extents(30.0f, 20.0f);
            l_DC.set_color(Math::Color(0.0f, 0.0f, 1.0f, 1.0f));
            l_DC.set_corner_radius(
                Math::Vector4(0, 12.0f, 12.0f, 22.0f));
          }

          g_SS2DCanvas = l_Canvas;
        }
        l_Initialized = true;
      }
    }

    static bool tick_thumbnail_creation(float p_Delta)
    {
      auto calculate_sphere_fit_distance =
          [](const Math::Sphere &p_Sphere,
             const RenderView p_View) -> float {
        const Math::UVector2 l_Dimensions = p_View.get_dimensions();
        const float l_Aspect =
            l_Dimensions.y > 0u
                ? (float)l_Dimensions.x / (float)l_Dimensions.y
                : 1.0f;
        const float l_VerticalFov =
            glm::radians(p_View.get_camera_fov());
        const float l_VerticalHalfTan =
            glm::tan(l_VerticalFov * 0.5f);
        const float l_HorizontalHalfTan =
            l_VerticalHalfTan * l_Aspect;
        const float l_Radius = glm::max(p_Sphere.radius, 0.05f);
        constexpr float l_Margin = 1.25f;

        const float l_VerticalDistance =
            l_Radius / glm::max(l_VerticalHalfTan, 0.001f);
        const float l_HorizontalDistance =
            l_Radius / glm::max(l_HorizontalHalfTan, 0.001f);

        return glm::max(l_VerticalDistance, l_HorizontalDistance) *
               l_Margin;
      };

      for (auto it = g_ThumbnailCreationSchedules.begin();
           it != g_ThumbnailCreationSchedules.end();) {
        if (it->state == ThumbnailCreationState::SCHEDULED) {
          if (it->mesh.get_state() != MeshState::LOADED) {
            ++it;
            continue;
          }
          if (it->material.is_alive() &&
              it->material.get_state() != MaterialState::LOADED) {
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
              (i_Direction *
               calculate_sphere_fit_distance(i_Sphere, it->view)));

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

      /*
      if (g_SS2DCanvas.is_alive() &&
          g_SS2DCanvas.get_out_image().is_alive() &&
          g_SS2DCanvas.get_out_image().get_gpu().is_alive()) {
        ImGui::Begin("TestOutput");
        ImGui::Image(g_SS2DCanvas.get_out_image()
                         .get_gpu()
                         .get_imgui_texture_id(),
                     ImVec2(1024, 1024));
        ImGui::End();
      }
      */

      ResourceManager::tick(p_Delta);
      SkinningSystem::tick(p_Delta);
      RenderObjectSystem::tick(p_Delta);
      tick_materials(p_Delta);

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
      _LOW_ASSERT(p_Schedule.view.is_alive());
      _LOW_ASSERT(p_Schedule.scene.is_alive());
      _LOW_ASSERT(p_Schedule.view.get_render_scene().is_alive());
      _LOW_ASSERT(p_Schedule.view.get_render_scene() ==
                  p_Schedule.scene);
      g_ThumbnailCreationSchedules.push_back(p_Schedule);
    }

    RenderView get_game_renderview()
    {
      return g_GameRenderView;
    }
    RenderView get_editor_renderview()
    {
      return g_EditorRenderView;
    }

    RenderScene get_global_renderscene()
    {
      return g_GlobalScene;
    }
  } // namespace Renderer
} // namespace Low
