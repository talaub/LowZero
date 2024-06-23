#pragma once

#include "LowEditorApi.h"

#include "imgui.h"

#include "LowUtilEnums.h"

#include "LowCoreEntity.h"
#include "LowCoreUiElement.h"

#include "LowRendererMaterial.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;
    struct EditingWidget;
    struct FlodeWidget;
    struct TypeMetadata;
    struct EnumMetadata;
    struct Widget;

    void initialize_main_window();
    void render_main_window(float p_Delta, Util::EngineState p_State);

    DetailsWidget *get_details_widget();
    EditingWidget *get_editing_widget();
    FlodeWidget *get_flode_widget();

    void set_widget_open(Util::Name p_Name, bool p_Open);

    bool get_gizmos_dragged();
    void set_gizmos_dragged(bool p_Dragged);

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
