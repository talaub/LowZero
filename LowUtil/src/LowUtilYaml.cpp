#include "LowUtilYaml.h"

#include <fstream>

namespace Low {
  namespace Util {
    namespace Yaml {
      Node load_file(const char *p_Path)
      {
        return YAML::LoadFile(p_Path);
      }

      void write_file(const char *p_Path, Node &p_Node)
      {
        std::ofstream l_Out(p_Path);
        l_Out << p_Node;
      }
    } // namespace Yaml
  }   // namespace Util
} // namespace Low
