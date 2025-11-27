#pragma once

#include "LowUtilApi.h"
#include "LowUtilContainers.h"

#include <yaml-cpp/yaml.h>

namespace Low {
  namespace Util {
    namespace Yaml {
      typedef YAML::Node Node;
      typedef YAML::Emitter Emitter;

      LOW_EXPORT Node load_file(const char *p_Path);
      LOW_EXPORT void write_file(const char *p_Path, Node &p_Node);
      LOW_EXPORT Node parse(const char *p_Yaml);
    } // namespace Yaml
  } // namespace Util
} // namespace Low
