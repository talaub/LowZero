#pragma once

#include "LowUtilName.h"
#include "LowUtilYaml.h"
#include "LowUtilContainers.h"

#include "LowMathVectorUtil.h"

namespace Low {
  namespace Renderer {
    namespace Interface {
      struct Context;
    }
    struct RenderFlow;

    namespace ResourceType {
      enum Enum
      {
        IMAGE,
        BUFFER
      };
    }

    struct BufferResourceConfig
    {
      uint32_t size;
    };

    namespace ImageResourceDimensionType {
      enum Enum
      {
        RELATIVE,
        ABSOLUTE
      };
    }

    namespace ImageResourceDimensionRelativeOptions {
      enum Enum
      {
        RENDERFLOW,
        CONTEXT
      };
    }

    struct ImageResourceDimensionRelativeConfig
    {
      uint8_t target;
      float multiplier;
    };

    struct DimensionsConfig
    {
      uint8_t type;

      union
      {
        Math::UVector2 absolute;
        ImageResourceDimensionRelativeConfig relative;
      };
    };

    void parse_dimensions_config(Util::Yaml::Node &p_Node,
                                 DimensionsConfig &p_Config);
    void apply_dimensions_config(Interface::Context p_Context,
                                 RenderFlow p_RenderFlow,
                                 DimensionsConfig &p_Config,
                                 Math::UVector2 &p_Dimensions);

    struct ImageResourceConfig
    {
      uint8_t format;
      uint8_t sampleFilter;
      bool depth;

      DimensionsConfig dimensions;
    };

    struct ResourceConfig
    {
      Util::Name name;
      uint8_t type;
      uint32_t arraySize;
      union
      {
        BufferResourceConfig buffer;
        ImageResourceConfig image;
      };
    };

    void parse_resource_configs(Util::Yaml::Node &p_Node,
                                Util::List<ResourceConfig> &p_Resources);

    namespace ResourceBindType {
      enum Enum
      {
        IMAGE,
        SAMPLER,
        BUFFER,
        UNBOUND_SAMPLER,
        TEXTURE2D
      };
    }

    namespace ResourceBindScope {
      enum Enum
      {
        LOCAL,
        RENDERFLOW,
        CONTEXT
      };
    }

    struct PipelineResourceBindingConfig
    {
      Util::Name resourceName;
      uint8_t bindType;
      uint8_t resourceScope;
    };

    namespace ComputeDispatchDimensionType {
      enum Enum
      {
        ABSOLUTE,
        RELATIVE
      };
    }

    namespace ComputeDispatchRelativeTarget {
      enum Enum
      {
        RENDERFLOW,
        CONTEXT
      };
    }

    struct ComputeDispatchRelativeDimensions
    {
      uint8_t target;
      float multiplier;
    };

    struct ComputeDispatchConfig
    {
      uint8_t dimensionType;
      union
      {
        ComputeDispatchRelativeDimensions relative;
        Math::UVector3 absolute;
      };
    };

    struct ComputePipelineConfig
    {
      Util::Name name;
      Util::String shader;
      Util::List<PipelineResourceBindingConfig> resourceBinding;
      ComputeDispatchConfig dispatchConfig;
    };

    void parse_pipeline_resource_binding(
        PipelineResourceBindingConfig &p_BindingConfig,
        Util::String &p_TargetString, Util::String &p_TypeName);

    void parse_compute_pipeline_configs(
        Util::Yaml::Node &p_Node, Util::List<ComputePipelineConfig> &p_Configs);

    struct GraphicsPipelineConfig
    {
      Util::Name name;
      Util::String vertexPath;
      Util::String fragmentPath;
      uint8_t cullMode;
      uint8_t frontFace;
      uint8_t polygonMode;
      bool translucency;
    };

    void load_graphics_pipeline_configs(Util::String p_RootPath);

    GraphicsPipelineConfig &get_graphics_pipeline_config(Util::Name p_Name);

  } // namespace Renderer
} // namespace Low
