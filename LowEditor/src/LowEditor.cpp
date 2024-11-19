#include "LowEditor.h"

#include "LowUtilJobManager.h"
#include "LowUtil.h"
#include "LowUtilProfiler.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilFileSystem.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"

#include "LowEditorMainWindow.h"
#include "LowEditorThemes.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorFlodeWidget.h"

#include "Flode.h"
#include "FlodeEditor.h"
#include "FlodeMathNodes.h"
#include "FlodeSyntaxNodes.h"
#include "FlodeDebugNodes.h"
#include "FlodeCastNodes.h"
#include "FlodeHandleNodes.h"
#include "FlodeBoolNodes.h"

namespace Low {
  namespace Editor {
    float g_DirectoryUpdateTimer = 2.0f;
    DirectoryWatchers g_DirectoryWatchers;

    ChangeList g_ChangeList;

    Util::Map<uint16_t, TypeMetadata> g_TypeMetadata;
    Util::List<EnumMetadata> g_EnumMetadata;

    Util::Map<Util::Name, Util::Variant> g_UserSettings;

    struct EditorJob
    {
      Util::String title;
      Util::Future<void> future;
      bool submitted;
      std::function<void()> func;

      bool is_ready()
      {
        return future.wait_for(std::chrono::seconds(0)) ==
               std::future_status::ready;
      }

      EditorJob(Util::String p_String, std::function<void()> p_Func)
          : title(p_String), submitted(false), func(p_Func)
      {
      }
    };

    Util::Queue<EditorJob> g_EditorJobQueue;

    static void tick_editor_jobs(float p_Delta)
    {
      if (g_EditorJobQueue.empty()) {
        return;
      }

      if (g_EditorJobQueue.front().submitted) {
        if (!g_EditorJobQueue.front().is_ready()) {
          return;
        }

        g_EditorJobQueue.pop();
        return;
      }

      if (!g_EditorJobQueue.front().submitted) {
        g_EditorJobQueue.front().submitted = true;
        g_EditorJobQueue.front().future =
            std::move(Util::JobManager::default_pool().enqueue(
                g_EditorJobQueue.front().func));
      }
    }

    void close_widget(Widget *p_Widget)
    {
      close_editor_widget(p_Widget);
    }

    TypeMetadata &get_type_metadata(uint16_t p_TypeId)
    {
      LOW_ASSERT(g_TypeMetadata.find(p_TypeId) !=
                     g_TypeMetadata.end(),
                 "Could not find type metadata");
      return g_TypeMetadata[p_TypeId];
    }

    EnumMetadata &get_enum_metadata(u16 p_EnumId)
    {
      for (EnumMetadata &i_Metadata : g_EnumMetadata) {
        if (i_Metadata.enumId == p_EnumId) {
          return i_Metadata;
        }
      }

      LOW_ASSERT(false, "Could not find enum metadata");
      return EnumMetadata();
    }

    EnumMetadata &get_enum_metadata(Util::Name p_EnumTypeName)
    {
      for (EnumMetadata &i_Metadata : g_EnumMetadata) {
        if (i_Metadata.name == p_EnumTypeName) {
          return i_Metadata;
        }
      }

      LOW_ASSERT(false, "Could not find enum metadata");
      return EnumMetadata();
    }

    Util::Map<u16, TypeMetadata> &get_type_metadata()
    {
      return g_TypeMetadata;
    }

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func)
    {
      g_EditorJobQueue.emplace(p_Title, p_Func);
    }

    Util::Handle g_SelectedHandle;

    void set_selected_handle(Util::Handle p_Handle)
    {
      g_SelectedHandle = p_Handle;

      get_details_widget()->clear();

      {
        Core::UI::Element l_Element = p_Handle.get_id();
        if (l_Element.is_alive()) {
          for (auto it = l_Element.get_components().begin();
               it != l_Element.get_components().end(); ++it) {
            get_details_widget()->add_section(it->second);
          }
        }
      }

      Core::Entity l_Entity = p_Handle.get_id();
      if (!l_Entity.is_alive()) {
        uint32_t l_Id = ~0u;
        get_editing_widget()
            ->m_RenderFlowWidget->get_renderflow()
            .get_resources()
            .get_buffer_resource(N(SelectedIdBuffer))
            .set(&l_Id);
        return;
      }
      uint32_t l_Id = l_Entity.get_index();

      get_editing_widget()
          ->m_RenderFlowWidget->get_renderflow()
          .get_resources()
          .get_buffer_resource(N(SelectedIdBuffer))
          .set(&l_Id);

      {
        HandlePropertiesSection l_Section(l_Entity, true);
        l_Section.render_footer = nullptr;
        get_details_widget()->add_section(l_Section);
      }

      for (auto it = l_Entity.get_components().begin();
           it != l_Entity.get_components().end(); ++it) {
        if (it->first == Core::Component::PrefabInstance::TYPE_ID) {
          continue;
        }
        get_details_widget()->add_section(it->second);
      }
    }

