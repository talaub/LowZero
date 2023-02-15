#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"
#include "LowUtilProfiler.h"

namespace Low {
  namespace Util {
    void initialize()
    {
      LOW_PROFILE_START(Util init);

      Name::initialize();
      Config::initialize();

      LOW_LOG_INFO("Util initialized");

      LOW_PROFILE_END();
    }

    void cleanup()
    {
      Name::cleanup();

      Profiler::evaluate_memory_allocation();

      LOW_LOG_INFO("Util shutdown");
    }
  } // namespace Util
} // namespace Low
