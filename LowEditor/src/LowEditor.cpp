#include "LowEditor.h"

#include "LowRendererEditorImage.h"
#include "LowRendererResourceImporter.h"
#include "LowRendererResourceManager.h"

#include "LowUtilHandle.h"
#include "LowUtilJobManager.h"
#include "LowUtil.h"
#include "LowUtilProfiler.h"
#include "LowUtilFileIO.h"
#include "LowUtilLogger.h"
#include "LowUtilFileSystem.h"
#include "LowUtilSerialization.h"

#include "LowCorePrefabInstance.h"
#include "LowCoreUiWidgetAsset.h"

#include "LowEditorMainWindow.h"
#include "LowEditorThemes.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorFlodeWidget.h"
#include "LowEditorIcons.h"
#include "LowEditorNotifications.h"
#include "LowEditorFonts.h"
#include "LowEditorScriptWidget.h"
#include "LowEditorSimpleAssetEditors.h"
#include "LowEditorUiWidgetEditor.h"

#include "Flode.h"
#include "FlodeEditor.h"
#include "FlodeMathNodes.h"
#include "FLodeSyntaxNodes.h"
#include "FlodeDebugNodes.h"
#include "FlodeCastNodes.h"
#include "FlodeHandleNodes.h"
#include "FlodeBoolNodes.h"
#include "FlodeOperatorNodes.h"
#include "imgui.h"

namespace Low {
  namespace Editor {
    float g_DirectoryUpdateTimer = 2.0f;
    DirectoryWatchers g_DirectoryWatchers;

    Util::FileSystem::Watcher g_RawAssetWatcher;

    ChangeList g_ChangeList;

    Util::Map<uint16_t, TypeMetadata> g_TypeMetadata;
    Util::List<EnumMetadata> g_EnumMetadata;

    Util::Map<Util::Name, Util::Variant> g_UserSettings;

    Util::Map<AssetType, Math::Color> g_AssetTypeColor;
    Util::Map<AssetType, Util::String> g_AssetTypeName;
    Util::Map<AssetType, Renderer::EditorImage>
        g_AssetTypeEditorImage;
    ;

    Util::List<Util::Handle> g_SelectedHandles;

    Math::Color get_color_for_asset_type(const AssetType p_AssetType)
    {
      auto l_Pos = g_AssetTypeColor.find(p_AssetType);
      if (l_Pos == g_AssetTypeColor.end()) {
        return g_AssetTypeColor[AssetType::File];
      }

      return l_Pos->second;
    }
    Util::String get_asset_type_name(const AssetType p_AssetType)
    {
      auto l_Pos = g_AssetTypeName.find(p_AssetType);
      if (l_Pos == g_AssetTypeName.end()) {
        return g_AssetTypeName[AssetType::File];
      }

      return l_Pos->second;
    }
    Renderer::EditorImage
    get_editor_image_for_asset_type(const AssetType p_AssetType)
    {
      auto l_Pos = g_AssetTypeEditorImage.find(p_AssetType);
      if (l_Pos == g_AssetTypeEditorImage.end()) {
        return g_AssetTypeEditorImage[AssetType::File];
      }

      return l_Pos->second;
    }

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
      auto l_Pos = g_TypeMetadata.find(p_TypeId);
      LOW_ASSERT(l_Pos != g_TypeMetadata.end(),
                 "Could not find type metadata");
      return l_Pos->second;
    }

    EnumMetadata &get_enum_metadata(u16 p_EnumId)
    {
      for (EnumMetadata &i_Metadata : g_EnumMetadata) {
        if (i_Metadata.enumId == p_EnumId) {
          return i_Metadata;
        }
      }

      LOW_ASSERT(false, "Could not find enum metadata");
      return g_EnumMetadata[0];
    }

    EnumMetadata &get_enum_metadata(Util::Name p_EnumTypeName)
    {
      for (EnumMetadata &i_Metadata : g_EnumMetadata) {
        if (i_Metadata.name == p_EnumTypeName) {
          return i_Metadata;
        }
      }

      LOW_ASSERT(false, "Could not find enum metadata");
      return g_EnumMetadata[0];
    }

    Util::Map<u16, TypeMetadata> &get_type_metadata()
    {
      return g_TypeMetadata;
    }

    bool is_selected(Util::Handle p_Handle)
    {
      for (Util::Handle i_Handle : g_SelectedHandles) {
        if (i_Handle.get_id() == p_Handle.get_id()) {
          return true;
        }
      }

      return false;
    }

