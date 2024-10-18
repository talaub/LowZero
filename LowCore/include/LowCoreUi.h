#pragma once

#include "LowCoreApi.h"
#include "LowCoreUiElement.h"

namespace Low {
  namespace Core {
    namespace UI {
      Element LOW_CORE_API get_hovered_element();
      void set_hovered_element(Element p_Element);
    } // namespace UI
  }   // namespace Core
} // namespace Low
