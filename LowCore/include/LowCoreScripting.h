#pragma once

#include "LowCoreApi.h"

#include "LowCoreScriptModule.h"
#include "LowCoreScriptClassInstance.h"
#include <vulkan/vulkan_core.h>
#include "LowCoreScriptAsset.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      enum class EventType
      {
        Error,
        Info,
        Warn
      };

      struct EventMessage
      {
        EventType type;
        int col;
        int row;
        Util::String msg;
        ScriptAsset script;
      };

      enum class TypeKind
      {
        Void,
        Bool,
        Int32,
        UInt32,
        UInt64,
        Float,
        String,
        Name,

        Handle,
        Enum
      };
      enum class TypeContainer
      {
        None,
        List,
      };

      struct TypeInfo
      {
        TypeKind kind = TypeKind::Void;

        Util::TypeIdentifier referenced_type;

        u8 pointer_level = 0;
        bool constant = false;

        bool reference = false;

        TypeContainer container = TypeContainer::None;
      };

      struct FunctionParameterInfo
      {
        Util::Name name;
        TypeInfo type;
      };

      struct VisualScriptFunctionInfo
      {};

      struct FunctionInfo
      {
        Util::Name name;
        Util::Name bind_name;
        Util::String bind_namespace = "";

        TypeInfo return_type;
        Util::List<FunctionParameterInfo> parameters;

        bool is_property;

        void *ptr;

        VisualScriptFunctionInfo visual_script_info;
      };

      struct EnumInfo
      {
        Util::Name bind_name;
        Util::String bind_namespace = "";
        Util::TypeIdentifier identifier;
        void *entry_name_ptr;
        void *entry_value_ptr;
      };

      void LOW_CORE_API initialize_as();
      void LOW_CORE_API cleanup_as();
      void LOW_CORE_API test_as();

      void LOW_CORE_API tick_as(const float p_Delta);

      void LOW_CORE_API build_module(Module p_Module);
      bool LOW_CORE_API fill_member_fields(ClassInstance p_Instance);

      struct StructInfo
      {
        Util::Name bind_name;
        Util::String bind_namespace = "";
        Util::TypeIdentifier identifier;
        void *default_constructor;
        void *copy_constructor;
        void *destructor;
        void *assign;
      };

      void LOW_CORE_API
      register_function(const FunctionInfo &p_FunctionInfo);
      void LOW_CORE_API
      register_enum(const EnumInfo &p_EnumInfo);
      void LOW_CORE_API
      register_struct(const StructInfo &p_StructInfo);
    } // namespace Scripting
  } // namespace Core
} // namespace Low
