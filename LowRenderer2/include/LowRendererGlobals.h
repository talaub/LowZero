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
      u32 materialIndex;
      u32 objectId;
    };

    struct DebugGeometryUpload
    {
      DebugGeometryUpload(Math::Matrix4x4 p_WorldTransform,
                          Math::Color p_Color, u32 p_EditorImageIndex,
                          u32 p_PickId)
          : world_transform(p_WorldTransform), color(p_Color),
            editorImageIndex(p_EditorImageIndex), pickId(p_PickId)
      {
      }

      alignas(16) Low::Math::Matrix4x4 world_transform;
      alignas(16) Math::Color color;
      u32 editorImageIndex;
      u32 pickId;
      u32 padding0;
      u32 padding1;
    };

    struct UiDrawCommandUpload
    {
      Math::Vector4 uvRect;
      Math::Color color;
      Math::Vector2 position;
      Math::Vector2 size;
      float rotation2D;
      u32 textureIndex;
      u32 materialIndex;
      u32 padding0;
    };

    struct SS2DDrawCommandUpload
    {
      Math::Color color;
      Math::Vector4 corners;
      Math::Vector2 position;
      Math::Vector2 half_extents;
      u32 type;
      u32 pad0;
      u32 pad1;
      u32 pad2;
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
