#include "LowUtilResource.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/convert.hpp>
#include <string>
#include <vcruntime_string.h>

namespace Low {
  namespace Util {
    namespace Resource {

      void load_image2d(String p_FilePath, Image2D &p_Image)
      {
        const uint32_t l_Channels = 4u;

        gli::texture2d l_Texture(gli::load(p_FilePath.c_str()));
        LOW_ASSERT(!l_Texture.empty(), "Could not load file");

        LOW_ASSERT(l_Texture.target() == gli::TARGET_2D,
                   "Expected Image2D data file");

        p_Image.dimensions.resize(l_Texture.levels());
        p_Image.data.resize(l_Texture.levels());

        LOW_LOG_DEBUG(std::to_string(p_Image.dimensions.size()).c_str());

        for (uint32_t i = 0u; i < p_Image.dimensions.size(); ++i) {
          p_Image.dimensions[i].x = l_Texture.extent(i).x;
          p_Image.dimensions[i].y = l_Texture.extent(i).y;

          p_Image.data[i].resize(p_Image.dimensions[i].x *
                                 p_Image.dimensions[i].y * l_Channels);
          memcpy(p_Image.data[i].data(), l_Texture.data(0, 0, i),
                 p_Image.data[i].size());
        }
      }

    } // namespace Resource
  }   // namespace Util
} // namespace Low
