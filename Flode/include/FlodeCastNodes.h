#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace CastNodes {
    struct FLODE_API CastNumberToString : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_FA_ARROWS_ALT_H "";
      }

      virtual void setup_default_pins() override;

      virtual bool is_compact() const override
      {
        return true;
      }

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;
    };

    FLODE_API void register_nodes();
  } // namespace CastNodes
} // namespace Flode
