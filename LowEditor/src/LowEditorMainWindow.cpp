#include "LowEditorMainWindow.h"

#include <iostream>

#include "LowMath.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "IconsFontAwesome5.h"
#include "IconsLucide.h"

#include "LowEditorLogWidget.h"
#include "LowEditorRenderViewWidget.h"
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
#include "LowEditorIcons.h"
#include "LowEditorResourceProcessorImage.h"
#include "LowEditorResourceProcessorMesh.h"
#include "LowEditorSaveHelper.h"
#include "LowEditorMetadata.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorResourceWidget.h"
#include "LowEditorThemes.h"
#include "LowEditorTypeManagerWidget.h"
#include "LowEditorScriptingErrorWidget.h"
#include "LowEditorAppearanceWidget.h"
#include "LowEditorScriptWidget.h"
#include "LowEditorEditWidget.h"
#include "LowEditor.h"
#include "LowEditorNodeGraph.h"
#include "LowEditorVisualScripting.h"
#include "LowEditorVisualScriptingHandleNodes.h"
#include "LowEditorVisualScriptEditor.h"
#include "LowEditorVisualScriptingBoolNodes.h"
#include "LowEditorVisualScriptingCastNodes.h"
#include "LowEditorVisualScriptingDebugNodes.h"
#include "LowEditorVisualScriptingMathNodes.h"
#include "LowEditorVisualScriptingOperatorNodes.h"
#include "LowEditorVisualScriptingSyntaxNodes.h"

#include "LowUtil.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileIO.h"
#include "LowUtilProfiler.h"

#include "LowRenderer.h"

