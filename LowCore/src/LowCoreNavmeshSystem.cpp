#include "LowCoreNavmeshSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreSystem.h"

#include <Recast.h>

namespace Low {
  namespace Core {
    namespace System {
      namespace Navmesh {
        rcConfig g_RecastConfig;
        rcContext g_RecastContext;

        static void start()
        {
          // LOW_LOG_INFO << "Setting up recast" << LOW_LOG_END;
          /*
                memset(&g_RecastConfig, 0, sizeof(g_RecastConfig));

                g_RecastConfig.height = 5;
                g_RecastConfig.cs = 0.3f;
                g_RecastConfig.ch = 0.5f;
                g_RecastConfig.walkableRadius = 1;
                g_RecastConfig.walkableSlopeAngle = 45.0f;

                g_RecastContext.enableLog(true);

                // Create a heightfield object
                rcHeightfield *heightfield = rcAllocHeightfield();
                LOW_ASSERT(rcCreateHeightfield(&g_RecastConfig, *heightfield),
                           "Failed to create heightfield");
          */

          // LOW_LOG_INFO << "Finished setting up recast" << LOW_LOG_END;
        }

        void tick(float p_Delta, Util::EngineState p_State)
        {
          LOW_PROFILE_CPU("Core", "Navmesh Tick");
          SYSTEM_ON_START(start);
        }
      } // namespace Navmesh
    }   // namespace System
  }     // namespace Core
} // namespace Low