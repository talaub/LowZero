#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "IconsFontAwesome5.h"

namespace Flode {
  namespace BoolNodes {
    struct FLODE_API NotNode : public Node
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

    struct FLODE_API OrNode : public Node
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

    struct FLODE_API AndNode : public Node
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

    struct FLODE_API EqualsNode : public Node
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

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void on_pin_connected(Pin *p_Pin) override;

      virtual bool accept_dynamic_pin_connection(
          Pin *p_Pin, Pin *p_ConnectedPin) const override;

    protected:
      PinType m_PinType = PinType::Dynamic;
      u16 m_PinTypeId;
    };

    FLODE_API void register_nodes();
  } // namespace BoolNodes
} // namespace Flode
