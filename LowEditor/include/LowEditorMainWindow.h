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

    struct EditorWidget
    {
      Util::String path;
      Widget *widget;
      Util::String name;
      bool open;
    };

    Util::Map<Util::String, EditorWidget> &get_editor_widgets();

    void initialize_main_window();
    void render_main_window(float p_Delta, Util::EngineState p_State);

    DetailsWidget *get_details_widget();
    EditingWidget *get_editing_widget();
    FlodeWidget *get_flode_widget();

    void set_widget_open(Util::String p_Path, bool p_Open);

    void register_editor_widget(Util::String p_Path, Widget *p_Widget,
                                bool p_DefaultOpen = false);

    bool get_gizmos_dragged();
    void set_gizmos_dragged(bool p_Dragged);

    void close_editor_widget(Widget *p_Widget);

    void _set_focused_widget(Widget *p_Widget);

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
