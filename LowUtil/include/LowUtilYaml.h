#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <yaml-cpp/yaml.h>

#define LOW_YAML_AS_STRING(x)                                        \
  Low::Util::String(x.as<std::string>().c_str())
#define LOW_YAML_AS_NAME(x)                                          \
  Low::Util::Name(x.as<std::string>().c_str())

namespace Low {
  namespace Util {
    namespace Yaml {
      typedef YAML::Node Node;
      typedef YAML::Emitter Emitter;

      LOW_EXPORT Node load_file(const char *p_Path);
      LOW_EXPORT void write_file(const char *p_Path, Node &p_Node);
      LOW_EXPORT Node parse(const char *p_Yaml);
    } // namespace Yaml
  }   // namespace Util
} // namespace Low
