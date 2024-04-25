#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "LowUtilHandle.h"

namespace Flode {
  namespace HandleNodes {
    struct FLODE_API TypeIdNode : public Node
    {
      TypeIdNode(u16 p_TypeId);

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

    protected:
      Low::Editor::TypeMetadata m_TypeMetadata;

      Low::Util::String m_CachedName;
    };

    struct FLODE_API FindByNameNode : public Node
    {
      FindByNameNode(u16 p_TypeId);

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;

    protected:
      Low::Editor::TypeMetadata m_TypeMetadata;

      Low::Util::String m_CachedName;
    };

    struct FLODE_API GetNode : public Node
    {
      GetNode(u16 p_TypeId, Low::Util::Name p_PropertyName);

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;

    protected:
      Low::Editor::TypeMetadata m_TypeMetadata;
      Low::Editor::PropertyMetadata m_PropertyMetadata;

      Low::Util::String m_CachedName;
    };

    struct FLODE_API SetNode : public Node
    {
      SetNode(u16 p_TypeId, Low::Util::Name p_PropertyName);

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;

    protected:
      Low::Editor::TypeMetadata m_TypeMetadata;
      Low::Editor::PropertyMetadata m_PropertyMetadata;

      Low::Util::String m_CachedName;
    };

    struct FLODE_API FunctionNode : public Node
    {
      FunctionNode(u16 p_TypeId, Low::Util::Name p_FunctionName);

      Low::Util::String get_name(NodeNameType p_Type) const override;

      virtual ImU32 get_color() const override;

      virtual void setup_default_pins() override;

      virtual void
      compile(Low::Util::StringBuilder &p_Builder) const override;
      virtual void
      compile_output_pin(Low::Util::StringBuilder &p_Builder,
                         NodeEd::PinId p_PinId) const override;

    protected:
      Low::Editor::TypeMetadata m_TypeMetadata;
      Low::Editor::FunctionMetadata m_FunctionMetadata;

      Low::Util::String m_CachedName;
    };
  } // namespace HandleNodes
} // namespace Flode
