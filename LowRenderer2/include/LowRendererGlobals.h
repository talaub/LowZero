#pragma once

#define IMAGE_MIPMAP_COUNT 4
#define IMAGE_CHANNEL_COUNT 4

#define MATERIAL_DATA_SIZE (sizeof(Low::Math::Vector4) * 4)

#define POINTLIGHT_COUNT 128

#define UI_DRAWCOMMAND_COUNT 1024

#define DEBUG_GEOMETRY_COUNT 1024

#include "LowMath.h"
#include "LowRendererRenderObject.h"

namespace Low {
  namespace Renderer {
    struct DrawCommandUpload
    {
      alignas(16) Low::Math::Matrix4x4 worldTransform;
      alignas(16) u32 materialIndex;
    };

    struct DebugGeometryUpload
    {
      DebugGeometryUpload(Math::Matrix4x4 p_WorldTransform,
                          Math::Color p_Color)
          : world_transform(p_WorldTransform), color(p_Color)
      {
      }

      alignas(16) Low::Math::Matrix4x4 world_transform;
      alignas(16) Math::Color color;
    };

    struct UiDrawCommandUpload
    {
      Math::Vector2 position;
      Math::Vector2 size;
      Math::Vector4 uvRect;
      float rotation2D;
      u32 textureIndex;
      u32 materialIndex;
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
