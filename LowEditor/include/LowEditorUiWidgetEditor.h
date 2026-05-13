#pragma once

#include "LowCoreUiElement.h"
#include "LowCoreUiWidgetInstance.h"
#include "LowCoreUiDisplay.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorViewport.h"
#include "LowEditorHandlePropertiesSection.h"

#include "LowCoreUiWidgetAsset.h"
#include "LowMath.h"
#include "LowRendererUiDrawCommand.h"
#include "LowUtilLogger.h"

#include "LowEditorVisualScriptEditor.h"

namespace Low {
  namespace Editor {
    struct UiWidgetEditor;
    struct UiWidgetInteractiveViewport : public UiViewport
    {
      friend struct UiWidgetEditor;
      UiWidgetInteractiveViewport(Core::UI::WidgetAsset p_Asset,
                                  const Math::UVector2 p_Dimensions)
          : UiViewport(p_Dimensions), m_Asset(p_Asset)
      {
        m_Instance = m_Asset.spawn_instance(m_Canvas);
      }
      ~UiWidgetInteractiveViewport() override
      {
        m_Instance.destroy();
      }

    protected:
      Core::UI::WidgetAsset m_Asset;
      Core::UI::WidgetInstance m_Instance;
    };

    struct UiWidgetEditor : public TypeEditor
    {
      UiWidgetEditor(Util::Handle p_Handle);
      ~UiWidgetEditor();

      enum class Mode
      {
        Widget,
        Controller
      };

      void render(const float p_Delta) override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{1200, 800};
      }

    private:
      void render_viewport();

    protected:
      Core::UI::Element create_element(const Util::Name p_Name,
                                       Renderer::UiCanvas p_Canvas,
                                       Core::UI::Element p_Parent);

      UiWidgetInteractiveViewport *m_Viewport;

      void create_local_controller();

      float m_LeftPaneWidth;
      float m_TopPaneHeight;

      VisualScript::Document m_CustomControllerDocument;
      VisualScript::Editor m_VisualScriptEditor;

      Mode m_Mode = Mode::Widget;

      char *m_ElementSearch;

      bool m_CreatedLocalController = false;

      Core::UI::Element m_SelectedElement;

      Renderer::UiDrawCommand m_SelectedMarker;

      bool draw_list_element(Core::UI::Element p_Element);

      void add_section(Util::Handle p_Handle);
      Util::List<HandlePropertiesSection> m_DetailsSections;

      void set_selected_element(Core::UI::Element p_Element);
    };
  } // namespace Editor
} // namespace Low
