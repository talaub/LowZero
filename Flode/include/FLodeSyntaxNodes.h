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
        u16 typeId;
        NodeEd::PinId pinId;
      };

      FunctionNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;
      Low::Util::String
      get_subtitle(NodeNameType p_Type) const override;

      Low::Util::String get_icon() const override
      {
        return ICON_CI_DEBUG_START;
      }

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

      Low::Util::List<FunctionNodeParameter> m_Parameters;
      Low::Util::Name m_Name;
      bool m_Editable;

      bool m_HasReturnValue;
      PinType m_ReturnType;
      u16 m_ReturnTypeId;

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

      virtual void set_variable(Variable *p_Variable);

    protected:
      Variable *m_Variable;

      void create_dynamic_pins();
    };

    struct FLODE_API SetVariableNode : public Node
    {
      SetVariableNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      Low::Util::String
      get_subtitle(NodeNameType p_Type) const override
      {
        return "Variable";
      }

      Low::Util::String get_icon() const override
      {
        return ICON_CI_SYMBOL_VARIABLE;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void render_data() override;

      virtual void
      serialize(Low::Util::Yaml::Node &p_Node) const override;
      virtual void
      deserialize(Low::Util::Yaml::Node &p_Node) override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

      virtual void set_variable(Variable *p_Variable);

    protected:
      Variable *m_Variable;

      void create_dynamic_pins();
    };

    struct FLODE_API ReturnNumberNode : public Node
    {
      ReturnNumberNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;
    };

    struct FLODE_API ReturnBoolNode : public Node
    {
      ReturnBoolNode();

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;
    };

    struct FLODE_API IfNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override;

      Low::Util::String
      get_subtitle(NodeNameType p_Type) const override
      {
        return "Branch";
      }

      Low::Util::String get_icon() const override
      {
        return ICON_CI_GIT_BRANCH;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;
    };

    struct FLODE_API ForNode : public Node
    {
      Low::Util::String get_name(NodeNameType p_Type) const override;
      Low::Util::String
      get_subtitle(NodeNameType p_Type) const override
      {
        return "Loop";
      }

      Low::Util::String get_icon() const override
      {
        return ICON_CI_DEBUG_RESTART;
      }

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;
    };

    FLODE_API void register_nodes();
  } // namespace SyntaxNodes
} // namespace Flode
