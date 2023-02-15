#include "LowAssetManagerImage.h"

#include "LowUtilProfiler.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_dds.hpp>
#include <gli/generate_mipmaps.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../../LowDependencies/stb/stb_image.h"

#include <iostream>

namespace Low {
  namespace AssetManager {
    namespace Image {
      void load_png(Util::String p_FilePath, Image2D &p_Image)
      {
        LOW_PROFILE_START(Load png);

        int l_Width, l_Height, l_Channels;

        const uint8_t *l_Data =
            stbi_load(p_FilePath.c_str(), &l_Width, &l_Height, &l_Channels, 4);

        p_Image.dimensions.x = l_Width;
        p_Image.dimensions.y = l_Height;
        p_Image.channels = l_Channels;

        p_Image.data.resize(l_Width * l_Height * l_Channels);
        memcpy(p_Image.data.data(), l_Data, l_Width * l_Height * l_Channels);

        LOW_PROFILE_END();
      }

      void process_to_dds(Util::String p_OutputPath, Image2D &p_Image)
      {
        struct Pixel
        {
          uint8_t r;
          uint8_t g;
          uint8_t b;
          uint8_t a;
        };

        LOW_PROFILE_START(Process DDS);

        gli::format f = gli::FORMAT_RGBA8_UNORM_PACK8;

        gli::texture2d l_Texture(f,
                                 {p_Image.dimensions.x, p_Image.dimensions.y});

        for (uint64_t y = 0ull; y < p_Image.dimensions.y; ++y) {
          for (uint64_t x = 0ull; x < p_Image.dimensions.x; ++x) {

            Pixel i_Data =
                *(Pixel *)&p_Image.data.data()[((y * p_Image.dimensions.x) +
                                                (p_Image.dimensions.x - x)) *
                                               p_Image.channels];

            gli::extent2d l_Extent;
            l_Extent.x = static_cast<uint32_t>(x);
            l_Extent.y = static_cast<uint32_t>(y);

            l_Texture.store<Pixel>(l_Extent, 0, i_Data);
          }
        }

        gli::texture2d l_TextureMipmaps =
            gli::generate_mipmaps(l_Texture, gli::FILTER_LINEAR);

        gli::save_dds(l_TextureMipmaps, p_OutputPath.c_str());

        LOW_PROFILE_END();
      }
    } // namespace Image
  }   // namespace AssetManager
} // namespace Low
