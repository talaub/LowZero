#pragma once

#include "imgui.h"

#include "LowCoreEntity.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;

    void initialize();
    void tick(float p_Delta);

    void set_selected_entity(Core::Entity p_Entity);
    Core::Entity get_selected_entity();
    Util::Handle get_selected_handle();
    void set_selected_handle(Util::Handle p_Handle);

    DetailsWidget *get_details_widget();
  } // namespace Editor
} // namespace Low
