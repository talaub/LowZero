#include "LowRendererFrontendConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilString.h"

#include "LowRendererBackend.h"

namespace Low {
  namespace Renderer {
    Util::Map<Util::Name, GraphicsPipelineConfig> g_GraphicsPipelineConfigs;

    static void parse_resource_image_config(Util::Yaml::Node &p_Node,
                                            ImageResourceConfig &p_Config)
    {
      LOW_ASSERT((bool)p_Node["dimensions"], "Missing dimension information");
      LOW_ASSERT((bool)p_Node["format"], "Missing format information");

      Util::Yaml::Node &l_DimensionConfig = p_Node["dimensions"];

      Util::String l_DimensionType =
          Util::String(l_DimensionConfig["type"].as<std::string>().c_str());

      if (l_DimensionType == "relative") {
        p_Config.dimensionType = ImageResourceDimensionType::RELATIVE;
        p_Config.dimensions.relative.multiplier =
            l_DimensionConfig["multiplier"].as<float>();
        Util::String l_RelativeString =
            Util::String(l_DimensionConfig["target"].as<std::string>().c_str());
        if (l_RelativeString == "renderflow") {
          p_Config.dimensions.relative.target =
              ImageResourceDimensionRelativeOptions::RENDERFLOW;
        } else if (l_RelativeString == "context") {
          p_Config.dimensions.relative.target =
              ImageResourceDimensionRelativeOptions::CONTEXT;
        } else {
          LOW_ASSERT(false, "Unknown dimension relative target option");
        }
      } else {
        LOW_ASSERT(false, "Unknown dimension type");
      }

      Util::String l_FormatString = LOW_YAML_AS_STRING(p_Node["format"]);
      if (l_FormatString == "RGBA8_UNORM") {
        p_Config.format = Backend::ImageFormat::RGBA8_UNORM;
      } else {
        LOW_ASSERT(false, "Unknown format");
      }
    }

    void parse_resource_configs(Util::Yaml::Node &p_Node,
                                Util::List<ResourceConfig> &p_Resources)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        ResourceConfig i_Resource;
        i_Resource.name = LOW_NAME(it->first.as<std::string>().c_str());
        i_Resource.arraySize = it->second["array_size"].as<int>();

        Util::String i_TypeString =
            it->second["type"].as<std::string>().c_str();

        if (i_TypeString == "image") {
          i_Resource.type = ResourceType::IMAGE;

          parse_resource_image_config(it->second, i_Resource.image);
        } else {
          LOW_ASSERT(false, "Unknown resource type");
        }

        p_Resources.push_back(i_Resource);
      }
    }

    static void parse_pipeline_resource_bindings(
        Util::Yaml::Node &p_Node,
        Util::List<PipelineResourceBindingConfig> &p_BindingConfigs)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        PipelineResourceBindingConfig i_BindingConfig;
        i_BindingConfig.resourceName =
            LOW_NAME(it->first.as<std::string>().c_str());

        Util::String i_TypeName = LOW_YAML_AS_STRING(it->second);

        i_BindingConfig.resourceScope = ResourceBindScope::LOCAL;

        if (i_TypeName == "image") {
          i_BindingConfig.bindType = ResourceBindType::IMAGE;
        } else {
          LOW_ASSERT(false, "Unknown resource binding type");
        }

        p_BindingConfigs.push_back(i_BindingConfig);
      }
    }

    void
    parse_compute_pipeline_configs(Util::Yaml::Node &p_Node,
                                   Util::List<ComputePipelineConfig> &p_Configs)
    {
      for (auto it = p_Node.begin(); it != p_Node.end(); ++it) {
        Util::Yaml::Node i_Node = *it;

        Util::Name i_Name = LOW_NAME(i_Node["name"].as<std::string>().c_str());
        Util::String i_Shader = i_Node["shader"].as<std::string>().c_str();

        ComputePipelineConfig i_Config;
        i_Config.name = i_Name;
        i_Config.shader = i_Shader;

        if (i_Node["resource_bindings"]) {
          parse_pipeline_resource_bindings(i_Node["resource_bindings"],
                                           i_Config.resourceBinding);
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
            LOW_ASSERT(false,
                       "Unknown compute dispatch relative dimension option");
          }
          i_Config.dispatchConfig.relative.multiplier =
              i_Node["dimensions"]["multiplier"].as<float>();
        } else {
          LOW_ASSERT(false, "Unknown compute dispatch dimension type");
        }

        p_Configs.push_back(i_Config);
      }
    }

    static void parse_graphics_pipeline_config(Util::Yaml::Node &p_Node,
                                               GraphicsPipelineConfig &p_Config)
    {
      p_Config.vertexPath = LOW_YAML_AS_STRING(p_Node["vertex"]);
      p_Config.fragmentPath = LOW_YAML_AS_STRING(p_Node["fragment"]);
      p_Config.translucency = p_Node["translucency"].as<bool>();

      {
        Util::String l_EnumString = LOW_YAML_AS_STRING(p_Node["cull_mode"]);
        if (l_EnumString == "back") {
          p_Config.cullMode = Backend::PipelineRasterizerCullMode::BACK;
        } else if (l_EnumString == "front") {
          p_Config.cullMode = Backend::PipelineRasterizerCullMode::FRONT;
        } else {
          LOW_ASSERT(false, "Unknown pipeline cull mode");
        }
      }
      {
        Util::String l_EnumString = LOW_YAML_AS_STRING(p_Node["front_face"]);
        if (l_EnumString == "clockwise") {
          p_Config.frontFace = Backend::PipelineRasterizerFrontFace::CLOCKWISE;
        } else if (l_EnumString == "counter_clockwise") {
          p_Config.frontFace =
              Backend::PipelineRasterizerFrontFace::COUNTER_CLOCKWISE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline front face");
        }
      }
      {
        Util::String l_EnumString = LOW_YAML_AS_STRING(p_Node["polygon_mode"]);
        if (l_EnumString == "fill") {
          p_Config.polygonMode = Backend::PipelineRasterizerPolygonMode::FILL;
        } else if (l_EnumString == "line") {
          p_Config.polygonMode = Backend::PipelineRasterizerPolygonMode::LINE;
        } else {
          LOW_ASSERT(false, "Unknown pipeline polygon mode");
        }
      }
    }

    void load_graphics_pipeline_configs(Util::String p_RootPath)
    {
      Util::String l_PipelineDirectory = p_RootPath + "/graphics_pipelines";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_PipelineDirectory.c_str(), l_FilePaths);
      Util::String l_Ending = ".graphicspipelines.yaml";

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::Yaml::Node i_Node = Util::Yaml::load_file(i_Path.c_str());

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

    GraphicsPipelineConfig &get_graphics_pipeline_config(Util::Name p_Name)
    {
      LOW_ASSERT(g_GraphicsPipelineConfigs.find(p_Name) !=
                     g_GraphicsPipelineConfigs.end(),
                 "Could not find graphics pipeline config");

      return g_GraphicsPipelineConfigs[p_Name];
    }
  } // namespace Renderer
} // namespace Low
