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

    } // namespace AssetCreation
  }   // namespace Editor
} // namespace Low
