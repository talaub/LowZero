#include "LowEditorMainWindow.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "IconsFontAwesome5.h"

#include "LowEditorLogWidget.h"
#include "LowEditorRenderFlowWidget.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorProfilerWidget.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorAssetWidget.h"
#include "LowEditorSceneWidget.h"
#include "LowEditorChangeWidget.h"
#include "LowEditorRegionWidget.h"
#include "LowEditorUiWidget.h"
#include "LowEditorFlodeWidget.h"
#include "LowEditorGui.h"
#include "LowEditorResourceProcessorImage.h"
#include "LowEditorResourceProcessorMesh.h"
#include "LowEditorSaveHelper.h"
#include "LowEditorMetadata.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorResourceWidget.h"
#include "LowEditorThemes.h"
#include "LowEditorTypeManagerWidget.h"

#include "LowUtil.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileIO.h"
#include "LowUtilProfiler.h"

#include "LowRendererTexture2D.h"
#include "LowRendererImGuiHelper.h"

#include "LowCoreMeshResource.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreRigidbody.h"
#include "LowCorePrefabInstance.h"
#include "LowCorePhysicsSystem.h"
#include "LowCoreMeshAsset.h"
#include "LowCoreMaterial.h"

#include "Flode.h"
#include "FlodeMathNodes.h"
#include "FlodeSyntaxNodes.h"
#include "FlodeDebugNodes.h"
#include "FlodeCastNodes.h"
#include "FlodeHandleNodes.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <future>

void *operator new[](size_t size, const char *pName, int flags,
                     unsigned debugFlags, const char *file, int line)
{
  return malloc(size);
}

void *operator new[](size_t size, size_t alignment,
                     size_t alignmentOffset, const char *pName,
                     int flags, unsigned debugFlags, const char *file,
                     int line)
{
  return malloc(size);
}

namespace Low {
  namespace Editor {
    const int g_DockSpaceId = 4785;
    bool g_CentralDockOpen = true;

    bool g_GizmosDragged = false;

    ChangeList g_ChangeList;

    Util::Map<uint16_t, TypeMetadata> g_TypeMetadata;
    Util::List<EnumMetadata> g_EnumMetadata;

    struct EditorWidget
    {
      Widget *widget;
      const char *name;
      bool open;
    };

    Util::List<EditorWidget> g_Widgets;

    Widget *g_FocusedWidget;

    EditingWidget *g_MainViewportWidget;
    DetailsWidget *g_DetailsWidget;

    Helper::SphericalBillboardMaterials g_SphericalBillboardMaterials;

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

    Util::Map<u16, TypeMetadata> &get_type_metadata()
    {
      return g_TypeMetadata;
    }

    void register_editor_job(Util::String p_Title,
                             std::function<void()> p_Func)
    {
      g_EditorJobQueue.emplace(p_Title, p_Func);
    }

    static void register_editor_widget(const char *p_Name,
                                       Widget *p_Widget,
                                       bool p_DefaultOpen = false)
    {
      g_Widgets.push_back({p_Widget, p_Name, p_DefaultOpen});
    }

    Util::Handle g_SelectedHandle;

    void set_selected_handle(Util::Handle p_Handle)
    {
      g_SelectedHandle = p_Handle;

      g_DetailsWidget->clear();

      {
        Core::UI::Element l_Element = p_Handle.get_id();
        if (l_Element.is_alive()) {
          for (auto it = l_Element.get_components().begin();
               it != l_Element.get_components().end(); ++it) {
            g_DetailsWidget->add_section(it->second);
          }
        }
      }

      Core::Entity l_Entity = p_Handle.get_id();
      if (!l_Entity.is_alive()) {
        uint32_t l_Id = ~0u;
        g_MainViewportWidget->m_RenderFlowWidget->get_renderflow()
            .get_resources()
            .get_buffer_resource(N(SelectedIdBuffer))
            .set(&l_Id);
        return;
      }
      uint32_t l_Id = l_Entity.get_index();

      g_MainViewportWidget->m_RenderFlowWidget->get_renderflow()
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
        g_DetailsWidget->add_section(it->second);
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

    static void save_user_settings()
    {
      Util::Yaml::Node l_Config;
      l_Config["loaded_scene"] =
          Core::Scene::get_loaded_scene().get_name().c_str();

      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        Util::Yaml::Node i_Widget;
        i_Widget["name"] = it->name;
        i_Widget["open"] = it->open;

        l_Config["widgets"].push_back(i_Widget);
      }
      l_Config["theme"] = theme_get_current_name().c_str();

      Util::String l_Path =
          Util::get_project().rootPath + "/user.yaml";
      Util::Yaml::write_file(l_Path.c_str(), l_Config);
    }

