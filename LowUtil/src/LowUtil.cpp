#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileSystem.h"

namespace Low {
  namespace Util {
    Project g_Project;

    void initialize()
    {
      g_Project.dataPath = "./data";
      g_Project.rootPath = "./";

      g_Project.engineRootPath = "./";
      g_Project.engineDataPath = "./LowData";

      Log::initialize();
      Memory::initialize();
      Name::initialize();
      Config::initialize();
      JobManager::initialize();

      LOW_LOG_INFO << "Util initialized" << LOW_LOG_END;
    }

    void tick(float p_Delta)
    {
      FileSystem::tick(p_Delta);
    };

    void cleanup()
    {
      JobManager::cleanup();
      Name::cleanup();
      Memory::cleanup();

      Profiler::evaluate_memory_allocation();

      LOW_LOG_INFO << "Util shutdown" << LOW_LOG_END;
      Log::cleanup();
    }

    const Project &get_project()
    {
      return g_Project;
    }
  } // namespace Util
} // namespace Low
