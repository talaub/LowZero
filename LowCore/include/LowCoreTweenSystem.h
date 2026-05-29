#pragma once

#include "LowUtilEnums.h"
#include "LowCoreApi.h"
#include "LowCoreTweenEase.h"

namespace Low {
  namespace Core {
    struct Tween;

    namespace System {
      namespace Tween {
        float LOW_CORE_API apply_ease(const TweenEase p_Ease,
                                      const float p_Progress);
        float LOW_CORE_API
        get_eased_progress(const Core::Tween &p_Tween);
        float LOW_CORE_API in_out_cubic(const float p_Progress);

        void tick(const float p_Delta,
                  const Util::EngineState p_State);
      }
    } // namespace System
  } // namespace Core
} // namespace Low
