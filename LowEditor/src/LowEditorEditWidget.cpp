#include "LowEditorEditWidget.h"

#include "LowEditor.h"
#include "LowMath.h"
#include <imgui.h>

namespace Low {
  namespace Editor {
    EditWidget::EditWidget(const Util::Handle p_Handle)
        : m_Handle(p_Handle),
          m_Metadata(get_type_metadata(p_Handle.get_type()))

    {
      m_Title = ((Util::Name *)m_Metadata.typeInfo.properties[N(name)]
                     .get_return(p_Handle))
                    ->c_str();
      m_Title += " (";
      m_Title += m_Metadata.name.c_str();
      m_Title += ")";

      m_Editor = TypeEditor::create(p_Handle);
    }

    EditWidget::~EditWidget()
    {
      delete m_Editor;
    }

    void EditWidget::render(float p_Delta)
    {
      const Math::UVector2 l_Dimensions =
          m_Editor->get_edit_widget_dimensions();
      ImGui::SetNextWindowSize(ImVec2(l_Dimensions.x, l_Dimensions.y),
                               ImGuiCond_FirstUseEver);
      ImGui::Begin(m_Title.c_str(), &m_Open,
                   ImGuiWindowFlags_NoSavedSettings);
      m_Editor->render();
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
