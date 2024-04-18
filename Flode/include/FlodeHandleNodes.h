#pragma once

#include "FlodeApi.h"

#include "Flode.h"

#include "LowUtilHandle.h"

namespace Flode {
  namespace HandleNodes {
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
      Low::Util::RTTI::TypeInfo m_TypeInfo;

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
      Low::Util::RTTI::TypeInfo m_TypeInfo;
      Low::Util::RTTI::PropertyInfo m_PropertyInfo;

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
      Low::Util::RTTI::TypeInfo m_TypeInfo;
      Low::Util::RTTI::PropertyInfo m_PropertyInfo;

      Low::Util::String m_CachedName;
    };
  } // namespace HandleNodes
} // namespace Flode
