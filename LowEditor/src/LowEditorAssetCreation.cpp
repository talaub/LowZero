#include "LowEditorAssetCreation.h"

#include <algorithm>

namespace Low {
  namespace Editor {
    namespace AssetCreation {

      static Util::List<Action> g_Actions;

      void register_action(const Action &p_Action)
      {
        for (Action &i_Action : g_Actions) {
          if (i_Action.id == p_Action.id) {
            i_Action = p_Action;
            return;
          }
        }

        g_Actions.push_back(p_Action);
      }

      void collect_actions(const Context &p_Context,
                           Util::List<Action *> &p_Actions)
      {
        for (Action &i_Action : g_Actions) {
          bool i_Visible = true;
          if (i_Action.is_visible) {
            i_Visible = i_Action.is_visible(p_Context);
          }

          if (i_Visible) {
            p_Actions.push_back(&i_Action);
          }
        }

        std::sort(p_Actions.begin(), p_Actions.end(),
                  [](const Action *p_Left, const Action *p_Right) {
                    return p_Left->priority < p_Right->priority;
                  });
      }

      void register_directory(const char *p_Label,
                              Util::Name p_DefaultName,
                              const i32 p_Priority)
      {
        Action l_Action;
        l_Action.id = Util::Name(p_Label);
        l_Action.label = p_Label;
        l_Action.assetType = AssetType::File;
        l_Action.priority = p_Priority;
        l_Action.defaultName = p_DefaultName;
        l_Action.create = [](const Context &p_Context,
                             Util::Name p_Name) -> Util::Handle {
          Util::StringBuilder l_PathBuilder;
          l_PathBuilder.append(p_Context.directoryPath)
              .append("/")
              .append(p_Name);
          Util::FileSystem::create_directory(l_PathBuilder.get());
          return Util::Handle::DEAD;
        };
        register_action(l_Action);
      }

    } // namespace AssetCreation
  } // namespace Editor
} // namespace Low
