#include "LowCoreAnimationBlender.h"

#include "LowMath.h"
#include "LowMathQuaternionUtil.h"
#include "LowMathVectorUtil.h"

namespace Low {
  namespace Core {
    namespace Animation {
      namespace {
        Math::Quaternion
        shortest_path(const Math::Quaternion &p_Reference,
                      const Math::Quaternion &p_Value)
        {
          if (Math::QuaternionUtil::dot(p_Reference, p_Value) <
              0.0f) {
            return -p_Value;
          }
          return p_Value;
        }
      } // namespace

      WeightedLocalPose::WeightedLocalPose(const LocalPose &p_Pose,
                                           float p_Weight)
          : pose(p_Pose), weight(p_Weight)
      {
      }

      namespace Blender {
        float normalize_weight(float p_Weight)
        {
          return Math::Util::clamp(p_Weight, 0.0f, 1.0f);
        }

        JointPose blend(const JointPose &p_A, const JointPose &p_B,
                        float p_Weight)
        {
          const float l_Weight = normalize_weight(p_Weight);

          JointPose l_Result;
          l_Result.position = Math::VectorUtil::lerp(
              p_A.position, p_B.position, l_Weight);
          l_Result.scale =
              Math::VectorUtil::lerp(p_A.scale, p_B.scale, l_Weight);
          l_Result.rotation = Math::QuaternionUtil::slerp(
              p_A.rotation, shortest_path(p_A.rotation, p_B.rotation),
              l_Weight);

          return l_Result;
        }

        bool blend(const LocalPose &p_A, const LocalPose &p_B,
                   float p_Weight, LocalPose &p_OutPose)
        {
          if (p_A.joint_count() != p_B.joint_count()) {
            return false;
          }

          p_OutPose.resize(p_A.joint_count());
          for (u32 i = 0u; i < p_A.joint_count(); ++i) {
            p_OutPose.joints[i] =
                blend(p_A.joints[i], p_B.joints[i], p_Weight);
          }

          return true;
        }

        bool blend(const Util::List<WeightedLocalPose> &p_Inputs,
                   LocalPose &p_OutPose)
        {
          u32 l_JointCount = 0u;
          float l_TotalWeight = 0.0f;
          const LocalPose *l_ReferencePose = nullptr;

          for (auto it = p_Inputs.begin(); it != p_Inputs.end();
               ++it) {
            const LocalPose &i_Pose = it->pose.get();
            if (it->weight <= 0.0f) {
              continue;
            }

            if (!l_ReferencePose) {
              l_ReferencePose = &i_Pose;
              l_JointCount = i_Pose.joint_count();
            } else if (i_Pose.joint_count() != l_JointCount) {
              return false;
            }

            l_TotalWeight += it->weight;
          }

          if (!l_ReferencePose || l_TotalWeight <= LOW_MATH_EPSILON) {
            return false;
          }

          p_OutPose.resize(l_JointCount);

          for (u32 i = 0u; i < l_JointCount; ++i) {
            Math::Vector3 l_Position(0.0f);
            Math::Vector3 l_Scale(0.0f);
            Math::Quaternion l_Rotation(0.0f, 0.0f, 0.0f, 0.0f);
            bool l_HasRotationReference = false;
            Math::Quaternion l_RotationReference =
                Math::QuaternionUtil::get_identity();

            for (auto it = p_Inputs.begin(); it != p_Inputs.end();
                 ++it) {
              if (it->weight <= 0.0f) {
                continue;
              }

              const float l_NormalizedWeight =
                  it->weight / l_TotalWeight;
              const JointPose &l_Joint = it->pose.get().joints[i];

              l_Position += l_Joint.position * l_NormalizedWeight;
              l_Scale += l_Joint.scale * l_NormalizedWeight;

              if (!l_HasRotationReference) {
                l_RotationReference = l_Joint.rotation;
                l_HasRotationReference = true;
              }

              l_Rotation += shortest_path(l_RotationReference,
                                          l_Joint.rotation) *
                            l_NormalizedWeight;
            }

            p_OutPose.joints[i].position = l_Position;
            p_OutPose.joints[i].scale = l_Scale;
            p_OutPose.joints[i].rotation =
                Math::QuaternionUtil::normalize(l_Rotation);
          }

          return true;
        }
      } // namespace Blender
    } // namespace Animation
  } // namespace Core
} // namespace Low
