#pragma once

#include "LowCoreUiElement.h"
#include "LowCoreUiWidgetInstance.h"
#include "LowEditorTypeEditor.h"
#include "LowEditorViewport.h"
#include "LowEditorHandlePropertiesSection.h"

#include "LowCoreUiWidgetAsset.h"
#include "LowMath.h"
#include "LowRendererUiDrawCommand.h"

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
      ~UiWidgetInteractiveViewport()
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

      void render() override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{1200, 800};
      }

    private:
      void render_viewport();

    protected:
      UiWidgetInteractiveViewport *m_Viewport;

      float m_LeftPaneWidth;
      float m_TopPaneHeight;
      bool m_Test;

      char *m_ElementSearch;

      Core::UI::Element m_SelectedElement;

      Renderer::UiDrawCommand m_SelectedMarker;

      bool draw_list_element(Core::UI::Element p_Element);

      void add_section(Util::Handle p_Handle);
      Util::List<HandlePropertiesSection> m_DetailsSections;

      void set_selected_element(Core::UI::Element p_Element);
    };
  } // namespace Editor
} // namespace Low
