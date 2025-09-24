#include "LowCoreUiImageSystem.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilProfiler.h"
#include "LowUtilConfig.h"

#include "LowCoreUiImage.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreTaskScheduler.h"
#include "LowCoreDebugGeometry.h"

#include <cmath>
#include "microprofile.h"
#include <stdint.h>

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Image {
          void tick(float p_Delta, Util::EngineState p_State)
          {
            if (p_State != Util::EngineState::PLAYING) {
              return;
            }

            MICROPROFILE_SCOPEI("Core", "UiImageSystem::TICK",
                                MP_RED);

            Component::Image *l_Images =
                Component::Image::living_instances();

            for (uint32_t i = 0u;
                 i < Component::Image::living_count(); ++i) {
              Component::Image i_Image = l_Images[i];

              if (i_Image.get_element()
                      .get_view()
                      .is_view_template()) {
                continue;
              }

              Component::Display i_Display =
                  i_Image.get_element().get_display();

              if (!i_Image.get_texture().is_alive()) {
                continue;
              }

              // TODO: IMPLEMENT
            }
          }
        } // namespace Image
      } // namespace System
    } // namespace UI
  } // namespace Core
} // namespace Low
