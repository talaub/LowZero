#include "LowCoreAnimationLocalPose.h"

#include "LowMathQuaternionUtil.h"

namespace Low {
  namespace Core {
    namespace Animation {
      JointPose::JointPose()
          : position(0.0f), rotation(Math::QuaternionUtil::get_identity()),
            scale(1.0f)
      {
      }

      void LocalPose::clear()
      {
        joints.clear();
      }

      void LocalPose::resize(u32 p_JointCount)
      {
        joints.resize(p_JointCount);
      }

      u32 LocalPose::joint_count() const
      {
        return static_cast<u32>(joints.size());
      }

      bool LocalPose::empty() const
      {
        return joints.empty();
      }
    } // namespace Animation
  } // namespace Core
} // namespace Low
