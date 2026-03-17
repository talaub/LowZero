#include "LowEditorEditWidget.h"

#include "LowEditor.h"

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
    }

    void EditWidget::render(float p_Delta)
    {
      ImGui::Begin(m_Title.c_str());
      ImGui::Text("This is somethign i display lol");
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
