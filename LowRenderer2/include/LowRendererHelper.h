#pragma once

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
      RGBA16_SFLOAT,
      RGB16_SFLOAT,
      R16_SFLOAT,
      DEPTH
    };

    struct GraphicsPipelineConfig
    {
      Util::String vertexShaderPath;
      Util::String fragmentShaderPath;
      bool depthTest;
      GraphicsPipelineCullMode cullMode;
      GraphicsPipelineFrontFace frontFace;
      Util::List<ImageFormat> colorAttachmentFormats;
      ImageFormat depthFormat;
    };
  } // namespace Renderer
} // namespace Low
