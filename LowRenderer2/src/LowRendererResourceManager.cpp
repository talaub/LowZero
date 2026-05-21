#include "LowRendererResourceManager.h"

#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererEditorImageGpu.h"
#include "LowRendererEditorImageStaging.h"
#include "LowRendererMaterialState.h"
#include "LowRendererMaterialType.h"
#include "LowRendererMeshResourceState.h"
#include "LowRendererMeshState.h"
#include "LowRendererMeshType.h"
#include "LowRendererTextureResource.h"
#include "LowRendererTextureState.h"
#include "LowRendererVulkanRenderer.h"
#include "LowRendererVulkanImage.h"
#include "LowRendererVkImage.h"
#include "LowRendererGlobals.h"
#include "LowRendererVulkan.h"
#include "LowRendererMeshGeometry.h"
#include "LowRendererGpuMesh.h"
#include "LowRendererGpuSubmesh.h"
#include "LowRendererSubmeshGeometry.h"
#include "LowRendererTextureStaging.h"
#include "LowRendererBase.h"
#include "LowRendererVulkanBuffer.h"

#include "LowUtil.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilJobManager.h"
#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilResource.h"
#include "LowUtilSerialization.h"
#include "LowUtilHashing.h"
#include "LowUtilString.h"

#include <iostream>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"

#define MESH_INDEX_SIZE sizeof(u32)

namespace Low {
  namespace Renderer {
    namespace ResourceManager {

      Util::Map<u64, u64> g_AssetRegistry;

      static bool mesh_schedule_gpu_upload(Mesh p_Mesh);
      static bool material_schedule_gpu_upload(Material p_Material);

      struct MeshEntry
      {
        union
        {
          struct
          {
            u32 lodPriority;
            u32 resourcePriority;
          };
          u64 priority;
        };

        Mesh mesh;

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
        Util::Set<Mesh> meshes;
        Util::Set<Texture> textures;
        Util::Set<Font> fonts;
        Util::Set<EditorImage> editorImages;
        Util::Set<Material> materials;
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
            Texture texture;
          } texture;
          struct
          {
            Mesh mesh;
            SubmeshGeometry submeshGeometry;
            GpuSubmesh gpuSubmesh;
            u32 submeshIndex;
          } mesh;
          struct
          {
            EditorImage ei;
          } editorImage;
        } data;

