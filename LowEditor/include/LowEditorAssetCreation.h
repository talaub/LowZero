#pragma once

#include "LowEditor.h"
#include "LowEditorApi.h"

#include "LowUtilAssetManager.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilName.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    namespace AssetCreation {

      struct Context
      {
        Util::String directoryPath;
      };

      struct DialogContext
      {
        Context context;
        const char *popupId;
      };

      struct Action
      {
        Util::Name id;
        Util::String label;
        Util::String icon;
        AssetType assetType = AssetType::File;
        i32 priority = 100;
        Util::Name defaultName;
        Util::Function<bool(const Context &)> is_visible;
        Util::Function<bool(const Context &)> is_enabled;
        Util::Function<Util::Handle(const Context &, Util::Name)>
            create;
        Util::Function<void(const DialogContext &)> render_dialog;
      };

      void LOW_EDITOR_API register_action(const Action &p_Action);
      void LOW_EDITOR_API collect_actions(
          const Context &p_Context, Util::List<Action *> &p_Actions);

      template <typename T>
      void register_default(const char *p_Label,
                            const Util::Name p_DefaultName,
                            const AssetType p_AssetType =
                                AssetType::File,
                            const i32 p_Priority = 100)
      {
        Action l_Action;
        l_Action.id = Util::Name(p_Label);
        l_Action.label = p_Label;
        l_Action.assetType = p_AssetType;
        l_Action.priority = p_Priority;
        l_Action.defaultName = p_DefaultName;
        l_Action.create = [](const Context &p_Context,
                             Util::Name p_Name) -> Util::Handle {
          return Util::AssetManager::create<T>(
              p_Name, p_Context.directoryPath);
        };
        register_action(l_Action);
      }

    } // namespace AssetCreation
  }   // namespace Editor
} // namespace Low
