#include "LowCoreLightSystem.h"

#include "LowCoreRegion.h"

#include "LowRenderer.h"

#include "LowMath.h"
#include "LowMathVectorUtil.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Region {
        void tick(float p_Delta, Util::EngineState p_State)
        {
          Math::Vector3 l_CameraPosition =
              Renderer::get_main_renderflow().get_camera_position();

          for (Core::Region i_Region : Core::Region::ms_LivingInstances) {
            if (!i_Region.is_streaming_enabled()) {
              continue;
            }

            Math::Vector3 i_DifferenceVector =
                i_Region.get_streaming_position() - l_CameraPosition;

            i_DifferenceVector.y = 0.0f;

            bool i_IsInRange =
                Math::VectorUtil::magnitude_squared(i_DifferenceVector) <
                i_Region.get_streaming_radius() *
                    i_Region.get_streaming_radius();

            if (i_IsInRange && !i_Region.is_loaded()) {
              i_Region.load_entities();
            } else if (!i_IsInRange && i_Region.is_loaded()) {
              i_Region.unload_entities();
            }
          }
        }
      } // namespace Region
    }   // namespace System
  }     // namespace Core
} // namespace Low
