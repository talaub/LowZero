#include "LowEditorResourceProcessorImage.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Editor {
    namespace ResourceProcessor {
      namespace Image {
        void load_png(Util::String p_FilePath, Image2D &p_Image)
        {
          int l_Width, l_Height, l_Channels;

          const uint8_t *l_Data =
              stbi_load(p_FilePath.c_str(), &l_Width, &l_Height,
                        &l_Channels, 0);

          p_Image.dimensions.x = l_Width;
          p_Image.dimensions.y = l_Height;
          p_Image.channels = 4;

          p_Image.data.resize(l_Width * l_Height * 4);

          for (uint32_t y = 0u; y < l_Height; ++y) {
            for (uint32_t x = 0u; x < l_Width; ++x) {
              uint32_t c = 0;
              for (; c < l_Channels; ++c) {
                p_Image.data[(((y * l_Width) + x) * 4) + c] =
                    l_Data[(((y * l_Width) + x) * l_Channels) + c];
              }
              for (; c < 4; ++c) {
                p_Image.data[(((y * l_Width) + x) * 4) + c] = 255;
              }
            }
          }
        }

        void process(Util::String p_OutputPath, Image2D &p_Image)
        {
          struct Pixel
          {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
          };

          gli::format f = gli::FORMAT_RGBA8_UNORM_PACK8;

          gli::texture2d l_Texture(
              f, {p_Image.dimensions.x, p_Image.dimensions.y});

          for (uint64_t y = 0ull; y < p_Image.dimensions.y; ++y) {
            for (uint64_t x = 0ull; x < p_Image.dimensions.x; ++x) {

              Pixel i_Data =
                  *(Pixel *)&p_Image.data
                       .data()[((y * p_Image.dimensions.x) + x) * 4];

              gli::extent2d l_Extent;
              l_Extent.x = static_cast<uint32_t>(x);
              l_Extent.y = static_cast<uint32_t>(y);

              l_Texture.store<Pixel>(l_Extent, 0, i_Data);
            }
          }

          gli::texture2d l_TextureMipmaps =
              gli::generate_mipmaps(l_Texture, gli::FILTER_LINEAR);

          gli::save_ktx(l_TextureMipmaps,
                        (p_OutputPath + ".ktx").c_str());
        }
      } // namespace Image
    }   // namespace ResourceProcessor
  }     // namespace Editor
} // namespace Low
