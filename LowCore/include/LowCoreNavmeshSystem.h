#pragma once

#include "LowMath.h"

#include "LowUtilEnums.h"

#include <DetourCrowd.h>

namespace Low {
  namespace Core {
    namespace Component {
      struct NavmeshAgent;
    }
    namespace System {
      namespace Navmesh {
        void tick(float p_Delta, Util::EngineState p_State);

        int add_agent(Math::Vector3 &p_Position, dtCrowdAgentParams *p_Params);
        void set_agent_target_position(Component::NavmeshAgent p_Agent,
                                       Math::Vector3 &p_Position);
      } // namespace Navmesh
    }   // namespace System
  }     // namespace Core
} // namespace Low
