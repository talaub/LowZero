#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"
#include "LowUtilMemory.h"

namespace Low {
  namespace Util {
    void initialize()
    {
      LOW_PROFILE_START(Util init);

      Memory::initialize();
      Name::initialize();
      Config::initialize();

      LOW_LOG_INFO << "Util initialized" << LOW_LOG_END;

      LOW_PROFILE_END();
    }

    void cleanup()
    {
      Name::cleanup();
      Memory::cleanup();

      Profiler::evaluate_memory_allocation();

      LOW_LOG_INFO << "Util shutdown" << LOW_LOG_END;
    }
  } // namespace Util
} // namespace Low
