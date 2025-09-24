#pragma once

#include "LowEditorApi.h"

#include "LowEditorChangeList.h"

#include "LowRendererEditorImage.h"

#include "LowCoreEntity.h"

#include "LowUtilEnums.h"
#include "LowUtilFileSystem.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget;
    struct EditingWidget;
    struct FlodeWidget;
    struct TypeMetadata;
    struct EnumMetadata;
    struct Widget;

    enum class AssetType
    {
      File,
      Material,
      Mesh,
      Model,
      Script,
      Flode,
      Texture,
      Font
    };

    struct DirectoryWatchers
    {
      Util::FileSystem::WatchHandle flodeDirectory;
    };

    void LOW_EDITOR_API set_selected_entity(Core::Entity p_Entity);
    Core::Entity LOW_EDITOR_API get_selected_entity();
    Util::Handle LOW_EDITOR_API get_selected_handle();
    void LOW_EDITOR_API set_selected_handle(Util::Handle p_Handle);

    void LOW_EDITOR_API set_focused_widget(Widget *p_Widget);

    void LOW_EDITOR_API initialize();
    void LOW_EDITOR_API tick(float p_Delta,
                             Util::EngineState p_State);

    void LOW_EDITOR_API load_user_settings();

    void LOW_EDITOR_API set_user_setting(Util::Name p_Name,
                                         Util::Variant p_Variant);
    Util::Variant LOW_EDITOR_API get_user_setting(Util::Name p_Name);

    void LOW_EDITOR_API save_user_settings();

    Util::String LOW_EDITOR_API prettify_name(Util::Name p_Name);
    Util::String LOW_EDITOR_API prettify_name(Util::String p_String);
    Util::String LOW_EDITOR_API
    technify_string(Util::String p_String);

    LOW_EDITOR_API ChangeList &get_global_changelist();

    void LOW_EDITOR_API duplicate(Util::List<Util::Handle> p_Handles);

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func);

    void LOW_EDITOR_API register_widget(Low::Util::String p_Path,
                                        Widget *p_Widget,
                                        bool p_DefaultOpen = false);

    LOW_EDITOR_API DirectoryWatchers &get_directory_watchers();

    Util::Map<u16, TypeMetadata> &get_type_metadata();
    TypeMetadata &get_type_metadata(uint16_t p_TypeId);
    EnumMetadata &get_enum_metadata(Util::Name p_EnumTypeName);
    EnumMetadata &get_enum_metadata(u16 p_EnumId);

    bool LOW_EDITOR_API is_editor_job_in_progress();
    Util::String LOW_EDITOR_API get_active_editor_job_name();

    void LOW_EDITOR_API open_flode_graph(Util::String p_Path);

    void LOW_EDITOR_API close_widget(Widget *p_Widget);

    void LOW_EDITOR_API
    delete_file_if_exists(Low::Util::String p_Path);

    Math::Color get_color_for_asset_type(const AssetType p_AssetType);
    Util::String get_asset_type_name(const AssetType p_AssetType);
    Renderer::EditorImage
    get_editor_image_for_asset_type(const AssetType p_AssetType);

    namespace History {
      Transaction LOW_EDITOR_API
      create_handle_transaction(Util::Handle p_Handle);
      Transaction LOW_EDITOR_API
      destroy_handle_transaction(Util::Handle p_Handle);
    } // namespace History

  } // namespace Editor
} // namespace Low
