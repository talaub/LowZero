#include "LowRendererFrontendConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

#include "LowRendererBackend.h"
#include "LowRendererRenderFlow.h"

namespace Low {
  namespace Renderer {
    Util::Map<Util::Name, GraphicsPipelineConfig>
        g_GraphicsPipelineConfigs;

    static void
    parse_resource_buffer_config(Util::Yaml::Node &p_Node,
                                 BufferResourceConfig &p_Config)
    {
      p_Config.size = p_Node["buffer_size"].as<int>();
    }

    void parse_dimensions_config(Util::Yaml::Node &p_Node,
                                 DimensionsConfig &p_Config)
    {
      Util::String l_DimensionType =
          Util::String(p_Node["type"].as<std::string>().c_str());

      if (l_DimensionType == "relative") {
        p_Config.type = ImageResourceDimensionType::RELATIVE;
        p_Config.relative.multiplier =
            p_Node["multiplier"].as<float>();
        Util::String l_RelativeString =
            Util::String(p_Node["target"].as<std::string>().c_str());
        if (l_RelativeString == "renderflow") {
          p_Config.relative.target =
              ImageResourceDimensionRelativeOptions::RENDERFLOW;
        } else if (l_RelativeString == "context") {
          p_Config.relative.target =
              ImageResourceDimensionRelativeOptions::CONTEXT;
        } else {
          LOW_ASSERT(false,
                     "Unknown dimension relative target option");
        }
      } else if (l_DimensionType == "absolute") {
        p_Config.type = ImageResourceDimensionType::ABSOLUTE;
        p_Config.absolute.x = p_Node["x"].as<int>();
        p_Config.absolute.y = p_Node["y"].as<int>();
      } else {
        LOW_ASSERT(false, "Unknown dimension type");
      }
    }

    void apply_dimensions_config(Interface::Context p_Context,
                                 RenderFlow p_RenderFlow,
                                 DimensionsConfig &p_Config,
                                 Math::UVector2 &p_Dimensions)
    {
      if (p_Config.type == ImageResourceDimensionType::ABSOLUTE) {
        p_Dimensions.x = p_Config.absolute.x;
        p_Dimensions.y = p_Config.absolute.y;
      } else if (p_Config.type ==
                 ImageResourceDimensionType::RELATIVE) {
        Math::Vector2 l_Dimensions;
        if (p_Config.relative.target ==
            ImageResourceDimensionRelativeOptions::RENDERFLOW) {
          l_Dimensions = p_RenderFlow.get_dimensions();
        } else if (p_Config.relative.target ==
                   ImageResourceDimensionRelativeOptions::CONTEXT) {
          l_Dimensions = p_Context.get_dimensions();
        } else {
          LOW_ASSERT(false,
                     "Unknown dimensions relative target option");
        }
        l_Dimensions *= p_Config.relative.multiplier;
        p_Dimensions = l_Dimensions;

      } else {
        LOW_ASSERT(false, "Unknown dimensions config type");
      }
    }

    static void
    parse_resource_image_config(Util::Yaml::Node &p_Node,
                                ImageResourceConfig &p_Config)
    {
      LOW_ASSERT((bool)p_Node["dimensions"],
                 "Missing dimension information");
      LOW_ASSERT((bool)p_Node["format"],
                 "Missing format information");

      Util::Yaml::Node &l_DimensionConfig = p_Node["dimensions"];
      parse_dimensions_config(l_DimensionConfig, p_Config.dimensions);

      p_Config.depth = false;
      if (p_Node["depth"]) {
        p_Config.depth = p_Node["depth"].as<bool>();
      }

      Util::String l_FormatString =
          LOW_YAML_AS_STRING(p_Node["format"]);
      if (l_FormatString == "RGBA8_UNORM") {
        p_Config.format = Backend::ImageFormat::RGBA8_UNORM;
      } else if (l_FormatString == "R8_UNORM") {
        p_Config.format = Backend::ImageFormat::R8_UNORM;
      } else if (l_FormatString == "R32_UINT") {
        p_Config.format = Backend::ImageFormat::R32_UINT;
      } else if (l_FormatString == "D32_SFLOAT") {
        p_Config.format = Backend::ImageFormat::D32_SFLOAT;
      } else if (l_FormatString == "D32_SFLOAT_S8_UINT") {
        p_Config.format = Backend::ImageFormat::D32_SFLOAT_S8_UINT;
      } else if (l_FormatString == "D24_UNORM_S8_UINT") {
        p_Config.format = Backend::ImageFormat::D24_UNORM_S8_UINT;
      } else if (l_FormatString == "RGBA32_SFLOAT") {
        p_Config.format = Backend::ImageFormat::RGBA32_SFLOAT;
      } else if (l_FormatString == "RGBA16_SFLOAT") {
        p_Config.format = Backend::ImageFormat::RGBA16_SFLOAT;
      } else {
        LOW_ASSERT(false, "Unknown format");
      }

      p_Config.sampleFilter = Backend::ImageSampleFilter::LINEAR;

      if (p_Node["sample_filter"]) {
        Util::String l_FilterString =
            LOW_YAML_AS_STRING(p_Node["sample_filter"]);

        if (l_FilterString == "LINEAR") {
          p_Config.sampleFilter = Backend::ImageSampleFilter::LINEAR;
        } else if (l_FilterString == "CUBIC") {
          p_Config.sampleFilter = Backend::ImageSampleFilter::CUBIC;
        } else {
          LOW_ASSERT(false, "Unknown sample filter");
        }
      }
    }

