#pragma once

#include "FlodeApi.h"

#include "Flode.h"

namespace Flode {
  namespace DebugNodes {
    struct FLODE_API LogNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override;
      Low::Util::String
      get_subtitle(NodeNameType p_Type) const override
      {
        return "Debug";
      }

      Low::Util::String get_icon() const override
      {
        return ICON_CI_BUG;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

    protected:
      Pin *m_MessagePin;
    };

    FLODE_API void register_nodes();
  } // namespace DebugNodes
} // namespace Flode
