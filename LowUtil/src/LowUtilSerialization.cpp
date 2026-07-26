#include "LowUtilSerialization.h"

#include "LowMath.h"
#include "LowUtilAssert.h"
#include "LowUtilYaml.h"
#include "LowUtilVariant.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    namespace Serial {
      namespace {
        bool scalar_to_variant(const Node::Scalar &p_Scalar,
                               Variant &p_Variant)
        {
          if (std::holds_alternative<bool>(p_Scalar.value)) {
            p_Variant = std::get<bool>(p_Scalar.value);
            return true;
          }
          if (std::holds_alternative<float>(p_Scalar.value)) {
            p_Variant = std::get<float>(p_Scalar.value);
            return true;
          }
          if (std::holds_alternative<u64>(p_Scalar.value)) {
            p_Variant = std::get<u64>(p_Scalar.value);
            return true;
          }
          if (std::holds_alternative<i64>(p_Scalar.value)) {
            const i64 l_Value = std::get<i64>(p_Scalar.value);
            if (l_Value >= LOW_INT_MIN && l_Value <= LOW_INT_MAX) {
              p_Variant = static_cast<i32>(l_Value);
            } else if (l_Value >= 0) {
              p_Variant = static_cast<u64>(l_Value);
            } else {
              return false;
            }
            return true;
          }
          if (std::holds_alternative<String>(p_Scalar.value)) {
            p_Variant = std::get<String>(p_Scalar.value);
            return true;
          }

          return false;
        }

        bool has_field(const Node &p_Node, const char *p_Field)
        {
          return p_Node.find(p_Field) != nullptr;
        }

        bool only_has_fields(const Node &p_Node,
                             const char **p_Fields,
                             const u32 p_FieldCount)
        {
          if (!p_Node.is_dict() || p_Node.size() != p_FieldCount) {
            return false;
          }

          for (u32 i = 0; i < p_FieldCount; ++i) {
            if (!has_field(p_Node, p_Fields[i])) {
              return false;
            }
          }

          return true;
        }

        bool is_quaternion(const Math::Vector4 &p_Value)
        {
          const float l_MagnitudeSquared =
              (p_Value.x * p_Value.x) + (p_Value.y * p_Value.y) +
              (p_Value.z * p_Value.z) + (p_Value.w * p_Value.w);

          return Math::Util::abs(l_MagnitudeSquared - 1.0f) <= 0.001f;
        }

        bool vector_node_to_variant(const Node &p_Node,
                                    Variant &p_Variant)
        {
          if (p_Node.is_seq()) {
            if (p_Node.size() == 2) {
              p_Variant = p_Node.as<Math::Vector2>();
              return true;
            }
            if (p_Node.size() == 3) {
              p_Variant = p_Node.as<Math::Vector3>();
              return true;
            }
            if (p_Node.size() == 4) {
              Math::Vector4 l_Value;
              l_Value.x = p_Node[0].as<float>();
              l_Value.y = p_Node[1].as<float>();
              l_Value.z = p_Node[2].as<float>();
              l_Value.w = p_Node[3].as<float>();
              p_Variant = l_Value;
              return true;
            }

            return false;
          }

          if (!p_Node.is_dict()) {
            return false;
          }

          const char *l_Vector2Fields[] = {"x", "y"};
          if (only_has_fields(p_Node, l_Vector2Fields, 2)) {
            p_Variant = p_Node.as<Math::Vector2>();
            return true;
          }

          const char *l_Vector3Fields[] = {"x", "y", "z"};
          if (only_has_fields(p_Node, l_Vector3Fields, 3)) {
            p_Variant = p_Node.as<Math::Vector3>();
            return true;
          }

          const char *l_Vector4Fields[] = {"x", "y", "z", "w"};
          if (only_has_fields(p_Node, l_Vector4Fields, 4)) {
            Math::Vector4 l_Value = p_Node.as<Math::Vector4>();
            if (is_quaternion(l_Value)) {
              p_Variant = Math::Quaternion(l_Value.w, l_Value.x,
                                           l_Value.y, l_Value.z);
            } else {
              p_Variant = l_Value;
            }
            return true;
          }

          return false;
        }
      } // namespace

      Node load_yaml_file(const char *p_Path)
      {
        Yaml::Node l_YamlNode = Yaml::load_file(p_Path);
        Node l_Node;
        l_Node = l_YamlNode;
        return l_Node;
      }

      void write_yaml_file(const char *p_Path, Node &p_Node)
      {
        Yaml::Node l_YamlNode;
        l_YamlNode = p_Node;
        Yaml::write_file(p_Path, l_YamlNode);
      }

      void serialize_variant(Node &p_Node, Variant p_Variant)
      {
        if (p_Variant.m_Type == VariantType::Bool) {
          p_Node["type"] = "Bool";
          p_Node["value"] = p_Variant.m_Bool;
        } else if (p_Variant.m_Type == VariantType::Int32) {
          p_Node["type"] = "Int32";
          p_Node["value"] = p_Variant.m_Int32;
        } else if (p_Variant.m_Type == VariantType::UInt32) {
          p_Node["type"] = "UInt32";
          p_Node["value"] = p_Variant.m_Uint32;
        } else if (p_Variant.m_Type == VariantType::UInt64) {
          p_Node["type"] = "UInt64";
          p_Node["value"] = p_Variant.m_Uint64;
        } else if (p_Variant.m_Type == VariantType::Float) {
          p_Node["type"] = "Float";
          p_Node["value"] = p_Variant.m_Float;
        } else if (p_Variant.m_Type == VariantType::UVector2) {
          p_Node["type"] = "UVector2";
          p_Node["value"] = p_Variant.m_UVector2;
        } else if (p_Variant.m_Type == VariantType::Vector2) {
          p_Node["type"] = "Vector2";
          p_Node["value"] = p_Variant.m_Vector2;
        } else if (p_Variant.m_Type == VariantType::Vector3) {
          p_Node["type"] = "Vector3";
          p_Node["value"] = p_Variant.m_Vector3;
        } else if (p_Variant.m_Type == VariantType::Vector4) {
          p_Node["type"] = "Vector4";
          p_Node["value"] = p_Variant.m_Vector4;
        } else if (p_Variant.m_Type == VariantType::Quaternion) {
          p_Node["type"] = "Quaternion";
          p_Node["value"] = p_Variant.m_Quaternion;
        } else if (p_Variant.m_Type == VariantType::Name) {
          p_Node["type"] = "Name";
          p_Node["value"] = p_Variant.as_name();
        } else if (p_Variant.m_Type == VariantType::String) {
          p_Node["type"] = "String";
          p_Node["value"] = p_Variant.as_string();
        } else if (p_Variant.m_Type == VariantType::Handle) {
          Handle l_Handle = p_Variant.m_Uint64;
          if (l_Handle.is_registered_type()) {
            RTTI::TypeInfo &l_TypeInfo =
                Handle::get_type_info(l_Handle.get_type());
            LOW_ASSERT(l_TypeInfo.properties.find(N(unique_id)) !=
                           l_TypeInfo.properties.end(),
                       "Can only serialize handle variant where the "
                       "handle has a "
                       "unique_id");

            p_Node["type"] = "Handle";
            UniqueId l_UniqueId;
            l_TypeInfo.properties[N(unique_id)].get(l_Handle,
                                                    &l_UniqueId);
            p_Node["value"] = l_UniqueId;
          }
        } else {
          LOW_ASSERT(false, "Cannot serialize variant of this type");
        }
      }

      void serialize_enum(Node &p_Node, u16 p_EnumId, u8 p_EnumValue)
      {
        RTTI::EnumInfo &l_EnumInfo = Util::get_enum_info(p_EnumId);
        if (l_EnumInfo.identifier.name.m_Index != 0) {
          p_Node["enum_identifier"] =
              ((Util::String)l_EnumInfo.identifier).c_str();
        } else {
          p_Node["enum_id"] = p_EnumId;
        }
        p_Node["enum_value"] =
            l_EnumInfo.entry_name(p_EnumValue).c_str();
      }

      void serialize_handle(Node &p_Node, Handle p_Handle)
      {
        if (!p_Handle.is_registered_type()) {
          p_Node = false;
          return;
        }

        RTTI::TypeInfo &l_TypeInfo =
            Handle::get_type_info(p_Handle.get_type());
        if (l_TypeInfo.properties.find(N(unique_id)) !=
            l_TypeInfo.properties.end()) {
          // Type has unique id
          UniqueId l_UniqueId;
          l_TypeInfo.properties[N(unique_id)].get(p_Handle,
                                                  &l_UniqueId);

          p_Node["uniqueid"] = l_UniqueId;
        } else if (l_TypeInfo.properties.find(N(name)) !=
                   l_TypeInfo.properties.end()) {
          p_Node["typeid"] = l_TypeInfo.typeId;
          p_Node["name"] =
              ((Name *)l_TypeInfo.properties[N(name)].get_return(
                   p_Handle))
                  ->c_str();
        } else {
          LOW_ASSERT(false, "The type does not have sufficient "
                            "information to be serialized properly");
        }
      }

      Handle deserialize_handle(Node &p_Node)
      {
        if (!p_Node.is_dict()) {
          return 0ull;
        }
        if (p_Node["uniqueid"]) {
          return find_handle_by_unique_id(
              p_Node["uniqueid"].as<u64>());
        } else if (p_Node["name"]) {
          u16 l_TypeId = p_Node["typeid"].as<u16>();

          if (Handle::is_registered_type(l_TypeId)) {
            RTTI::TypeInfo &l_TypeInfo =
                Handle::get_type_info(l_TypeId);

            if (l_TypeInfo.find_by_name) {
              return l_TypeInfo.find_by_name(
                  p_Node["name"].as<Name>());
            }
          }
        }

        return 0ull;
      }

      Variant deserialize_variant(Node &p_Node)
      {
        if (p_Node["type"].as<String>() == "Bool") {
          return Variant(p_Node["value"].as<bool>());
        }
        if (p_Node["type"].as<String>() == "Int32") {
          return Variant(p_Node["value"].as<int>());
        }
        if (p_Node["type"].as<String>() == "UInt32") {
          return Variant(p_Node["value"].as<uint32_t>());
        }
        if (p_Node["type"].as<String>() == "UInt64") {
          return Variant(p_Node["value"].as<uint64_t>());
        }
        if (p_Node["type"].as<String>() == "Float") {
          return Variant(p_Node["value"].as<float>());
        }
        if (p_Node["type"].as<String>() == "UVector2") {
          return Variant(p_Node["value"].as<Math::UVector2>());
        }
        if (p_Node["type"].as<String>() == "Vector2") {
          return Variant(p_Node["value"].as<Math::Vector2>());
        }
        if (p_Node["type"].as<String>() == "Vector3") {
          return Variant(p_Node["value"].as<Math::Vector3>());
        }
        if (p_Node["type"].as<String>() == "Vector4") {
          return Variant(p_Node["value"].as<Math::Vector4>());
        }
        if (p_Node["type"].as<String>() == "Quaternion") {
          return Variant(p_Node["value"].as<Math::Quaternion>());
        }
        if (p_Node["type"].as<String>() == "Name") {
          return Variant(p_Node["value"].as<Name>());
        }
        if (p_Node["type"].as<String>() == "String") {
          return Variant(p_Node["value"].as<String>());
        }
        if (p_Node["type"].as<String>() == "Handle") {
          return Variant::from_handle(find_handle_by_unique_id(
              p_Node["value"].as<uint64_t>()));
        }

        LOW_ASSERT(false,
                   "Could not deserialize variant. Unknown type");
        return Variant(0);
      }

      bool node_to_variant(const Node &p_Node, Variant &p_Variant)
      {
        if (const Node::Scalar *l_Scalar =
                std::get_if<Node::Scalar>(&p_Node.data)) {
          return scalar_to_variant(*l_Scalar, p_Variant);
        }

        return vector_node_to_variant(p_Node, p_Variant);
      }

      Variant node_to_variant(const Node &p_Node)
      {
        Variant l_Variant;
        LOW_ASSERT(node_to_variant(p_Node, l_Variant),
                   "Could not convert node to variant");
        return l_Variant;
      }

      u8 deserialize_enum(Node &p_Node)
      {
        RTTI::EnumInfo *l_EnumInfo = nullptr;

        if (p_Node["enum_identifier"]) {
          TypeIdentifier l_Identifier = TypeIdentifier::from_string(
              p_Node["enum_identifier"].as<String>());
          l_EnumInfo = &get_enum_info(l_Identifier);
        } else {
          l_EnumInfo = &get_enum_info(p_Node["enum_id"].as<u16>());
        }

        return l_EnumInfo->entry_value(
            p_Node["enum_value"].as<Name>());
      }
      static void serialize_field(Node &p_Node,
                                    const RTTI::StructFieldInfo &p_Field,
                                    void *p_Instance)
      {
        void *l_FieldPtr =
            static_cast<char *>(p_Instance) + p_Field.offset;

        switch (p_Field.type) {
        case RTTI::PropertyType::FLOAT:
          p_Node = *static_cast<float *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::BOOL:
          p_Node = *static_cast<bool *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::INT:
          p_Node = *static_cast<int *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::UINT8:
          p_Node = *static_cast<u8 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::UINT16:
          p_Node = *static_cast<u16 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::UINT32:
          p_Node = *static_cast<u32 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::UINT64:
          p_Node = *static_cast<u64 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::NAME:
          p_Node = *static_cast<Name *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::STRING:
          p_Node = *static_cast<String *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::VECTOR2:
          p_Node = *static_cast<Math::Vector2 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::VECTOR3:
          p_Node = *static_cast<Math::Vector3 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::VECTOR4:
          p_Node = *static_cast<Math::Vector4 *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::QUATERNION:
          p_Node = *static_cast<Math::Quaternion *>(l_FieldPtr);
          break;
        case RTTI::PropertyType::HANDLE:
          serialize_handle(p_Node,
                           *static_cast<Handle *>(l_FieldPtr));
          break;
        case RTTI::PropertyType::ENUM:
          serialize_enum(p_Node,
                         get_enum_id(p_Field.referenced_type),
                         *static_cast<u8 *>(l_FieldPtr));
          break;
        case RTTI::PropertyType::STRUCT:
          serialize_struct(p_Node, p_Field.referenced_type,
                           l_FieldPtr);
          break;
        default:
          break;
        }
      }

      static void deserialize_field(Node &p_Node,
                                     const RTTI::StructFieldInfo &p_Field,
                                     void *p_Instance)
      {
        void *l_FieldPtr =
            static_cast<char *>(p_Instance) + p_Field.offset;

        switch (p_Field.type) {
        case RTTI::PropertyType::FLOAT:
          *static_cast<float *>(l_FieldPtr) = p_Node.as<float>();
          break;
        case RTTI::PropertyType::BOOL:
          *static_cast<bool *>(l_FieldPtr) = p_Node.as<bool>();
          break;
        case RTTI::PropertyType::INT:
          *static_cast<int *>(l_FieldPtr) = p_Node.as<int>();
          break;
        case RTTI::PropertyType::UINT8:
          *static_cast<u8 *>(l_FieldPtr) = p_Node.as<u8>();
          break;
        case RTTI::PropertyType::UINT16:
          *static_cast<u16 *>(l_FieldPtr) = p_Node.as<u16>();
          break;
        case RTTI::PropertyType::UINT32:
          *static_cast<u32 *>(l_FieldPtr) = p_Node.as<u32>();
          break;
        case RTTI::PropertyType::UINT64:
          *static_cast<u64 *>(l_FieldPtr) = p_Node.as<u64>();
          break;
        case RTTI::PropertyType::NAME:
          *static_cast<Name *>(l_FieldPtr) = p_Node.as<Name>();
          break;
        case RTTI::PropertyType::STRING:
          *static_cast<String *>(l_FieldPtr) = p_Node.as<String>();
          break;
        case RTTI::PropertyType::VECTOR2:
          *static_cast<Math::Vector2 *>(l_FieldPtr) =
              p_Node.as<Math::Vector2>();
          break;
        case RTTI::PropertyType::VECTOR3:
          *static_cast<Math::Vector3 *>(l_FieldPtr) =
              p_Node.as<Math::Vector3>();
          break;
        case RTTI::PropertyType::VECTOR4:
          *static_cast<Math::Vector4 *>(l_FieldPtr) =
              p_Node.as<Math::Vector4>();
          break;
        case RTTI::PropertyType::QUATERNION:
          *static_cast<Math::Quaternion *>(l_FieldPtr) =
              p_Node.as<Math::Quaternion>();
          break;
        case RTTI::PropertyType::HANDLE:
          *static_cast<Handle *>(l_FieldPtr) =
              deserialize_handle(p_Node);
          break;
        case RTTI::PropertyType::ENUM:
          *static_cast<u8 *>(l_FieldPtr) = deserialize_enum(p_Node);
          break;
        case RTTI::PropertyType::STRUCT:
          deserialize_struct(p_Node, l_FieldPtr);
          break;
        default:
          break;
        }
      }

      void serialize_struct(Node &p_Node, TypeIdentifier p_Identifier,
                            void *p_Instance)
      {
        RTTI::StructInfo &l_Info = get_struct_info(p_Identifier);

        p_Node["struct_type"] =
            ((String)l_Info.identifier).c_str();

        Node &l_Fields = p_Node["fields"];
        for (const RTTI::StructFieldInfo &i_Field : l_Info.fields) {
          serialize_field(l_Fields[i_Field.name.c_str()], i_Field,
                          p_Instance);
        }
      }

      void deserialize_struct(Node &p_Node, void *p_Instance)
      {
        LOW_ASSERT(p_Node["struct_type"] && p_Node["fields"],
                   "Node is not a serialized struct");

        TypeIdentifier l_Identifier = TypeIdentifier::from_string(
            p_Node["struct_type"].as<String>());
        RTTI::StructInfo &l_Info = get_struct_info(l_Identifier);

        Node &l_Fields = p_Node["fields"];
        for (const RTTI::StructFieldInfo &i_Field : l_Info.fields) {
          if (l_Fields[i_Field.name.c_str()]) {
            deserialize_field(l_Fields[i_Field.name.c_str()], i_Field,
                              p_Instance);
          }
        }
      }
    } // namespace Serial
  } // namespace Util
} // namespace Low
