#include "LowRendererResourceImporter.h"

#include "LowMath.h"
#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHashing.h"
#include "LowUtilYaml.h"
#include "LowUtilString.h"
#include "LowUtilSerialization.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

namespace Low {
  namespace Renderer {
    namespace ResourceImporter {
      static bool is_reimport()
      {
        return false;
      }

      bool import_texture(Util::String p_ImportPath,
                          Util::String p_OutputPath)
      {
        const int l_RequestedChannles = 4;
        int l_Width, l_Height, l_Channels;

        const u8 *l_Data =
            stbi_load(p_ImportPath.c_str(), &l_Width, &l_Height,
                      &l_Channels, l_RequestedChannles);

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

        const bool l_Reimport = is_reimport();

        const u64 l_AssetHash =
            Util::fnv1a_64(l_Data, sizeof(u8) * l_Width * l_Height *
                                       l_RequestedChannles);
        // TODO: Fix reimport
        const u64 l_TextureId = l_Reimport ? 0 : l_AssetHash;

        const Util::String l_BaseAssetPath =
            Util::get_project().assetCachePath + "\\" +
            Util::hash_to_string(l_TextureId);

        const Util::String l_KtxPath = l_BaseAssetPath + ".ktx";
        const Util::String l_SidecarPath =
            l_BaseAssetPath + ".texture.yaml";

        const Util::String l_ResourcePath =
            Util::get_project().dataPath + "\\" + p_OutputPath +
            ".texresource.yaml";

        Util::Yaml::Node l_SidecarInfo;
        {
          l_SidecarInfo["version"] = 1;
          l_SidecarInfo["name"] = l_FileName.c_str();
          Util::Serialization::serialize(l_SidecarInfo["dimensions"],
                                         l_Dimensions);
          l_SidecarInfo["channels"] = l_RequestedChannles;

#if 0
          Util::Yaml::Node l_Sampler;
          {
            l_Sampler["filter"] = "LINEAR";
            l_Sampler["address_mode"] = "REPEAT";
            l_Sampler["border_color"] = "BLACK";
          }

          l_SidecarInfo["sampler"] = l_Sampler;
#endif
        }

        Util::Yaml::Node l_ResourceNode;
        {
          l_ResourceNode["version"] = 1;
          l_ResourceNode["texture_id"] =
              Util::hash_to_string(l_TextureId).c_str();
          l_ResourceNode["asset_hash"] =
              Util::hash_to_string(l_AssetHash).c_str();
          l_ResourceNode["source_file"] = p_ImportPath.c_str();
          l_ResourceNode["name"] = l_FileName.c_str();
        }

        Util::Yaml::write_file(l_SidecarPath.c_str(), l_SidecarInfo);
        Util::Yaml::write_file(l_ResourcePath.c_str(),
                               l_ResourceNode);

        gli::save_ktx(l_TextureMipmaps, l_KtxPath.c_str());

        return true;
      }
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
