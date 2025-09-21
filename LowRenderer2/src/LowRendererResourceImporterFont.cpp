#include "LowRendererResourceImporter.h"

#include <EASTL/sort.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#define FT_LOAD_DEFAULT
#include <msdfgen.h>
#include <msdfgen-ext.h>

#include "../../LowDependencies/msdfgen/core/BitmapRef.hpp"

#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"
#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/Charset.h"
#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/GlyphGeometry.h"
#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/FontGeometry.h"
#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/TightAtlasPacker.h"
#include "../../LowDependencies/msdf-atlas-gen/msdf-atlas-gen/ImmediateAtlasGenerator.h"

#include <gli/gli.hpp>
#include <gli/make_texture.hpp>
#include <gli/save_ktx.hpp>
#include <gli/generate_mipmaps.hpp>

#include "LowMath.h"
#include "LowUtilAssert.h"
#include "LowUtil.h"
#include "LowUtilYaml.h"
#include "LowUtilHashing.h"
#include "LowUtilSerialization.h"

// Optional (for PNG). Add stb_image_write to your importer tool.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../LowDependencies/stb/stb_image.h"
#include "../../LowDependencies/stb/stb_image_write.h"

#undef FT_LOAD_DEFAULT

#define CHANNEL_COUNT 3
#define DESIRED_CHANNEL_COUNT 4
struct Pixel
{
  u8 r;
  u8 g;
  u8 b;
};
struct PixelRGBA
{
  u8 r;
  u8 g;
  u8 b;
  u8 a;
};

namespace Low {
  namespace Renderer {
    namespace ResourceImporter {
      static bool is_reimport()
      {
        return false;
      }

      static bool populate_sidecar_info(
          FT_Face p_Face, const Math::UVector2 p_AtlasDimensions,
          std::vector<msdf_atlas::GlyphGeometry> p_Glyphs,
          Util::Yaml::Node &p_Node)
      {
        p_Node["version"] = 1;
        p_Node["name"] = p_Face->family_name;
        p_Node["style"] = p_Face->style_name;
        p_Node["msdf"] = true;

        {
          for (auto it = p_Glyphs.begin(); it != p_Glyphs.end();
               ++it) {
            Util::Yaml::Node i_Glyph;
            i_Glyph["codepoint"] = it->getCodepoint();

            double ax0, ay0, ax1, ay1; // atlas-space (in *pixels*)
            it->getQuadAtlasBounds(ax0, ay0, ax1, ay1);

            const double say1 = ay1;
            ay1 = p_AtlasDimensions.y - ay0;
            ay0 = p_AtlasDimensions.y - say1;

            double l, b, r, t;
            it->getQuadPlaneBounds(
                l, b, r,
                t); // em units, relative to baseline & pen position

            Math::Vector2 l_Bearing(l, t);
            Math::Vector2 l_Size(r - l, t - b);

            const double l_Advance = it->getAdvance();

            // Normalize to UVs:
            Math::Vector2 uvMin = {
                float(ax0 / double(p_AtlasDimensions.x)),
                float(ay0 / double(p_AtlasDimensions.y))};
            Math::Vector2 uvMax = {
                float(ax1 / double(p_AtlasDimensions.x)),
                float(ay1 / double(p_AtlasDimensions.y))};

            Util::String i_CodePointString =
                "" + (char)it->getCodepoint();

            i_Glyph["advance"] = l_Advance;

            Util::Serialization::serialize(i_Glyph["bearing"],
                                           l_Bearing);
            Util::Serialization::serialize(i_Glyph["size"], l_Size);

            Util::Serialization::serialize(i_Glyph["uv_min"], uvMin);
            Util::Serialization::serialize(i_Glyph["uv_max"], uvMax);
            i_Glyph["codepoint_char"] = i_CodePointString.c_str();

            p_Node["glyphs"].push_back(i_Glyph);
          }
        }

        return true;
      }

      bool g_I = false;
      Util::List<Pixel> g_Pixels;

