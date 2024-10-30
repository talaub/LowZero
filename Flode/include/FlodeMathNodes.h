#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "IconsCodicons.h"
#include "IconsLucide.h"

namespace Flode {
  namespace MathNodes {
    struct FLODE_API AddNode : public Node
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
    };

    struct FLODE_API SubtractNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_LC_MINUS;
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

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void on_pin_connected(Pin *p_Pin) override;

      virtual bool accept_dynamic_pin_connection(
          Pin *p_Pin, Pin *p_ConnectedPin) const override;

    protected:
      PinType m_PinType = PinType::Dynamic;
    };

    struct FLODE_API MultiplyNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_LC_X;
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

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void on_pin_connected(Pin *p_Pin) override;

      virtual bool accept_dynamic_pin_connection(
          Pin *p_Pin, Pin *p_ConnectedPin) const override;

    protected:
      PinType m_PinType = PinType::Dynamic;
    };

    struct FLODE_API DivideNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override
      {
        return ICON_LC_DIVIDE;
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

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void on_pin_connected(Pin *p_Pin) override;

      virtual bool accept_dynamic_pin_connection(
          Pin *p_Pin, Pin *p_ConnectedPin) const override;

    protected:
      PinType m_PinType = PinType::Dynamic;
    };

    FLODE_API void register_nodes();
  } // namespace MathNodes
} // namespace Flode
