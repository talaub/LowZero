#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowUtilFileSystem.h"

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
      Util::FileSystem::WatchHandle rootDirectoryWatchHandle;
      Util::FileSystem::WatchHandle currentDirectoryWatchHandle;
      void (*render)(AssetTypeConfig &, ImRect);
    };

    struct AssetWidget : public Widget
    {
      AssetWidget();

      void render(float p_Delta) override;

      static void save_prefab_asset(Util::Handle p_Handle);
      static void save_material_asset(Util::Handle p_Handle);

    protected:
      int m_SelectedCategory;

      Util::List<AssetTypeConfig> m_TypeConfigs;

      float m_UpdateCounter;
    };
  } // namespace Editor
} // namespace Low
