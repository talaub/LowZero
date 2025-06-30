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
      Util::String message;
      float time_remaining;
      Math::Color color;

      Notification(const Util::String &icon,
                   const Util::String &title, const Util::String &msg,
                   float duration, Math::Color color)
          : icon(icon), title(title), message(msg),
            time_remaining(duration), color(color)
      {
      }
    };

    void LOW_EDITOR_API push_notification(
        const Util::String icon, const Util::String title,
        const Util::String msg, float duration = 5.0f,
        Math::Color color = Math::Color(0.2f, 0.6f, 1.0f, 1.0f));

    void render_notifications(float p_Delta);

  } // namespace Editor
} // namespace Low
