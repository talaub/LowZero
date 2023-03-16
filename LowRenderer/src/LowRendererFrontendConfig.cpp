#include "LowRendererFrontendConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"

#include "LowRendererBackend.h"

namespace Low {
  namespace Renderer {
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
  } // namespace Renderer
} // namespace Low