        bool is_texture_upload()
        {
          return Texture::is_alive(data.texture.texture);
        }
        bool is_mesh_upload()
        {
          return Mesh::is_alive(data.mesh.mesh);
        }
        bool is_editor_image_upload()
        {
          return EditorImage::is_alive(data.editorImage.ei);
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

      static bool texture_can_load(Texture p_Texture)
      {
        return p_Texture.get_state() == TextureState::UNLOADED &&
               p_Texture.get_resource().is_alive() &&
               !p_Texture.get_gpu().is_alive();
      }

      bool override_loaded_texture(Texture p_Texture)
      {
        if (p_Texture.get_state() != TextureState::LOADED) {
          return false;
        }
        const u64 l_TextureId =
            p_Texture.get_resource().get_texture_id();

        if (l_TextureId != 0) {
          register_asset(l_TextureId, p_Texture);
        }

        p_Texture.get_gpu().loaded_mips().clear();
        p_Texture.set_state(TextureState::SCHEDULEDTOLOAD);

        g_LoadSchedules.textures.insert(p_Texture);
        return true;
      }

      bool load_texture(Texture p_Texture)
      {
        // Skip out on scheduling the load for this texture
        // because it is either already loaded or in the process of
        // being loaded/unloaded
        if (!texture_can_load(p_Texture)) {
          return false;
        }

        const u64 l_TextureId =
            p_Texture.get_resource().get_texture_id();

        if (l_TextureId != 0) {
          register_asset(l_TextureId, p_Texture);
        }

        p_Texture.set_state(TextureState::SCHEDULEDTOLOAD);

        g_LoadSchedules.textures.insert(p_Texture);

        return true;
      }

      static bool material_schedule_memory_load(Material p_Material)
      {
        const u64 l_HandleId = p_Material.get_id();
        const Util::String l_DataPath =
            p_Material.get_resource().get_data_path();

        p_Material.set_state(MaterialState::LOADINGTOMEMORY);

        Util::JobManager::IO::schedule_read_yaml(
            l_DataPath,
            [l_HandleId](bool p_Success, Util::Serial::Node &p_Node) {
              Material l_Material = l_HandleId;
              LOW_ASSERT(
                  (bool)p_Node["material_type"],
                  "Material file does not contain material type.");

              Util::Name l_MaterialTypeName =
                  p_Node["material_type"].as<Util::Name>();

              MaterialType l_MaterialType =
                  MaterialType::find_by_name(l_MaterialTypeName);
              if (!l_MaterialType.is_alive()) {
                l_MaterialType =
                    MaterialType::find_by_name(N(solid_base));
                LOW_LOG_WARN
                    << "Could not find material type with name "
                    << l_MaterialTypeName
                    << " falling back so solid_base." << LOW_LOG_END;
              }

              l_Material.set_material_type(l_MaterialType);

              Util::List<Util::Name> l_PropertyNames;
              l_MaterialType.fill_input_names(l_PropertyNames);

              l_Material.get_properties().clear();

              Util::Serial::Node &l_PropertiesNode =
                  p_Node["properties"];

              for (Util::Name i_Name : l_PropertyNames) {
                MaterialTypeInputType i_InputType =
                    l_MaterialType.get_input_type(i_Name);
                if (l_PropertiesNode[i_Name.c_str()]) {
                  switch (i_InputType) {
                  case MaterialTypeInputType::FLOAT:
                    l_Material.set_property_float(
                        i_Name,
                        l_PropertiesNode[i_Name.c_str()].as<float>());
                    break;
                  case MaterialTypeInputType::VECTOR2:
                    l_Material.set_property_vector2(
                        i_Name, l_PropertiesNode[i_Name.c_str()]
                                    .as<Math::Vector2>());
                    break;
                  case MaterialTypeInputType::VECTOR3:
                    l_Material.set_property_vector3(
                        i_Name, l_PropertiesNode[i_Name.c_str()]
                                    .as<Math::Vector3>());
                    break;
                  case MaterialTypeInputType::VECTOR4:
                    l_Material.set_property_vector4(
                        i_Name, l_PropertiesNode[i_Name.c_str()]
                                    .as<Math::Vector4>());
                    break;
                  case MaterialTypeInputType::TEXTURE: {
                    Texture i_Texture =
                        ResourceManager::find_asset<Texture>(
                            l_PropertiesNode[i_Name.c_str()]
                                .as<Util::U64Id>());
                    l_Material.set_property_texture(i_Name,
                                                    i_Texture);
                    break;
                  }
                  default:
                    LOW_ASSERT(false,
                               "Unsupported material input type.");
                    break;
                  }
                } else {
                  switch (i_InputType) {
                  case MaterialTypeInputType::FLOAT:
                    l_Material.set_property_float(i_Name, 0.0f);
                    break;
                  case MaterialTypeInputType::VECTOR2:
                    l_Material.set_property_vector2(
                        i_Name, Math::Vector2(0, 0));
                    break;
                  case MaterialTypeInputType::VECTOR3:
                    l_Material.set_property_vector3(
                        i_Name, Math::Vector3(0, 0, 0));
                    break;
                  case MaterialTypeInputType::VECTOR4:
                    l_Material.set_property_vector4(
                        i_Name, Math::Vector4(0, 0, 0, 1));
                    break;
                  case MaterialTypeInputType::TEXTURE: {
                    l_Material.set_property_texture(
                        i_Name, get_default_texture());
                    break;
                  }
                  default:
                    LOW_ASSERT(false,
                               "Unsupported material input type.");
                    break;
                  }
                }
              }
              l_Material.set_state(MaterialState::MEMORYLOADED);
            });
        return true;
      }

      bool load_material(Material p_Material)
      {
        if (p_Material.is_alive()) {
          register_asset(p_Material.get_unique_id(), p_Material);
        }

        if (p_Material.get_state() == MaterialState::MEMORYLOADED) {
          LOW_ASSERT_ERROR_RETURN_FALSE(
              material_schedule_gpu_upload(p_Material),
              "Failed to schedule material for GPU upload.");

          return true;
        } else if (p_Material.get_state() ==
                   MaterialState::UNLOADED) {
          p_Material.set_state(MaterialState::SCHEDULEDTOLOAD);
          g_LoadSchedules.materials.insert(p_Material);
        }
        return false;
      }

      bool load_font(Font p_Font)
      {
        g_LoadSchedules.fonts.insert(p_Font);

        register_asset(p_Font.get_resource().get_font_id(), p_Font);

        return true;
      }

      static bool mesh_can_load(Mesh p_Mesh)
      {
        return p_Mesh.get_state() == MeshState::UNLOADED &&
               !p_Mesh.get_geometry().is_alive() &&
               !p_Mesh.get_gpu().is_alive() &&
               p_Mesh.get_resource().is_alive();
      }

      bool reload_mesh(Mesh p_Mesh)
      {
        if (p_Mesh.get_state() == MeshState::LOADED) {
          LOW_ASSERT_ERROR_RETURN_FALSE(unload_mesh(p_Mesh),
                                        "Failed to unload mesh.");
        }
        LOW_ASSERT_ERROR_RETURN_FALSE(
            ResourceManager::load_mesh(p_Mesh),
            "Failed to load mesh.");

        return true;
      }

      bool unload_mesh(Mesh p_Mesh)
      {
        LOW_ASSERT_ERROR_RETURN_FALSE(
            p_Mesh.get_state() == MeshState::LOADED,
            "Cannot unload mesh that is not loaded.");

        GpuMesh l_Gpu = p_Mesh.get_gpu();

        for (GpuSubmesh i_Submesh : l_Gpu.get_submeshes()) {
          Vulkan::Global::get_mesh_vertex_buffer().free(
              i_Submesh.get_vertex_start(),
              i_Submesh.get_vertex_count());
          Vulkan::Global::get_mesh_index_buffer().free(
              i_Submesh.get_index_start(),
              i_Submesh.get_index_count());
          i_Submesh.destroy();
        }
        l_Gpu.destroy();

        for (auto it = g_MeshEntriesContainer.begin();
             it != g_MeshEntriesContainer.end();) {
          if (it->mesh == p_Mesh) {
            it = g_MeshEntriesContainer.erase(it);
          } else {
            ++it;
          }
        }

        p_Mesh.set_state(MeshState::UNLOADED);

        return true;
      }

      bool load_mesh(Mesh p_Mesh)
      {
        if (p_Mesh.is_alive() && p_Mesh.get_resource().is_alive()) {
          register_asset(p_Mesh.get_resource().get_mesh_id(), p_Mesh);
        }

        if (p_Mesh.is_alive() &&
            p_Mesh.get_state() == MeshState::MEMORYLOADED &&
            p_Mesh.get_geometry().is_alive()) {

          LOW_ASSERT_ERROR_RETURN_FALSE(
              mesh_schedule_gpu_upload(p_Mesh),
              "Failed to schedule mesh for GPU upload. Mesh was "
              "submitted for load to resource manager but is already "
              "marked as memoryloaded.");

          return true;
        }
        // Skip out on scheduling the load for this mesh resource
        // because it is either already loaded or in the process of
        // being loaded/unloaded
        if (!mesh_can_load(p_Mesh)) {
          return false;
        }

        p_Mesh.set_state(MeshState::SCHEDULEDTOLOAD);

        g_LoadSchedules.meshes.insert(p_Mesh);

        return true;
      }

      bool load_editor_image(EditorImage p_EditorImage)
      {
        if (p_EditorImage.get_state() == TextureState::UNLOADED) {
          g_LoadSchedules.editorImages.insert(p_EditorImage);

          p_EditorImage.set_state(TextureState::SCHEDULEDTOLOAD);

          return true;
        }
        return false;
      }

      static VkFormat
      get_vk_format(Util::Resource::Image2DFormat p_Format)
      {
        switch (p_Format) {
        case Util::Resource::Image2DFormat::R8:
          return VK_FORMAT_R8_UNORM;
        case Util::Resource::Image2DFormat::RGBA8:
          return VK_FORMAT_R8G8B8A8_UNORM;
        case Util::Resource::Image2DFormat::RGB8:
          return VK_FORMAT_R8G8B8_UNORM;
        }
        LOW_ASSERT(false, "Unsupported util format");

        return VK_FORMAT_UNDEFINED;
      }

      static bool texture_schedule_gpu_upload(Texture p_Texture)
      {
        const Math::UVector2 l_Dimensions =
            p_Texture.get_staging().get_mip0().get_dimensions();

        for (int i = IMAGE_MIPMAP_COUNT - 1; i >= 0; --i) {
          UploadEntry i_Entry;
          i_Entry.uploadedSize = 0;
          i_Entry.data.texture.texture = p_Texture;
          i_Entry.progressPriority = 0;
          i_Entry.lodPriority = i;
          i_Entry.waitPriority = 0;
          i_Entry.resourcePriority = 0;
          g_UploadSchedules.emplace(i_Entry);
        }

        GpuTexture l_GpuTexture = p_Texture.get_gpu();

        if (!l_GpuTexture.is_alive()) {
          if (GpuTexture::living_count() >=
              GpuTexture::get_capacity()) {
            // We have too many gpu textures living and we have to
            // unload one first in order to be able to create this
            // one.
            // TODO: Implement
            LOW_NOT_IMPLEMENTED;
          }
          l_GpuTexture = GpuTexture::make(p_Texture.get_name());
          p_Texture.set_gpu(l_GpuTexture);
        } else {
          Vulkan::Image l_OldImage = l_GpuTexture.get_data_handle();
          if (l_OldImage.is_alive()) {
            Vulkan::ImageUtil::destroy(l_OldImage);
            l_OldImage.destroy();
          }
        }

        Vulkan::Image l_Image =
            Vulkan::Image::make(p_Texture.get_name());

        VkExtent3D l_Extent;
        // Uses the mip0 extent since it's the full resolution
        l_Extent.width = l_Dimensions.x;
        l_Extent.height = l_Dimensions.y;
        l_Extent.depth = 1;

        Vulkan::ImageUtil::create(
            l_Image, l_Extent,
            get_vk_format(
                p_Texture.get_staging().get_mip0().get_format()),
            VK_IMAGE_USAGE_SAMPLED_BIT, true);
        l_GpuTexture.set_data_handle(l_Image.get_id());
        // TODO: Don't hardcode it to 4
        l_GpuTexture.set_full_mip_count(IMAGE_MIPMAP_COUNT);

        // TODO: Should be changed to transfer command buffer
        Vulkan::ImageUtil::cmd_transition(
            Vulkan::Global::get_current_command_buffer(), l_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        p_Texture.set_state(TextureState::UPLOADINGTOGPU);
        return true;
      }

      static bool
      editor_image_schedule_gpu_upload(EditorImage p_EditorImage)
      {
        const Math::UVector2 l_Dimensions =
            p_EditorImage.get_staging().get_dimensions();

        UploadEntry i_Entry;
        i_Entry.uploadedSize = 0;
        i_Entry.data.editorImage.ei = p_EditorImage;
        i_Entry.progressPriority = 0;
        i_Entry.lodPriority = 0;
        i_Entry.waitPriority = 0;
        i_Entry.resourcePriority = 1;
        g_UploadSchedules.emplace(i_Entry);

        Vulkan::Image l_Image =
            Vulkan::Image::make(p_EditorImage.get_name());

        VkExtent3D l_Extent;
        l_Extent.width = l_Dimensions.x;
        l_Extent.height = l_Dimensions.y;
        l_Extent.depth = 1;

        Vulkan::ImageUtil::create(
            l_Image, l_Extent,
            get_vk_format(p_EditorImage.get_staging().get_format()),
            VK_IMAGE_USAGE_SAMPLED_BIT, true);

        EditorImageGpu l_Gpu =
            EditorImageGpu::make(p_EditorImage.get_name());
        p_EditorImage.set_gpu(l_Gpu);
        l_Gpu.set_data_handle(l_Image.get_id());

        // TODO: Should be changed to transfer command buffer
        Vulkan::ImageUtil::cmd_transition(
            Vulkan::Global::get_current_command_buffer(), l_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        p_EditorImage.set_state(TextureState::UPLOADINGTOGPU);
        return true;
      }

      static TexturePixels
      create_texture_pixels(Util::Name p_Name,
                            Util::Resource::Image2D &p_Image2D)
      {
        TexturePixels l_Pixels = TexturePixels::make(N(Mip0));

        l_Pixels.set_state(TextureState::MEMORYLOADED);
        // TODO: Maybe not force 4 channels and not hardcode format
        l_Pixels.set_channels(4);
        l_Pixels.set_format(p_Image2D.format);
        /*
        for (u32 i = 0; i < l_Pixels.get_pixel_data().size(); ++i) {
          l_Pixels.get_pixel_data()[i] = p_Image2D.data[i];
        }
        */
        l_Pixels.set_pixel_data(p_Image2D.data);
        l_Pixels.set_data_size(p_Image2D.size);
        l_Pixels.set_dimensions(p_Image2D.dimensions);

        return l_Pixels;
      }

      static bool font_schedule_memory_load(Font p_Font)
      {
        u64 l_FontId = p_Font.get_id();
        Util::String l_SidecarPath =
            p_Font.get_resource().get_sidecar_path();

        Util::JobManager::IO::schedule_read_yaml(
            l_SidecarPath,
            [l_FontId](bool p_Success, Util::Serial::Node &p_Node) {
              Font l_Font = l_FontId;

              l_Font.set_ascender(p_Node["ascender"].as<float>());
              l_Font.set_descender(p_Node["descender"].as<float>());
              l_Font.set_line_height(
                  p_Node["line_height"].as<float>());
              l_Font.set_import_height(
                  p_Node["import_height"].as<float>());

              for (auto [k, v] : p_Node["glyphs"]) {
                Util::Serial::Node i_Node = v;
                Glyph i_Glyph;
                char i_Codepoint = i_Node["codepoint"].as<u8>();
                i_Glyph.uvMin = i_Node["uv_min"].as<Math::Vector2>();
                i_Glyph.uvMax = i_Node["uv_max"].as<Math::Vector2>();
                i_Glyph.bearing =
                    i_Node["bearing"].as<Math::Vector2>();
                i_Glyph.size = i_Node["size"].as<Math::Vector2>();
                i_Glyph.advance = i_Node["advance"].as<float>();
                l_Font.get_glyphs()[i_Codepoint] = i_Glyph;
              }

              l_Font.set_sidecar_loaded(true);

              if (l_Font.get_texture().get_state() ==
                  TextureState::UNLOADED) {
                load_texture(l_Font.get_texture());
              }
            });

        return true;
      }

      static bool texture_schedule_memory_load(Texture p_Texture)
      {
        LOW_ASSERT(p_Texture.get_state() ==
                       TextureState::SCHEDULEDTOLOAD,
                   "Texture is either not scheduled for loading or "
                   "already loading/loaded.");

        u64 l_TextureId = p_Texture.get_id();
        Util::String l_Path =
            p_Texture.get_resource().get_texture_path();

        p_Texture.set_state(TextureState::LOADINGTOMEMORY);

        Util::JobManager::IO::schedule_read_texture(
            l_Path,
            [l_TextureId](bool p_Success,
                          Util::Resource::ImageMipMaps &p_MipMaps) {
              Texture l_Texture = l_TextureId;

              TextureStaging l_Staging =
                  TextureStaging::make(l_Texture.get_name());
              l_Texture.set_staging(l_Staging);

              // TODO: Remove requirement for all miplevels to be
              // present
              l_Staging.set_mip0(
                  create_texture_pixels(N(Mip0), p_MipMaps.mip0));
              l_Staging.set_mip1(
                  create_texture_pixels(N(Mip1), p_MipMaps.mip1));
              l_Staging.set_mip2(
                  create_texture_pixels(N(Mip2), p_MipMaps.mip2));
              l_Staging.set_mip3(
                  create_texture_pixels(N(Mip3), p_MipMaps.mip3));

              l_Texture.set_state(TextureState::MEMORYLOADED);
            });

        return true;
      }

      static bool
      edtior_image_schedule_memory_load(EditorImage p_EditorImage)
      {
        LOW_ASSERT(
            p_EditorImage.get_state() ==
                TextureState::SCHEDULEDTOLOAD,
            "EditorImage is either not scheduled for loading or "
            "already loading/loaded.");

        u64 l_EditorImageId = p_EditorImage.get_id();
        Util::String l_Path = p_EditorImage.get_path();

        p_EditorImage.set_state(TextureState::LOADINGTOMEMORY);

        Util::JobManager::IO::schedule_read_raw(
            l_Path,
            [l_EditorImageId](bool p_Success,
                              Util::List<uint8_t> &p_RawData) {
              EditorImage l_EditorImage = l_EditorImageId;

              int l_Width, l_Height, l_Channels;
              const uint8_t *l_Data = stbi_load_from_memory(
                  p_RawData.data(), (int)p_RawData.size(), &l_Width,
                  &l_Height, &l_Channels, 0);

              EditorImageStaging l_Staging =
                  EditorImageStaging::make(l_EditorImage.get_name());
              l_EditorImage.set_staging(l_Staging);

              l_Staging.set_channels(l_Channels);
              l_Staging.set_data_size(l_Width * l_Height *
                                      l_Channels);
              l_Staging.set_dimensions_x(l_Width);
              l_Staging.set_dimensions_y(l_Height);
              l_Staging.set_format(
                  Util::Resource::Image2DFormat::RGBA8);
              l_Staging.get_pixel_data().resize(l_Width * l_Height *
                                                4);

              for (uint32_t i_Y = 0u; i_Y < l_Height; ++i_Y) {
                for (uint32_t i_X = 0u; i_X < l_Width; ++i_X) {
                  uint32_t i_C = 0;
                  for (; i_C < l_Channels; ++i_C) {
                    l_Staging.get_pixel_data()
                        [(((i_Y * l_Width) + i_X) * 4) + i_C] =
                        l_Data[(((i_Y * l_Width) + i_X) *
                                l_Channels) +
                               i_C];
                  }
                  for (; i_C < 4; ++i_C) {
                    l_Staging.get_pixel_data()
                        [(((i_Y * l_Width) + i_X) * 4) + i_C] = 255;
                  }
                }
              }

              stbi_image_free((void *)l_Data);

              l_EditorImage.set_state(TextureState::MEMORYLOADED);
            });

        return true;
      }

      static bool mesh_schedule_memory_load(Mesh p_Mesh)
      {
        LOW_ASSERT(p_Mesh.get_state() == MeshState::SCHEDULEDTOLOAD,
                   "Mesh is either not scheduled for loading or "
                   "already loading/loaded.");

        u64 l_MeshId = p_Mesh.get_id();
        Util::String l_MeshPath =
            p_Mesh.get_resource().get_mesh_path();
        Util::String l_SidecarPath =
            p_Mesh.get_resource().get_sidecar_path();

        p_Mesh.set_state(MeshState::LOADINGTOMEMORY);

        Util::JobManager::IO::schedule_read_mesh(
            l_MeshPath, l_SidecarPath,
            [l_MeshId](
                bool p_Success,
                Util::JobManager::IO::MeshLoadResult &p_Result) {
              Mesh l_Mesh = l_MeshId;

              MeshGeometry l_MeshGeometry =
                  MeshGeometry::make(l_Mesh.get_name());
              l_Mesh.set_geometry(l_MeshGeometry);

              l_MeshGeometry.set_aabb(p_Result.aabb);
              l_MeshGeometry.set_bounding_sphere(
                  p_Result.bounding_sphere);

              Skeleton l_Skeleton = Util::find_handle_by_unique_id(
                  p_Result.skeleton_id);
              l_Mesh.set_skeleton(l_Skeleton);
              if (!l_Skeleton.is_alive() &&
                  p_Result.skeleton_id != 0) {
                Util::resolve_handle_reference_by_unique_id(
                    l_Mesh, N(skeleton), p_Result.skeleton_id);
              }

              u32 l_SubmeshCount = 0u;
              for (auto &i_Submesh : p_Result.mesh.submeshes) {
                for (auto &i_MeshInfo : i_Submesh.meshInfos) {
                  SubmeshGeometry i_SubmeshGeometry =
                      SubmeshGeometry::make(i_MeshInfo.name);
                  i_SubmeshGeometry.set_vertex_count(
                      i_MeshInfo.vertices.size());
                  i_SubmeshGeometry.set_index_count(
                      i_MeshInfo.indices.size());
                  i_SubmeshGeometry.set_state(
                      MeshState::MEMORYLOADED);
                  i_SubmeshGeometry.set_vertices(i_MeshInfo.vertices);
                  i_SubmeshGeometry.set_indices(i_MeshInfo.indices);
                  i_SubmeshGeometry.set_transform(
                      i_Submesh.transform);
                  i_SubmeshGeometry.set_parent_transform(
                      i_Submesh.parentTransform);
                  i_SubmeshGeometry.set_local_transform(
                      i_Submesh.localTransform);

                  auto i_AabbIt =
                      p_Result.submesh_aabbs.find(i_MeshInfo.name);
                  if (i_AabbIt != p_Result.submesh_aabbs.end()) {
                    i_SubmeshGeometry.set_aabb(i_AabbIt->second);
                  }

                  auto i_SphereIt =
                      p_Result.submesh_bounding_spheres.find(
                          i_MeshInfo.name);
                  if (i_SphereIt !=
                      p_Result.submesh_bounding_spheres.end()) {
                    i_SubmeshGeometry.set_bounding_sphere(
                        i_SphereIt->second);
                  }

                  l_MeshGeometry.get_submeshes().push_back(
                      i_SubmeshGeometry);
                  l_SubmeshCount++;
                }
              }

              l_MeshGeometry.set_submesh_count(l_SubmeshCount);
              l_Mesh.set_submesh_count(l_SubmeshCount);
              l_Mesh.set_state(MeshState::MEMORYLOADED);
            });

        return true;
      }

      static bool mesh_schedule_gpu_upload(Mesh p_Mesh)
      {
        LOW_ASSERT(!p_Mesh.get_gpu().is_alive(),
                   "This mesh already has a GPU entry.");
        LOW_ASSERT(p_Mesh.get_geometry().is_alive(),
                   "This mesh cannot be uploaded to the gpu because "
                   "it does not have any geometry.");
        LOW_ASSERT(p_Mesh.get_state() == MeshState::MEMORYLOADED,
                   "This mesh cannot be uploaded to the gpu because "
                   "it is not marked as memory loaded.");

        GpuMesh l_GpuMesh = GpuMesh::make(p_Mesh.get_name());
        l_GpuMesh.set_uploaded_submesh_count(0);

        l_GpuMesh.set_bounding_sphere(
            p_Mesh.get_geometry().get_bounding_sphere());
        l_GpuMesh.set_aabb(p_Mesh.get_geometry().get_aabb());

        p_Mesh.set_gpu(l_GpuMesh);

        int l_Index = 0;
        for (auto it = p_Mesh.get_geometry().get_submeshes().begin();
             it != p_Mesh.get_geometry().get_submeshes().end();
             ++it) {

          LOW_ASSERT(it->is_alive(),
                     "Cannot upload dead submesh geometry to gpu.");
          GpuSubmesh i_GpuSubmesh = GpuSubmesh::make(it->get_name());
          i_GpuSubmesh.set_state(MeshState::MEMORYLOADED);
          i_GpuSubmesh.set_vertex_count(it->get_vertex_count());
          i_GpuSubmesh.set_index_count(it->get_index_count());
          i_GpuSubmesh.set_uploaded_vertex_count(0);
          i_GpuSubmesh.set_uploaded_index_count(0);

          i_GpuSubmesh.set_bounding_sphere(it->get_bounding_sphere());
          i_GpuSubmesh.set_aabb(it->get_aabb());
          i_GpuSubmesh.set_transform(it->get_transform());
          i_GpuSubmesh.set_parent_transform(
              it->get_parent_transform());
          i_GpuSubmesh.set_local_transform(it->get_local_transform());

          l_GpuMesh.get_submeshes().push_back(i_GpuSubmesh);

          UploadEntry i_Entry;
          i_Entry.data.mesh.mesh = p_Mesh;
          i_Entry.data.mesh.submeshIndex = l_Index;
          i_Entry.data.mesh.gpuSubmesh = i_GpuSubmesh;
          i_Entry.data.mesh.submeshGeometry = it->get_id();
          i_Entry.progressPriority = 0;
          i_Entry.lodPriority = 0;
          i_Entry.waitPriority = 0;
          i_Entry.resourcePriority = 0;
          i_Entry.uploadedSize = 0;
          g_UploadSchedules.emplace(i_Entry);

          l_Index++;
        }

        p_Mesh.get_gpu().set_submesh_count(l_Index);

        p_Mesh.set_state(MeshState::UPLOADINGTOGPU);
        return true;
      }

      static bool material_schedule_gpu_upload(Material p_Material)
      {
        LOW_ASSERT_ERROR_RETURN_FALSE(
            p_Material.is_alive(),
            "Tried to gpu load material but it was dead");
        LOW_ASSERT_ERROR_RETURN_FALSE(
            p_Material.get_state() == MaterialState::MEMORYLOADED,
            "Cannot gpu load material that is not in state memory "
            "loaded.");
        LOW_ASSERT_ERROR_RETURN_FALSE(
            !p_Material.get_gpu().is_alive(),
            "Cannot gpu load material that already has a GPUMaterial "
            "assigned.");

        if (GpuMaterial::living_count() >=
            GpuMaterial::get_capacity()) {
          LOW_NOT_IMPLEMENTED;
          // TODO: Unload materials?
        }

        p_Material.set_gpu(GpuMaterial::make(p_Material.get_name()));
        p_Material.update_gpu();

        p_Material.set_state(MaterialState::UPLOADINGTOGPU);

        return true;
      }

      static bool materials_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.materials.begin();
             it != g_LoadSchedules.materials.end();) {
          if (it->get_state() == MaterialState::MEMORYLOADED) {
            if (!material_schedule_gpu_upload(*it)) {
              LOW_LOG_ERROR << "Failed to schedule Material '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() ==
                     MaterialState::SCHEDULEDTOLOAD) {
            if (!material_schedule_memory_load(*it)) {
              LOW_LOG_ERROR
                  << "Failed to schedule material '" << it->get_name()
                  << "' for loading to memory." << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() == MaterialState::LOADED ||
                     it->get_state() == MaterialState::UNLOADED ||
                     it->get_state() == MaterialState::UNKNOWN) {
            it = g_LoadSchedules.materials.erase(it);
          } else {
            it++;
          }
        }

        return true;
      }

      static bool meshes_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.meshes.begin();
             it != g_LoadSchedules.meshes.end();) {
          if (it->get_state() == MeshState::MEMORYLOADED) {
            // As soon as we see that the mesh has
            // successfully been loaded to memory we'll start
            // uploading the data to the GPU
            if (!mesh_schedule_gpu_upload(*it)) {
              LOW_LOG_ERROR << "Failed to schedule Mesh '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() == MeshState::SCHEDULEDTOLOAD) {
            // If the mesh is scheduled to be loaded but
            // currently still marked as unloaded we will try to
            // schedule the memory load.
            if (!mesh_schedule_memory_load(*it)) {
              LOW_LOG_ERROR
                  << "Failed to schedule mesh '" << it->get_name()
                  << "' for loading to memory." << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() == MeshState::LOADED ||
                     it->get_state() == MeshState::UNLOADED ||
                     it->get_state() == MeshState::UNKNOWN) {
            it = g_LoadSchedules.meshes.erase(it);
          } else {
            it++;
          }
        }

        return true;
      }

      static bool textures_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.textures.begin();
             it != g_LoadSchedules.textures.end();) {
          TextureState i_State = it->get_state();
          if (it->get_state() == TextureState::MEMORYLOADED) {
            // As soon as we see that the texture has
            // successfully been loaded to memory we'll start
            // uploading the data to the GPU
            if (texture_schedule_gpu_upload(*it)) {
              it = g_LoadSchedules.textures.erase(it);
            } else {
              LOW_LOG_ERROR << "Failed to schedule texture '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
              it++;
            }
            continue;
          } else if (it->get_state() ==
                     TextureState::SCHEDULEDTOLOAD) {
            // If the texture is scheduled to be loaded but
            // currently still marked as unloaded we will try to
            // schedule the memory load.
            if (!texture_schedule_memory_load(*it)) {
              LOW_LOG_ERROR
                  << "Failed to schedule texture '" << it->get_name()
                  << "' for loading to memory." << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() == TextureState::LOADED ||
                     it->get_state() == TextureState::UNLOADED ||
                     it->get_state() == TextureState::UNKNOWN) {
            it = g_LoadSchedules.textures.erase(it);
          } else {
            it++;
          }
        }

        return true;
      }

      static bool fonts_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.fonts.begin();
             it != g_LoadSchedules.fonts.end();) {
          if (!it->is_sidecar_loaded()) {
            // TODO: Introduce state enum to fonts and use it here
            if (!font_schedule_memory_load(*it)) {
              LOW_LOG_ERROR
                  << "Failed to schedule font '" << it->get_name()
                  << "' for loading to memory." << LOW_LOG_END;
            }

            it = g_LoadSchedules.fonts.erase(it);

            continue;
          } else {
            it = g_LoadSchedules.fonts.erase(it);
          }
        }

        return true;
      }

