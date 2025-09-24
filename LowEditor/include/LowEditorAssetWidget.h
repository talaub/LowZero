#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowUtilFileSystem.h"

struct ImRect;

namespace Low {
  namespace Editor {
    struct AssetWidget : public Widget
    {
      AssetWidget();

      void render(float p_Delta) override;

      static void save_prefab_asset(Util::Handle p_Handle);

    protected:
      Util::FileSystem::WatchHandle m_SelectedDirectory;

      float m_UpdateCounter;

      Util::FileSystem::WatchHandle m_DataWatcher;

      void
      render_directory_list(const Util::FileSystem::DirectoryWatcher
                                &p_DirectoryWatcher,
                            const Util::String p_DisplayName = "");

      void render_directory_content(
          const Util::FileSystem::DirectoryWatcher
              &p_DirectoryWatcher);
    };
  } // namespace Editor
} // namespace Low
