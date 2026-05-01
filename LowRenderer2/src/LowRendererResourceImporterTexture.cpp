#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include "LowRendererResourceImporter.h"
#include "LowRendererTextureResource.h"
#include "LowRendererResourceManager.h"

#include "LowMath.h"
#include "LowRendererTextureState.h"
#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHashing.h"
#include "LowUtilLogger.h"
#include "LowUtilYaml.h"
#include "LowUtilString.h"
#include "LowUtilSerialization.h"
#include "LowUtilFileIO.h"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"
#include "../../LowDependencies/stb/stb_image_write.h"
#include "../../LowDependencies/stb/stb_image_resize.h"

namespace Low {
  namespace Renderer {
    namespace ResourceImporter {
      static TextureResource
      get_reimport(const u64 p_AssetHash,
                   const Util::String p_SourcePath)
      {
        for (u32 i = 0; i < TextureResource::living_count(); ++i) {
          TextureResource i_TextureResource =
              TextureResource::living_instances()[i];
          if (i_TextureResource.get_source_file() != p_SourcePath) {
            continue;
          }
          return i_TextureResource;
        }

        return Util::Handle::DEAD;
      }

      bool import_texture(Util::String p_ImportPath,
                          Util::String p_OutputPath)
      {

        if (!Util::FileIO::file_exists_sync(p_ImportPath.c_str())) {
          LOW_LOG_ERROR << "File does not exist" << LOW_LOG_END;
          return false;
        }

        const int l_RequestedChannles = 4;
        int l_Width, l_Height, l_Channels;

        const u8 *l_Data =
            stbi_load(p_ImportPath.c_str(), &l_Width, &l_Height,
                      &l_Channels, l_RequestedChannles);

        const u64 l_AssetHash =
            Util::fnv1a_64(l_Data, sizeof(u8) * l_Width * l_Height *
                                       l_RequestedChannles);

        const TextureResource l_OriginalTextureResource =
            get_reimport(l_AssetHash, p_ImportPath);

        const bool l_Reimport = l_OriginalTextureResource.is_alive();

        if (l_Reimport) {
          if (l_OriginalTextureResource.get_asset_hash() ==
              l_AssetHash) {
            LOW_LOG_DEBUG << "Exiting reimport early because texture "
                             "didn't change."
                          << LOW_LOG_END;
            return "";
          }
        }

        const u64 l_TextureId =
            l_Reimport ? l_OriginalTextureResource.get_texture_id()
                       : l_AssetHash;

        // Generate thumbnail
        {
          Util::String l_ThumbName =
              "texture_" + Util::hash_to_string(l_TextureId) + ".png";
          Util::String l_ThumbnailPath =
              Util::project_editor_images_path()
                  .join("thumbnails")
                  .join(l_ThumbName)
                  .get();
          Util::List<u8> l_ThumbnailPixels;
          l_ThumbnailPixels.resize(500 * 500 * 4);

          stbir_resize_uint8(l_Data, l_Width, l_Height, 0,
                             l_ThumbnailPixels.data(), 500, 500, 0,
                             4);

          stbi_write_png(l_ThumbnailPath.c_str(), 500, 500, 4,
                         l_ThumbnailPixels.data(), 500 * 4);
        }

        Math::UVector2 l_Dimensions(l_Width, l_Height);

        Util::String l_FileName =
            Util::PathHelper::get_base_name_no_ext(p_ImportPath);

        struct Pixel
        {
          u8 r;
          u8 g;
          u8 b;
          u8 a;
        };

        const gli::format f = gli::FORMAT_RGBA8_UNORM_PACK8;

        gli::texture2d l_Texture(f, {l_Width, l_Height});

        for (int y = 0; y < l_Height; ++y) {
          for (int x = 0; x < l_Width; ++x) {
            Pixel i_Data = *(Pixel *)&l_Data[((y * l_Width) + x) *
                                             l_RequestedChannles];

            gli::extent2d l_Extent;
            l_Extent.x = static_cast<u32>(x);
            l_Extent.y = static_cast<u32>(y);

            l_Texture.store<Pixel>(l_Extent, 0, i_Data);
          }
        }

        // TODO: Allow different filter methods
        gli::texture2d l_TextureMipmaps =
            gli::generate_mipmaps(l_Texture, gli::FILTER_LINEAR);

        const Util::String l_BaseAssetPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(l_TextureId);

        const Util::String l_KtxPath = l_BaseAssetPath + ".ktx";
        const Util::String l_SidecarPath =
            l_BaseAssetPath + ".texture.yaml";

        const Util::String l_ResourcePath =
            Util::get_project().dataPath + "\\" + p_OutputPath +
            ".texresource.yaml";

        Util::Serial::Node l_SidecarInfo;
        {
          l_SidecarInfo["version"] = 1;
          l_SidecarInfo["name"] = l_FileName.c_str();
          l_SidecarInfo["dimension"] = l_Dimensions;
          l_SidecarInfo["channels"] = l_RequestedChannles;

#if 0
          Util::Serial::Node l_Sampler;
          {
            l_Sampler["filter"] = "LINEAR";
            l_Sampler["address_mode"] = "REPEAT";
            l_Sampler["border_color"] = "BLACK";
          }

          l_SidecarInfo["sampler"] = l_Sampler;
#endif
        }

        Util::Serial::Node l_ResourceNode;
        {
          l_ResourceNode["version"] = 1;
          l_ResourceNode["texture_id"] =
              Util::hash_to_string(l_TextureId).c_str();
          l_ResourceNode["asset_hash"] =
              Util::hash_to_string(l_AssetHash).c_str();
          l_ResourceNode["source_file"] = p_ImportPath.c_str();
          l_ResourceNode["name"] = l_FileName.c_str();
        }

        Util::Serial::write_yaml_file(l_SidecarPath.c_str(),
                                      l_SidecarInfo);
        Util::Serial::write_yaml_file(l_ResourcePath.c_str(),
                                      l_ResourceNode);

        gli::save_ktx(l_TextureMipmaps, l_KtxPath.c_str());

        if (l_Reimport) {
          Texture l_ReimprotedTexture =
              ResourceManager::find_asset<Texture>(l_TextureId);
          l_ReimprotedTexture.get_resource().set_asset_hash(
              l_AssetHash);
          if (l_ReimprotedTexture.get_state() ==
              TextureState::LOADED) {
            ResourceManager::override_loaded_texture(
                l_ReimprotedTexture);
          }
        }

        return true;
      }
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