    Core::Entity get_selected_entity()
    {
      return g_SelectedHandle.get_id();
    }

    void set_selected_entity(Core::Entity p_Entity)
    {
      set_selected_handle(p_Entity);
    }

    Util::Handle get_selected_handle()
    {
      return g_SelectedHandle;
    }

    static inline void load_user_settings()
    {
      Util::String l_Path =
          Util::get_project().rootPath + "/user.yaml";

      if (!Util::FileIO::file_exists_sync(l_Path.c_str())) {
        return;
      }

      Util::Yaml::Node l_Root = Util::Yaml::load_file(l_Path.c_str());

      Low::Core::Scene l_Scene =
          Low::Core::Scene::find_by_name(N(TestScene));
      if (l_Root["loaded_scene"]) {
        Core::Scene l_LocalScene = Core::Scene::find_by_name(
            LOW_YAML_AS_NAME(l_Root["loaded_scene"]));
        if (l_LocalScene.is_alive()) {
          l_Scene = l_LocalScene;
        }
      }

      l_Scene.load();

      if (l_Root["widgets"]) {
        for (uint32_t i = 0; i < l_Root["widgets"].size(); ++i) {
          set_widget_open(
              LOW_YAML_AS_NAME(l_Root["widgets"][i]["name"]),
              l_Root["widgets"][i]["open"].as<bool>());
        }
      }

      if (l_Root["theme"]) {
        theme_apply(LOW_YAML_AS_NAME(l_Root["theme"]));
      } else if (theme_exists(N(dracula))) {
        theme_apply(N(dracula));
      }

      if (l_Root["custom"]) {
        for (auto it = l_Root["custom"].begin();
             it != l_Root["custom"].end(); ++it) {
          g_UserSettings[LOW_YAML_AS_NAME(it->first)] =
              Util::Serialization::deserialize_variant(it->second);
        }
      }

      g_UserSettings[N(theme)] = theme_get_current_name();
      g_UserSettings[N(loaded_scene)] = l_Scene.get_name();
    }

    static void register_type_nodes()
    {
      for (auto it = get_type_metadata().begin();
           it != get_type_metadata().end(); ++it) {
        if (it->second.scriptingExpose) {
          Flode::register_nodes_for_type(it->second.typeId);
        }
      }
    }

    ChangeList &get_global_changelist()
    {
      return g_ChangeList;
    }

    static void
    parse_editor_type_metadata(TypeEditorMetadata &p_Metadata,
                               Util::Yaml::Node &p_Node)
    {
      p_Metadata.manager = false;
      if (p_Node["manager"]) {
        p_Metadata.manager = p_Node["manager"].as<bool>();
      }
      if (p_Node["saveable"]) {
        p_Metadata.saveable = p_Node["saveable"].as<bool>();
      }
    }

    static void parse_property_metadata(PropertyMetadata &p_Metadata,
                                        Util::Yaml::Node &p_Node)
    {
      if (p_Node["multiline"]) {
        p_Metadata.multiline = p_Node["multiline"].as<bool>();
      }
    }

