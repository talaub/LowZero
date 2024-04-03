#pragma once

#include "FlodeApi.h"

#include "Flode.h"

namespace Flode {
  namespace DebugNodes {
    struct FLODE_API LogNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override;

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
