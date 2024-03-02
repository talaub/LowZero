#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowEditorHandlePropertiesSection.h"
#include "imgui.h"

namespace Low {
  namespace Editor {
    struct TestManagerWidget : public Widget
    {
      TestManagerWidget(uint16_t p_TypeId);
      void render(float p_Delta) override;

    private:
      Util::RTTI::TypeInfo m_TypeInfo;
      ImGuiID m_DockSpaceId;
      bool m_LayoutConstructed;
      Util::String m_ListWindowName;
      Util::String m_InfoWindowName;
    };
  } // namespace Editor
} // namespace Low
