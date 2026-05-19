#pragma once

#include "LowRendererShaderVariant.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    enum class GraphicsPipelineCullMode
    {
      BACK,
      FRONT,
      NONE
    };
    enum class GraphicsPipelineFrontFace
    {
      CLOCKWISE,
      COUNTER_CLOCKWISE
    };

    enum class ImageFormat
    {
      UNDEFINED,
      RGBA8_UNORM,
      RGBA16_SFLOAT,
      RGB16_SFLOAT,
      R16_SFLOAT,
      R32_UINT,
      DEPTH
    };

    struct GraphicsPipelineConfig
    {
      ShaderVariant vertexShader;
      ShaderVariant fragmentShader;
      bool depthTest;
      GraphicsPipelineCullMode cullMode;
      GraphicsPipelineFrontFace frontFace;
      Util::List<ImageFormat> colorAttachmentFormats;
      ImageFormat depthFormat;
      bool alphaBlending;
      bool wireframe;
      float lineStrength;
    };
  } // namespace Renderer
} // namespace Low
