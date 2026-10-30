#pragma once

#include "LowCoreGameMode.h"

namespace Low {
  namespace Core {
    LOW_FUNCTION(scripting, bind_namespace = "Runtime",
                 vs = (category = "Runtime",
                       title = "Is in playmode?"))
    bool is_playmode();
    LOW_FUNCTION(scripting, bind_namespace = "Runtime",
                 vs = (category = "Runtime", title = "Push GameMode"))
    void push_gamemode(LOW_PARAM(in) GameMode p_GameMode);
    LOW_FUNCTION(scripting, bind_namespace = "Runtime",
                 vs = (category = "Runtime", title = "Pop GameMode"))
    void pop_gamemode();
  } // namespace Core
} // namespace Low
