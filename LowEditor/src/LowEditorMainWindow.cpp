#include "LowEditorMainWindow.h"

#include "imgui.h"

#include "IconsFontAwesome5.h"

#include "LowEditorLogWidget.h"
#include "LowEditorRenderFlowWidget.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorProfilerWidget.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorAssetWidget.h"
#include "LowEditorSceneWidget.h"
#include "LowEditorRegionWidget.h"
#include "LowEditorStateGraphWidget.h"
#include "LowEditorGui.h"
#include "LowEditorResourceProcessorImage.h"
#include "LowEditorResourceProcessorMesh.h"
#include "LowEditorSaveHelper.h"

#include "LowUtilContainers.h"
#include "LowUtilString.h"

#include "LowRendererTexture2D.h"

#include "LowCoreMeshResource.h"
#include "LowCoreDebugGeometry.h"
#include "LowCoreRigidbody.h"
#include "LowCorePhysicsSystem.h"

#include <functional>

namespace Low {
  namespace Editor {
    const int g_DockSpaceId = 4785;
    bool g_CentralDockOpen = true;

    struct EditorWidget
    {
      Widget *widget;
      const char *name;
      bool open;
    };

    Util::List<EditorWidget> g_Widgets;

    EditingWidget *g_MainViewportWidget;
    DetailsWidget *g_DetailsWidget;

    Helper::SphericalBillboardMaterials g_SphericalBillboardMaterials;

    static void register_editor_widget(const char *p_Name, Widget *p_Widget,
                                       bool p_DefaultOpen = false)
    {
      g_Widgets.push_back({p_Widget, p_Name, p_DefaultOpen});
    }

    Util::Handle g_SelectedHandle;

    void set_selected_handle(Util::Handle p_Handle)
    {
      g_SelectedHandle = p_Handle;

      g_DetailsWidget->clear();

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

    static void render_menu_bar(float p_Delta)
    {
      bool l_CreateScene = false;

      // Menu
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
          for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
            if (ImGui::MenuItem(it->name, nullptr, it->open)) {
              it->open = !it->open;
            }
          }
          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene")) {
          if (ImGui::MenuItem("New", NULL, nullptr)) {
            l_CreateScene = true;
          }
          if (ImGui::MenuItem("Save", NULL, nullptr)) {
            SaveHelper::save_scene(Core::Scene::get_loaded_scene());
          }
          ImGui::Separator();
          for (auto it = Core::Scene::ms_LivingInstances.begin();
               it != Core::Scene::ms_LivingInstances.end(); ++it) {
            if (ImGui::MenuItem(it->get_name().c_str())) {
              if (!it->is_loaded()) {
                it->load();
              }
            }
          }

          ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources")) {
          if (ImGui::MenuItem("Import", NULL, nullptr)) {
            Util::String l_Path = Gui::FileExplorer();

            if (!l_Path.empty()) {
              if (Util::StringHelper::ends_with(l_Path, Util::String(".png"))) {
                ResourceProcessor::Image::Image2D l_Image;
                ResourceProcessor::Image::load_png(l_Path, l_Image);
                Util::String l_FileName = l_Path.substr(
                    l_Path.find_last_of('\\') + 1,
                    l_Path.find_last_of('.') - l_Path.find_last_of('\\') - 1);
                ResourceProcessor::Image::process(Util::String(LOW_DATA_PATH) +
                                                      "\\resources\\img2d\\" +
                                                      l_FileName,
                                                  l_Image);

                LOW_LOG_INFO << "Image file '" << l_FileName
                             << "' has been successfully imported."
                             << LOW_LOG_END;
              } else if (Util::StringHelper::ends_with(l_Path,
                                                       Util::String(".obj")) ||
                         Util::StringHelper::ends_with(l_Path,
                                                       Util::String(".glb"))) {
                Util::String l_FileName = l_Path.substr(
                    l_Path.find_last_of('\\') + 1,
                    l_Path.find_last_of('.') - l_Path.find_last_of('\\') - 1);
                bool l_HasAnimations = ResourceProcessor::Mesh::process(
                    l_Path.c_str(), Util::String(LOW_DATA_PATH) +
                                        "\\resources\\meshes\\" + l_FileName +
                                        ".glb");

                Core::MeshResource::make(Util::String(l_FileName + ".glb"));

                LOW_LOG_INFO << "Mesh file '" << l_FileName
                             << "' has been successfully imported."
                             << LOW_LOG_END;

                if (l_HasAnimations) {
                  ResourceProcessor::Mesh::process_animations(
                      l_Path.c_str(), Util::String(LOW_DATA_PATH) +
                                          "\\resources\\skeletal_animations\\" +
                                          l_FileName + ".glb");

                  LOW_LOG_INFO << "Animations from file '" << l_FileName
                               << "' have been successfully imported."
                               << LOW_LOG_END;
                }
              }
            }
          }

          ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
      }

      if (l_CreateScene) {
        ImGui::OpenPopup("Create scene");
      }

      if (ImGui::BeginPopupModal("Create scene")) {
        ImGui::Text(
            "You are about to create a new scene. Please select a name.");
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
              LOW_LOG_ERROR << "Cannot create scene. The chosen name '"
                            << l_Name << "' is already in use" << LOW_LOG_END;
              l_Ok = false;
              break;
            }
          }

          if (l_Ok) {
            Core::Scene l_Scene = Core::Scene::make(l_Name);
            l_Scene.load();
          }
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }

    static void render_central_docking_space(float p_Delta)
    {
      // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
      // window not dockable into, because it would be confusing to have two
      // docking targets within each others.
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      window_flags |= ImGuiWindowFlags_NoTitleBar |
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove;
      window_flags |=
          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

      if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace", &g_CentralDockOpen, window_flags);
      ImGui::PopStyleVar(3);

      ImGuiIO &io = ImGui::GetIO();
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      ImGui::End();
    }

    static void initialize_spherical_billboard_materials()
    {
      Util::String l_DataPath(LOW_DATA_PATH);

      g_SphericalBillboardMaterials.sun =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath + "/_internal/assets/editor_icons/ktx/sun.ktx");
      g_SphericalBillboardMaterials.bulb =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath + "/_internal/assets/editor_icons/ktx/bulb.ktx");
      g_SphericalBillboardMaterials.camera =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath + "/_internal/assets/editor_icons/ktx/camera.ktx");
      g_SphericalBillboardMaterials.region =
          Core::DebugGeometry::create_spherical_billboard_material(
              l_DataPath + "/_internal/assets/editor_icons/ktx/region.ktx");
    }

    static void initialize_billboard_materials()
    {
      initialize_spherical_billboard_materials();
    }

    void initialize()
    {
      initialize_billboard_materials();

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
      register_editor_widget("StateGraph", new StateGraphWidget(), true);
    }

    void tick(float p_Delta, Util::EngineState p_State)
    {
      render_menu_bar(p_Delta);

      render_central_docking_space(p_Delta);

      for (auto it = g_Widgets.begin(); it != g_Widgets.end(); ++it) {
        if (it->open) {
          it->widget->render(p_Delta);
        }
      }
      // ImGui::ShowDemoWindow();
    }

    DetailsWidget *get_details_widget()
    {
      return g_DetailsWidget;
    }

    namespace Helper {
      SphericalBillboardMaterials get_spherical_billboard_materials()
      {
        return g_SphericalBillboardMaterials;
      }
    } // namespace Helper
  }   // namespace Editor
} // namespace Low
