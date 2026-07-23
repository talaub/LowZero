#pragma once

#include "LowEditorWidget.h"
#include "LowCoreNavigation.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    struct NavigationDebugWidget : public Widget
    {
      void render(float p_Delta) override;

    private:
      bool m_BuildAttempted = false;
      bool m_LastBuildSucceeded = false;
      u32 m_LastVertexCount = 0u;
      u32 m_LastTriangleCount = 0u;
      bool m_BuildSettingsInitialized = false;
      Low::Core::Navigation::BuildSettings m_BuildSettings;
      Low::Util::String m_LastBuildMessage;
      Low::Util::String m_SettingsSaveMessage;
      Low::Math::Vector3 m_PathStart = Low::Math::Vector3(0.0f);
      Low::Math::Vector3 m_PathEnd = Low::Math::Vector3(0.0f);
      Low::Math::Vector3 m_PathHalfExtents =
          Low::Math::Vector3(1.0f, 2.0f, 1.0f);
      Low::Core::Navigation::PathResult m_LastPathResult;
      bool m_PathQueryAttempted = false;
      bool m_LastPathQuerySucceeded = false;
      Low::Util::String m_LastPathQueryMessage;
    };
  } // namespace Editor
} // namespace Low
