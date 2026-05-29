#pragma once

#ifndef LOW_CORE_ANIMATION_INTERNAL
#error "LowCoreAnimationLocalPose.h is private to the LowCore animation module."
#endif

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Core {
    namespace Animation {
      struct JointPose
      {
        Math::Vector3 position;
        Math::Quaternion rotation;
        Math::Vector3 scale;

        JointPose();
      };

      struct LocalPose
      {
        Util::List<JointPose> joints;

        void clear();
        void resize(u32 p_JointCount);
        u32 joint_count() const;
        bool empty() const;
      };
    } // namespace Animation
  } // namespace Core
} // namespace Low
