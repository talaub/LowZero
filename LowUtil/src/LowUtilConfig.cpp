#include "LowUtilConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilName.h"
#include "LowUtilYaml.h"

#include <string>

namespace Low {
  namespace Util {
    namespace Config {
      Map<Name, uint32_t> g_Capacities;

      static void load_capacities()
      {
        std::string l_FilePath =
            std::string(LOW_DATA_PATH) + "/_internal/type_capacities.yaml";

        Yaml::Node l_RootNode = Yaml::load_file(l_FilePath.c_str());

        for (auto it = l_RootNode.begin(); it != l_RootNode.end(); ++it) {
          Name i_TypeName = LOW_NAME(it->first.as<std::string>().c_str());
          uint32_t i_Capacity = it->second.as<uint32_t>();

          g_Capacities[i_TypeName] = i_Capacity;
        }
      }

      void initialize()
      {
        load_capacities();

        LOW_LOG_DEBUG("Config loaded");
      }

      uint32_t get_capacity(Name p_TypeName)
      {
        return g_Capacities[p_TypeName];
      }

    } // namespace Config
  }   // namespace Util
} // namespace Low
