#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowEditorHandlePropertiesSection.h"
#include "imgui.h"

namespace Low {
  namespace Editor {
    struct TypeManagerWidget : public Widget
    {
      TypeManagerWidget(uint16_t p_TypeId);
      void render(float p_Delta) override;

    private:
      Util::RTTI::TypeInfo m_TypeInfo;
      ImGuiID m_DockSpaceId;
      bool m_LayoutConstructed;
      Util::String m_ListWindowName;
      Util::String m_InfoWindowName;
      Util::Handle m_Selected;

      Util::RTTI::PropertyInfo m_NamePropertyInfo;
      Util::List<HandlePropertiesSection> m_Sections;

      void render_list(float p_Delta);
      void render_info(float p_Delta);

      void select(Util::Handle p_Handle);
    };
  } // namespace Editor
} // namespace Low
