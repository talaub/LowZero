#pragma once

#define IMAGE_MIPMAP_COUNT 4
#define IMAGE_CHANNEL_COUNT 4

#include "LowMath.h"
#include "LowRendererRenderObject.h"

namespace Low {
  namespace Renderer {
    struct RenderObjectUpload
    {
      alignas(16) Low::Math::Matrix4x4 world_transform;
    };

    struct RenderEntry
    {
      union
      {
        struct
        {
          u32 meshInfoIndex;
          // This will be used for material type index later
          u32 empty;
        };
        u64 sortIndex;
      };
      RenderObject renderObject;
      u32 slot;
    };

  } // namespace Renderer
} // namespace Low