      bool import_font(Util::String p_ImportPath,
                       Util::String p_OutputPath)
      {
        using namespace msdf_atlas;

        bool success = false;

        FT_Library ft;
        if (FT_Init_FreeType(&ft) != 0)
          return false;

        FT_Face face;
        if (FT_New_Face(ft, p_ImportPath.c_str(), 0, &face) != 0) {
          FT_Done_FreeType(ft);

          return false;
        }

        if (msdfgen::FontHandle *font =
                msdfgen::adoptFreetypeFont(face)) {
          // Storage for glyph geometry and their coordinates in the
          // atlas
          std::vector<GlyphGeometry> glyphs;
          // FontGeometry is a helper class that loads a set of
          // glyphs from a single font. It can also be used to get
          // additional font metrics, kerning information, etc.
          FontGeometry fontGeometry(&glyphs);
          // Load a set of character glyphs:
          // The second argument can be ignored unless you mix
          // different font sizes in one atlas. In the last
          // argument, you can specify a charset other than ASCII.
          // To load specific glyph indices, use loadGlyphs instead.
          fontGeometry.loadCharset(font, 1.0, Charset::ASCII);
          // Apply MSDF edge coloring. See edge-coloring.h for other
          // coloring strategies.
          const double maxCornerAngle = 3.0;
          for (GlyphGeometry &glyph : glyphs) {
            glyph.edgeColoring(&msdfgen::edgeColoringInkTrap,
                               maxCornerAngle, glyph.getCodepoint());
          }
          // TightAtlasPacker class computes the layout of the
          // atlas.
          TightAtlasPacker packer;
          // Set atlas parameters:
          // setDimensions or setDimensionsConstraint to find the
          // best value
          packer.setDimensionsConstraint(
              DimensionsConstraint::SQUARE);
          // setScale for a fixed size or setMinimumScale to use the
          // largest that fits
          // packer.setMinimumScale(167.0);
          //  setPixelRange or setUnitRange
          packer.setDimensions(2048, 2048);
          packer.setPixelRange(8.0);
          packer.setSpacing(4); // or 4
          // Compute atlas layout - pack glyphs
          packer.pack(glyphs.data(), glyphs.size());
          // Get final atlas dimensions
          int width = 0, height = 0;
          packer.getDimensions(width, height);
          // The ImmediateAtlasGenerator class facilitates the
          // generation of the atlas bitmap.
          ImmediateAtlasGenerator<
              float, CHANNEL_COUNT, msdfGenerator,
              BitmapAtlasStorage<byte, CHANNEL_COUNT>>
              generator(width, height);
          // GeneratorAttributes can be modified to change the
          // generator's default settings.
          GeneratorAttributes attributes;
          attributes.scanlinePass = true;
          generator.setAttributes(attributes);
          generator.setThreadCount(4);
          // Generate atlas bitmap
          generator.generate(glyphs.data(), glyphs.size());

          msdfgen::BitmapConstRef<byte, CHANNEL_COUNT> bitmap =
              (msdfgen::BitmapConstRef<byte, CHANNEL_COUNT>)
                  generator.atlasStorage();

          Util::List<PixelRGBA> l_Pixels;
          l_Pixels.resize(bitmap.width * bitmap.height);

          Pixel *l_BitmapPixels = (Pixel *)bitmap.pixels;

          u32 pixelcount = bitmap.width * bitmap.height;

          gli::format f = gli::FORMAT_RGBA8_UNORM_PACK8;

          gli::texture2d l_Texture(f, {bitmap.width, bitmap.height});

          for (u32 x = 0; x < bitmap.width; ++x) {
            for (u32 y = 0; y < bitmap.height; ++y) {
              const u32 i_Index = y * bitmap.width + x;
              const u32 i_InverseIndex =
                  (bitmap.height - (y + 1)) * bitmap.width + x;

              Pixel i_Pixel = l_BitmapPixels[i_InverseIndex];
              l_Pixels[i_Index] = {i_Pixel.r, i_Pixel.g, i_Pixel.b,
                                   255};

              gli::extent2d i_Extent;
              i_Extent.x = x;
              i_Extent.y = y;

              l_Texture.store<PixelRGBA>(i_Extent, 0,
                                         l_Pixels[i_Index]);
            }
          }

          const u64 l_AssetHash =
              Util::fnv1a_64((void *)l_Pixels.data(),
                             l_Pixels.size() * sizeof(PixelRGBA));

          // TODO: Fix reimport
          const u64 l_FontId = is_reimport() ? 0 : l_AssetHash;

          success = true;

          const Util::String l_BaseAssetPath =
              Util::get_project().assetCachePath + "\\" +
              Util::hash_to_string(l_FontId);

#if 1
          const Util::String l_SdfPath =
              l_BaseAssetPath + ".msdf.ktx";
#else
          const Util::String l_SdfPath =
              Util::get_project().assetCachePath + "\\test.msdf.ktx";
#endif
          const Util::String l_SidecarPath =
              l_BaseAssetPath + ".font.yaml";

          Util::Yaml::Node l_Sidecar;
          LOW_ASSERT_ERROR_RETURN_FALSE(
              populate_sidecar_info(face,
                                    {bitmap.width, bitmap.height},
                                    glyphs, l_Sidecar),
              "Failed to populate sidecar info");
#if 0
          stbi_write_png(
              "E:\\testfont_atlas_2.png", bitmap.width, bitmap.height,
              DESIRED_CHANNEL_COUNT, l_Pixels.data(),
              bitmap.width * DESIRED_CHANNEL_COUNT *
                  (sizeof(PixelRGBA) / DESIRED_CHANNEL_COUNT));
#endif

          gli::texture2d l_TextureMipmaps =
              gli::generate_mipmaps(l_Texture, gli::FILTER_NEAREST);

          gli::save_ktx(l_TextureMipmaps, l_SdfPath.c_str());

          const Util::String l_FileName = p_OutputPath.substr(
              p_OutputPath.find_last_of("/\\") + 1);

          Util::Yaml::Node l_ResourceNode;
          {
            l_ResourceNode["version"] = 1;
            l_ResourceNode["font_id"] =
                Util::hash_to_string(l_FontId).c_str();
            l_ResourceNode["asset_hash"] =
                Util::hash_to_string(l_AssetHash).c_str();
            l_ResourceNode["source_file"] = p_ImportPath.c_str();
            l_ResourceNode["name"] = l_FileName.c_str();
          }

          const Util::String l_ResourcePath =
              Util::get_project().dataPath + "\\" + p_OutputPath +
              ".fontresource.yaml";

          Util::Yaml::write_file(l_SidecarPath.c_str(), l_Sidecar);
          Util::Yaml::write_file(l_ResourcePath.c_str(),
                                 l_ResourceNode);

          // Cleanup
          msdfgen::destroyFont(font);
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return success;
      }
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
