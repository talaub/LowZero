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
    void initialize()
    {
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
  } // namespace Util
} // namespace Low
