#include "LowUtilSerialization.h"

#include "LowMath.h"
#include "LowUtilAssert.h"
#include "LowUtilYaml.h"
#include "LowUtilVariant.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    namespace Serial {
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
        p_Node["enum_id"] = p_EnumId;
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

      u8 deserialize_enum(Node &p_Node)
      {
        RTTI::EnumInfo &l_EnumInfo =
            get_enum_info(p_Node["enum_id"].as<u16>());
        return l_EnumInfo.entry_value(
            p_Node["enum_value"].as<Name>());
      }
    } // namespace Serial
  } // namespace Util
} // namespace Low
