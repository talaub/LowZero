#include "LowCoreRuntime.h"

#include "LowCore.h"
#include "LowCoreGameplaySystemInstance.h"
#include "LowUtilAssert.h"
#include "LowUtilEnums.h"
#include "LowCoreGameModeInstance.h"
#include "LowUtilLogger.h"

namespace Low {
  namespace Core {
    struct GameModeEntry
    {
      GameModeInstance instance;
      bool blocking = true;
    };
    Util::List<GameModeEntry> g_GameModeStack;

    bool is_playmode()
    {
      return get_engine_state() == Util::EngineState::PLAYING;
    }

    void push_gamemode(GameMode p_GameMode)
    {
      GameModeEntry l_Entry;
      l_Entry.instance = GameModeInstance::make(p_GameMode);

      l_Entry.blocking = p_GameMode.blocks_underlying();

      if (l_Entry.blocking) {
        for (auto it = g_GameModeStack.rbegin();
             it != g_GameModeStack.rend(); ++it) {
          for (GameplaySystemInstance i_Instance :
               it->instance.get_systems()) {
            i_Instance.suspend();
          }
          if (it->blocking) {
            // We don't need to continue suspending, the ones below
            // are already suspended
            break;
          }
        }
      }

      g_GameModeStack.push_back(l_Entry);
      for (GameplaySystemInstance i_Instance :
           l_Entry.instance.get_systems()) {
        i_Instance.mount();
      }
    }

    void pop_gamemode()
    {
      LOW_ASSERT(!g_GameModeStack.empty(),
                 "Tried to pop gamemode from empty stack.");
      GameModeEntry l_PoppedEntry = g_GameModeStack.back();
      g_GameModeStack.pop_back();

      if (l_PoppedEntry.blocking) {
        for (auto it = g_GameModeStack.rbegin();
             it != g_GameModeStack.rend(); ++it) {
          for (GameplaySystemInstance i_Instance :
               it->instance.get_systems()) {
            i_Instance.resume();
          }
          if (it->blocking) {
            // The ones below should remain suspended
            break;
          }
        }
      }

      for (GameplaySystemInstance i_Instance :
           l_PoppedEntry.instance.get_systems()) {
        i_Instance.unmount();
      }

      l_PoppedEntry.instance.destroy();
    }

    void tick_gamemodes(const float p_Delta)
    {
      if (!is_playmode()) {
        return;
      }

      for (auto it = g_GameModeStack.rbegin();
           it != g_GameModeStack.rend(); ++it) {
        for (GameplaySystemInstance i_Instance :
             it->instance.get_systems()) {
          GameplaySystem i_System = i_Instance.get_gameplay_system();
          i_Instance.tick(p_Delta);
        }
        if (it->blocking) {
          // This gamemode is blocking the ones below from being
          // executed
          break;
        }
      }
    }

    void clear_gamemodes()
    {
      while (!g_GameModeStack.empty()) {
        pop_gamemode();
      }
    }
  } // namespace Core
} // namespace Low
