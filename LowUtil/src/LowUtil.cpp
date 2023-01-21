#include "LowUtil.h"

#include "LowUtilLogger.h"
#include "LowUtilConfig.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    void initialize()
    {
      Name::initialize();
      Config::initialize();
      Instances::initialize();

      LOW_LOG_INFO("Util initialized");
    }

    void cleanup()
    {
      Instances::cleanup();
      Name::cleanup();

      LOW_LOG_INFO("Util shutdown");
    }
  } // namespace Util
} // namespace Low
