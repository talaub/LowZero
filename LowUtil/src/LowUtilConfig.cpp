#include "LowUtilConfig.h"

#include "LowUtilAssert.h"
#include "LowUtilFileSystem.h"
#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilName.h"
#include "LowUtilYaml.h"
#include "LowUtilSerialization.h"
#include "LowUtil.h"

#include <string>

namespace Low {
  namespace Util {
    namespace Config {
      Map<Name, Map<Name, uint32_t>> g_Capacities;

      static void load_capacities()
      {
        String l_FilePath = get_project().dataPath +
                            "/_internal/type_capacities.yaml";

        Serial::Node l_RootNode =
            Serial::load_yaml_file(l_FilePath.c_str());

        for (auto [i_Name, i_Value] : l_RootNode) {
          Name i_ModuleName = LOW_NAME(i_Name->c_str());
          g_Capacities[i_ModuleName] = Map<Name, uint32_t>();

          for (auto [i_TypeNameString, i_TypeValue] : i_Value) {
            Name i_TypeName = LOW_NAME(i_TypeNameString->c_str());
            const u32 i_Capacity = i_TypeValue.as<u32>();
            g_Capacities[i_ModuleName][i_TypeName] = i_Capacity;
          }
        }
      }

      void initialize()
      {
        load_capacities();

        LOW_LOG_DEBUG << "Config loaded" << LOW_LOG_END;
      }

      uint32_t get_capacity(Name p_ModuleName, Name p_TypeName)
      {
        return g_Capacities[p_ModuleName][p_TypeName];
      }

    } // namespace Config
  } // namespace Util
} // namespace Low
