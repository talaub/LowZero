#pragma once

#include "LowUtilApi.h"

#include <yaml-cpp/yaml.h>

namespace Low {
  namespace Util {
    namespace Yaml {
      typedef YAML::Node Node;

      LOW_EXPORT Node load_file(const char *p_Path);
    } // namespace Yaml
  }   // namespace Util
} // namespace Low
