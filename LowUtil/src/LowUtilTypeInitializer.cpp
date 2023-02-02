#include "LowUtilHandle.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

namespace Low {
  namespace Util {
    namespace Instances {
      void initialize()
      {

        LOW_LOG_DEBUG("Type buffers initialized");
      }
      void cleanup()
      {
        LOW_LOG_DEBUG("Cleaned up type buffers");
      }
    } // namespace Instances
  }   // namespace Util
} // namespace Low
