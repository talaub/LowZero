#include "LowEditorUiWidgetEditor.h"
#include "LowCore.h"
#include "LowCoreUiText.h"
#include "LowCoreUiWidgetAsset.h"
#include "LowEditorGui.h"
#include "LowEditorThemes.h"
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
#include "LowUtilLogger.h"
#include "LowUtilString.h"
#include <IconsCodicons.h>
#include <corecrt_io.h>
#include <imgui.h>
#include "ImGuizmo.h"
#include "glm/matrix.hpp"

#define SEARCH_LENGTH 300

#define PAN_SPEED 50.0f
#define ZOOM_SPEED 1.0f

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
      LOW_LOG_DEBUG << "Handle: " << p_Handle << LOW_LOG_END;
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

    bool
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

      const ImVec2 l_RectMin = ImGui::GetItemRectMin();
      const ImVec2 l_RectMax = ImGui::GetItemRectMax();
      const float l_RectHeight = l_RectMax.y - l_RectMin.y;

      if (ImGui::BeginDragDropSource()) {
        Core::UI::Element l_PayloadElement = p_Element;

        ImGui::SetDragDropPayload("UI_ELEMENT", &l_PayloadElement,
                                  sizeof(Core::UI::Element));

        ImGui::TextUnformatted(p_Element.get_name().c_str());

        ImGui::EndDragDropSource();
      }

      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("UI_ELEMENT")) {
          LOW_ASSERT(l_Payload->DataSize == sizeof(Core::UI::Element),
                     "Invalid drag payload size");

          Core::UI::Element l_DraggedElement =
              *static_cast<const Core::UI::Element *>(
                  l_Payload->Data);

          if (l_DraggedElement != p_Element) {
            const ImVec2 l_MousePos = ImGui::GetMousePos();

            enum class DropPosition
            {
              Before,
              On,
              After
            };

            DropPosition l_DropPosition = DropPosition::On;

            const float l_UpperThreshold =
                l_RectMin.y + l_RectHeight * 0.25f;
            const float l_LowerThreshold =
                l_RectMax.y - l_RectHeight * 0.25f;

            /*
            if (l_MousePos.y < l_UpperThreshold) {
              l_DropPosition = DropPosition::Before;
            } else if (l_MousePos.y > l_LowerThreshold) {
              l_DropPosition = DropPosition::After;
            } else {
              l_DropPosition = DropPosition::On;
            }

            switch (l_DropPosition) {
            case DropPosition::Before:
              move_element_before(l_DraggedElement, p_Element);
              break;
            case DropPosition::On:
              move_element_as_child(l_DraggedElement, p_Element);
              break;
            case DropPosition::After:
              move_element_after(l_DraggedElement, p_Element);
              break;
            }
            */

            l_DraggedElement.get_display().set_parent(
                p_Element.get_display());
          }
        }

        ImGui::EndDragDropTarget();
      }

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

      bool l_Destroy = false;
      if (l_Action == ElementAction::Rename) {
        Gui::Rename("Rename element", p_Element.get_id());
      } else if (l_Action == ElementAction::DeleteHierarchy) {
        p_Element.destroy_with_hierarchy();
        l_Destroy = true;
      } else if (l_Action == ElementAction::Delete) {
        for (Core::UI::Component::Display i_Display :
             p_Element.get_display().get_children()) {
          i_Display.set_parent(p_Element.get_display().get_parent());
        }
        p_Element.destroy();
        l_Destroy = true;
      }

      if (!l_Leaf && l_Open) {
        if (!l_Destroy) {
          for (Core::UI::Component::Display i_Display :
               l_Display.get_children()) {
            l_Destroy = draw_list_element(i_Display.get_element());
          }
        }
        ImGui::TreePop();
      }
      return l_Destroy;
    }

    static Math::Matrix4x4
    build_ui_element_matrix(const Math::Vector2 &p_Position,
                            float p_RotationRadians,
                            const Math::Vector2 &p_Size)
    {
      Math::Matrix4x4 l_Translation = glm::translate(
          Math::Matrix4x4(1.0f),
          Math::Vector3(p_Position.x, p_Position.y, 0.0f));

      Math::Matrix4x4 l_Rotation = glm::toMat4(glm::angleAxis(
          p_RotationRadians, Math::Vector3(0.0f, 0.0f, 1.0f)));

      Math::Matrix4x4 l_Scale =
          glm::scale(Math::Matrix4x4(1.0f),
                     Math::Vector3(p_Size.x, p_Size.y, 1.0f));

      return l_Translation * l_Rotation * l_Scale;
    }

    static void decompose_ui_element_matrix(
        const Math::Matrix4x4 &p_Matrix, Math::Vector2 &p_Position,
        float &p_RotationRadians, Math::Vector2 &p_Size)
    {
      Math::Vector3 l_Skew;
      Math::Vector4 l_Perspective;
      Math::Vector3 l_Translation;
      Math::Vector3 l_Scale;
      Math::Quaternion l_Rotation;

      glm::decompose(p_Matrix, l_Scale, l_Rotation, l_Translation,
                     l_Skew, l_Perspective);

      p_Position = Math::Vector2(l_Translation.x, l_Translation.y);
      p_Size = Math::Vector2(l_Scale.x, l_Scale.y);

      Math::Vector3 l_Euler = glm::eulerAngles(l_Rotation);
      p_RotationRadians = l_Euler.z;
    }

    void UiWidgetEditor::render(const float p_Delta)
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

      if (m_Mode == Mode::Widget) {
        if (l_Asset.get_controller().is_alive() &&
            l_Asset.get_controller().is_script_controller()) {
          ImGui::SameLine();
          Gui::VerticalSeparator();
          ImGui::SameLine();
          if (l_Asset.has_custom_controller()) {
            if (Gui::Button("Controller", false,
                            LOW_EDITOR_ICON_CONTROLLER,
                            theme_get_current().controller)) {
              m_Mode = Mode::Controller;
            }
          } else {
            if (Gui::Button("Open controller", false,
                            LOW_EDITOR_ICON_CONTROLLER,
                            theme_get_current().controller)) {
              // TODO: Don't switch type, just open controller in new
              // window
              m_Mode = Mode::Controller;
            }
          }
        }
      } else {
        ImGui::SameLine();
        Gui::VerticalSeparator();
        ImGui::SameLine();
        if (Gui::Button("Widget", false, LOW_EDITOR_ICON_ELEMENT,
                        theme_get_current().info)) {
          m_Mode = Mode::Widget;
        }
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
            if (draw_list_element(i_Display.get_element())) {
              break;
            }
          }
          // Make the whole remaining content region a drop zone
          ImVec2 l_CursorScreenPos = ImGui::GetCursorScreenPos();
          ImVec2 l_Avail = ImGui::GetContentRegionAvail();

          if (l_Avail.y < 20.0f) {
            l_Avail.y = 20.0f;
          }

          ImGui::InvisibleButton("##root_drop_target", l_Avail);

          if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *l_Payload =
                    ImGui::AcceptDragDropPayload("UI_ELEMENT")) {
              LOW_ASSERT(l_Payload->DataSize ==
                             sizeof(Core::UI::Element),
                         "Invalid drag payload size");

              Core::UI::Element l_DraggedElement =
                  *static_cast<const Core::UI::Element *>(
                      l_Payload->Data);

              l_DraggedElement.get_display().set_parent(
                  m_Viewport->m_Instance.get_root().get_display());
            }

            ImGui::EndDragDropTarget();
          }
        }
        ImGui::EndChild();

        // Bottom-left pane fills the rest
        ImGui::BeginChild("LeftBottom", ImVec2(0.0f, 0.0f),
                          ImGuiChildFlags_Borders);
        {
          if (m_DetailsSections.empty()) {
            if (!l_Asset.has_custom_controller() &&
                !l_Asset.get_controller().is_alive()) {
              show_editor(N(controller));
              ImGui::Dummy(ImVec2(0, 10.0f));
              if (Gui::Button("Create controller", false,
                              LOW_EDITOR_ICON_CONTROLLER,
                              theme_get_current().controller)) {
              }
            }
          } else {
            for (auto it = m_DetailsSections.begin();
                 it != m_DetailsSections.end(); ++it) {
              it->render(LOW_DELTA_TIME);
            }
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
      const float l_Delta = LOW_DELTA_TIME;
      const ImVec2 l_Avail = ImGui::GetContentRegionAvail();
      const ImVec2 l_CursorPos = ImGui::GetCursorScreenPos();

      if (m_Viewport->m_Instance.is_alive()) {
        Core::UI::Element l_Root = m_Viewport->m_Instance.get_root();
        Core::UI::Component::Display l_RootDisplay =
            l_Root.get_display();

        l_RootDisplay.pixel_position(l_Avail.x / 2.0f,
                                     l_Avail.y / 2.0f);
      }

      ImGuiIO &l_Io = ImGui::GetIO();

      if (m_Viewport->is_hovered()) {

        if (l_Io.MouseWheel != 0.0f) {
          float l_ZoomLevel =
              m_Viewport->m_RenderView.get_ui_camera_zoom();
          l_ZoomLevel += l_Io.MouseWheel * l_Delta * ZOOM_SPEED;
          m_Viewport->m_RenderView.set_ui_camera_zoom(l_ZoomLevel);
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          const ImVec2 l_MouseDelta = l_Io.MouseDelta;

          if (l_MouseDelta.x != 0.0f || l_MouseDelta.y != 0.0f) {
            const ImVec2 l_MoveDelta =
                l_MouseDelta * PAN_SPEED * l_Delta;
            Math::Vector2 l_Pan =
                m_Viewport->m_RenderView.get_ui_camera_position();
            l_Pan += Math::Vector2(-l_MoveDelta.x, l_MoveDelta.y);
            m_Viewport->m_RenderView.set_ui_camera_position(l_Pan);
          }
        }
      }

      m_Viewport->tick(LOW_DELTA_TIME);
      m_Viewport->set_dimensions(l_Avail.x, l_Avail.y);

      if (m_SelectedElement.is_alive()) {
        Core::UI::Component::Display l_Display =
            m_SelectedElement.get_display();
        Core::UI::Component::Display l_Parent =
            l_Display.get_parent();

        Math::Matrix4x4 l_ParentModel = build_ui_element_matrix(
            l_Parent.get_absolute_pixel_position(),
            l_Parent.get_absolute_rotation(),
            l_Parent.get_absolute_pixel_scale());

        Math::Matrix4x4 l_Model = build_ui_element_matrix(
            l_Display.get_absolute_pixel_position(),
            l_Display.get_absolute_rotation(),
            l_Display.get_absolute_pixel_scale());

        const Math::Matrix4x4 l_View =
            m_Viewport->m_RenderView.get_ui_view_matrix();
        const Math::Matrix4x4 l_Projection =
            m_Viewport->m_RenderView.get_ui_projection_matrix();

        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(l_CursorPos.x, l_CursorPos.y, l_Avail.x,
                          l_Avail.y);

        const bool l_Changed = ImGuizmo::Manipulate(
            &l_View[0][0], &l_Projection[0][0], ImGuizmo::TRANSLATE,
            ImGuizmo::WORLD, &l_Model[0][0], nullptr, nullptr);

        if (l_Changed) {
          Math::Vector2 l_Pos;
          Math::Vector2 l_Size;
          float l_Rot;

          // l_Model = glm::inverse(l_ParentModel) * l_Model;

          decompose_ui_element_matrix(l_Model, l_Pos, l_Rot, l_Size);
          l_Pos -= l_Parent.get_absolute_pixel_position();
          l_Rot -= l_Parent.get_absolute_rotation();

          l_Display.pixel_position(l_Pos);
          l_Display.pixel_scale(l_Size);
          l_Display.rotation(l_Rot);
        }
      }
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
