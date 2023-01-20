#include "LowUtilYaml.h"

namespace Low {
  namespace Util {
    namespace Yaml {
      Node load_file(const char *p_Path)
      {
        return YAML::LoadFile(p_Path);
      }
    } // namespace Yaml
  }   // namespace Util
} // namespace Low
