#pragma once

#ifndef LOW_CORE_ANIMATION_INTERNAL
#error "LowCoreAnimationBlender.h is private to the LowCore animation module."
#endif

#include "LowCoreAnimationLocalPose.h"

#include <functional>

namespace Low {
  namespace Core {
    namespace Animation {
      struct WeightedLocalPose
      {
        WeightedLocalPose(const LocalPose &p_Pose, float p_Weight);

        std::reference_wrapper<const LocalPose> pose;
        float weight;
      };

      namespace Blender {
        float normalize_weight(float p_Weight);

        JointPose blend(const JointPose &p_A, const JointPose &p_B,
                        float p_Weight);

        bool blend(const LocalPose &p_A, const LocalPose &p_B,
                   float p_Weight, LocalPose &p_OutPose);

        bool blend(const Util::List<WeightedLocalPose> &p_Inputs,
                   LocalPose &p_OutPose);
      } // namespace Blender
    }   // namespace Animation
  }     // namespace Core
} // namespace Low
