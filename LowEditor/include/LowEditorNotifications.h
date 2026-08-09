#pragma once

#include "LowUtilContainers.h"
#include "LowMath.h"

#include "LowEditorApi.h"

namespace Low {
  namespace Editor {
    struct Notification
    {
      Util::String icon;
      Util::String title;
      Util::String subtitle;
      Util::String message;
      float duration;
      float time_remaining;
      float age = 0.0f;
      Math::Color color;

      Notification(const Util::String &icon,
                   const Util::String &title,
                   const Util::String &subtitle,
                   const Util::String &msg, float duration,
                   Math::Color color)
          : icon(icon), title(title), subtitle(subtitle), message(msg),
            duration(duration), time_remaining(duration), color(color)
      {
      }
    };

    // A notification is made up of up to four visual pieces, all
    // optional except the title:
    //   icon     - shown in a colored badge on the left
    //   title    - bold, primary line
    //   subtitle - smaller line directly under the title (e.g. an
    //              asset/graph name)
    //   message  - wrapped body text below the title/subtitle
    void LOW_EDITOR_API push_notification(
        const Util::String &icon, const Util::String &title,
        const Util::String &subtitle, const Util::String &message = "",
        float duration = 5.0f,
        Math::Color color = Math::Color(0.2f, 0.6f, 1.0f, 1.0f));

    void render_notifications(float p_Delta);

  } // namespace Editor
} // namespace Low