      static bool editor_images_tick(float p_Delta)
      {
        for (auto it = g_LoadSchedules.editorImages.begin();
             it != g_LoadSchedules.editorImages.end();) {
          TextureState i_State = it->get_state();
          if (it->get_state() == TextureState::MEMORYLOADED) {
            if (editor_image_schedule_gpu_upload(*it)) {
              it = g_LoadSchedules.editorImages.erase(it);
            } else {
              LOW_LOG_ERROR << "Failed to schedule edtior image '"
                            << it->get_name() << "' upload to gpu."
                            << LOW_LOG_END;
              it++;
            }
            continue;
          } else if (it->get_state() ==
                     TextureState::SCHEDULEDTOLOAD) {
            if (!edtior_image_schedule_memory_load(*it)) {
              LOW_LOG_ERROR << "Failed to schedule editor_image '"
                            << it->get_name()
                            << "' for loading to memory."
                            << LOW_LOG_END;
            }
            ++it;
            continue;
          } else if (it->get_state() == TextureState::LOADED ||
                     it->get_state() == TextureState::UNLOADED ||
                     it->get_state() == TextureState::UNKNOWN) {
            it = g_LoadSchedules.editorImages.erase(it);
          } else {
            it++;
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

          // TODO: Check if mesh is actually unloadable

          unload_mesh(g_MeshEntries.top().mesh);
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
        SubmeshGeometry l_SubmeshGeometry =
            p_UploadEntry.data.mesh.submeshGeometry;
        GpuSubmesh l_GpuSubmesh = p_UploadEntry.data.mesh.gpuSubmesh;

        {
          if (l_SubmeshGeometry.get_state() ==
              MeshState::MEMORYLOADED) {
            l_SubmeshGeometry.set_state(MeshState::UPLOADINGTOGPU);

            u32 l_VertexStart;
            u32 l_IndexStart;
            if (!request_mesh_buffer_space(
                    p_UploadEntry,
                    l_SubmeshGeometry.get_vertex_count(),
                    l_SubmeshGeometry.get_index_count(),
                    &l_VertexStart, &l_IndexStart)) {
              LOW_LOG_WARN << "Could not load mesh since mesh buffer "
                              "space could not be reserved"
                           << LOW_LOG_END;
              return true;
            }

            l_GpuSubmesh.set_state(MeshState::UPLOADINGTOGPU);

            l_GpuSubmesh.set_vertex_start(l_VertexStart);
            l_GpuSubmesh.set_index_start(l_IndexStart);
          }
        }

        Mesh l_Mesh = p_UploadEntry.data.mesh.mesh;

        const u64 l_VertexDataSize =
            l_SubmeshGeometry.get_vertex_count() *
            sizeof(Low::Util::Resource::Vertex);
        const u64 l_IndexDataSize =
            l_SubmeshGeometry.get_index_count() * MESH_INDEX_SIZE;

        if (l_GpuSubmesh.get_uploaded_vertex_count() <
            l_GpuSubmesh.get_vertex_count()) {
          const u64 l_FullSize = l_VertexDataSize;
          const u64 l_UploadedSize = p_UploadEntry.uploadedSize;

          u8 *l_Data = (u8 *)l_SubmeshGeometry.get_vertices().data();

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
          l_CopyRegion.dstOffset = (l_GpuSubmesh.get_vertex_start() *
                                    sizeof(Util::Resource::Vertex)) +
                                   l_UploadedSize;
          l_CopyRegion.size = l_FrameUploadSpace;
          // TODO: Change to transfer queue command buffer
          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_current_resource_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_mesh_vertex_buffer()
                  .m_Buffer.buffer,
              1, &l_CopyRegion);

          Vulkan::BufferUtil::cmd_buffer_barrier(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_mesh_vertex_buffer(), l_CopyRegion,
              VK_PIPELINE_STAGE_2_TRANSFER_BIT,
              VK_ACCESS_2_TRANSFER_WRITE_BIT,
              VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
              VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);

          p_UploadEntry.uploadedSize += l_FrameUploadSpace;
          p_UploadEntry.data.mesh.gpuSubmesh
              .set_uploaded_vertex_count(
                  p_UploadEntry.uploadedSize /
                  sizeof(Util::Resource::Vertex));
          p_UploadEntry.progressPriority = l_UploadProgressPercent;

          return l_FrameUploadSpace > 0.0f;
        } else if (l_GpuSubmesh.get_uploaded_index_count() <
                   l_GpuSubmesh.get_index_count()) {
          const u64 l_FullSize = l_IndexDataSize;
          const u64 l_UploadedSize =
              l_GpuSubmesh.get_uploaded_index_count() *
              MESH_INDEX_SIZE;

          u8 *l_Data = (u8 *)l_SubmeshGeometry.get_indices().data();

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
              (l_GpuSubmesh.get_index_start() * MESH_INDEX_SIZE) +
              l_UploadedSize;
          l_CopyRegion.size = l_FrameUploadSpace;
          // TODO: Change to transfer queue command buffer
          vkCmdCopyBuffer(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_current_resource_staging_buffer()
                  .buffer.buffer,
              Vulkan::Global::get_mesh_index_buffer().m_Buffer.buffer,
              1, &l_CopyRegion);

          Vulkan::BufferUtil::cmd_buffer_barrier(
              Vulkan::Global::get_current_command_buffer(),
              Vulkan::Global::get_mesh_index_buffer(), l_CopyRegion,
              VK_PIPELINE_STAGE_2_TRANSFER_BIT,
              VK_ACCESS_2_TRANSFER_WRITE_BIT,
              VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
              VK_ACCESS_2_INDEX_READ_BIT);

          p_UploadEntry.uploadedSize += l_FrameUploadSpace;
          p_UploadEntry.data.mesh.gpuSubmesh.set_uploaded_index_count(
              p_UploadEntry.uploadedSize / MESH_INDEX_SIZE);
          p_UploadEntry.progressPriority = l_UploadProgressPercent;

          return l_FrameUploadSpace > 0.0f;
        }

        LOW_ASSERT(false, "Unknown situation, mesh is most likely "
                          "already fully uploaded");
        return false;
      }

      static bool texture_upload(UploadEntry &p_UploadEntry)
      {
        Texture l_Texture = p_UploadEntry.data.texture.texture;

        // Get the correct texturepixels for the miplevel
        TexturePixels l_Mip =
            l_Texture.get_staging().get_pixels_for_miplevel(
                p_UploadEntry.lodPriority);

        const u64 l_FullSize = l_Mip.get_data_size();
        const u64 l_UploadedSize = p_UploadEntry.uploadedSize;

        u8 *l_ImageData = (u8 *)l_Mip.get_pixel_data().data();

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
          Vulkan::Image l_Image =
              l_Texture.get_gpu().get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          const u32 l_Channels = l_Mip.get_channels();

          int32_t l_ImagePixelOffset =
              static_cast<int32_t>(l_UploadedSize) /
              l_Mip.get_channels();
          u64 l_PixelUpload = static_cast<u64>(l_FrameUploadSpace) /
                              l_Mip.get_channels();

          const u32 l_ImageWidth = l_Mip.get_dimensions().x;

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

            u64 i_PixelsCopiedThisRegion =
                l_ImageWidth - i_PixelsAlreadyPresentInRow;

            if (i_PixelsCopiedThisRegion > l_PixelUpload) {
              i_PixelsCopiedThisRegion = l_PixelUpload;
            }
            i_Region.imageExtent = {
                static_cast<u32>(i_PixelsCopiedThisRegion), 1,
                1}; // Exact only small portions uploaded

            l_ImagePixelOffset += i_PixelsCopiedThisRegion;
            l_StagingOffset +=
                i_PixelsCopiedThisRegion * l_Mip.get_channels();

            if (i_PixelsCopiedThisRegion > l_PixelUpload) {
              l_PixelUpload = 0;
            } else {
              l_PixelUpload -= i_PixelsCopiedThisRegion;
            }
          }

          if (!l_Regions.empty()) {
            // TODO: Change to transfer queue command buffer

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(), l_Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            vkCmdCopyBufferToImage(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                l_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                l_Regions.size(), l_Regions.data());

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(), l_Image,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }
        }

