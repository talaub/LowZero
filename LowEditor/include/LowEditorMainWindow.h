#pragma once

#include "imgui.h"

#include "LowUtilEnums.h"

#include "LowCoreEntity.h"
#include "LowRendererMaterial.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;

    void initialize();
    void tick(float p_Delta, Util::EngineState p_State);

    void set_selected_entity(Core::Entity p_Entity);
    Core::Entity get_selected_entity();
    Util::Handle get_selected_handle();
    void set_selected_handle(Util::Handle p_Handle);

    DetailsWidget *get_details_widget();

    namespace Helper {
      struct SphericalBillboardMaterials
      {
        Renderer::Material sun;
        Renderer::Material bulb;
        Renderer::Material camera;
        Renderer::Material region;
      };

      SphericalBillboardMaterials get_spherical_billboard_materials();
    } // namespace Helper
  }   // namespace Editor
} // namespace Low
