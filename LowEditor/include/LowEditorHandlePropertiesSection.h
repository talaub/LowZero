#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    struct HandlePropertiesSection : public Widget
    {
      HandlePropertiesSection(const Util::Handle p_Handle,
                              bool p_DefaultOpen = false);
      void render(float p_Delta) override;

      void (*render_footer)(const Util::Handle, Util::RTTI::TypeInfo &);

    private:
      const Util::Handle m_Handle;
      Util::RTTI::TypeInfo m_TypeInfo;
      bool m_DefaultOpen;

      void render_default(float p_Delta);
      void render_material(float p_Delta);
    };
  } // namespace Editor
} // namespace Low
