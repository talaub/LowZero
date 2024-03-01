#pragma once

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    namespace Input {
      bool keyboard_button_down(Util::KeyboardButton p_Button);
      bool keyboard_button_up(Util::KeyboardButton p_Button);

      bool mouse_button_down(Util::MouseButton p_Button);
      bool mouse_button_up(Util::MouseButton p_Button);
    } // namespace Input
  }   // namespace Core
} // namespace Low
