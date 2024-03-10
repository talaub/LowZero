#include "LowUtilSerialization.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Util {
    namespace Serialization {
      void serialize(Yaml::Node &p_Node, Math::Quaternion &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
        p_Node["w"] = p_Value.w;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector4 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
        p_Node["a"] = p_Value.a;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector3 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
      }

      void serialize(Yaml::Node &p_Node, Math::Vector2 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
      }

      void serialize(Yaml::Node &p_Node, Math::UVector2 &p_Value)
      {
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
      }

      void serialize(Yaml::Node &p_Node, Math::Box &p_Box)
      {
        serialize(p_Node["position"], p_Box.position);
        serialize(p_Node["rotation"], p_Box.rotation);
        serialize(p_Node["half_extents"], p_Box.halfExtents);
      }

      void serialize(Yaml::Node &p_Node, Math::Shape &p_Shape)
      {
        if (p_Shape.type == Math::ShapeType::BOX) {
          serialize(p_Node["box"], p_Shape.box);
        } else {
          LOW_ASSERT(false,
                     "Serialization of shape type not supported");
        }
      }

      void serialize_variant(Yaml::Node &p_Node, Variant p_Variant)
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
          serialize(p_Node["value"], p_Variant.m_UVector2);
        } else if (p_Variant.m_Type == VariantType::Vector2) {
          p_Node["type"] = "Vector2";
          serialize(p_Node["value"], p_Variant.m_Vector2);
        } else if (p_Variant.m_Type == VariantType::Vector3) {
          p_Node["type"] = "Vector3";
          serialize(p_Node["value"], p_Variant.m_Vector3);
        } else if (p_Variant.m_Type == VariantType::Vector4) {
          p_Node["type"] = "Vector4";
          serialize(p_Node["value"], p_Variant.m_Vector4);
        } else if (p_Variant.m_Type == VariantType::Quaternion) {
          p_Node["type"] = "Quaternion";
          serialize(p_Node["value"], p_Variant.m_Quaternion);
        } else if (p_Variant.m_Type == VariantType::Name) {
          p_Node["type"] = "Name";
          p_Node["value"] = ((Name)p_Variant).c_str();
        } else if (p_Variant.m_Type == VariantType::Handle) {
          Handle l_Handle = p_Variant.m_Uint64;
          RTTI::TypeInfo &l_TypeInfo =
              Handle::get_type_info(l_Handle.get_type());
          LOW_ASSERT(l_TypeInfo.properties.find(N(unique_id)) !=
                         l_TypeInfo.properties.end(),
                     "Can only serialize handle variant where the "
                     "handle has a "
                     "unique_id");

          p_Node["type"] = "Handle";
          p_Node["value"] =
              *(UniqueId *)l_TypeInfo.properties[N(unique_id)].get(
                  l_Handle);
        } else {
          LOW_ASSERT(false, "Cannot serialize variant of this type");
        }
      }

      Math::Quaternion deserialize_quaternion(Yaml::Node &p_Node)
      {
        Math::Quaternion l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();
        l_Result.w = p_Node["w"].as<float>();

        return l_Result;
      }

      Math::Vector4 deserialize_vector4(Yaml::Node &p_Node)
      {
        Math::Vector4 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();
        l_Result.a = p_Node["a"].as<float>();

        return l_Result;
      }

      Math::Vector3 deserialize_vector3(Yaml::Node &p_Node)
      {
        Math::Vector3 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();
        l_Result.z = p_Node["z"].as<float>();

        return l_Result;
      }

      Math::Vector2 deserialize_vector2(Yaml::Node &p_Node)
      {
        Math::Vector2 l_Result;
        l_Result.x = p_Node["x"].as<float>();
        l_Result.y = p_Node["y"].as<float>();

        return l_Result;
      }

      Math::UVector2 deserialize_uvector2(Yaml::Node &p_Node)
      {
        Math::UVector2 l_Result;
        l_Result.x = p_Node["x"].as<uint32_t>();
        l_Result.y = p_Node["y"].as<uint32_t>();

        return l_Result;
      }

      Math::Box deserialize_box(Yaml::Node &p_Node)
      {
        Math::Box l_Result;

        l_Result.position = deserialize_vector3(p_Node["position"]);
        l_Result.rotation =
            deserialize_quaternion(p_Node["rotation"]);
        l_Result.halfExtents =
            deserialize_vector3(p_Node["half_extents"]);

        return l_Result;
      }

      Math::Shape deserialize_shape(Yaml::Node &p_Node)
      {
        Math::Shape l_Shape;
        if (p_Node["box"]) {
          l_Shape.type = Math::ShapeType::BOX;
          l_Shape.box = deserialize_box(p_Node["box"]);
        } else {
          LOW_ASSERT(false, "Could not deserialize shape");
        }

        return l_Shape;
      }

      Variant deserialize_variant(Yaml::Node &p_Node)
      {
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Bool") {
          return Variant(p_Node["value"].as<bool>());
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Int32") {
          return Variant(p_Node["value"].as<int>());
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "UInt32") {
          return Variant(p_Node["value"].as<uint32_t>());
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "UInt64") {
          return Variant(p_Node["value"].as<uint64_t>());
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Float") {
          return Variant(p_Node["value"].as<float>());
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "UVector2") {
          return Variant(deserialize_uvector2(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Vector2") {
          return Variant(deserialize_vector2(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Vector3") {
          return Variant(deserialize_vector3(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Vector4") {
          return Variant(deserialize_vector4(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Quaternion") {
          return Variant(deserialize_quaternion(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Name") {
          return Variant(LOW_YAML_AS_NAME(p_Node["value"]));
        }
        if (LOW_YAML_AS_STRING(p_Node["type"]) == "Handle") {
          return Variant::from_handle(find_handle_by_unique_id(
              p_Node["value"].as<uint64_t>()));
        }

        LOW_ASSERT(false,
                   "Could not deserialize variant. Unknown type");
      }
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