        p_UploadEntry.uploadedSize += l_FrameUploadSpace;

        p_UploadEntry.progressPriority = l_UploadProgressPercent;

        return l_FrameUploadSpace > 0;
      }

      static bool editor_image_upload(UploadEntry &p_UploadEntry)
      {
        EditorImage l_EditorImage = p_UploadEntry.data.editorImage.ei;

        EditorImageStaging l_Staging = l_EditorImage.get_staging();

        const u64 l_FullSize = l_Staging.get_data_size();
        const u64 l_UploadedSize = p_UploadEntry.uploadedSize;

        u8 *l_ImageData = (u8 *)l_Staging.get_pixel_data().data();

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
          Vulkan::Image l_Image =
              l_EditorImage.get_gpu().get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          const u32 l_Channels = l_Staging.get_channels();

          int32_t l_ImagePixelOffset =
              static_cast<int32_t>(l_UploadedSize) / l_Channels;
          u64 l_PixelUpload =
              static_cast<u64>(l_FrameUploadSpace) / l_Channels;

          const u32 l_ImageWidth = l_Staging.get_dimensions().x;

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

            u64 i_PixelsCopiedThisRegion =
                l_ImageWidth - i_PixelsAlreadyPresentInRow;

            if (i_PixelsCopiedThisRegion > l_PixelUpload) {
              i_PixelsCopiedThisRegion = l_PixelUpload;
            }
            i_Region.imageExtent = {
                static_cast<u32>(i_PixelsCopiedThisRegion), 1,
                1}; // Exact only small portions uploaded

            l_ImagePixelOffset += i_PixelsCopiedThisRegion;
            l_StagingOffset += i_PixelsCopiedThisRegion * l_Channels;

            if (i_PixelsCopiedThisRegion > l_PixelUpload) {
              l_PixelUpload = 0;
            } else {
              l_PixelUpload -= i_PixelsCopiedThisRegion;
            }
          }

