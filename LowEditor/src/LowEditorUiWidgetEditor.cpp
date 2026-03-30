#include "LowEditorUiWidgetEditor.h"
#include "LowCore.h"
#include "LowCoreUiWidgetAsset.h"
#include "LowEditorGui.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorUiWidget.h"

#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiImage.h"
#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererPrimitives.h"
#include "LowRendererTexture.h"
#include "LowRendererUiDrawCommand.h"
#include "LowUtilAssetManager.h"
#include "LowUtilHandle.h"
#include <imgui.h>

#define SEARCH_LENGTH 300

namespace Low {
  namespace Editor {
    UiWidgetEditor::UiWidgetEditor(Util::Handle p_Handle)
        : TypeEditor(p_Handle), m_LeftPaneWidth(280.0f),
          m_TopPaneHeight(400.0f), m_Test(false),
          m_ElementSearch((char *)calloc(SEARCH_LENGTH, sizeof(char)))
    {
      m_Viewport = new UiWidgetInteractiveViewport(
          p_Handle, Math::UVector2(500, 500));

      m_SelectedMarker = Renderer::UiDrawCommand::make_standalone(
          m_Viewport->m_Canvas, Renderer::get_primitives()
                                    .unitQuad.get_gpu()
                                    .get_submeshes()[0]);
      m_SelectedMarker.set_material(
          Renderer::get_default_material_ui_outline());
      m_SelectedMarker.set_z_sorting(5000);
      m_SelectedMarker.set_color(Math::Color(1.0f, 0.0f, 0.0f, 0.0f));
    }
    UiWidgetEditor::~UiWidgetEditor()
    {
      m_SelectedMarker.destroy();
      delete m_Viewport;
    }

    void
    UiWidgetEditor::draw_list_element(Core::UI::Element p_Element)
    {
      ImGuiTreeNodeFlags l_BaseFlags =
          ImGuiTreeNodeFlags_OpenOnArrow |
          ImGuiTreeNodeFlags_OpenOnDoubleClick |
          ImGuiTreeNodeFlags_SpanAvailWidth;

      Core::UI::Component::Display l_Display =
          p_Element.get_display();

      const bool l_Leaf = l_Display.get_children().empty();

      if (l_Leaf) {
        l_BaseFlags |= ImGuiTreeNodeFlags_Leaf |
                       ImGuiTreeNodeFlags_NoTreePushOnOpen;
      }

      ImGui::PushID(l_Display.get_id());

      const bool l_Open = ImGui::TreeNodeEx(
          "##row", l_BaseFlags, "%s", p_Element.get_name().c_str());

      if (ImGui::IsItemClicked()) {
        set_selected_element(p_Element);
      }

      if (!l_Leaf && l_Open) {
        for (Core::UI::Component::Display i_Display :
             l_Display.get_children()) {
          draw_list_element(i_Display.get_element());
        }
        ImGui::TreePop();
      }

      ImGui::PopID();
    }

    void UiWidgetEditor::render()
    {
      Core::UI::WidgetAsset l_Asset = m_Handle.get_id();

      if (m_SelectedElement.is_alive()) {
        m_SelectedMarker.set_color(
            Math::Color(1.0f, 0.0f, 0.0f, 1.0f));
        Core::UI::Component::Display l_Display =
            m_SelectedElement.get_display();
        const Math::Vector2 l_PixelPosition =
            l_Display.get_absolute_pixel_position();
        m_SelectedMarker.set_position(l_PixelPosition.x,
                                      l_PixelPosition.y, 0);
        m_SelectedMarker.set_size(
            l_Display.get_absolute_pixel_scale());

      } else {
        m_SelectedMarker.set_color(
            Math::Color(1.0f, 0.0f, 0.0f, 0.0f));
      }

#if 0 
      m_Test = true;
      if (!m_Test) {
        m_Test = true;
        Core::UI::ElementDescriptor desc;
        desc.name = N(main);
        Core::UI::Element e = Core::UI::Element::make(N(main));
        Core::UI::Component::Display d =
            Core::UI::Component::Display::make(e);
        d.pixel_position(20, 50);
        d.pixel_scale(120, 80);
        Core::UI::ComponentDescriptor dd;
        dd.typeId = Core::UI::Component::Display::type_id();
        d.serialize(dd.data);

        Core::UI::Component::Image i =
            Core::UI::Component::Image::make(e);
        i.set_texture(
            Renderer::Texture::find_by_name(N(emperor_icon)));
        i.set_material(Renderer::get_default_material_ui());
        Core::UI::ComponentDescriptor id;
        id.typeId = Core::UI::Component::Image::type_id();
        i.serialize(id.data);

        desc.components.push_back(dd);
        desc.components.push_back(id);

        l_Asset.get_content().push_back(desc);

        Util::AssetManager::save(l_Asset);
      }
#endif

      // ----- Left side -----
      ImGui::BeginChild(
          "LeftContainer", ImVec2(m_LeftPaneWidth, 0.0f),
          ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
      {
        // Top-left pane
        ImGui::BeginChild("LeftTop", ImVec2(0.0f, m_TopPaneHeight),
                          ImGuiChildFlags_Borders |
                              ImGuiChildFlags_ResizeY);
        {
          {
            if (Gui::AddButton()) {
            }
            ImGui::SameLine();
            if (Gui::SearchField("##searchelementinwidget",
                                 m_ElementSearch, SEARCH_LENGTH)) {
            }
            ImGui::Dummy(ImVec2{0, 5});
            ImGui::Separator();
          }
          for (Core::UI::Component::Display i_Display :
               m_Viewport->m_Instance.get_root()
                   .get_display()
                   .get_children()) {
            draw_list_element(i_Display.get_element());
          }
        }
        ImGui::EndChild();

        // Bottom-left pane fills the rest
        ImGui::BeginChild("LeftBottom", ImVec2(0.0f, 0.0f),
                          ImGuiChildFlags_Borders);
        {
          for (auto it = m_DetailsSections.begin();
               it != m_DetailsSections.end(); ++it) {
            it->render(LOW_DELTA_TIME);
          }
        }
        ImGui::EndChild();
      }
      ImGui::EndChild();

      ImGui::SameLine();

      // ----- Right side -----
      ImGui::BeginChild("RightPane", ImVec2(0.0f, 0.0f),
                        ImGuiChildFlags_Borders);
      {
        render_viewport();
      }
      ImGui::EndChild();
    }

    void UiWidgetEditor::render_viewport()
    {
      const ImVec2 l_Avail = ImGui::GetContentRegionAvail();
      m_Viewport->tick(LOW_DELTA_TIME);
      m_Viewport->set_dimensions(l_Avail.x, l_Avail.y);
    }

    void UiWidgetEditor::add_section(Util::Handle p_Handle)
    {
      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_Handle.get_type());

      if (!l_TypeInfo.is_alive(p_Handle)) {
        return;
      }

      HandlePropertiesSection l_Section(p_Handle, false);
      l_Section.render_footer = nullptr;

      m_DetailsSections.push_back(l_Section);
    }

    void
    UiWidgetEditor::set_selected_element(Core::UI::Element p_Element)
    {
      if (m_SelectedElement == p_Element) {
        return;
      }
      m_SelectedElement = p_Element;
      m_DetailsSections.clear();

      if (!p_Element.is_alive()) {
        return;
      }

      for (auto it = p_Element.get_components().begin();
           it != p_Element.get_components().end(); ++it) {
        add_section(it->second);
      }
    }
  } // namespace Editor
} // namespace Low