    static void parse_type_metadata(TypeMetadata &p_Metadata,
                                    Util::Yaml::Node &p_Node)
    {
      p_Metadata.typeInfo =
          Util::Handle::get_type_info(p_Metadata.typeId);

      {
        // Initializing the editor part of the metadata
        p_Metadata.editor.manager = false;
        p_Metadata.editor.saveable = false;
      }

      if (p_Node["editor"]) {
        parse_editor_type_metadata(p_Metadata.editor,
                                   p_Node["editor"]);
      }

      const char *l_PropertiesName = "properties";

      if (p_Node[l_PropertiesName]) {
        if (!p_Node["component"]) {
          PropertyMetadata l_Metadata;
          l_Metadata.name = N(name);
          l_Metadata.friendlyName = prettify_name(l_Metadata.name);
          l_Metadata.editor = false;
          l_Metadata.enumType = false;
          l_Metadata.getterName = "get_name";
          l_Metadata.setterName = "set_name";
          if (p_Node["name_editable"]) {
            l_Metadata.editor = true;
          }
          l_Metadata.propInfo =
              p_Metadata.typeInfo.properties[l_Metadata.name];

          p_Metadata.properties.push_back(l_Metadata);
        }
        for (auto it = p_Node[l_PropertiesName].begin();
             it != p_Node[l_PropertiesName].end(); ++it) {
          PropertyMetadata i_Metadata;
          i_Metadata.name = LOW_YAML_AS_NAME(it->first);
          i_Metadata.friendlyName = prettify_name(i_Metadata.name);
          i_Metadata.editor = false;
          if (it->second["editor_editable"]) {
            i_Metadata.editor =
                it->second["editor_editable"].as<bool>();
          }
          i_Metadata.scriptingExpose = false;
          if (it->second["expose_scripting"]) {
            i_Metadata.scriptingExpose =
                it->second["expose_scripting"].as<bool>();
          }
          i_Metadata.enumType = false;
          if (it->second["enum"]) {
            i_Metadata.enumType = it->second["enum"].as<bool>();
          }
          i_Metadata.propInfo =
              p_Metadata.typeInfo.properties[i_Metadata.name];

          {
            i_Metadata.multiline = false;
          }
          if (it->second["metadata"]) {
            parse_property_metadata(i_Metadata,
                                    it->second["metadata"]);
          }

          i_Metadata.getterName = "get_";
          i_Metadata.getterName += i_Metadata.name.c_str();
          if (it->second["getter_name"]) {
            i_Metadata.getterName =
                LOW_YAML_AS_STRING(it->second["getter_name"]);
          }
          i_Metadata.setterName = "set_";
          i_Metadata.setterName += i_Metadata.name.c_str();
          if (it->second["setter_name"]) {
            i_Metadata.setterName =
                LOW_YAML_AS_STRING(it->second["setter_name"]);
          }

          p_Metadata.properties.push_back(i_Metadata);
        }
      }
      {
        const char *l_FunctionsName = "functions";
        const char *l_ParametersName = "parameters";

        if (p_Node[l_FunctionsName]) {
          for (auto it = p_Node[l_FunctionsName].begin();
               it != p_Node[l_FunctionsName].end(); ++it) {
            FunctionMetadata i_Func;
            i_Func.name = LOW_YAML_AS_NAME(it->first);
            i_Func.friendlyName = prettify_name(i_Func.name);
            i_Func.functionInfo =
                p_Metadata.typeInfo.functions[i_Func.name];

            i_Func.scriptingExpose = false;
            if (it->second["expose_scripting"]) {
              i_Func.scriptingExpose =
                  it->second["expose_scripting"].as<bool>();
            }

            i_Func.hideFlode = false;
            if (it->second["flode_hide"]) {
              i_Func.hideFlode = it->second["flode_hide"].as<bool>();
            }

            i_Func.isStatic = false;
            if (it->second["static"]) {
              i_Func.isStatic = it->second["static"].as<bool>();
            }

            i_Func.hasReturnValue = i_Func.functionInfo.type !=
                                    Util::RTTI::PropertyType::VOID;

            if (it->second[l_ParametersName]) {
              int i = 0;
              for (auto pit = it->second[l_ParametersName].begin();
                   pit != it->second[l_ParametersName].end(); ++pit) {
                Util::Yaml::Node &i_ParamNode = *pit;

                ParameterMetadata i_Param;
                i_Param.name = LOW_YAML_AS_NAME(i_ParamNode["name"]);
                i_Param.friendlyName = prettify_name(i_Param.name);
                i_Param.paramInfo = i_Func.functionInfo.parameters[i];

                i_Func.parameters.push_back(i_Param);

                i++;
              }
            }

            p_Metadata.functions.push_back(i_Func);
          }
        }
      }
    }

