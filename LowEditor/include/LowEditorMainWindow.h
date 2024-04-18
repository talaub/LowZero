#pragma once

#include "LowEditorApi.h"

#include "imgui.h"

#include "LowUtilEnums.h"

#include "LowCoreEntity.h"
#include "LowCoreUiElement.h"

#include "LowRendererMaterial.h"

#include "LowEditorChangeList.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;
    struct EditingWidget;
    struct TypeMetadata;
    struct EnumMetadata;
    struct Widget;

    void LOW_EDITOR_API initialize();
    void LOW_EDITOR_API tick(float p_Delta,
                             Util::EngineState p_State);

    void set_selected_entity(Core::Entity p_Entity);
    Core::Entity get_selected_entity();
    Util::Handle get_selected_handle();
    void set_selected_handle(Util::Handle p_Handle);

    void set_focused_widget(Widget *p_Widget);

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func);

    DetailsWidget *get_details_widget();
    EditingWidget *get_editing_widget();

    Util::Map<u16, TypeMetadata> &get_type_metadata();
    TypeMetadata &get_type_metadata(uint16_t p_TypeId);
    EnumMetadata &get_enum_metadata(Util::String p_EnumTypeName);

    void duplicate(Util::List<Util::Handle> p_Handles);

    ChangeList &get_global_changelist();

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

      Transaction create_handle_transaction(Util::Handle p_Handle);
      Transaction destroy_handle_transaction(Util::Handle p_Handle);
    } // namespace Helper
  }   // namespace Editor
} // namespace Low
