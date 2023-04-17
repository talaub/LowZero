#pragma once

#include "LowCoreEntity.h"

namespace Low {
  namespace Editor {
    void initialize();
    void tick(float p_Delta);

    void set_selected_entity(Core::Entity p_Entity);
    Core::Entity get_selected_entity();
  } // namespace Editor
} // namespace Low