    static inline void parse_metadata(Util::Yaml::Node &p_Node,
                                      Util::Yaml::Node &p_TypeIdsNode)
    {
      Util::String l_ModuleString =
          LOW_YAML_AS_STRING(p_Node["module"]);

      Util::String l_NamespaceString;
      Util::List<Util::String> l_Namespaces;
      int i = 0;
      for (auto it = p_Node["namespace"].begin();
           it != p_Node["namespace"].end(); ++it) {
        Util::Yaml::Node &i_Node = *it;

        Util::String i_Namespace = LOW_YAML_AS_STRING(i_Node);
        l_Namespaces.push_back(i_Namespace);

        if (i) {
          l_NamespaceString += "::";
        }
        l_NamespaceString += i_Namespace;
        i++;
      }

      for (auto it = p_Node["types"].begin();
           it != p_Node["types"].end(); ++it) {
        Util::String i_TypeName = LOW_YAML_AS_STRING(it->first);
        TypeMetadata i_Metadata;
        i_Metadata.name = LOW_YAML_AS_NAME(it->first);
        i_Metadata.friendlyName = prettify_name(i_Metadata.name);
        i_Metadata.module = l_ModuleString;
        i_Metadata.typeId =
            p_TypeIdsNode[l_ModuleString.c_str()][i_TypeName.c_str()]
                .as<uint16_t>();

        i_Metadata.namespaces = l_Namespaces;
        i_Metadata.namespaceString = l_NamespaceString;
        {
          // Construct full name out of namespace path + name of the
          // type. If there are namespaces we need to add an ::
          // between the namespaces and the name of the type
          i_Metadata.fullTypeString = l_NamespaceString;
          if (!i_Metadata.fullTypeString.empty()) {
            i_Metadata.fullTypeString += "::";
          }
          i_Metadata.fullTypeString += i_Metadata.name.c_str();
        }

        i_Metadata.scriptingExpose = false;
        if (it->second["scripting_expose"]) {
          i_Metadata.scriptingExpose =
              it->second["scripting_expose"].as<bool>();
        }

        parse_type_metadata(i_Metadata, it->second);

        g_TypeMetadata[i_Metadata.typeId] = i_Metadata;
      }
    }

    static inline void
    parse_enum_metadata(Util::Yaml::Node &p_Node,
                        Util::Yaml::Node &p_EnumIdsNode)
    {
      Util::String l_ModuleString =
          LOW_YAML_AS_STRING(p_Node["module"]);

      Util::String l_NamespaceString;
      Util::List<Util::String> l_Namespaces;
      int i = 0;
      for (auto it = p_Node["namespace"].begin();
           it != p_Node["namespace"].end(); ++it) {
        Util::Yaml::Node &i_Node = *it;

        Util::String i_Namespace = LOW_YAML_AS_STRING(i_Node);
        l_Namespaces.push_back(i_Namespace);

        if (i) {
          l_NamespaceString += "::";
        }
        l_NamespaceString += i_Namespace;
        i++;
      }

      for (auto it = p_Node["enums"].begin();
           it != p_Node["enums"].end(); ++it) {
        Util::String i_EnumName = LOW_YAML_AS_STRING(it->first);
        EnumMetadata i_Metadata;
        i_Metadata.name = LOW_YAML_AS_NAME(it->first);
        i_Metadata.module = l_ModuleString;

        i_Metadata.enumId =
            p_EnumIdsNode[l_ModuleString.c_str()][i_EnumName.c_str()]
                .as<uint16_t>();

        i_Metadata.namespaces = l_Namespaces;
        i_Metadata.namespaceString = l_NamespaceString;
        {
          // Construct full name out of namespace path + name of the
          // type. If there are namespaces we need to add an ::
          // between the namespaces and the name of the type
          i_Metadata.fullTypeString = l_NamespaceString;
          if (!i_Metadata.fullTypeString.empty()) {
            i_Metadata.fullTypeString += "::";
          }
          i_Metadata.fullTypeString += i_Metadata.name.c_str();
        }

        for (auto oit = it->second["options"].begin();
             oit != it->second["options"].end(); ++oit) {
          Util::Yaml::Node &i_Node = *oit;

          EnumEntryMetadata i_Entry;
          i_Entry.name = LOW_YAML_AS_NAME(i_Node["name"]);

          i_Metadata.options.push_back(i_Entry);
        }

        g_EnumMetadata.push_back(i_Metadata);
      }
    }