    static void schedule_save_scene(Core::Scene p_Scene)
    {

      uint64_t l_SceneId = p_Scene.get_id();

      Util::String l_JobTitle = "Saving scene ";
      l_JobTitle += p_Scene.get_name().c_str();

      register_editor_job(l_JobTitle, [l_SceneId] {
        SaveHelper::save_scene(l_SceneId);
      });
    }

    static void schedule_import_image(Util::String p_Path)
    {
      Util::String l_JobTitle = "Importing image resource";

      register_editor_job(l_JobTitle, [p_Path] {
        ResourceProcessor::Image::Image2D l_Image;
        ResourceProcessor::Image::load_png(p_Path, l_Image);
        Util::String l_FileName = p_Path.substr(
            p_Path.find_last_of('\\') + 1,
            p_Path.find_last_of('.') - p_Path.find_last_of('\\') - 1);
        ResourceProcessor::Image::process(
            Util::get_project().dataPath + "\\resources\\img2d\\" +
                l_FileName,
            l_Image);

        LOW_LOG_INFO << "Image file '" << l_FileName
                     << "' has been successfully imported."
                     << LOW_LOG_END;
      });
    }

    static void schedule_import_mesh(Util::String p_Path)
    {
      Util::String l_JobTitle = "Importing mesh resource";

      register_editor_job(l_JobTitle, [p_Path] {
        Util::String l_FileName = p_Path.substr(
            p_Path.find_last_of('\\') + 1,
            p_Path.find_last_of('.') - p_Path.find_last_of('\\') - 1);
        bool l_HasAnimations = ResourceProcessor::Mesh::process(
            p_Path.c_str(), Util::get_project().dataPath +
                                "\\resources\\meshes\\" + l_FileName +
                                ".glb");

        Core::MeshResource::make(Util::String(l_FileName + ".glb"));

        LOW_LOG_INFO << "Mesh file '" << l_FileName
                     << "' has been successfully imported."
                     << LOW_LOG_END;

        if (l_HasAnimations) {
          ResourceProcessor::Mesh::process_animations(
              p_Path.c_str(),
              Util::get_project().dataPath +
                  "\\resources\\skeletal_animations\\" + l_FileName +
                  ".glb");

          LOW_LOG_INFO << "Animations from file '" << l_FileName
                       << "' have been successfully imported."
                       << LOW_LOG_END;
        }
      });
    }

