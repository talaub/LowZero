#include "LowCoreScripting.h"

#include "LowUtilAssert.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      Util::String parameter_direction_to_string(ParameterDirection p_Direction)
      {
        switch (p_Direction) {
        case ParameterDirection::In:
          return "in";
        case ParameterDirection::Out:
          return "out";
        case ParameterDirection::InOut:
          return "inout";
        default:
          return "";
        }
      }

      Util::String simple_type_kind_to_string(const TypeKind p_Kind)
      {
        static Util::String l_Float = "float";
        static Util::String l_Void = "void";
        static Util::String l_Int32 = "int";
        static Util::String l_UInt32 = "u32";
        static Util::String l_UInt64 = "u64";
        static Util::String l_String = "string";
        static Util::String l_Name = "Name";
        static Util::String l_Handle = "Handle";
        static Util::String l_Bool = "bool";

        switch (p_Kind) {
        case TypeKind::Void:
          return l_Void;
        case TypeKind::Bool:
          return l_Bool;
        case TypeKind::Int32:
          return l_Int32;
        case TypeKind::UInt32:
          return l_UInt32;
        case TypeKind::UInt64:
          return l_UInt64;
        case TypeKind::Float:
          return l_Float;
        case TypeKind::String:
          return l_String;
        case TypeKind::Name:
          return l_String;
        case TypeKind::Handle:
          return l_Handle;
        }

        LOW_ASSERT(false, "Unsupported type kind.");
      }

      void produce_type_info_string(Util::StringBuilder &p_Builder,
                                    const TypeInfo &p_Type)
      {
        const bool l_IsContainer =
            p_Type.container != TypeContainer::None;

        if (p_Type.constant) {
          p_Builder.append("const ");
        }

        if (p_Type.container == TypeContainer::List) {
          p_Builder.append("array<");
        }

        if (p_Type.as_type.length()) {
          p_Builder.append(p_Type.as_type);
        } else if (p_Type.kind == TypeKind::Handle) {
          p_Builder.append(simple_type_kind_to_string(p_Type.kind));
        } else if (p_Type.kind == TypeKind::Enum) {

        } else {
          p_Builder.append(simple_type_kind_to_string(p_Type.kind));
        }

        if (l_IsContainer) {
          p_Builder.append(">");
        }

        for (int i = 0; i < p_Type.pointer_level; ++i) {
          p_Builder.append(l_IsContainer ? "@" : "*");
        }

        if (p_Type.reference) {
          p_Builder.append(" &");
          const Util::String l_Dir =
              parameter_direction_to_string(p_Type.direction);
          if (l_Dir.length()) {
            p_Builder.append(l_Dir);
          }
        }
      }

      void produce_as_function_signature(
          Util::StringBuilder &p_Builder,
          const FunctionInfo &p_FunctionInfo)
      {
        produce_type_info_string(p_Builder,
                                 p_FunctionInfo.return_type);
        p_Builder.append(" ")
            .append(p_FunctionInfo.bind_name)
            .append("(");

        int i = 0;
        for (const FunctionParameterInfo &i_Param :
             p_FunctionInfo.parameters) {
          if (i) {
            p_Builder.append(", ");
          }

          produce_type_info_string(p_Builder, i_Param.type);

          i++;
        }

        p_Builder.append(")");
        if (p_FunctionInfo.is_property) {
          p_Builder.append(" property");
        }
      }
    } // namespace Scripting
  } // namespace Core
} // namespace Low
