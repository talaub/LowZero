#include "LowEditorUiWidgetEditor.h"
#include "LowCore.h"
#include "LowCoreUiText.h"
#include "LowCoreUiWidgetAsset.h"
#include "LowEditorGui.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorIcons.h"
#include "LowEditorUiWidget.h"

#include "LowCoreUiElement.h"
#include "LowCoreUiDisplay.h"
#include "LowCoreUiImage.h"
#include "LowMath.h"
#include "LowRenderer.h"
#include "LowRendererFont.h"
#include "LowRendererPrimitives.h"
#include "LowRendererTexture.h"
#include "LowRendererUiCanvas.h"
#include "LowRendererUiDrawCommand.h"
#include "LowUtilAssetManager.h"
#include "LowUtilHandle.h"
#include "LowUtilString.h"
#include <corecrt_io.h>
#include <imgui.h>

#define SEARCH_LENGTH 300

namespace Low {
  namespace Editor {
    enum class ElementAction
    {
      None,
      Rename,
      Delete,
      DeleteHierarchy
    };

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

    static Core::UI::Element
    create_element(const Util::Name p_Name,
                   Renderer::UiCanvas p_Canvas,
                   Core::UI::Element p_Parent)
    {

      Core::UI::Element l_Element =
          Core::UI::Element::make(p_Name, p_Canvas);
      Core::UI::Component::Display l_Display =
          Core::UI::Component::Display::make(l_Element);

      l_Display.pixel_scale(100, 100);

      l_Display.set_parent(p_Parent.get_display());

      return l_Element;
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

      if (m_SelectedElement == p_Element) {
        l_BaseFlags |= ImGuiTreeNodeFlags_Selected;
      }

      Util::StringBuilder l_IdBuilder;
      l_IdBuilder.append("##row_").append(p_Element.get_id());

      const bool l_Open =
          ImGui::TreeNodeEx(l_IdBuilder.get().c_str(), l_BaseFlags,
                            "%s", p_Element.get_name().c_str());

      if (ImGui::IsItemClicked()) {
        set_selected_element(p_Element);
      }

      l_IdBuilder.append("element_context_menu");

      ElementAction l_Action = ElementAction::None;

      if (ImGui::BeginPopupContextItem(l_IdBuilder.get().c_str())) {
        set_selected_element(p_Element);

        if (ImGui::MenuItem("Rename")) {
          l_Action = ElementAction::Rename;
        }
        if (ImGui::MenuItem("Delete")) {
          l_Action = ElementAction::Delete;
        }
        if (ImGui::MenuItem("Delete hierarchy")) {
          l_Action = ElementAction::DeleteHierarchy;
        }
        ImGui::EndPopup();
      }
      if (l_Action == ElementAction::Rename) {
        Gui::Rename("Rename element", p_Element.get_id());
      } else if (l_Action == ElementAction::Delete) {
      }

      if (!l_Leaf && l_Open) {
        for (Core::UI::Component::Display i_Display :
             l_Display.get_children()) {
          draw_list_element(i_Display.get_element());
        }
        ImGui::TreePop();
      }
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

      if (Gui::SaveButton()) {
        l_Asset.fill_content_from_instance(m_Viewport->m_Instance);
        Util::AssetManager::save(l_Asset);
      }

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
          if (ImGui::BeginPopup("create_element_selection_popup")) {
            if (ImGui::Selectable(LOW_EDITOR_ICON_ELEMENT " Empty")) {
              Core::UI::Element l_Element =
                  create_element("Empty", m_Viewport->m_Canvas,
                                 m_Viewport->m_Instance.get_root());
              set_selected_element(l_Element);
            }
            ImGui::Separator();
            if (ImGui::Selectable(LOW_EDITOR_ICON_IMAGE " Image")) {
              Core::UI::Element l_Element =
                  create_element("Image", m_Viewport->m_Canvas,
                                 m_Viewport->m_Instance.get_root());
              Core::UI::Component::Image l_Image =
                  Core::UI::Component::Image::make(l_Element);
              l_Image.set_material(
                  Renderer::get_default_material_ui());
              l_Image.set_texture(Renderer::get_default_texture());
              set_selected_element(l_Element);
            }
            if (ImGui::Selectable(LOW_EDITOR_ICON_TEXT " Text")) {
              Core::UI::Element l_Element =
                  create_element("Text", m_Viewport->m_Canvas,
                                 m_Viewport->m_Instance.get_root());
              Core::UI::Component::Text l_Text =
                  Core::UI::Component::Text::make(l_Element);
              l_Text.set_font(Renderer::Font::living_instances()[0]);
              l_Text.set_text("Text");
              l_Text.set_size(32);
              l_Text.set_color(Math::Color(1, 1, 1, 1));
              set_selected_element(l_Element);
            }
            ImGui::EndPopup();
          }
          {
            if (Gui::AddButton()) {
              ImGui::OpenPopup("create_element_selection_popup");
            }
            ImGui::SameLine();
            if (Gui::SearchField("##searchelementinwidget",
                                 m_ElementSearch, SEARCH_LENGTH)) {
            }
            ImGui::Dummy(ImVec2{0, 5});
            ImGui::Separator();
          }

          Gui::RenamePopup("Rename element");

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

      if (m_Viewport->m_Instance.is_alive()) {
        Core::UI::Element l_Root = m_Viewport->m_Instance.get_root();
        Core::UI::Component::Display l_RootDisplay =
            l_Root.get_display();

        l_RootDisplay.pixel_position(l_Avail.x / 2.0f,
                                     l_Avail.y / 2.0f);
      }

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

      HandlePropertiesSection l_Section(p_Handle, true);
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
