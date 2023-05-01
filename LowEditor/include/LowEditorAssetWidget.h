#pragma once

#include "LowEditorWidget.h"

namespace Low {
  namespace Editor {
    struct AssetWidget : public Widget
    {
      AssetWidget();

      void render(float p_Delta) override;

    protected:
      int m_SelectedCategory;

      void render_meshes();
      void render_materials();
    };
  } // namespace Editor
} // namespace Low