          if (!l_Regions.empty()) {
            // TODO: Change to transfer queue command buffer

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(), l_Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            vkCmdCopyBufferToImage(
                Vulkan::Global::get_current_command_buffer(),
                Vulkan::Global::get_current_resource_staging_buffer()
                    .buffer.buffer,
                l_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                l_Regions.size(), l_Regions.data());

            Vulkan::ImageUtil::cmd_transition(
                Vulkan::Global::get_current_command_buffer(), l_Image,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          }
        }

        p_UploadEntry.uploadedSize += l_FrameUploadSpace;

        p_UploadEntry.progressPriority = l_UploadProgressPercent;

        return l_FrameUploadSpace > 0;
      }

      static bool conclude_resource_upload(UploadEntry &p_UploadEntry)
      {
        if (p_UploadEntry.is_texture_upload()) {
          Texture l_Texture = p_UploadEntry.data.texture.texture;
          GpuTexture l_GpuTexture = l_Texture.get_gpu();
          TextureStaging l_Staging = l_Texture.get_staging();

          // We can set it to laoded even though we might load some
          // miplevels later, because it allows us to use the texture
          // already even though not all miplevels may be loaded yet
          l_Texture.set_state(TextureState::LOADED);

          // We can safely destroy the texturepixels of this mip level
          // because it will not be needed any longer
          l_Staging.get_pixels_for_miplevel(p_UploadEntry.lodPriority)
              .destroy();

          Vulkan::Image l_Image = l_GpuTexture.get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          // TODO: Check if this can stay graphics queue command
          // buffer or needs changing. Maybe we also need additional
          // synchronization
          Vulkan::ImageUtil::cmd_transition(
              Vulkan::Global::get_current_command_buffer(), l_Image,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          // Add the loaded mip to the list of mips loaded and sort
          // said list so that it is easier later on to find the
          // lowest and highest mip loaded for this specific image
          l_GpuTexture.loaded_mips().push_back(
              p_UploadEntry.lodPriority);
          std::sort(l_GpuTexture.loaded_mips().begin(),
                    l_GpuTexture.loaded_mips().end());

          if (l_GpuTexture.loaded_mips().size() >=
              l_GpuTexture.get_full_mip_count()) {
            // If this is the case we have successfully loaded the
            // last mip level of this texture
            // So we will delete the staging all together since we
            // don't need it anymore
            // At this point all texturepixels associated with this
            // staging should already be destroyed
            l_Staging.destroy();
          }
        } else if (p_UploadEntry.is_editor_image_upload()) {
          EditorImage l_EditorImage =
              p_UploadEntry.data.editorImage.ei;
          EditorImageGpu l_Gpu = l_EditorImage.get_gpu();
          EditorImageStaging l_Staging = l_EditorImage.get_staging();

          l_EditorImage.set_state(TextureState::LOADED);

          l_Staging.destroy();

          Vulkan::Image l_Image = l_Gpu.get_data_handle();

          VkImage l_VkImage = l_Image.get_allocated_image().image;

          // TODO: Check if this can stay graphics queue command
          // buffer or needs changing. Maybe we also need additional
          // synchronization
          Vulkan::ImageUtil::cmd_transition(
              Vulkan::Global::get_current_command_buffer(), l_Image,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        } else if (p_UploadEntry.is_mesh_upload()) {
          const char *l_Name =
              p_UploadEntry.data.mesh.mesh.get_name().c_str();
          GpuSubmesh l_GpuSubmesh =
              p_UploadEntry.data.mesh.gpuSubmesh;
          SubmeshGeometry l_SubmeshGeometry =
              p_UploadEntry.data.mesh.submeshGeometry;
          Mesh l_Mesh = p_UploadEntry.data.mesh.mesh;
          // At this point one submesh has been fully
          // uploaded to we set it to LOADED

          l_SubmeshGeometry.set_state(MeshState::LOADED);
          l_GpuSubmesh.set_state(MeshState::LOADED);

          l_Mesh.get_gpu().set_uploaded_submesh_count(
              l_Mesh.get_gpu().get_uploaded_submesh_count() + 1);

          // We can now safely destroy the submesh geometry because it
          // is now longer needed
          l_SubmeshGeometry.destroy();

          if (l_Mesh.get_gpu().get_uploaded_submesh_count() >=
              l_Mesh.get_gpu().get_submesh_count()) {
            l_Mesh.set_state(MeshState::LOADED);
            l_Mesh.get_geometry().destroy();
          }
        }

        return true;
      }

      static bool upload_resource(UploadEntry &p_UploadEntry)
      {
        volatile int i = 0;
        if (p_UploadEntry.is_texture_upload() &&
            p_UploadEntry.progressPriority < 100) {
          return texture_upload(p_UploadEntry);
        } else if (p_UploadEntry.is_mesh_upload() &&
                   p_UploadEntry.progressPriority < 100) {
          return mesh_upload(p_UploadEntry);
        } else if (p_UploadEntry.is_editor_image_upload() &&
                   p_UploadEntry.progressPriority < 100) {
          return editor_image_upload(p_UploadEntry);
        }

        LOW_ASSERT(false, "ResourceManager encountered unknown "
                          "resource type to upload");

        return false;
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
        _LOW_ASSERT(fonts_tick(p_Delta));
        _LOW_ASSERT(meshes_tick(p_Delta));
        _LOW_ASSERT(textures_tick(p_Delta));
        _LOW_ASSERT(materials_tick(p_Delta));
        _LOW_ASSERT(editor_images_tick(p_Delta));
        _LOW_ASSERT(uploads_tick(p_Delta));
      }

      bool parse_mesh_resource_config(Util::String p_Path,
                                      Util::Serial::Node &p_Node,
                                      MeshResourceConfig &p_Config)
      {
        LOWR_ASSERT_RETURN(p_Node["version"],
                           "Could not find version");
        const u32 l_Version = p_Node["version"].as<u32>();

        if (l_Version == 1) {
          LOWR_ASSERT_RETURN(p_Node["name"],
                             "Could not find mesh name");
          p_Config.name = p_Node["name"].as<Util::Name>();

          LOWR_ASSERT_RETURN(p_Node["mesh_id"],
                             "Could not find mesh id");
          p_Config.meshId = p_Node["mesh_id"].as<Util::U64Id>().val;

          LOWR_ASSERT_RETURN(p_Node["asset_hash"],
                             "Could not find asset hash");
          p_Config.assetHash =
              p_Node["asset_hash"].as<Util::U64Id>().val;

          LOWR_ASSERT_RETURN(p_Node["source_file"],
                             "Could not find source file");
          p_Config.sourceFile =
              p_Node["source_file"].as<Util::String>();

          LOWR_ASSERT_RETURN(p_Node["submesh_count"],
                             "Could not find submesh count");
          p_Config.submeshCount = p_Node["submesh_count"].as<u32>();

          p_Config.sidecarPath =
              Util::get_project().assetCachePath + "\\" +
              Util::hash_to_string(p_Config.meshId) + ".mesh.yaml";
          p_Config.meshPath =
              Util::get_project().assetCachePath + "\\" +
              Util::hash_to_string(p_Config.meshId) + ".glb";

          p_Config.path = p_Path;

          p_Config.type = MeshType::STATIC;

          if (p_Node["type"]) {
            const Util::Name l_TypeString =
                p_Node["type"].as<Util::Name>();

            p_Config.type =
                MeshTypeEnumHelper::entry_value(l_TypeString);
          }
          return true;
        }
        LOW_ASSERT(false, "Unsupported mesh resource version.");

        return true;
      }

      bool
      parse_texture_resource_config(Util::String p_Path,
                                    Util::Serial::Node &p_Node,
                                    TextureResourceConfig &p_Config)
      {
        LOWR_ASSERT_RETURN(p_Node["version"],
                           "Could not find version");
        const u32 l_Version = p_Node["version"].as<u32>();

        if (l_Version == 1) {
          LOWR_ASSERT_RETURN(p_Node["name"],
                             "Could not find texture name");
          p_Config.name = p_Node["name"].as<Util::Name>();

          LOWR_ASSERT_RETURN(p_Node["texture_id"],
                             "Could not find texture id");
          p_Config.textureId =
              p_Node["texture_id"].as<Util::U64Id>().val;

          LOWR_ASSERT_RETURN(p_Node["asset_hash"],
                             "Could not find asset hash");
          p_Config.assetHash =
              p_Node["asset_hash"].as<Util::U64Id>().val;

          LOWR_ASSERT_RETURN(p_Node["source_file"],
                             "Could not find source file");
          p_Config.sourceFile =
              p_Node["source_file"].as<Util::String>();

          p_Config.sidecarPath =
              Util::get_project().assetCachePath + "/" +
              Util::hash_to_string(p_Config.textureId) +
              ".texture.yaml";
          p_Config.texturePath =
              Util::get_project().assetCachePath + "/" +
              Util::hash_to_string(p_Config.textureId) + ".ktx";

          p_Config.path = Util::PathHelper::normalize(p_Path);
          return true;
        }
        LOW_ASSERT(false, "Unsupported texture resource version.");
        return true;
      }

      bool
      parse_skeleton_resource_config(Util::String p_Path,
                                     Util::Serial::Node &p_Node,
                                     SkeletonResourceConfig &p_Config)
      {
        LOWR_ASSERT_RETURN(p_Node["version"],
                           "Could not find version");
        const u32 l_Version = p_Node["version"].as<u32>();

        if (l_Version == 1) {
          LOWR_ASSERT_RETURN(p_Node["skeleton_id"],
                             "Could not find skeleton id");
          p_Config.skeleton_id =
              p_Node["skeleton_id"].as<Util::U64Id>().val;

          p_Config.data_path =
              Util::get_project().assetCachePath + "/" +
              Util::hash_to_string(p_Config.skeleton_id) +
              ".skeleton.yaml";

          p_Config.path = Util::PathHelper::normalize(p_Path);
          p_Config.bone_count = p_Node["bone_count"].as<i32>();

          p_Config.name = LOW_NAME(
              Util::PathHelper::get_base_name_no_ext(p_Path).c_str());

          return true;
        }
        LOW_ASSERT(false, "Unsupported material resource version.");
        return true;
      }

      bool
      parse_material_resource_config(Util::String p_Path,
                                     Util::Serial::Node &p_Node,
                                     MaterialResourceConfig &p_Config)
      {
        LOWR_ASSERT_RETURN(p_Node["version"],
                           "Could not find version");
        const u32 l_Version = p_Node["version"].as<u32>();

        if (l_Version == 1) {
          LOWR_ASSERT_RETURN(p_Node["name"],
                             "Could not find material name");
          p_Config.name = p_Node["name"].as<Util::Name>();

          LOWR_ASSERT_RETURN(p_Node["material_id"],
                             "Could not find material id");
          p_Config.material_id =
              p_Node["material_id"].as<Util::U64Id>().val;

          p_Config.data_path =
              Util::get_project().assetCachePath + "/" +
              Util::hash_to_string(p_Config.material_id) +
              ".material.yaml";

          p_Config.path = Util::PathHelper::normalize(p_Path);
          return true;
        }
        LOW_ASSERT(false, "Unsupported material resource version.");
        return true;
      }

      void register_asset_id(const u64 p_AssetId,
                             const u64 p_AssetHandleId)
      {
        if (p_AssetId == 0) {
          LOW_LOG_WARN << "Tried to register asset with asset id 0. "
                          "Asset will therefore not be registered "
                          "with the renderer's ResourceManager."
                       << LOW_LOG_END;
          return;
        }

        auto l_Pos = g_AssetRegistry.find(p_AssetId);
        LOW_ASSERT(l_Pos == g_AssetRegistry.end() ||
                       l_Pos->second == p_AssetHandleId,
                   "Encountered asset ID collision.");
        g_AssetRegistry[p_AssetId] = p_AssetHandleId;
      }

      u64 find_asset_by_id(const u64 p_AssetId)
      {
        auto pos = g_AssetRegistry.find(p_AssetId);

        if (pos == g_AssetRegistry.end()) {
          return Util::Handle::DEAD;
        }

        return pos->second;
      }
    } // namespace ResourceManager
  } // namespace Renderer
} // namespace Low
