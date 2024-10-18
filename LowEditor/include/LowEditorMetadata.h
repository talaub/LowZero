#pragma once

#include "LowUtilName.h"
#include "LowUtilHandle.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct PropertyMetadata
    {
      Util::Name name;
      bool editor;
      Util::RTTI::PropertyInfo propInfo;
      bool multiline;
      bool enumType;
      bool scriptingExpose;

      Util::String friendlyName;

      Util::String getterName;
      Util::String setterName;
    };

    struct ParameterMetadata
    {
      Util::Name name;
      Util::RTTI::ParameterInfo paramInfo;
      Util::String friendlyName;
    };

    struct FunctionMetadata
    {
      Util::Name name;
      Util::RTTI::FunctionInfo functionInfo;
      bool hideFlode;
      bool scriptingExpose;
      bool hasReturnValue;
      bool isStatic;

      Util::String friendlyName;

      Util::List<ParameterMetadata> parameters;
    };

    struct TypeEditorMetadata
    {
      bool manager;
      bool saveable;
    };

    struct TypeMetadata
    {
      Util::Name name;
      Util::String module;
      Util::List<Util::String> namespaces;
      Util::String namespaceString;
      Util::String fullTypeString;
      uint16_t typeId;
      Util::RTTI::TypeInfo typeInfo;
      Util::List<PropertyMetadata> properties;
      TypeEditorMetadata editor;
      bool scriptingExpose;
      Util::List<FunctionMetadata> functions;

      Util::String friendlyName;

      PropertyMetadata find_property_by_name(Util::Name p_Name) const;
    };

    struct EnumEntryMetadata
    {
      Util::Name name;
    };

    struct EnumMetadata
    {
      Util::Name name;
      Util::List<Util::String> namespaces;
      Util::String namespaceString;
      Util::String fullTypeString;
      Util::String module;
      Util::List<EnumEntryMetadata> options;
      u16 enumId;
    };
  } // namespace Editor
} // namespace Low
