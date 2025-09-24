#include "LowCoreUiTextSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiDisplay.h"
#include "LowCoreUiText.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include "LowCoreUiImageSystem.h"
#include <cmath>
#include "microprofile.h"
#include <stdint.h>

#define RENDER_BOUNDING_BOXES 0

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Text {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            if (p_State != Util::EngineState::PLAYING) {
              return;
            }

            MICROPROFILE_SCOPEI("Core", "UiTextSystem::TICK", MP_RED);

            Component::Text *l_Texts =
                Component::Text::living_instances();

            for (uint32_t i = 0u; i < Component::Text::living_count();
                 ++i) {
              Component::Text i_Text = l_Texts[i];

              if (i_Text.get_element()
                      .get_view()
                      .is_view_template()) {
                continue;
              }

              // TODO: Implement
            }
          }

        } // namespace Text
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
