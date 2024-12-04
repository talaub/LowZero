#pragma once

#include "LowCoreApi.h"

#include "LowUtilEnums.h"
#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace Input {
      bool LOW_CORE_API
      keyboard_button_down(Util::KeyboardButton p_Button);
      bool LOW_CORE_API
      keyboard_button_up(Util::KeyboardButton p_Button);

      bool LOW_CORE_API mouse_button_down(Util::MouseButton p_Button);
      bool LOW_CORE_API mouse_button_up(Util::MouseButton p_Button);

      void LOW_CORE_API mouse_position(Math::Vector2 &p_Position);

      bool LOW_CORE_API
      mouse_button_released(Util::MouseButton p_Button);
      bool LOW_CORE_API
      mouse_button_pressed(Util::MouseButton p_Button);

      void late_tick(float p_Delta);
    } // namespace Input
  }   // namespace Core
} // namespace Low
