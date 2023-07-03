#pragma once

#include "imgui.h"

#include "LowUtilEnums.h"

#include "LowCoreEntity.h"
#include "LowRendererMaterial.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;
    struct TypeMetadata;
    struct Widget;

    void initialize();
    void tick(float p_Delta, Util::EngineState p_State);

    void set_selected_entity(Core::Entity p_Entity);
    Core::Entity get_selected_entity();
    Util::Handle get_selected_handle();
    void set_selected_handle(Util::Handle p_Handle);

    void set_focused_widget(Widget *p_Widget);

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func);

    DetailsWidget *get_details_widget();

    TypeMetadata &get_type_metadata(uint16_t p_TypeId);

    void duplicate(Util::List<Util::Handle> p_Handles);

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
