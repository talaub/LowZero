#include "LowUtilSerialization.h"

#include "LowUtilAssert.h"

#define SHORT_VECTORS 1

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
#if SHORT_VECTORS
        p_Node.push_back(p_Value.x);
        p_Node.push_back(p_Value.y);
        p_Node.push_back(p_Value.z);
#else
        p_Node["x"] = p_Value.x;
        p_Node["y"] = p_Value.y;
        p_Node["z"] = p_Value.z;
#endif
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

      void serialize(Yaml::Node &p_Node, Math::AABB &p_AABB)
      {
        serialize(p_Node["bounds"], p_AABB.bounds);
        serialize(p_Node["center"], p_AABB.center);
      }

      void serialize(Yaml::Node &p_Node, Math::Bounds &p_Bounds)
      {
        serialize(p_Node["min"], p_Bounds.min);
        serialize(p_Node["max"], p_Bounds.max);
      }

      void serialize(Yaml::Node &p_Node, Math::Sphere &p_Sphere)
      {
        serialize(p_Node["position"], p_Sphere.position);
        p_Node["radius"] = p_Sphere.radius;
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
          UniqueId l_UniqueId;
          l_TypeInfo.properties[N(unique_id)].get(l_Handle,
                                                  &l_UniqueId);
          p_Node["value"] = l_UniqueId;
        } else {
          LOW_ASSERT(false, "Cannot serialize variant of this type");
        }
      }

      void serialize_enum(Yaml::Node &p_Node, u16 p_EnumId,
                          u8 p_EnumValue)
      {
        RTTI::EnumInfo &l_EnumInfo = Util::get_enum_info(p_EnumId);
        p_Node["enum_id"] = p_EnumId;
        p_Node["enum_value"] =
            l_EnumInfo.entry_name(p_EnumValue).c_str();
      }

      void serialize_handle(Yaml::Node &p_Node, Handle p_Handle)
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

      Handle deserialize_handle(Yaml::Node &p_Node)
      {
        if (!p_Node.IsMap()) {
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
                  LOW_YAML_AS_NAME(p_Node["name"]));
            }
          }
        }

        return 0ull;
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
        if (p_Node.IsSequence()) {
          l_Result.x = p_Node[0].as<float>();
          l_Result.y = p_Node[1].as<float>();
          l_Result.z = p_Node[2].as<float>();
        } else {
          l_Result.x = p_Node["x"].as<float>();
          l_Result.y = p_Node["y"].as<float>();
          l_Result.z = p_Node["z"].as<float>();
        }

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

      Math::Bounds deserialize_bounds(Yaml::Node &p_Node)
      {
        Math::Bounds l_Bounds;

        l_Bounds.min = deserialize_vector3(p_Node["min"]);
        l_Bounds.max = deserialize_vector3(p_Node["max"]);

        return l_Bounds;
      }

      Math::AABB deserialize_aabb(Yaml::Node &p_Node)
      {
        Math::AABB l_AABB;

        l_AABB.center = deserialize_vector3(p_Node["center"]);
        l_AABB.bounds = deserialize_bounds(p_Node["bounds"]);

        return l_AABB;
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

      Math::Sphere deserialize_sphere(Yaml::Node &p_Node)
      {
        Math::Sphere l_Result;

        l_Result.position = deserialize_vector3(p_Node["position"]);
        l_Result.radius = p_Node["radius"].as<float>();

        return l_Result;
      }

      Math::Shape deserialize_shape(Yaml::Node &p_Node)
      {
        Math::Shape l_Shape;
        if (p_Node["box"]) {
          l_Shape.type = Math::ShapeType::BOX;
          l_Shape.box = deserialize_box(p_Node["box"]);
        } else if (p_Node["sphere"]) {
          l_Shape.type = Math::ShapeType::SPHERE;
          l_Shape.sphere = deserialize_sphere(p_Node["sphere"]);
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

      u8 deserialize_enum(Yaml::Node &p_Node)
      {
        RTTI::EnumInfo &l_EnumInfo =
            get_enum_info(p_Node["enum_id"].as<u16>());
        return l_EnumInfo.entry_value(
            LOW_YAML_AS_NAME(p_Node["enum_value"]));
      }
    } // namespace Serialization
  }   // namespace Util
} // namespace Low
