#pragma once

#include <imgui.h>

#include "FlodeApi.h"

namespace Flode {
  namespace Helper {
    bool FLODE_API BeginNodeCombo(const char *label,
                                  const char *preview_value,
                                  ImGuiComboFlags flags = 0);
    void FLODE_API EndNodeCombo();
  } // namespace Helper
} // namespace Flode
