#include "LowUtilModuleInfo.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Util {
    static List<RegisteredModuleInfo> g_ModuleInfos;

    static String to_string_or_unknown(const char *p_Value)
    {
      if (!p_Value) {
        return "unknown";
      }
      return p_Value;
    }

    void register_module_info(const ModuleInfo *p_Info)
    {
      if (!p_Info) {
        return;
      }

      if (p_Info->structVersion < 1) {
        LOW_LOG_WARN << "Ignoring module info with unknown struct version "
                     << p_Info->structVersion << LOW_LOG_END;
        return;
      }

      RegisteredModuleInfo l_Info;
      l_Info.name = to_string_or_unknown(p_Info->name);
      l_Info.version = to_string_or_unknown(p_Info->version);
      l_Info.projectBuildId = to_string_or_unknown(p_Info->projectBuildId);
      l_Info.engineVersion = to_string_or_unknown(p_Info->engineVersion);
      l_Info.engineBuildId = to_string_or_unknown(p_Info->engineBuildId);

      LOW_LOG_INFO << "Module " << l_Info.name << " " << l_Info.version
                   << " (" << l_Info.projectBuildId
                   << ", built against LowEngine " << l_Info.engineVersion
                   << " " << l_Info.engineBuildId << ")" << LOW_LOG_END;

      g_ModuleInfos.push_back(l_Info);
    }

    const List<RegisteredModuleInfo> &get_module_infos()
    {
      return g_ModuleInfos;
    }
  } // namespace Util
} // namespace Low
