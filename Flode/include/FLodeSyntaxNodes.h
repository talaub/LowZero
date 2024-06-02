#pragma once

#include "FlodeApi.h"

#include "Flode.h"

namespace Flode {
  namespace SyntaxNodes {
    struct FLODE_API FunctionNode : public Node
    {
      struct FunctionNodeParameter
      {
        Low::Util::Name name;
        PinType type;
        NodeEd::PinId pinId;
        u16 typeId;
      };

      FunctionNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void render_data() override;

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;

    protected:
      Low::Util::List<FunctionNodeParameter> m_Parameters;
      Low::Util::Name m_Name;

      void create_dynamic_pins();
    };

    struct FLODE_API GetVariableNode : public Node
    {
      GetVariableNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual bool is_compact() const override
      {
        return true;
      }

      virtual void setup_default_pins() override;

      virtual void render_data() override;

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;

    protected:
      Variable *m_Variable;

      void create_dynamic_pins();
    };

    struct FLODE_API SetVariableNode : public Node
    {
      SetVariableNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void render_data() override;

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

    protected:
      Variable *m_Variable;

      void create_dynamic_pins();
    };

    FLODE_API void register_nodes();
  } // namespace SyntaxNodes
} // namespace Flode
