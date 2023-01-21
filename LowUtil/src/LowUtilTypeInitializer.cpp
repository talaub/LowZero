#include "LowUtilHandle.h"

#include "LowUtilLogger.h"
#include "LowUtilAssert.h"

#include "LowUtilTestType.h"

namespace Low {
  namespace Util {
    namespace Instances {
      void initialize()
      {
        initialize_buffer(&Low::Util::TestType::ms_Buffer,
                          Low::Util::TestTypeData::get_size(),
                          Low::Util::TestType::get_capacity(),
                          &Low::Util::TestType::ms_Slots);

        LOW_LOG_DEBUG("Type buffers initialized");
      }
    } // namespace Instances
  }   // namespace Util
} // namespace Low
