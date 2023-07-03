#pragma once

#include "LowUtilLogger.h"

namespace Low {
  namespace Editor {
    struct Widget
    {
      virtual void render(float p_Delta)
      {
      }

      virtual bool handle_shortcuts(float p_Delta)
      {
        return false;
      }
    };
  } // namespace Editor
} // namespace Low
