#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <yaml-cpp/yaml.h>

#define LOW_YAML_AS_STRING(x) Low::Util::String(x.as<std::string>().c_str())
#define LOW_YAML_AS_NAME(x) Low::Util::Name(x.as<std::string>().c_str())

namespace Low {
  namespace Util {
    namespace Yaml {
      typedef YAML::Node Node;

      LOW_EXPORT Node load_file(const char *p_Path);
    } // namespace Yaml
  }   // namespace Util
} // namespace Low
