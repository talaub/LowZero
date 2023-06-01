#include "LowCoreTransformSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreTransform.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Transform {
        void tick(float p_Delta, Util::EngineState p_State)
        {
        }

        void late_tick(float p_Delta, Util::EngineState p_State)
        {
          Component::Transform *l_Transforms =
              Component::Transform::living_instances();

          /*
                for (uint32_t i = 0u; i < Component::Transform::living_count();
             ++i) { Component::Transform i_Transform = l_Transforms[i];

                  i_Transform.set_world_dirty(true);
                }

                for (uint32_t i = 0u; i < Component::Transform::living_count();
             ++i) { Component::Transform i_Transform = l_Transforms[i];

                  if (i_Transform.is_world_dirty()) {
                                 i_Transform.recalculate_world_transform();
                  }
                }
          */
        }
      } // namespace Transform
    }   // namespace System
  }     // namespace Core
} // namespace Low