    void
    parse_resource_configs(Util::Yaml::Node &p_Node,
                           Util::List<ResourceConfig> &p_Resources)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        ResourceConfig i_Resource;
        i_Resource.name =
            LOW_NAME(it->first.as<std::string>().c_str());
        i_Resource.arraySize = it->second["array_size"].as<int>();

        Util::String i_TypeString =
            it->second["type"].as<std::string>().c_str();

        if (i_TypeString == "image") {
          i_Resource.type = ResourceType::IMAGE;

          parse_resource_image_config(it->second, i_Resource.image);
        } else if (i_TypeString == "buffer") {
          i_Resource.type = ResourceType::BUFFER;

          parse_resource_buffer_config(it->second, i_Resource.buffer);
        } else {
          LOW_ASSERT(false, "Unknown resource type");
        }

        p_Resources.push_back(i_Resource);
      }
    }

    void parse_pipeline_resource_binding(
        PipelineResourceBindingConfig &p_BindingConfig,
        Util::String &p_TargetString, Util::String &p_TypeName)
    {
      Util::String l_TargetString = p_TargetString;

      Util::String l_ContextPrefix = "context:";
      Util::String l_RenderFlowPrefix = "renderflow:";

      p_BindingConfig.resourceScope = ResourceBindScope::LOCAL;

      Util::String i_Name = l_TargetString;

      if (Util::StringHelper::begins_with(l_TargetString,
                                          l_ContextPrefix)) {
        i_Name = l_TargetString.substr(l_ContextPrefix.length());
        p_BindingConfig.resourceScope = ResourceBindScope::CONTEXT;
      } else if (Util::StringHelper::begins_with(
                     l_TargetString, l_RenderFlowPrefix)) {
        i_Name = l_TargetString.substr(l_RenderFlowPrefix.length());
        p_BindingConfig.resourceScope = ResourceBindScope::RENDERFLOW;
      }
      p_BindingConfig.resourceName = LOW_NAME(i_Name.c_str());

      if (p_TypeName == "image") {
        p_BindingConfig.bindType = ResourceBindType::IMAGE;
      } else if (p_TypeName == "sampler") {
        p_BindingConfig.bindType = ResourceBindType::SAMPLER;
      } else if (p_TypeName == "buffer") {
        p_BindingConfig.bindType = ResourceBindType::BUFFER;
      } else if (p_TypeName == "texture2d") {
        p_BindingConfig.bindType = ResourceBindType::TEXTURE2D;
      } else if (p_TypeName == "unbound_sampler") {
        p_BindingConfig.bindType = ResourceBindType::UNBOUND_SAMPLER;
      } else {
        LOW_ASSERT(false, "Unknown resource binding type");
      }
    }

    void parse_pipeline_resource_bindings(
        Util::Yaml::Node &p_Node,
        Util::List<PipelineResourceBindingConfig> &p_BindingConfigs)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        PipelineResourceBindingConfig i_BindingConfig;
        Util::String i_TargetString = LOW_YAML_AS_STRING(it->first);
        Util::String i_TypeName = LOW_YAML_AS_STRING(it->second);

        parse_pipeline_resource_binding(i_BindingConfig,
                                        i_TargetString, i_TypeName);

        p_BindingConfigs.push_back(i_BindingConfig);
      }
    }

    void parse_compute_pipeline_configs(
        Util::Yaml::Node &p_Node,
        Util::List<ComputePipelineConfig> &p_Configs)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        Util::Yaml::Node i_Node = *it;

        Util::Name i_Name =
            LOW_NAME(i_Node["name"].as<std::string>().c_str());
        Util::String i_Shader =
            i_Node["shader"].as<std::string>().c_str();

        ComputePipelineConfig i_Config;
        i_Config.name = i_Name;
        i_Config.shader = i_Shader;

        if (i_Node["resource_bindings"]) {
          parse_pipeline_resource_bindings(
              i_Node["resource_bindings"], i_Config.resourceBinding);
        }

        Util::String i_DimensionType =
            LOW_YAML_AS_STRING(i_Node["dimensions"]["type"]);

        if (i_DimensionType == "absolute") {
          i_Config.dispatchConfig.dimensionType =
              ComputeDispatchDimensionType::ABSOLUTE;
          i_Config.dispatchConfig.absolute.x =
              i_Node["dimensions"]["x"].as<int>();
          i_Config.dispatchConfig.absolute.y =
              i_Node["dimensions"]["y"].as<int>();
          i_Config.dispatchConfig.absolute.z =
              i_Node["dimensions"]["z"].as<int>();
        } else if (i_DimensionType == "relative") {
          i_Config.dispatchConfig.dimensionType =
              ComputeDispatchDimensionType::RELATIVE;
          Util::String i_RelativeTarget =
              LOW_YAML_AS_STRING(i_Node["dimensions"]["target"]);

          if (i_RelativeTarget == "context") {
            i_Config.dispatchConfig.relative.target =
                ComputeDispatchRelativeTarget::CONTEXT;
          } else if (i_RelativeTarget == "renderflow") {
            i_Config.dispatchConfig.relative.target =
                ComputeDispatchRelativeTarget::RENDERFLOW;
          } else {
            LOW_ASSERT(
                false,
                "Unknown compute dispatch relative dimension option");
          }
          i_Config.dispatchConfig.relative.multiplier =
              i_Node["dimensions"]["multiplier"].as<float>();
        } else {
          LOW_ASSERT(false,
                     "Unknown compute dispatch dimension type");
        }

        p_Configs.push_back(i_Config);
      }
    }

    static void
    parse_graphics_pipeline_config(Util::Yaml::Node &p_Node,
                                   GraphicsPipelineConfig &p_Config)
    {
      p_Config.vertexPath = LOW_YAML_AS_STRING(p_Node["vertex"]);
      p_Config.fragmentPath = LOW_YAML_AS_STRING(p_Node["fragment"]);
      p_Config.translucency = p_Node["translucency"].as<bool>();

      {
        Util::String l_EnumString =
            LOW_YAML_AS_STRING(p_Node["cull_mode"]);
        if (l_EnumString == "back") {
          p_Config.cullMode =
              Backend::PipelineRasterizerCullMode::BACK;
        } else if (l_EnumString == "front") {
          p_Config.cullMode =
              Backend::PipelineRasterizerCullMode::FRONT;
        } else if (l_EnumString == "none") {
          p_Config.cullMode =
              Backend::PipelineRasterizerCullMode::NONE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline cull mode");
        }
      }
      {
        Util::String l_EnumString =
            LOW_YAML_AS_STRING(p_Node["front_face"]);
        if (l_EnumString == "clockwise") {
          p_Config.frontFace =
              Backend::PipelineRasterizerFrontFace::CLOCKWISE;
        } else if (l_EnumString == "counter_clockwise") {
          p_Config.frontFace =
              Backend::PipelineRasterizerFrontFace::COUNTER_CLOCKWISE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline front face");
        }
      }
      {
        Util::String l_EnumString =
            LOW_YAML_AS_STRING(p_Node["polygon_mode"]);
        if (l_EnumString == "fill") {
          p_Config.polygonMode =
              Backend::PipelineRasterizerPolygonMode::FILL;
        } else if (l_EnumString == "line") {
          p_Config.polygonMode =
              Backend::PipelineRasterizerPolygonMode::LINE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline polygon mode");
        }
      }
    }

    void load_graphics_pipeline_configs(Util::String p_RootPath)
    {
      Util::String l_PipelineDirectory =
          p_RootPath + "/graphics_pipelines";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_PipelineDirectory.c_str(),
                                   l_FilePaths);
      Util::String l_Ending = ".graphicspipelines.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());

          for (auto it = i_Node.begin(); it != i_Node.end(); ++it) {
            Util::Name i_Name = LOW_YAML_AS_NAME(it->first);

            GraphicsPipelineConfig i_Config;
            parse_graphics_pipeline_config(it->second, i_Config);
            i_Config.name = i_Name;
            g_GraphicsPipelineConfigs[i_Name] = i_Config;
          }
        }
      }
    }

    GraphicsPipelineConfig &
    get_graphics_pipeline_config(Util::Name p_Name)
    {
      LOW_ASSERT(g_GraphicsPipelineConfigs.find(p_Name) !=
                     g_GraphicsPipelineConfigs.end(),
                 "Could not find graphics pipeline config");

      return g_GraphicsPipelineConfigs[p_Name];
    }

    void generate_graphics_pipeline_config_fullscreen_triangle(
        GraphicsPipelineConfig &p_Config,
        Util::String p_FragmentShaderName)
    {
      p_Config.name = LOW_NAME(p_FragmentShaderName.c_str());
      p_Config.cullMode = Backend::PipelineRasterizerCullMode::BACK;
      p_Config.polygonMode =
          Backend::PipelineRasterizerPolygonMode::FILL;
      p_Config.frontFace =
          Backend::PipelineRasterizerFrontFace::CLOCKWISE;
      p_Config.translucency = false;
      p_Config.vertexPath = "fs.vert";
      p_Config.fragmentPath = p_FragmentShaderName + ".frag";
    }
  } // namespace Renderer
} // namespace Low
