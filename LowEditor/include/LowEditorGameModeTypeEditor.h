#pragma once

#include "LowCoreGameMode.h"
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

    private:
      void render_storage(Core::GameModeSystemStorage &p_Storage);
    };
  } // namespace Editor
} // namespace Low
