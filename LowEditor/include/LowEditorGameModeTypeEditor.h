#pragma once

#include "LowEditorTypeEditor.h"

namespace Low {
  namespace Editor {
    struct GameModeTypeEditor : public TypeEditor
    {
      GameModeTypeEditor(Low::Util::Handle p_Handle)
          : TypeEditor(p_Handle)
      {
      }
      void render(float p_Delta) override;
    };
  } // namespace Editor
} // namespace Low
