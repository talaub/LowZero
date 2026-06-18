#pragma once

#include "LowEditorTypeEditor.h"

namespace Low {
  namespace Editor {
    struct ConvexHullColliderTypeEditor : public TypeEditor
    {
      ConvexHullColliderTypeEditor(Low::Util::Handle p_Handle)
          : TypeEditor(p_Handle)
      {
      }
      virtual void render(const float p_Delta) override;
    };
  } // namespace Editor
} // namespace Low