    const Util::List<Util::Handle> &get_selected_handles()
    {
      return g_SelectedHandles;
    }

    bool is_entity_selected(Core::Entity p_Entity)
    {
      return is_selected(p_Entity.get_id());
    }

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func)
    {
      g_EditorJobQueue.emplace(p_Title, p_Func);
    }

    static void update_details_widget()
    {
      if (g_SelectedHandles.empty()) {
        get_details_widget()->clear();
        return;
      }
      if (g_SelectedHandles.size() > 1) {
        get_details_widget()->clear();
        return;
      }

      Util::Handle l_Handle = g_SelectedHandles[0];

      if (l_Handle == get_details_widget()->m_DisplayedHandle) {
        return;
      }
      get_details_widget()->clear();
      get_details_widget()->m_DisplayedHandle = l_Handle;

      if (Util::Handle::is_registered_type(l_Handle.get_type())) {
        Util::RTTI::TypeInfo &l_Info =
            Util::Handle::get_type_info(l_Handle.get_type());
      }

      get_details_widget()->clear();

      {
        Core::UI::Element l_Element = l_Handle.get_id();
        if (l_Element.is_alive()) {
          for (auto it = l_Element.get_components().begin();
               it != l_Element.get_components().end(); ++it) {
            get_details_widget()->add_section(it->second);
          }
        }
      }

      Core::Entity l_Entity = l_Handle.get_id();

      if (l_Entity.is_alive()) {
        {
          HandlePropertiesSection l_Section(l_Entity, true);
          l_Section.render_footer = nullptr;
          get_details_widget()->add_section(l_Section);
        }

        for (auto it = l_Entity.get_components().begin();
             it != l_Entity.get_components().end(); ++it) {
          if (it->first ==
              Core::Component::PrefabInstance::type_id()) {
            continue;
          }
          get_details_widget()->add_section(it->second);
        }
      }
    }

    void deselect_everything()
    {
      g_SelectedHandles.clear();
      update_details_widget();
    }

    void set_selected_handle(Util::Handle p_Handle)
    {
      deselect_everything();

      if (!p_Handle.is_registered_type()) {
        return;
      }
      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_Handle.get_type());
      if (l_TypeInfo.is_alive(p_Handle)) {
        g_SelectedHandles.push_back(p_Handle);
      }

      update_details_widget();
    }

    void set_selected_entity(Core::Entity p_Entity)
    {
      if (!p_Entity.is_alive()) {
        deselect_everything();
        return;
      }
      set_selected_handle(p_Entity);
    }

    void load_user_settings()
    {
      Util::String l_Path =
          Util::get_project().rootPath + "/user.yaml";

      if (!Util::FileIO::file_exists_sync(l_Path.c_str())) {
        return;
      }

      Util::Serial::Node l_Root =
          Util::Serial::load_yaml_file(l_Path.c_str());

      Low::Core::Scene l_Scene =
          Low::Core::Scene::find_by_name(N(TestScene));
      if (l_Root["loaded_scene"]) {
        Core::Scene l_LocalScene = Core::Scene::find_by_name(
            l_Root["loaded_scene"].as<Util::Name>());
        if (l_LocalScene.is_alive()) {
          l_Scene = l_LocalScene;
        }
      }

      if (!l_Scene.is_alive()) {
        l_Scene = Core::Scene::make(N(Default Scene));
      }

      l_Scene.load();

      if (l_Root["widgets"]) {
        for (uint32_t i = 0; i < l_Root["widgets"].size(); ++i) {
          if (l_Root["widgets"][i]["path"]) {
            set_widget_open(
                l_Root["widgets"][i]["path"].as<Util::String>(),
                l_Root["widgets"][i]["open"].as<bool>());
          }
        }
      }

      if (l_Root["theme"]) {
        theme_apply(l_Root["theme"].as<Util::Name>());
      } else if (theme_exists(N(dracula))) {
        theme_apply(N(dracula));
      }

      if (l_Root["custom"]) {
        for (auto [i_Key, i_Value] : l_Root["custom"]) {
          g_UserSettings[LOW_NAME(i_Key->c_str())] =
              Util::Serial::deserialize_variant(i_Value);
        }
      }

      g_UserSettings[N(theme)] = theme_get_current_name();
      g_UserSettings[N(loaded_scene)] = l_Scene.get_name();

      /*
      LOW_LOG_DEBUG << "Debug" << LOW_LOG_END;
      LOW_LOG_INFO << "Info" << LOW_LOG_END;
      LOW_LOG_WARN << "Warning" << LOW_LOG_END;
      LOW_LOG_ERROR << "Error" << LOW_LOG_END;
      LOW_LOG_PROFILE << "Profile" << LOW_LOG_END;
      */
    }

    static void register_type_nodes()
    {
      for (auto it = get_type_metadata().begin();
           it != get_type_metadata().end(); ++it) {
        if (it->second.scriptingExpose) {
          // Flode::register_nodes_for_type(it->second.typeId);
        }
      }
    }

    ChangeList &get_global_changelist()
    {
      return g_ChangeList;
    }

    static void
    parse_editor_type_metadata(TypeEditorMetadata &p_Metadata,
                               Util::Serial::Node p_Node)
    {
      p_Metadata.manager = false;
      if (p_Node["manager"]) {
        p_Metadata.manager = p_Node["manager"].as<bool>();
      }
      if (p_Node["saveable"]) {
        p_Metadata.saveable = p_Node["saveable"].as<bool>();
      }
      p_Metadata.hasIcon = false;
      if (p_Node["icon"]) {
        p_Metadata.hasIcon = true;
        p_Metadata.iconName = p_Node["icon"].as<Util::Name>();
        p_Metadata.icon = get_icon_by_name(p_Metadata.iconName);
      }
      p_Metadata.managerWidgetPath = "";
      if (p_Node["manager_widget_path"]) {
        p_Metadata.managerWidgetPath =
            p_Node["manager_widget_path"].as<Util::String>();
      }
    }

    static void parse_property_metadata(PropertyMetadata &p_Metadata,
                                        Util::Serial::Node &p_Node)
    {
      if (p_Node["multiline"]) {
        p_Metadata.multiline = p_Node["multiline"].as<bool>();
      }
    }

    static void parse_type_metadata(TypeMetadata &p_Metadata,
                                    Util::Serial::Node &p_Node)
    {
      if (Util::Handle::is_registered_type(p_Metadata.identifier)) {
        p_Metadata.hasTypeInfo = true;
        p_Metadata.typeInfo =
            Util::Handle::get_type_info(p_Metadata.typeId);
      } else {
        p_Metadata.hasTypeInfo = false;
        LOW_LOG_WARN << "Could not find type info for type "
                     << p_Metadata.name << " ("
                     << (Util::String)p_Metadata.identifier << ")"
                     << LOW_LOG_END;
      }

      {
        // Initializing the editor part of the metadata
        p_Metadata.editor.manager = false;
        p_Metadata.editor.saveable = false;
        p_Metadata.editor.hasIcon = false;
      }

      if (p_Node["editor"]) {
        const char *t = p_Metadata.name.c_str();
        parse_editor_type_metadata(p_Metadata.editor,
                                   p_Node["editor"]);
      }

      const char *l_PropertiesName = "properties";
      const char *l_VirtualPropertiesName = "virtual_properties";

      if (p_Node[l_PropertiesName]) {
        if (!p_Node["component"]) {
          PropertyMetadata l_Metadata;
          l_Metadata.name = N(name);
          l_Metadata.friendlyName = prettify_name(l_Metadata.name);
          l_Metadata.editor = false;
          l_Metadata.scriptingExpose = true;
          l_Metadata.hideFlode = false;
          l_Metadata.hideGetterFlode = false;
          l_Metadata.hideSetterFlode = false;
          l_Metadata.enumType = false;
          l_Metadata.getterName = "get_name";
          l_Metadata.setterName = "set_name";
          if (p_Node["name_editable"]) {
            l_Metadata.editor = true;
          }
          l_Metadata.propInfo =
              p_Metadata.typeInfo.properties[l_Metadata.name];
          l_Metadata.propInfoBase = l_Metadata.propInfo;

          p_Metadata.properties.push_back(l_Metadata);
        }
        if (p_Metadata.typeInfo.component &&
            !p_Node[l_PropertiesName]["entity"] &&
            p_Metadata.typeInfo.properties.find(N(entity)) !=
                p_Metadata.typeInfo.properties.end()) {
          PropertyMetadata l_Metadata;
          l_Metadata.name = N(entity);
          l_Metadata.friendlyName = prettify_name(l_Metadata.name);
          l_Metadata.editor = false;
          l_Metadata.scriptingExpose = true;
          l_Metadata.hideFlode = false;
          l_Metadata.hideGetterFlode = false;
          l_Metadata.hideSetterFlode = false;
          l_Metadata.enumType = false;
          l_Metadata.getterName = "get_entity";
          l_Metadata.setterName = "set_entity";
          l_Metadata.propInfo =
              p_Metadata.typeInfo.properties[l_Metadata.name];
          l_Metadata.propInfoBase = l_Metadata.propInfo;

          p_Metadata.properties.push_back(l_Metadata);
        }
        for (auto [i_PropName, i_Prop] : p_Node[l_PropertiesName]) {
          PropertyMetadata i_Metadata;
          i_Metadata.name = LOW_NAME(i_PropName->c_str());
          i_Metadata.friendlyName = prettify_name(i_Metadata.name);
          i_Metadata.editor = false;
          if (i_Prop["editor_editable"]) {
            i_Metadata.editor = i_Prop["editor_editable"].as<bool>();
          }
          i_Metadata.scriptingExpose = false;
          if (i_Prop["expose_scripting"]) {
            i_Metadata.scriptingExpose =
                i_Prop["expose_scripting"].as<bool>();
          }
          i_Metadata.enumType = false;
          if (i_Prop["enum"]) {
            i_Metadata.enumType = i_Prop["enum"].as<bool>();
          }
          i_Metadata.propInfo =
              p_Metadata.typeInfo.properties[i_Metadata.name];

          {
            i_Metadata.multiline = false;
          }
          if (i_Prop["metadata"]) {
            parse_property_metadata(i_Metadata, i_Prop["metadata"]);
          }

          {
            i_Metadata.hideFlode = false;

            if (i_Prop["flode_hide"]) {
              i_Metadata.hideFlode = i_Prop["flode_hide"].as<bool>();
            }
            i_Metadata.hideGetterFlode = i_Metadata.hideFlode;
            i_Metadata.hideSetterFlode = i_Metadata.hideFlode;

            if (!i_Metadata.hideFlode) {
              if (i_Prop["flode_hide_setter"]) {
                i_Metadata.hideSetterFlode =
                    i_Prop["flode_hide_setter"].as<bool>();
              }
              if (i_Prop["flode_hide_getter"]) {
                i_Metadata.hideGetterFlode =
                    i_Prop["flode_hide_getter"].as<bool>();
              }
            }
          }

          i_Metadata.getterName = "get_";
          i_Metadata.getterName += i_Metadata.name.c_str();
          if (i_Prop["getter_name"]) {
            i_Metadata.getterName =
                i_Prop["getter_name"].as<Util::String>();
          }
          i_Metadata.setterName = "set_";
          i_Metadata.setterName += i_Metadata.name.c_str();
          if (i_Prop["setter_name"]) {
            i_Metadata.setterName =
                i_Prop["setter_name"].as<Util::String>();
          }

          i_Metadata.propInfoBase = i_Metadata.propInfo;

          p_Metadata.properties.push_back(i_Metadata);
        }
      }

      if (p_Node[l_VirtualPropertiesName]) {
        for (auto [i_VirtPropName, i_VirtProp] :
             p_Node[l_VirtualPropertiesName]) {
          VirtualPropertyMetadata i_Metadata;
          i_Metadata.name = LOW_NAME(i_VirtPropName->c_str());
          i_Metadata.friendlyName = prettify_name(i_Metadata.name);
          i_Metadata.editor = false;
          if (i_VirtProp["editor_editable"]) {
            i_Metadata.editor =
                i_VirtProp["editor_editable"].as<bool>();
          }
          i_Metadata.scriptingExpose = false;
          if (i_VirtProp["expose_scripting"]) {
            i_Metadata.scriptingExpose =
                i_VirtProp["expose_scripting"].as<bool>();
          }
          i_Metadata.enumType = false;
          if (i_VirtProp["enum"]) {
            i_Metadata.enumType = i_VirtProp["enum"].as<bool>();
          }
          i_Metadata.virtPropInfo =
              p_Metadata.typeInfo.virtualProperties[i_Metadata.name];

          {
            i_Metadata.multiline = false;
          }
          /*
          if (it->second["metadata"]) {
            parse_property_metadata(i_Metadata,
                                    it->second["metadata"]);
          }
          */

          i_Metadata.getterName = "get_";
          i_Metadata.getterName += i_Metadata.name.c_str();
          if (i_VirtProp["getter_name"]) {
            i_Metadata.getterName =
                i_VirtProp["getter_name"].as<Util::String>();
          }
          i_Metadata.setterName = "set_";
          i_Metadata.setterName += i_Metadata.name.c_str();
          if (i_VirtProp["setter_name"]) {
            i_Metadata.setterName =
                i_VirtProp["setter_name"].as<Util::String>();
          }

          i_Metadata.propInfoBase = i_Metadata.virtPropInfo;

          p_Metadata.virtualProperties.push_back(i_Metadata);
        }
      }
      {
        const char *l_FunctionsName = "functions";
        const char *l_ParametersName = "parameters";

        if (p_Node[l_FunctionsName]) {
          for (auto [i_FuncName, i_FuncNode] :
               p_Node[l_FunctionsName]) {
            FunctionMetadata i_Func;
            i_Func.name = LOW_NAME(i_FuncName->c_str());
            i_Func.friendlyName = prettify_name(i_Func.name);
            i_Func.functionInfo =
                p_Metadata.typeInfo.functions[i_Func.name];

            i_Func.scriptingExpose = false;
            if (i_FuncNode["expose_scripting"]) {
              i_Func.scriptingExpose =
                  i_FuncNode["expose_scripting"].as<bool>();
            }

            i_Func.hideFlode = false;
            if (i_FuncNode["flode_hide"]) {
              i_Func.hideFlode = i_FuncNode["flode_hide"].as<bool>();
            }

            i_Func.isStatic = false;
            if (i_FuncNode["static"]) {
              i_Func.isStatic = i_FuncNode["static"].as<bool>();
            }

            i_Func.hasReturnValue = i_Func.functionInfo.type !=
                                    Util::RTTI::PropertyType::VOID;

            if (i_FuncNode[l_ParametersName]) {
              int i = 0;
              for (auto [_, i_ParamNode] :
                   i_FuncNode[l_ParametersName]) {
                ParameterMetadata i_Param;
                i_Param.name = i_ParamNode["name"].as<Util::Name>();
                i_Param.friendlyName = prettify_name(i_Param.name);

                if (p_Metadata.hasTypeInfo) {
                  i_Param.paramInfo =
                      i_Func.functionInfo.parameters[i];
                }

                i_Func.parameters.push_back(i_Param);

                i++;
              }
            }

            p_Metadata.functions.push_back(i_Func);
          }
        }
      }
    }

    static inline void parse_metadata(Util::Serial::Node &p_Node)
    {
      Util::String l_ModuleString =
          p_Node["module"].as<Util::String>();

      Util::String l_NamespaceString;
      Util::List<Util::String> l_Namespaces;
      int i = 0;
      for (auto [_, i_Node] : p_Node["namespace"]) {
        Util::String i_Namespace = i_Node.as<Util::String>();
        l_Namespaces.push_back(i_Namespace);

        if (i) {
          l_NamespaceString += "::";
        }
        l_NamespaceString += i_Namespace;
        i++;
      }

      for (auto [i_TypeName, i_TypeNode] : p_Node["types"]) {
        const Util::TypeIdentifier i_TypeIdentifier(
            LOW_NAME(l_ModuleString.c_str()),
            LOW_NAME(i_TypeName->c_str()));
        TypeMetadata i_Metadata(i_TypeIdentifier);

        i_Metadata.name = LOW_NAME(i_TypeName->c_str());

        i_Metadata.friendlyName = prettify_name(i_Metadata.name);
        i_Metadata.module = l_ModuleString;
        if (Util::Handle::is_registered_type(i_Metadata.identifier)) {
          i_Metadata.typeId =
              Util::Handle::type_id(i_Metadata.identifier);
        }

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

        i_Metadata.scriptingName = i_Metadata.name.c_str();
        if (i_TypeNode["scripting_name"]) {
          i_Metadata.scriptingName =
              i_TypeNode["scripting_name"].as<Util::String>();
        }

        i_Metadata.scriptingNamespace = "";
        if (i_TypeNode["scripting_namespace"]) {
          if (i_TypeNode["scripting_namespace"].is_scalar()) {
            i_Metadata.scriptingNamespace =
                i_TypeNode["scripting_namespace"].as<Util::String>();
          } else {
            bool l_First = true;
            for (auto [_, i_ScriptingNamespaceNode] :
                 i_TypeNode["scripting_namespace"]) {
              if (!l_First) {
                i_Metadata.scriptingNamespace += "::";
              }
              i_Metadata.scriptingNamespace +=
                  i_ScriptingNamespaceNode.as<Util::String>();
              l_First = false;
            }
          }
        }

        i_Metadata.fullScriptingTypeString = i_Metadata.scriptingName;
        if (!i_Metadata.scriptingNamespace.empty()) {
          i_Metadata.fullScriptingTypeString =
              i_Metadata.scriptingNamespace +
              "::" + i_Metadata.scriptingName;
        }

        i_Metadata.scriptingExpose = false;
        if (i_TypeNode["scripting_expose"]) {
          i_Metadata.scriptingExpose =
              i_TypeNode["scripting_expose"].as<bool>();
        }

        parse_type_metadata(i_Metadata, i_TypeNode);

        g_TypeMetadata.insert(
            eastl::make_pair(i_Metadata.typeId, i_Metadata));
      }
    }

    static inline void
    parse_enum_metadata(Util::Serial::Node &p_Node,
                        Util::Serial::Node &p_EnumIdsNode)
    {
      Util::String l_ModuleString =
          p_Node["module"].as<Util::String>();

      Util::String l_NamespaceString;
      Util::List<Util::String> l_Namespaces;
      int i = 0;
      for (auto [_, i_Node] : p_Node["namespace"]) {
        Util::String i_Namespace = i_Node.as<Util::String>();
        l_Namespaces.push_back(i_Namespace);

        if (i) {
          l_NamespaceString += "::";
        }
        l_NamespaceString += i_Namespace;
        i++;
      }

      for (auto [i_EnumName, i_Enum] : p_Node["enums"]) {
        EnumMetadata i_Metadata;
        i_Metadata.name = LOW_NAME(i_EnumName->c_str());
        i_Metadata.module = l_ModuleString;

        i_Metadata.enumId =
            p_EnumIdsNode[l_ModuleString.c_str()][*i_EnumName]
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

        for (auto [_, i_Node] : i_Enum["options"]) {
          EnumEntryMetadata i_Entry;
          i_Entry.name = i_Node["name"].as<Util::Name>();

          i_Metadata.options.push_back(i_Entry);
        }

        g_EnumMetadata.push_back(i_Metadata);
      }
    }

    static inline void load_project_metadata()
    {
      Util::String l_TypeConfigPath =
          Util::get_project().dataPath + "/_internal/type_configs";

      Util::Serial::Node l_EnumIdsNode = Util::Serial::load_yaml_file(
          (l_TypeConfigPath + "/enumids.yaml").c_str());

      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_TypeConfigPath.c_str(),
                                   l_FilePaths);

      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".types.yaml")) {
          continue;
        }

        Util::Serial::Node i_Node =
            Util::Serial::load_yaml_file(it->c_str());
        parse_metadata(i_Node);
      }
      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".enums.yaml")) {
          continue;
        }

        Util::Serial::Node i_Node =
            Util::Serial::load_yaml_file(it->c_str());
        parse_enum_metadata(i_Node, l_EnumIdsNode);
      }
    }

    static inline void load_low_metadata()
    {
      Util::String l_TypeConfigPath =
          Util::get_project().engineDataPath + "/type_configs";

      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_TypeConfigPath.c_str(),
                                   l_FilePaths);

      for (auto it = l_FilePaths.begin(); it != l_FilePaths.end();
           ++it) {
        if (!Util::StringHelper::ends_with(*it, ".types.yaml")) {
          continue;
        }

        Util::Serial::Node i_Node =
            Util::Serial::load_yaml_file(it->c_str());
        parse_metadata(i_Node);
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

      {
        Util::String l_BasePath =
            Util::get_project().engineDataPath + "\\fonts\\";
        // During engine/editor init:
        Fonts::set_paths({
            /* roboto_regular_ttf = */ l_BasePath +
                "Roboto-Regular.ttf",
            /* roboto_medium_ttf  = */
            l_BasePath + "Roboto-Medium.ttf",
            /* roboto_bold_ttf    = */
            l_BasePath + "Roboto-Bold.ttf",
            /* roboto_light_ttf    = */
            l_BasePath + "Roboto-Light.ttf",
            /* firacode_regular_ttf = */ l_BasePath +
                "FiraCode-Regular.ttf",
            /* firacode_medium_ttf  = */
            l_BasePath + "FiraCode-Medium.ttf",
            /* firacode_bold_ttf    = */
            l_BasePath + "FiraCode-Bold.ttf",
            /* firacode_light_ttf    = */
            l_BasePath + "FiraCode-Light.ttf",
            /* codicons_ttf       = */
            l_BasePath + "codicon.ttf",
            /* lucide_ttf         = */
            l_BasePath + "lucide.ttf", // if you have it
        });

        Fonts::set_preset_sizes(
            {12, 14, 17, 19, 25, 28, 33, 40}); // whatever you like
                                               //
        float l_Dpi = ImGui::GetMainViewport()->DpiScale;

        Fonts::initialize(l_Dpi);

        ImGui::GetIO().FontDefault = Fonts::UI(17);
      }

      Flode::initialize();

      initialize_main_window();

      // Load editor images
      {
        Renderer::ResourceManager::load_editor_image(
            Renderer::EditorImage::find_by_name(N(point_light)));
      }

      {
        g_AssetTypeColor[AssetType::Script] =
            color_from_hex("#41bf5c");
        g_AssetTypeColor[AssetType::Material] =
            color_from_hex("#7c40c1");
        g_AssetTypeColor[AssetType::Texture] =
            color_from_hex("#bf5641");
        g_AssetTypeColor[AssetType::Font] = color_from_hex("#b744ac");
        g_AssetTypeColor[AssetType::Mesh] = color_from_hex("#baa343");
        g_AssetTypeColor[AssetType::Flode] =
            color_from_hex("#424ebc");
        g_AssetTypeColor[AssetType::File] = color_from_hex("#b2b2b2");
        g_AssetTypeColor[AssetType::UiWidget] =
            color_from_hex("#4bb3bb");

        g_AssetTypeName[AssetType::File] = "File";
        g_AssetTypeName[AssetType::Texture] = "Texture";
        g_AssetTypeName[AssetType::Material] = "Material";
        g_AssetTypeName[AssetType::Script] = "Script";
        g_AssetTypeName[AssetType::Font] = "Font";
        g_AssetTypeName[AssetType::Flode] = "Flode";
        g_AssetTypeName[AssetType::Mesh] = "Mesh";
        g_AssetTypeName[AssetType::Model] = "Model";
        g_AssetTypeName[AssetType::UiWidget] = "UI-Widget";

        g_AssetTypeEditorImage[AssetType::File] =
            Renderer::EditorImage::find_by_name(N(filetype_file));
        g_AssetTypeEditorImage[AssetType::Texture] =
            Renderer::EditorImage::find_by_name(N(filetype_texture));
        g_AssetTypeEditorImage[AssetType::Mesh] =
            Renderer::EditorImage::find_by_name(N(filetype_mesh));
        g_AssetTypeEditorImage[AssetType::Material] =
            Renderer::EditorImage::find_by_name(N(filetype_material));
        g_AssetTypeEditorImage[AssetType::Script] =
            Renderer::EditorImage::find_by_name(N(filetype_script));
        g_AssetTypeEditorImage[AssetType::UiWidget] =
            Renderer::EditorImage::find_by_name(N(filetype_uiwidget));
        g_AssetTypeEditorImage[AssetType::Font] =
            Renderer::EditorImage::find_by_name(N(filetype_font));

        // Load all of them
        for (auto it = g_AssetTypeEditorImage.begin();
             it != g_AssetTypeEditorImage.end(); ++it) {
          Renderer::ResourceManager::load_editor_image(it->second);
        }
      }

      {
        Flode::MathNodes::register_nodes();
        Flode::SyntaxNodes::register_nodes();
        Flode::DebugNodes::register_nodes();
        Flode::CastNodes::register_nodes();
        Flode::BoolNodes::register_nodes();
        Flode::OperatorNodes::register_nodes();

        register_type_nodes();
      }

      {
        LOW_ASSERT(
            g_RawAssetWatcher.start(Util::get_project().dataPath),
            "Failed to start raw asset file watcher.");
      }

      {
        TypeEditor::register_type<MeshAssetEditor>(
            Renderer::Mesh::type_id());
        TypeEditor::register_type<TextureAssetEditor>(
            Renderer::Texture::type_id());
        TypeEditor::register_type<FontAssetEditor>(
            Renderer::Font::type_id());
        TypeEditor::register_type<UiWidgetEditor>(
            Core::UI::WidgetAsset::type_id());
      }
    }

    void cleanup()
    {
      {
        g_RawAssetWatcher.stop();
      }
    }

    void tick(float p_Delta, Util::EngineState p_State)
    {
      LOW_PROFILE_CPU("Editor", "TICK");

      static bool t = false;
      if (!t) {
        t = true;
        Flode::tmp_build_mapping();
      }

      render_main_window(p_Delta, p_State);

      tick_editor_jobs(p_Delta);

      render_notifications(p_Delta);

      {
        auto l_Events = g_RawAssetWatcher.poll();

        for (auto &i_Event : l_Events) {
          if (i_Event.type ==
              Util::FileSystem::Watcher::EventType::Overflow) {
            continue;
          }
          continue;

          const bool i_Png = (i_Event.path.extension() == ".png");

          if (i_Png) {
            const Util::String l_Path = Util::get_project().dataPath +
                                        "/" +
                                        i_Event.path.string().c_str();
            const Util::String l_Output =
                i_Event.path.replace_extension("").string().c_str();
            if (!Renderer::ResourceImporter::import_texture(
                    l_Path, l_Output)) {
              LOW_LOG_ERROR << "Failed to import texture."
                            << LOW_LOG_END;
              continue;
            }
          }
        }
      }
    }

    Util::String prettify_name(Util::String p_String)
    {
      return Util::StringHelper::prettify_name(p_String);
    }

    Util::String prettify_name(Util::Name p_Name)
    {
      return prettify_name(Util::String(p_Name.c_str()));
    }

    Util::String technify_string(Util::String p_String)
    {
      return Util::StringHelper::technify_string(p_String);
    }

    DirectoryWatchers &get_directory_watchers()
    {
      return g_DirectoryWatchers;
    }

    static Core::Entity duplicate_entity(Core::Entity p_Entity)
    {
      Util::Serial::Node l_Node;
      p_Entity.serialize(l_Node);
      l_Node.remove("unique_id");
      Util::String l_NameString = l_Node["name"].as<Util::String>();
      l_NameString += " Clone";
      l_Node["name"] = l_NameString.c_str();

      Util::Serial::Node l_ComponentsNode = l_Node["components"];
      for (auto [_, i_ComponentNode] : l_ComponentsNode) {
        i_ComponentNode["properties"].remove("unique_id");
      }

      return Core::Entity::deserialize(l_Node, p_Entity.get_region())
          .get_id();
    }

    static Util::Handle duplicate_handle(Util::Handle p_Handle)
    {
      Util::Handle l_Handle = 0;

      if (p_Handle.get_type() == Core::Entity::type_id()) {
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

    void register_widget(Low::Util::String p_Path, Widget *p_Widget,
                         bool p_DefaultOpen)
    {
      register_editor_widget(p_Path, p_Widget, p_DefaultOpen);
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

    void open_file_in_code_editor(Util::String p_Path)
    {
      get_script_widget()->show(0.0f);
      get_script_widget()->load_file(p_Path);
      open_editor_widget(get_script_widget());
      ImGui::SetWindowFocus(ICON_LC_BRACES " Code Editor");
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
      Util::Serial::Node l_Config;
      l_Config["loaded_scene"] =
          Core::Scene::get_loaded_scene().get_name().c_str();

      Util::Map<Util::String, EditorWidget> &l_Widgets =
          get_editor_widgets();

      for (auto it = l_Widgets.begin(); it != l_Widgets.end(); ++it) {
        Util::Serial::Node i_Widget;
        i_Widget["path"] = it->first.c_str();
        i_Widget["open"] = it->second.open;

        l_Config["widgets"].push_back(i_Widget);
      }
      l_Config["theme"] = theme_get_current_name().c_str();

      Util::Serial::Node l_CustomSettings;

      for (auto it = g_UserSettings.begin();
           it != g_UserSettings.end(); ++it) {
        const char *i_Name = it->first.c_str();
        Util::Serial::Node i_Node;
        Util::Serial::serialize_variant(i_Node, it->second);
        l_CustomSettings[i_Name] = i_Node;
      }

      l_Config["custom"] = l_CustomSettings;

      Util::String l_Path =
          Util::get_project().rootPath + "/user.yaml";
      Util::Serial::write_yaml_file(l_Path.c_str(), l_Config);
    }

    void set_focused_widget(Widget *p_Widget)
    {
      _set_focused_widget(p_Widget);
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