    static inline void load_project_metadata()
    {
      Util::String l_TypeConfigPath =
          Util::get_project().dataPath + "/_internal/type_configs";

      Util::Yaml::Node l_TypeIdsNode = Util::Yaml::load_file(
          (l_TypeConfigPath + "/typeids.yaml").c_str());
      Util::Yaml::Node l_EnumIdsNode = Util::Yaml::load_file(
          (l_TypeConfigPath + "/enumids.yaml").c_str());

      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_TypeConfigPath.c_str(),
                                   l_FilePaths);

      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".types.yaml")) {
          continue;
        }

        Util::Yaml::Node i_Node = Util::Yaml::load_file(it->c_str());
        parse_metadata(i_Node, l_TypeIdsNode);
      }
      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".enums.yaml")) {
          continue;
        }

        Util::Yaml::Node i_Node = Util::Yaml::load_file(it->c_str());
        parse_enum_metadata(i_Node, l_EnumIdsNode);
      }
    }

    static inline void load_low_metadata()
    {
      Util::String l_TypeConfigPath =
          Util::get_project().engineDataPath + "/type_configs";

      Util::Yaml::Node l_TypeIdsNode = Util::Yaml::load_file(
          (l_TypeConfigPath + "/typeids.yaml").c_str());

      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_TypeConfigPath.c_str(),
                                   l_FilePaths);

      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".types.yaml")) {
          continue;
        }

        Util::Yaml::Node i_Node = Util::Yaml::load_file(it->c_str());
        parse_metadata(i_Node, l_TypeIdsNode);
      }
    }

    void initialize()
    {
      load_low_metadata();
      load_project_metadata();

      Util::String l_DataPath = Util::get_project().dataPath;

      g_DirectoryWatchers.flodeDirectory =
          Util::FileSystem::watch_directory(
              l_DataPath + "/assets/flode",
              [](Util::FileSystem::FileWatcher &p_FileWatcher) {
                return (Util::Handle)0;
              },
              g_DirectoryUpdateTimer);

      Flode::initialize();

      initialize_main_window();

      {
        Flode::MathNodes::register_nodes();
        Flode::SyntaxNodes::register_nodes();
        Flode::DebugNodes::register_nodes();
        Flode::CastNodes::register_nodes();
        Flode::BoolNodes::register_nodes();

        register_type_nodes();
      }

      load_user_settings();
    }

    void tick(float p_Delta, Util::EngineState p_State)
    {
      LOW_PROFILE_CPU("Editor", "TICK");

      render_main_window(p_Delta, p_State);

      tick_editor_jobs(p_Delta);
    }

    Util::String prettify_name(Util::String p_String)
    {
      Util::String l_String = p_String;
      if (Util::StringHelper::begins_with(l_String, "p_")) {
        l_String = l_String.substr(2);
      }
      l_String = Util::StringHelper::replace(p_String, '_', ' ');
      l_String[0] = toupper(l_String[0]);
      {
        Util::String l_Friendly;
        for (u32 i = 0; i < l_String.size(); ++i) {
          if (i) {
            if (islower(l_String[i - 1]) && !islower(l_String[i])) {
              l_Friendly += " ";
            }
          }

          l_Friendly += l_String[i];
        }
        l_String = l_Friendly;
      }
      return l_String;
    }

    Util::String prettify_name(Util::Name p_Name)
    {
      return prettify_name(Util::String(p_Name.c_str()));
    }

    DirectoryWatchers &get_directory_watchers()
    {
      return g_DirectoryWatchers;
    }

    static Core::Entity duplicate_entity(Core::Entity p_Entity)
    {
      Util::Yaml::Node l_Node;
      get_selected_entity().serialize(l_Node);
      l_Node.remove("unique_id");
      Util::String l_NameString = LOW_YAML_AS_STRING(l_Node["name"]);
      l_NameString += " Clone";
      l_Node["name"] = l_NameString.c_str();

      Util::Yaml::Node &l_ComponentsNode = l_Node["components"];
      for (auto it = l_ComponentsNode.begin();
           it != l_ComponentsNode.end(); ++it) {
        Util::Yaml::Node &i_ComponentNode = *it;
        i_ComponentNode["properties"].remove("unique_id");
      }

      return Core::Entity::deserialize(
                 l_Node, get_selected_entity().get_region())
          .get_id();
    }

    static Util::Handle duplicate_handle(Util::Handle p_Handle)
    {
      Util::Handle l_Handle = 0;

      if (p_Handle.get_type() == Core::Entity::TYPE_ID) {
        l_Handle = duplicate_entity(p_Handle.get_id());
      }

      return l_Handle;
    }

    void duplicate(Util::List<Util::Handle> p_Handles)
    {
      Transaction l_Transaction("Duplicate objects");

      Util::List<Util::Handle> l_NewHandles;
      for (Util::Handle i_Handle : p_Handles) {
        Util::Handle i_NewHandle = duplicate_handle(i_Handle);
        l_Transaction.add_operation(
            new CommonOperations::HandleCreateOperation(i_NewHandle));
        l_NewHandles.push_back(i_NewHandle);
      }

      if (l_NewHandles.size() == 1) {
        set_selected_handle(l_NewHandles[0]);
      }

      get_global_changelist().add_entry(l_Transaction);
    }

    bool is_editor_job_in_progress()
    {
      return !g_EditorJobQueue.empty() &&
             g_EditorJobQueue.front().submitted;
    }

    void register_widget(const char *p_Name, Widget *p_Widget,
                         bool p_DefaultOpen)
    {
      register_editor_widget(p_Name, p_Widget, p_DefaultOpen);
    }

    Util::String get_active_editor_job_name()
    {
      if (!is_editor_job_in_progress()) {
        return "";
      }
      return g_EditorJobQueue.front().title;
    }

    void open_flode_graph(Util::String p_Path)
    {
      get_flode_widget()->m_Editor->load(p_Path);
      ImGui::SetWindowFocus(ICON_LC_WORKFLOW " Flode");
    }

    void delete_file_if_exists(Low::Util::String p_Path)
    {
      if (Low::Util::FileIO::file_exists_sync(p_Path.c_str())) {
        Low::Util::FileIO::delete_sync(p_Path.c_str());
      }
    }

    void set_user_setting(Util::Name p_Name, Util::Variant p_Variant)
    {
      g_UserSettings[p_Name] = p_Variant;
      save_user_settings();
    }

    Util::Variant get_user_setting(Util::Name p_Name)
    {
      return g_UserSettings[p_Name];
    }

    void save_user_settings()
    {
      Util::Yaml::Node l_Config;
      l_Config["loaded_scene"] =
          Core::Scene::get_loaded_scene().get_name().c_str();

      Util::List<EditorWidget> &l_Widgets = get_editor_widgets();

      for (auto it = l_Widgets.begin(); it != l_Widgets.end(); ++it) {
        Util::Yaml::Node i_Widget;
        i_Widget["name"] = it->name;
        i_Widget["open"] = it->open;

        l_Config["widgets"].push_back(i_Widget);
      }
      l_Config["theme"] = theme_get_current_name().c_str();

      Util::Yaml::Node l_CustomSettings;

      for (auto it = g_UserSettings.begin();
           it != g_UserSettings.end(); ++it) {
        const char *i_Name = it->first.c_str();
        Util::Yaml::Node i_Node;
        Util::Serialization::serialize_variant(i_Node, it->second);
        l_CustomSettings[i_Name] = i_Node;
      }

      l_Config["custom"] = l_CustomSettings;

      Util::String l_Path =
          Util::get_project().rootPath + "/user.yaml";
      Util::Yaml::write_file(l_Path.c_str(), l_Config);
    }

    namespace History {
      Transaction create_handle_transaction(Util::Handle p_Handle)
      {
        Transaction l_Transaction("Create objects");

        l_Transaction.add_operation(
            new CommonOperations::HandleCreateOperation(p_Handle));

        return l_Transaction;
      }

      Transaction destroy_handle_transaction(Util::Handle p_Handle)
      {
        Transaction l_Transaction("Delete objects");

        l_Transaction.add_operation(
            new CommonOperations::HandleDestroyOperation(p_Handle));

        return l_Transaction;
      }
    } // namespace History

  } // namespace Editor
} // namespace Low
