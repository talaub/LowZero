#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace MathNodes {
    struct FLODE_API AddNumberNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual bool is_compact() const override
      {
        return true;
      }

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;
    };

    struct FLODE_API SubtractNumberNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_FA_MINUS;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual bool is_compact() const override
      {
        return true;
      }

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;
    };

    struct FLODE_API MultiplyNumberNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_FA_TIMES;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual bool is_compact() const override
      {
        return true;
      }

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;
    };

    struct FLODE_API DivideNumberNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_FA_DIVIDE;
      }

      virtual ImU32 get_color() const override;

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
  } // namespace MathNodes
} // namespace Flode
