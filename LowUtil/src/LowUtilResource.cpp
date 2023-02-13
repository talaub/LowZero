#include "LowUtilResource.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/convert.hpp>

namespace Low {
  namespace Util {
    namespace Resource {

      void load_dds(String p_FilePath, Image2D &p_Image)
      {
        gli::texture2d l_Texture(gli::load(p_FilePath.c_str()));
        LOW_ASSERT(!l_Texture.empty(), "Could not load file");

        LOW_ASSERT(l_Texture.target() == gli::TARGET_2D,
                   "Expected Image2D data file");

        gli::format l_Format = l_Texture.format();

        gli::texture2d l_Converted =
            gli::convert(l_Texture, gli::FORMAT_RGBA8_UINT_PACK8);

        p_Image.data.resize(l_Texture.size());
        memcpy(p_Image.data.data(), l_Texture.data(), p_Image.data.size());

        p_Image.dimensions.x = l_Texture.extent().x;
        p_Image.dimensions.y = l_Texture.extent().y;
      }

    } // namespace Resource
  }   // namespace Util
} // namespace Low
