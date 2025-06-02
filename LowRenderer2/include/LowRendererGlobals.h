#pragma once

#define IMAGE_MIPMAP_COUNT 4
#define IMAGE_CHANNEL_COUNT 4

#define MATERIAL_DATA_SIZE (sizeof(Low::Math::Vector4) * 4)

#include "LowMath.h"
#include "LowRendererRenderObject.h"

namespace Low {
  namespace Renderer {
    struct DrawCommandUpload
    {
      alignas(16) Low::Math::Matrix4x4 world_transform;
    };

    struct RenderEntry
    {
      union
      {
        struct
        {
          u32 materialTypeIndex;
          u32 meshInfoIndex;
        };
        u64 sortIndex;
      };
      RenderObject renderObject;
      u32 slot;
    };

  } // namespace Renderer
} // namespace Low
