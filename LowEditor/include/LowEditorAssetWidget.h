#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

struct ImRect;

namespace Low {
  namespace Editor {

    struct FileElement
    {
      bool directory;
      Util::String name;
      Util::Handle handle;
    };

    struct AssetTypeConfig
    {
      uint16_t typeId;
      Util::List<FileElement> elements;
      Util::String rootPath;
      Util::String currentPath;
      bool subfolder;
      Util::String suffix;
      bool update;
      void (*render)(AssetTypeConfig &, ImRect);
    };

    struct AssetWidget : public Widget
    {
      AssetWidget();

      void render(float p_Delta) override;

    protected:
      int m_SelectedCategory;

      Util::List<AssetTypeConfig> m_TypeConfigs;

      float m_UpdateCounter;
    };
  } // namespace Editor
} // namespace Low
