#pragma once

#include "LowRenderer2Api.h"

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    struct SkeletalRenderObject;
    struct Skeleton;

    namespace SkinningSystem {
      LOW_RENDERER2_API bool evaluate_global_pose_for_skeletal_renderobject(
          SkeletalRenderObject p_SRO, Skeleton p_Skeleton,
          const Util::List<Math::Matrix4x4> &p_GlobalPose);
      LOW_RENDERER2_API void tick(const float p_Delta);
    }
  } // namespace Renderer
} // namespace Low