#include "LowCoreDebugGeometry.h"
#include "LowCoreRigidbody.h"
#include "LowCorePrefabInstance.h"
#include "LowCorePhysicsSystem.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <ctype.h>

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
    namespace {
      struct BeginEventNodeClass : public VisualScript::NodeClass
      {
        virtual Util::Name get_name() const override
        {
          return N(vs_begin_event);
        }

        virtual Util::String
        get_title(const VisualScript::Graph &p_Graph,
                  NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "On Begin";
        }

        virtual Util::String
        get_subtitle(const VisualScript::Graph &p_Graph,
                     NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "Event";
        }

        virtual Util::String
        get_category(const VisualScript::Graph &p_Graph,
                     NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "Events";
        }

        virtual Util::String
        get_icon(const VisualScript::Graph &p_Graph,
                 NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return ICON_LC_PLAY;
        }

        virtual ImU32 get_color(const VisualScript::Graph &p_Graph,
                                NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return IM_COL32(177, 61, 156, 255);
        }

        virtual void setup_default_pins(
            VisualScript::Graph &p_Graph, NodeId p_NodeId,
            const NodeGraphSchema *p_Schema) const override
        {
          Editor::Pin l_ExecOut =
              VisualScript::make_output_pin(p_Graph, p_NodeId);
          VisualScript::Pin l_ExecOutMetadata =
              VisualScript::make_execution_pin_metadata("Exec");

          p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);
        }
      };

      struct PrintStringNodeClass : public VisualScript::NodeClass
      {
        virtual Util::Name get_name() const override
        {
          return N(vs_print_string);
        }

        virtual Util::String
        get_title(const VisualScript::Graph &p_Graph,
                  NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "Print String";
        }

        virtual Util::String
        get_subtitle(const VisualScript::Graph &p_Graph,
                     NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "Debug";
        }

        virtual Util::String
        get_category(const VisualScript::Graph &p_Graph,
                     NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return "Debug";
        }

        virtual Util::String
        get_icon(const VisualScript::Graph &p_Graph,
                 NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return LOW_EDITOR_ICON_TEXT;
        }

        virtual ImU32 get_color(const VisualScript::Graph &p_Graph,
                                NodeId p_NodeId) const override
        {
          (void)p_Graph;
          (void)p_NodeId;
          return IM_COL32(68, 129, 214, 255);
        }

        virtual void setup_default_pins(
            VisualScript::Graph &p_Graph, NodeId p_NodeId,
            const NodeGraphSchema *p_Schema) const override
        {
          Editor::Pin l_ExecIn =
              VisualScript::make_input_pin(p_Graph, p_NodeId);
          VisualScript::Pin l_ExecInMetadata =
              VisualScript::make_execution_pin_metadata("Exec");
          p_Graph.add_pin(l_ExecIn, l_ExecInMetadata, p_Schema);

          Editor::Pin l_MessageIn =
              VisualScript::make_input_pin(p_Graph, p_NodeId);
          VisualScript::Pin l_MessageInMetadata =
              VisualScript::make_string_pin_metadata("Message");
          l_MessageInMetadata.default_value =
              Util::Variant(Util::String("Hello from LowEditor"));
          p_Graph.add_pin(l_MessageIn, l_MessageInMetadata, p_Schema);

          Editor::Pin l_EnabledIn =
              VisualScript::make_input_pin(p_Graph, p_NodeId);
          VisualScript::Pin l_EnabledInMetadata =
              VisualScript::make_bool_pin_metadata("Enabled", true);
          p_Graph.add_pin(l_EnabledIn, l_EnabledInMetadata, p_Schema);

          Editor::Pin l_ExecOut =
              VisualScript::make_output_pin(p_Graph, p_NodeId);
          VisualScript::Pin l_ExecOutMetadata =
              VisualScript::make_execution_pin_metadata("Then");
          p_Graph.add_pin(l_ExecOut, l_ExecOutMetadata, p_Schema);
        }
      };

      BeginEventNodeClass g_BeginEventNodeClass;
      PrintStringNodeClass g_PrintStringNodeClass;

      static void
      register_test_visual_script_nodes(VisualScript::Graph &p_Graph)
      {
        p_Graph.register_node_class(g_BeginEventNodeClass);
        p_Graph.register_node_class(g_PrintStringNodeClass);
        VisualScript::BoolNodes::register_nodes(p_Graph);
        VisualScript::CastNodes::register_nodes(p_Graph);
        VisualScript::DebugNodes::register_nodes(p_Graph);
        VisualScript::HandleNodes::register_nodes(p_Graph);
        VisualScript::MathNodes::register_nodes(p_Graph);
        VisualScript::OperatorNodes::register_nodes(p_Graph);
        VisualScript::SyntaxNodes::register_nodes(p_Graph);

        VisualScript::NodeSpawnEntry l_BeginEventEntry;
        l_BeginEventEntry.id = N(vs_spawn_begin_event);
        l_BeginEventEntry.category = "Events";
        l_BeginEventEntry.title = "On Begin";
        l_BeginEventEntry.subtitle = "Event";
        l_BeginEventEntry.search_text = "on begin event start";
        l_BeginEventEntry.node_class =
            g_BeginEventNodeClass.get_name();
        p_Graph.register_spawn_entry(l_BeginEventEntry);

        VisualScript::NodeSpawnEntry l_PrintStringEntry;
        l_PrintStringEntry.id = N(vs_spawn_print_string);
        l_PrintStringEntry.category = "Debug";
        l_PrintStringEntry.title = "Print String";
        l_PrintStringEntry.subtitle = "Debug";
        l_PrintStringEntry.search_text = "print string debug log";
        l_PrintStringEntry.node_class =
            g_PrintStringNodeClass.get_name();
        p_Graph.register_spawn_entry(l_PrintStringEntry);
      }
    } // namespace

    const int g_DockSpaceId = 4785;
    bool g_CentralDockOpen = true;

    VisualScript::Document g_TestVisualScriptDocument;
    VisualScript::Editor g_TestVisualScriptEditor;

    bool g_GizmosDragged = false;

    Widget *g_FocusedWidget;

    EditingWidget *g_MainViewportWidget;
    DetailsWidget *g_DetailsWidget;
    FlodeWidget *g_FlodeWidget;
    ScriptWidget *g_ScriptWidget;

    Low::Util::Map<Util::Handle, EditWidget *> g_EditWidgets;

    Low::Util::Map<Util::String, EditorWidget> g_Widgets;

    struct MenuEntry
    {
      Low::Util::String title;
      Low::Util::List<MenuEntry> submenus;
      Low::Util::List<Util::String> widgets;
    };

    Low::Util::List<MenuEntry> g_MenuEntries;

    Helper::SphericalBillboardMaterials g_SphericalBillboardMaterials;

    Util::Map<Util::String, EditorWidget> &get_editor_widgets()
    {
      return g_Widgets;
    }

    void open_editor_widget(Widget *p_Widget)
    {
      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        if (it->second.widget == p_Widget) {
          it->second.open = true;
        }
      }
      save_user_settings();
    }

    void close_editor_widget(Widget *p_Widget)
    {
      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        if (it->second.widget == p_Widget) {
          it->second.open = false;
        }
      }
      save_user_settings();
    }

    static MenuEntry *
    find_menu_entry(Util::List<Util::String> p_Parts,
                    MenuEntry *p_MenuEntry, bool p_Create = false)
    {
      if (p_Parts.empty() && !p_MenuEntry) {
        return nullptr;
      }

      if (p_Parts.empty() && p_MenuEntry) {
        return p_MenuEntry;
      }

      Util::String l_Title = p_Parts.front();
      p_Parts.erase(p_Parts.begin());

      if (!p_MenuEntry) {
        for (u32 i = 0; i < g_MenuEntries.size(); ++i) {
          if (g_MenuEntries[i].title == l_Title) {
            return find_menu_entry(p_Parts, &g_MenuEntries[i]);
          }
        }
        MenuEntry l_MenuEntry;
        l_MenuEntry.title = l_Title;
        g_MenuEntries.push_back(l_MenuEntry);
        return find_menu_entry(p_Parts, &g_MenuEntries.back());
      }

      for (u32 i = 0; i < p_MenuEntry->submenus.size(); ++i) {
        if (p_MenuEntry->submenus[i].title == l_Title) {
          return find_menu_entry(p_Parts, &p_MenuEntry->submenus[i]);
        }
      }

      MenuEntry l_MenuEntry;
      l_MenuEntry.title = l_Title;
      p_MenuEntry->submenus.push_back(l_MenuEntry);
      return find_menu_entry(p_Parts, &p_MenuEntry->submenus.back());
    }

    static MenuEntry *find_menu_entry(Util::String p_Path,
                                      MenuEntry *p_MenuEntry,
                                      bool p_Create = false)
    {
      Util::List<Util::String> l_Parts;
      Util::StringHelper::split(p_Path, '/', l_Parts);

      return find_menu_entry(l_Parts, p_MenuEntry, p_Create);
    }

    void register_editor_widget(Low::Util::String p_Path,
                                Widget *p_Widget, bool p_DefaultOpen,
                                bool p_AddToViewMenu)
    {
      Util::List<Util::String> l_Parts;
      Util::StringHelper::split(p_Path, '/', l_Parts);

      EditorWidget l_Widget;
      l_Widget.path = p_Path;
      l_Widget.name = l_Parts.back().c_str();
      l_Widget.open = p_DefaultOpen;
      l_Widget.widget = p_Widget;
      l_Widget.viewMenu = p_AddToViewMenu;

      g_Widgets[p_Path] = l_Widget;

      l_Parts.pop_back();

      MenuEntry *l_MenuEntry =
          find_menu_entry(l_Parts, nullptr, true);
      if (l_MenuEntry) {
        l_MenuEntry->widgets.push_back(l_Widget.path);
        return;
      }

      bool l_Exists = false;
      for (u32 i = 0; i < g_MenuEntries.size(); ++i) {
        if (g_MenuEntries[i].title == "Widgets") {
          l_Exists = true;
          g_MenuEntries[i].widgets.push_back(l_Widget.path);
          break;
        }
      }

      if (!l_Exists) {
        MenuEntry l_MenuEntry;
        l_MenuEntry.title = "Widgets";
        l_MenuEntry.widgets.push_back(l_Widget.path);
        g_MenuEntries.push_back(l_MenuEntry);
      }
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

      register_editor_job(l_JobTitle, [p_Path] {});
    }

    static void render_menu_entry(MenuEntry &p_MenuEntry)
    {
      if (ImGui::BeginMenu(p_MenuEntry.title.c_str())) {
        for (auto it = p_MenuEntry.widgets.begin();
             it != p_MenuEntry.widgets.end(); ++it) {
          EditorWidget &i_Widget = g_Widgets[*it];
          if (!i_Widget.viewMenu) {
            continue;
          }
          if (ImGui::MenuItem(i_Widget.name.c_str(), nullptr,
                              i_Widget.open)) {
            i_Widget.open = !i_Widget.open;
            save_user_settings();
          }
        }
        for (auto it = p_MenuEntry.submenus.begin();
             it != p_MenuEntry.submenus.end(); ++it) {
          render_menu_entry(*it);
        }
        ImGui::EndMenu();
      }
    }

    static void render_title_bar(float p_Delta)
    {
      const float topBarHeight = 40.0f; // Taller than default
      /*
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(
          ImVec2(ImGui::GetIO().DisplaySize.x, topBarHeight));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      */

      ImGui::PushStyleColor(
          ImGuiCol_ChildBg,
          color_to_imvec4(theme_get_current().menubar));
      ImGui::BeginChild(
          "TopBar", ImVec2(ImGui::GetContentRegionAvail().x, 40.0f),
          true, 0);

      ImGui::PopStyleColor();

      // ImGui::PopStyleVar(2);

      ImGui::BeginGroup();
      ImGui::SetCursorPosY((topBarHeight - 24.0f) *
                           0.5f); // Center icon vertically

      // Example: ImGui::Image for your logo
      /*
      ImGui::Image(Low::Renderer::get_default_texture_id(),
                   ImVec2(24, 24)); // Replace with your ID
      ImGui::SameLine();
      */
      // ImGui::Text("LowEngine"); // Optional name

      ImGui::EndGroup();

      ImGui::SameLine(); // Adjust spacing based on logo size

      // ImGui::SetCursorScreenPos(ImVec2(50, 24.0f));

      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Test")) {
        }
        ImGui::EndMenu();
      }
      ImGui::SameLine();
      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("TestEdit")) {
        }
        ImGui::EndMenu();
      }

      ImGui::EndChild();
    }

    static void render_menu_bar(float p_Delta)
    {
      bool l_CreateScene = false;

      // ImGui::Dummy({0.0f, 20.0f});

      // Menu
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Edit")) {
          if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr)) {
            get_global_changelist().undo();
          }
          if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr)) {
            get_global_changelist().redo();
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
          if (ImGui::MenuItem("Appearance")) {
            g_Widgets["Appearance"].open =
                !g_Widgets["Appearance"].open;
            save_user_settings();
          }
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

        for (auto it = g_MenuEntries.begin();
             it != g_MenuEntries.end(); ++it) {
          render_menu_entry(*it);
        }

        static bool show_metrics = false;
        static bool show_id_stack = false;
        static bool show_debug_log = false;

        if (ImGui::BeginMenu("Tools")) {
          ImGui::MenuItem("ImGui Metrics/Debugger", nullptr,
                          &show_metrics);
          ImGui::MenuItem("ImGui ID Stack Tool", nullptr,
                          &show_id_stack);
          ImGui::MenuItem("ImGui Debug Log", nullptr,
                          &show_debug_log);
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
        ImGui::PushFont(Fonts::UI(12));
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(cursor + ImVec2(0.0f, 3.0f));
        ImGui::Text(l_VersionString.c_str());
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::EndMainMenuBar();

        if (show_metrics)
          ImGui::ShowMetricsWindow(
              &show_metrics); // main “widget debugger”
        if (show_id_stack)
          ImGui::ShowIDStackToolWindow(&show_id_stack);
        if (show_debug_log)
          ImGui::ShowDebugLogWindow(&show_debug_log);
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
        if (is_editor_job_in_progress()) {
          Gui::spinner("Loading", 6.0f, 2.0f,
                       Math::Color(0.0f, 0.7f, 0.73f, 1.0f));
          ImGui::SameLine();
          ImGui::Text(get_active_editor_job_name().c_str());
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
      // render_title_bar(p_Delta);

      ImGuiIO &io = ImGui::GetIO();
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       dockspace_flags);

      ImGui::End();
    }

    static void initialize_spherical_billboard_materials()
    {
      Util::String l_DataPath = Util::get_project().dataPath;

      // TODO: Fix
      /*
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
              */
    }

    static void initialize_billboard_materials()
    {
      initialize_spherical_billboard_materials();
    }

    static void initialize_test_visual_script_graph()
    {
      if (!g_TestVisualScriptDocument.graph.graph.nodes.empty()) {
        return;
      }

      g_TestVisualScriptDocument.initialize();
      g_TestVisualScriptEditor.load_document(
          g_TestVisualScriptDocument);

      register_test_visual_script_nodes(
          g_TestVisualScriptDocument.graph);

      {
        VisualScript::Variable l_HealthVariable;
        l_HealthVariable.name = "Health";
        l_HealthVariable.type = VisualScript::PinType::Number;
        l_HealthVariable.number_subtype =
            VisualScript::NumberSubtype::Float;
        l_HealthVariable.default_value = Util::Variant(100.0f);
        g_TestVisualScriptDocument.graph.add_variable(
            l_HealthVariable);
      }

      {
        VisualScript::Variable l_IsAliveVariable;
        l_IsAliveVariable.name = "IsAlive";
        l_IsAliveVariable.type = VisualScript::PinType::Bool;
        l_IsAliveVariable.default_value = Util::Variant(true);
        g_TestVisualScriptDocument.graph.add_variable(
            l_IsAliveVariable);
      }

      auto l_BeginNodeResult =
          g_TestVisualScriptDocument.graph
              .create_node_from_spawn_entry(
                  N(vs_spawn_begin_event),
                  Math::Vector2(120.0f, 120.0f),
                  &g_TestVisualScriptDocument.schema);
      auto l_PrintNodeResult =
          g_TestVisualScriptDocument.graph
              .create_node_from_spawn_entry(
                  N(vs_spawn_print_string),
                  Math::Vector2(460.0f, 220.0f),
                  &g_TestVisualScriptDocument.schema);

      if (!l_BeginNodeResult.succeeded() ||
          !l_PrintNodeResult.succeeded()) {
        return;
      }

      Util::List<Editor::Pin *> l_BeginPins =
          g_TestVisualScriptDocument.graph.graph.get_node_pins(
              l_BeginNodeResult.value->id);
      Util::List<Editor::Pin *> l_PrintPins =
          g_TestVisualScriptDocument.graph.graph.get_node_pins(
              l_PrintNodeResult.value->id);

      PinId l_BeginExecOut;
      PinId l_PrintExecIn;

      for (Editor::Pin *i_Pin : l_BeginPins) {
        const VisualScript::Pin *l_PinMetadata =
            g_TestVisualScriptDocument.graph.find_pin(i_Pin->id);
        if (l_PinMetadata &&
            l_PinMetadata->type == VisualScript::PinType::Execution &&
            i_Pin->direction == PinDirection::Output) {
          l_BeginExecOut = i_Pin->id;
        }
      }

      for (Editor::Pin *i_Pin : l_PrintPins) {
        const VisualScript::Pin *l_PinMetadata =
            g_TestVisualScriptDocument.graph.find_pin(i_Pin->id);
        if (l_PinMetadata &&
            l_PinMetadata->type == VisualScript::PinType::Execution &&
            i_Pin->direction == PinDirection::Input) {
          l_PrintExecIn = i_Pin->id;
        }
      }

      if (l_BeginExecOut.is_valid() && l_PrintExecIn.is_valid()) {
        Editor::Link l_Link;
        l_Link.id =
            LinkId{g_TestVisualScriptDocument.graph.id_counter++};
        l_Link.start_pin = l_BeginExecOut;
        l_Link.end_pin = l_PrintExecIn;
        g_TestVisualScriptDocument.graph.add_link(
            l_Link, &g_TestVisualScriptDocument.schema);
      }
    }

    void initialize_main_window()
    {
      themes_load();

      initialize_billboard_materials();
      initialize_test_visual_script_graph();
      g_TestVisualScriptEditor.load_document(
          g_TestVisualScriptDocument);

      LogWidget::initialize();

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
      // register_editor_widget("Resources", new ResourceWidget());
      g_FlodeWidget = new FlodeWidget();
      register_editor_widget("Flode", g_FlodeWidget, false);
      register_editor_widget("UI-Views", new UiWidget(), false);
      register_editor_widget("Scripting errors",
                             new ScriptingErrorWidget, false);
      register_editor_widget("Appearance", new AppearanceWidget,
                             false, false);
      g_ScriptWidget = new ScriptWidget;
      register_editor_widget("Code Editor", g_ScriptWidget, false);

      for (auto &it : get_type_metadata()) {
        if (it.second.editor.manager) {
          Util::String i_Path = it.second.name.c_str();
          if (!it.second.editor.managerWidgetPath.empty()) {
            i_Path = it.second.editor.managerWidgetPath;
          }
          register_editor_widget(
              i_Path, new TypeManagerWidget(it.second.typeId));
        }
      }
    }

    static void handle_shortcuts(float p_Delta)
    {
      if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_D)) {
          duplicate({get_selected_handle()});
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
          schedule_save_scene(Core::Scene::get_loaded_scene());
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
          get_global_changelist().undo();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
          get_global_changelist().redo();
        }
      }
    }

    void render_main_window(float p_Delta, Util::EngineState p_State)
    {
      LOW_PROFILE_CPU("MainWindow", "Render");

      render_menu_bar(p_Delta);

      /*
      ImGui::Begin("TestWindow", nullptr, ImGuiWindowFlags_MenuBar);
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          ImGui::MenuItem("New");
          ImGui::MenuItem("Open");
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
          ImGui::MenuItem("Undo");
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }
      ImGui::Text("Content");
      ImGui::End();
      */

      render_status_bar(p_Delta);

      render_central_docking_space(p_Delta);

      static bool l_DisplayVersion = true;

      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        if (it->second.open) {
          it->second.widget->show(p_Delta);
        }
      }

      for (auto it = g_EditWidgets.begin();
           it != g_EditWidgets.end();) {
        if (it->second->handle_is_alive()) {
          it->second->show(p_Delta);
        }
        if (it->second->is_open()) {
          it++;
        } else {
          if (g_FocusedWidget == it->second) {
            _set_focused_widget(nullptr);
          }
          delete it->second;
          it = g_EditWidgets.erase(it);
        }
      }

      if (!g_FocusedWidget ||
          !g_FocusedWidget->handle_shortcuts(p_Delta)) {
        handle_shortcuts(p_Delta);
      }
      ImGui::ShowDemoWindow();

      ImGui::Begin("Visual Scripting");
      g_TestVisualScriptEditor.render("Graph",
                                      Math::Vector2(0.0f, 0.0f));
      ImGui::End();

      // ImGui::ShowMetricsWindow();
    }

    Widget *_open_widget_for_handle(Util::Handle p_Handle)
    {
      for (auto it = g_EditWidgets.begin();
           it != g_EditWidgets.end();) {
        if (!it->second->handle_is_alive()) {
          delete it->second;

          it = g_EditWidgets.erase(it);
          continue;
        }

        if (it->first == p_Handle) {
          _set_focused_widget(it->second);
          return it->second;
        }

        it++;
      }

      EditWidget *l_NewWidget = new EditWidget(p_Handle);
      _set_focused_widget(l_NewWidget);
      g_EditWidgets[p_Handle] = l_NewWidget;
    }

    DetailsWidget *get_details_widget()
    {
      return g_DetailsWidget;
    }

    EditingWidget *get_editing_widget()
    {
      return g_MainViewportWidget;
    }

    FlodeWidget *get_flode_widget()
    {
      return g_FlodeWidget;
    }

    ScriptWidget *get_script_widget()
    {
      return g_ScriptWidget;
    }

    void _set_focused_widget(Widget *p_Widget)
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

    void set_widget_open(Util::String p_Path, bool p_Open)
    {
      for (auto &it : g_Widgets) {
        if (it.first == p_Path) {
          it.second.open = p_Open;
        }
      }
    }

    namespace Helper {
      SphericalBillboardMaterials get_spherical_billboard_materials()
      {
        return g_SphericalBillboardMaterials;
      }

    } // namespace Helper
  } // namespace Editor
} // namespace Low