    static void render_menu_bar(float p_Delta)
    {
      bool l_CreateScene = false;

      // Menu
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Edit")) {
          if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr)) {
            g_ChangeList.undo();
          }
          if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr)) {
            g_ChangeList.redo();
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
          for (auto it = g_Widgets.begin(); it != g_Widgets.end();
               ++it) {
            if (ImGui::MenuItem(it->name, nullptr, it->open)) {
              it->open = !it->open;
              save_user_settings();
            }
          }
          ImGui::Separator();
          if (ImGui::BeginMenu("Themes")) {
            if (themes_render_menu()) {
              save_user_settings();
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene")) {
          if (ImGui::MenuItem("New", NULL, nullptr)) {
            l_CreateScene = true;
          }
          if (ImGui::MenuItem("Save", NULL, nullptr)) {
            schedule_save_scene(Core::Scene::get_loaded_scene());
          }
          ImGui::Separator();
          for (auto it = Core::Scene::ms_LivingInstances.begin();
               it != Core::Scene::ms_LivingInstances.end(); ++it) {
            if (ImGui::MenuItem(it->get_name().c_str())) {
              if (!it->is_loaded()) {
                it->load();
                save_user_settings();
              }
            }
          }

          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources")) {
          if (ImGui::MenuItem("Import", NULL, nullptr)) {
            Util::String l_Path = Gui::FileExplorer();

            if (!l_Path.empty()) {
              if (Util::StringHelper::ends_with(
                      l_Path, Util::String(".png"))) {
                schedule_import_image(l_Path);
              } else if (Util::StringHelper::ends_with(
                             l_Path, Util::String(".obj")) ||
                         Util::StringHelper::ends_with(
                             l_Path, Util::String(".glb"))) {
                schedule_import_mesh(l_Path);
              }
            }
          }

          ImGui::EndMenu();
        }

        Util::String l_VersionString = "";
        l_VersionString += LOW_VERSION_YEAR;
        l_VersionString += ".";
        l_VersionString += LOW_VERSION_MAJOR;
        l_VersionString += ".";
        l_VersionString += LOW_VERSION_MINOR;

        ImGuiViewport *l_Viewport = ImGui::GetMainViewport();
        ImVec2 l_Cursor = ImGui::GetCursorPos();
        float l_Margin = 73.0f;
        if (Util::StringHelper::ends_with(l_VersionString, "DEV")) {
          l_Margin = 89.0f;
        }
        float l_PointToAchieve = l_Viewport->WorkSize.x - l_Margin;
        float l_CurrentMargin = l_Cursor.x;
        float l_Spacing = l_PointToAchieve - l_CurrentMargin;

        ImGui::Dummy({l_Spacing, 0.0f});
        int l_GreyVal = 120;

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            color_to_imvec4(theme_get_current().subtext));
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_300);
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(cursor + ImVec2(0.0f, 3.0f));
        ImGui::Text(l_VersionString.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::EndMainMenuBar();
      }

      if (l_CreateScene) {
        ImGui::OpenPopup("Create scene");
      }

      if (ImGui::BeginPopupModal("Create scene")) {
        ImGui::Text("You are about to create a new scene. Please "
                    "select a name.");
        static char l_NameBuffer[255];
        ImGui::InputText("##name", l_NameBuffer, 255);

        ImGui::Separator();
        if (ImGui::Button("Cancel")) {
          ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Create")) {
          Util::Name l_Name = LOW_NAME(l_NameBuffer);

          bool l_Ok = true;

          for (auto it = Core::Scene::ms_LivingInstances.begin();
               it != Core::Scene::ms_LivingInstances.end(); ++it) {
            if (l_Name == it->get_name()) {
              LOW_LOG_ERROR
                  << "Cannot create scene. The chosen name '"
                  << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::Scene l_Scene = Core::Scene::make(l_Name);
            l_Scene.load();
            save_user_settings();
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }

    static inline void render_status_bar(float p_Delta)
    {
      float height = ImGui::GetFrameHeight();

      ImGui::BeginViewportSideBar(
          "Statusbar", ImGui::GetMainViewport(), ImGuiDir_Down,
          height,
          ImGuiWindowFlags_NoScrollbar |
              ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_MenuBar);
      if (ImGui::BeginMenuBar()) {
        if (!g_EditorJobQueue.empty() &&
            g_EditorJobQueue.front().submitted) {
          Gui::spinner("Loading", 6.0f, 2.0f,
                       Math::Color(0.0f, 0.7f, 0.73f, 1.0f));
          ImGui::SameLine();
          ImGui::Text(g_EditorJobQueue.front().title.c_str());
        }
        ImGui::EndMenuBar();
      }

      ImGui::End();
    }

    static void render_central_docking_space(float p_Delta)
    {
      // We are using the ImGuiWindowFlags_NoDocking flag to make the
      // parent window not dockable into, because it would be
      // confusing to have two docking targets within each others.
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      static ImGuiDockNodeFlags dockspace_flags =
          ImGuiDockNodeFlags_None;

      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      window_flags |=
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus |
                      ImGuiWindowFlags_NoNavFocus;

      if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace", &g_CentralDockOpen, window_flags);
      ImGui::PopStyleVar(3);

      ImGuiIO &io = ImGui::GetIO();
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       dockspace_flags);

      ImGui::End();
    }

    static void initialize_spherical_billboard_materials()
    {
      Util::String l_DataPath = Util::get_project().dataPath;

      g_SphericalBillboardMaterials.sun =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath +
              "/_internal/assets/editor_icons/ktx/sun.ktx");
      g_SphericalBillboardMaterials.bulb =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath +
              "/_internal/assets/editor_icons/ktx/bulb.ktx");
      g_SphericalBillboardMaterials.camera =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath +
              "/_internal/assets/editor_icons/ktx/camera.ktx");
      g_SphericalBillboardMaterials.region =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath +
              "/_internal/assets/editor_icons/ktx/region.ktx");
    }

    static void initialize_billboard_materials()
    {
      initialize_spherical_billboard_materials();
    }

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

    static inline void parse_enum_metadata(Util::Yaml::Node &p_Node)
    {
      Util::String l_ModuleString =
          LOW_YAML_AS_STRING(p_Node["module"]);

      for (auto it = p_Node["enums"].begin();
           it != p_Node["enums"].end(); ++it) {
        Util::String i_EnumName = LOW_YAML_AS_STRING(it->first);
        EnumMetadata i_Metadata;
        i_Metadata.name = LOW_YAML_AS_NAME(it->first);
        i_Metadata.module = l_ModuleString;

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
          for (auto it = g_Widgets.begin(); it != g_Widgets.end();
               ++it) {
            if (LOW_YAML_AS_STRING(l_Root["widgets"][i]["name"]) ==
                it->name) {
              it->open = l_Root["widgets"][i]["open"].as<bool>();
              break;
            }
          }
        }
      }

      if (l_Root["theme"]) {
        theme_apply(LOW_YAML_AS_NAME(l_Root["theme"]));
      } else if (theme_exists(N(dracula))) {
        theme_apply(N(dracula));
      }
    }

    static void register_type_nodes(u16 p_TypeId)
    {
      Util::String l_NodeNamePrefix = "Handle_";
      l_NodeNamePrefix += LOW_TO_STRING(p_TypeId);
      l_NodeNamePrefix += "_";

      Util::RTTI::TypeInfo l_TypeInfo =
          Util::Handle::get_type_info(p_TypeId);
      TypeMetadata l_TypeMetadata = get_type_metadata(p_TypeId);

      {
        Util::Name l_NodeTypeName = LOW_NAME(
            Util::String(l_NodeNamePrefix + "FindByName").c_str());
        Flode::register_node(
            l_NodeTypeName, [p_TypeId]() -> Flode::Node * {
              return new Flode::HandleNodes::FindByNameNode(p_TypeId);
            });

        Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                   "Find by name", l_NodeTypeName);
      }

      {
        for (auto it = l_TypeMetadata.properties.begin();
             it != l_TypeMetadata.properties.end(); ++it) {
          if (!it->scriptingExpose) {
            continue;
          }

          Util::Name i_PropertyName = it->name;

          {
            Util::Name i_NodeTypeName =
                LOW_NAME(Util::String(l_NodeNamePrefix + "Get" +
                                      i_PropertyName.c_str())
                             .c_str());
            Flode::register_node(
                i_NodeTypeName,
                [p_TypeId, i_PropertyName]() -> Flode::Node * {
                  return new Flode::HandleNodes::GetNode(
                      p_TypeId, i_PropertyName);
                });

            Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                       Util::String("Get ") +
                                           i_PropertyName.c_str(),
                                       i_NodeTypeName);
          }
          {
            Util::Name i_NodeTypeName =
                LOW_NAME(Util::String(l_NodeNamePrefix + "Set" +
                                      i_PropertyName.c_str())
                             .c_str());
            Flode::register_node(
                i_NodeTypeName,
                [p_TypeId, i_PropertyName]() -> Flode::Node * {
                  return new Flode::HandleNodes::SetNode(
                      p_TypeId, i_PropertyName);
                });

            Flode::register_spawn_node(l_TypeInfo.name.c_str(),
                                       Util::String("Set ") +
                                           i_PropertyName.c_str(),
                                       i_NodeTypeName);
          }
        }
      }
    }

    static void register_type_nodes()
    {
      register_type_nodes(Core::Entity::TYPE_ID);
    }

    void initialize()
    {
      themes_load();
      load_low_metadata();
      load_project_metadata();

      Flode::initialize();

      initialize_billboard_materials();

      LogWidget::initialize();

      {
        Flode::MathNodes::register_nodes();
        Flode::SyntaxNodes::register_nodes();
        Flode::DebugNodes::register_nodes();
        Flode::CastNodes::register_nodes();

        register_type_nodes();
      }

      g_MainViewportWidget = new EditingWidget();
      register_editor_widget("Viewport", g_MainViewportWidget, true);
      register_editor_widget("Log", new LogWidget(), true);
      g_DetailsWidget = new DetailsWidget();
      register_editor_widget("Details", g_DetailsWidget, true);
      register_editor_widget("Profiler", new ProfilerWidget());
      register_editor_widget("Assets", new AssetWidget(), true);
      register_editor_widget("Scene", new SceneWidget(), true);
      register_editor_widget("Regions", new RegionWidget(), true);
      register_editor_widget("History", new ChangeWidget());
      register_editor_widget("Resources", new ResourceWidget());
      register_editor_widget("Flode", new FlodeWidget(), false);
      register_editor_widget("UI-Views", new UiWidget(), false);

      for (auto &it : g_TypeMetadata) {
        if (it.second.editor.manager) {
          register_editor_widget(
              it.second.name.c_str(),
              new TypeManagerWidget(it.second.typeId));
        }
      }

      Util::String l_Path =
          Util::get_project().dataPath + "/assets/meshes";

      load_user_settings();
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

    ChangeList &get_global_changelist()
    {
      return g_ChangeList;
    }

    static void handle_shortcuts(float p_Delta)
    {
      if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D))) {
          duplicate({get_selected_handle()});
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S))) {
          schedule_save_scene(Core::Scene::get_loaded_scene());
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
          g_ChangeList.undo();
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
          g_ChangeList.redo();
        }
      }
    }

    void tick(float p_Delta, Util::EngineState p_State)
    {
      LOW_PROFILE_CPU("Editor", "TICK");

      render_menu_bar(p_Delta);

      render_status_bar(p_Delta);

      render_central_docking_space(p_Delta);

      tick_editor_jobs(p_Delta);

      static bool l_DisplayVersion = true;

      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        if (it->open) {
          it->widget->render(p_Delta);
        }
      }

      if (!g_FocusedWidget ||
          !g_FocusedWidget->handle_shortcuts(p_Delta)) {
        handle_shortcuts(p_Delta);
      }
      // ImGui::ShowDemoWindow();
    }

    DetailsWidget *get_details_widget()
    {
      return g_DetailsWidget;
    }

    EditingWidget *get_editing_widget()
    {
      return g_MainViewportWidget;
    }

    TypeMetadata &get_type_metadata(uint16_t p_TypeId)
    {
      LOW_ASSERT(g_TypeMetadata.find(p_TypeId) !=
                     g_TypeMetadata.end(),
                 "Could not find type metadata");
      return g_TypeMetadata[p_TypeId];
    }

    EnumMetadata &get_enum_metadata(Util::String p_EnumTypeName)
    {
      for (EnumMetadata &i_Metadata : g_EnumMetadata) {
        if (i_Metadata.fullTypeName == p_EnumTypeName) {
          return i_Metadata;
        }
      }

      LOW_ASSERT(false, "Could not find enum metadata");
      return EnumMetadata();
    }

    void set_focused_widget(Widget *p_Widget)
    {
      g_FocusedWidget = p_Widget;
    }

    bool get_gizmos_dragged()
    {
      return g_GizmosDragged;
    }

    void set_gizmos_dragged(bool p_Dragged)
    {
      g_GizmosDragged = p_Dragged;
    }

    namespace Helper {
      SphericalBillboardMaterials get_spherical_billboard_materials()
      {
        return g_SphericalBillboardMaterials;
      }

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
    } // namespace Helper
  }   // namespace Editor
} // namespace Low
