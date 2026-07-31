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

      LOW_FUNCTION(scripting, bind_namespace = "Input")
      LOW_CORE_API void
      mouse_position(LOW_PARAM(out) Math::Vector2 &p_Position);

      LOW_FUNCTION(scripting, bind_namespace = "Input")
      LOW_CORE_API bool
      mouse_world_ray(LOW_PARAM(out) Math::Vector3 *p_Origin,
                      LOW_PARAM(out) Math::Vector3 *p_Direction);

      bool LOW_CORE_API
      mouse_button_released(Util::MouseButton p_Button);
      bool LOW_CORE_API
      mouse_button_pressed(Util::MouseButton p_Button);

      void late_tick(float p_Delta);

      void initialize();
      void cleanup();
    } // namespace Input
  } // namespace Core
} // namespace Low
