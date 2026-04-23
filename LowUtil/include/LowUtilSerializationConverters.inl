// -------- Generic  converters for common containers --------
#include <variant>
#define BASIC_CONVERTER(ttype)                                       \
  template <> struct Converter<ttype, void>                          \
  {                                                                  \
    static Node encode(const ttype &v)                               \
    {                                                                \
      Node n;                                                        \
      n = v;                                                         \
      return n;                                                      \
    }                                                                \
    static bool decode(const Node &n, ttype &out)                    \
    {                                                                \
      if (auto sc = std::get_if<Node::Scalar>(&n.data)) {            \
        if (std::holds_alternative<ttype>(sc->value)) {              \
          out = std::get<ttype>(sc->value);                          \
          return true;                                               \
        }                                                            \
      }                                                              \
      return false;                                                  \
    }                                                                \
  };

template <> struct Converter<u64, void>
{
  static Node encode(const i64 &v)
  {
    Node n;
    n = v;
    return n;
  }
  static bool decode(const Node &n, u64 &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<u64>(sc->value)) {
        out = std::get<u64>(sc->value);
        return true;
      } else if (std::holds_alternative<i64>(sc->value)) {
        out = static_cast<u64>(std::get<i64>(sc->value));
        return true;
      } else if (std::holds_alternative<float>(sc->value)) {
        out = static_cast<u64>(std::get<float>(sc->value));
        return true;
      } else if (std::holds_alternative<bool>(sc->value)) {
        out = static_cast<u64>(std::get<bool>(sc->value));
        return true;
      }
    }
    return false;
  }
};

template <> struct Converter<i64, void>
{
  static Node encode(const i64 &v)
  {
    Node n;
    n = v;
    return n;
  }
  static bool decode(const Node &n, i64 &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<i64>(sc->value)) {
        out = std::get<i64>(sc->value);
        return true;
      } else if (std::holds_alternative<u64>(sc->value)) {
        out = static_cast<i64>(std::get<u64>(sc->value));
        return true;
      } else if (std::holds_alternative<float>(sc->value)) {
        out = static_cast<i64>(std::get<float>(sc->value));
        return true;
      } else if (std::holds_alternative<bool>(sc->value)) {
        out = static_cast<i64>(std::get<bool>(sc->value));
        return true;
      }
    }
    return false;
  }
};

template <> struct Converter<float, void>
{
  static Node encode(const float &v)
  {
    Node n;
    n = v;
    return n;
  }
  static bool decode(const Node &n, float &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<float>(sc->value)) {
        out = std::get<float>(sc->value);
        return true;
      } else if (std::holds_alternative<u64>(sc->value)) {
        out = static_cast<float>(std::get<u64>(sc->value));
        return true;
      } else if (std::holds_alternative<i64>(sc->value)) {
        out = static_cast<float>(std::get<i64>(sc->value));
        return true;
      } else if (std::holds_alternative<bool>(sc->value)) {
        out = static_cast<float>(std::get<bool>(sc->value));
        return true;
      }
    }
    return false;
  }
};

template <> struct Converter<bool, void>
{
  static Node encode(const bool &v)
  {
    Node n;
    n = v;
    return n;
  }
  static bool decode(const Node &n, bool &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<bool>(sc->value)) {
        out = std::get<bool>(sc->value);
        return true;
      } else if (std::holds_alternative<u64>(sc->value)) {
        out = static_cast<bool>(std::get<u64>(sc->value));
        return true;
      } else if (std::holds_alternative<i64>(sc->value)) {
        out = static_cast<bool>(std::get<i64>(sc->value));
        return true;
      } else if (std::holds_alternative<float>(sc->value)) {
        out = static_cast<bool>(std::get<float>(sc->value));
        return true;
      }
    }
    return false;
  }
};

template <> struct Converter<String, void>
{
  static Node encode(const String &v)
  {
    Node n;
    n = v;
    return n;
  }
  static bool decode(const Node &n, String &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<String>(sc->value)) {
        out = std::get<String>(sc->value);
        return true;
      } else if (std::holds_alternative<bool>(sc->value)) {
        const bool b = std::get<bool>(sc->value);
        out = b ? "true" : "false";
        return true;
      } else if (std::holds_alternative<u64>(sc->value)) {
        out += std::get<u64>(sc->value);
        return true;
      } else if (std::holds_alternative<i64>(sc->value)) {
        out += std::get<i64>(sc->value);
        return true;
      } else if (std::holds_alternative<float>(sc->value)) {
        out += std::get<float>(sc->value);
        return true;
      }
    }
    return false;
  }
};

#define CAST_CONVERTER(stype, ttype)                                 \
  template <> struct Converter<stype, void>                          \
  {                                                                  \
    static Node encode(const stype &v)                               \
    {                                                                \
      Node n;                                                        \
      n = static_cast<ttype>(v);                                     \
      return n;                                                      \
    }                                                                \
    static bool decode(const Node &n, stype &out)                    \
    {                                                                \
      if (auto sc = std::get_if<Node::Scalar>(&n.data)) {            \
        out = static_cast<stype>(n.as<ttype>());                     \
        return true;                                                 \
      }                                                              \
      return false;                                                  \
    }                                                                \
  };

CAST_CONVERTER(i32, i64);
CAST_CONVERTER(u8, u64);
CAST_CONVERTER(u16, u64);
CAST_CONVERTER(u32, u64);
CAST_CONVERTER(double, float);

template <> struct Converter<Math::Vector3, void>
{
  static Node encode(const Math::Vector3 &v)
  {
    Node n;
    n["x"] = v.x;
    n["y"] = v.y;
    n["z"] = v.z;
    return n;
  }
  static bool decode(const Node &n, Math::Vector3 &out)
  {
    if (n.is_seq()) {
      out.x = n[0].as<float>();
      out.y = n[1].as<float>();
      out.z = n[2].as<float>();
      return true;
    } else if (n.is_dict()) {
      if (n["x"]) {
        out.x = n["x"].as<float>();
      }
      if (n["y"]) {
        out.y = n["y"].as<float>();
      }
      if (n["z"]) {
        out.z = n["z"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Vector2, void>
{
  static Node encode(const Math::Vector2 &v)
  {
    Node n;
    n["x"] = v.x;
    n["y"] = v.y;
    return n;
  }
  static bool decode(const Node &n, Math::Vector2 &out)
  {
    if (n.is_seq()) {
      out.x = n[0].as<float>();
      out.y = n[1].as<float>();
      return true;
    } else if (n.is_dict()) {
      if (n["x"]) {
        out.x = n["x"].as<float>();
      }
      if (n["y"]) {
        out.y = n["y"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::UVector2, void>
{
  static Node encode(const Math::UVector2 &v)
  {
    Node n;
    n["x"] = v.x;
    n["y"] = v.y;
    return n;
  }
  static bool decode(const Node &n, Math::UVector2 &out)
  {
    if (n.is_dict()) {
      if (n["x"]) {
        out.x = n["x"].as<u32>();
      }
      if (n["y"]) {
        out.y = n["y"].as<u32>();
      }
      return true;
    }
    return false;
  }
};

static void encode_scalar(const Yaml::Node &v, Node &out)
{
  using YAML::BadConversion;

  // 1) Try bool
  try {
    bool b = v.as<bool>();
    out = b; // Node::operator= does the rest
    return;
  } catch (const BadConversion &) {
  }

  // 2) Try signed integer
  try {
    i64 i = v.as<i64>();
    out = i;
    return;
  } catch (const BadConversion &) {
  }

  // 3) Try unsigned integer
  try {
    u64 u = v.as<u64>();
    out = u;
    return;
  } catch (const BadConversion &) {
  }

  // 4) Try floating point
  try {
    double d = v.as<double>();
    out = static_cast<float>(d);
    return;
  } catch (const BadConversion &) {
  }

  // 5) Fallback: string
  out = String(v.Scalar().c_str());
}

template <> struct Converter<Yaml::Node, void>
{
  static Node encode(const Yaml::Node &v)
  {
    Node n;
    if (v.IsSequence()) {
      for (const auto &elem : v) {
        n.push_back(Converter<Yaml::Node>::encode(elem)); // recursive
      }
    } else if (v.IsMap()) {
      for (const auto &it : v) {
        String key = it.first.as<std::string>().c_str();
        n[key] =
            Converter<Yaml::Node>::encode(it.second); // recursive
      }
    } else if (v.IsScalar()) {
      encode_scalar(v, n);
    }
    return n;
  }
  static bool decode(const Node &n, Yaml::Node &out)
  {
    return true;
  }
};

template <> struct Converter<Math::Vector4, void>
{
  static Node encode(const Math::Vector4 &v)
  {
    Node n;
    n["x"] = v.x;
    n["y"] = v.y;
    n["z"] = v.z;
    n["w"] = v.w;
    return n;
  }
  static bool decode(const Node &n, Math::Vector4 &out)
  {
    if (n.is_seq()) {
      out.x = n[0].as<float>();
      out.y = n[1].as<float>();
      out.z = n[2].as<float>();
      out.w = n[3].as<float>();
      return true;
    } else if (n.is_dict()) {
      if (n["x"]) {
        out.x = n["x"].as<float>();
      }
      if (n["y"]) {
        out.y = n["y"].as<float>();
      }
      if (n["z"]) {
        out.z = n["z"].as<float>();
      }
      if (n["w"]) {
        out.w = n["w"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Quaternion, void>
{
  static Node encode(const Math::Quaternion &v)
  {
    Node n;
    n["x"] = v.x;
    n["y"] = v.y;
    n["z"] = v.z;
    n["w"] = v.w;
    return n;
  }
  static bool decode(const Node &n, Math::Quaternion &out)
  {
    if (n.is_dict()) {
      if (n["x"]) {
        out.x = n["x"].as<float>();
      }
      if (n["y"]) {
        out.y = n["y"].as<float>();
      }
      if (n["z"]) {
        out.z = n["z"].as<float>();
      }
      if (n["w"]) {
        out.w = n["w"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Box, void>
{
  static Node encode(const Math::Box &v)
  {
    Node n;
    n["position"] = v.position;
    n["rotation"] = v.rotation;
    n["half_extents"] = v.halfExtents;
    return n;
  }
  static bool decode(const Node &n, Math::Box &out)
  {
    if (n.is_dict()) {
      if (n["position"]) {
        out.position = n["position"].as<Math::Vector3>();
      }
      if (n["rotation"]) {
        out.rotation = n["rotation"].as<Math::Quaternion>();
      }
      if (n["half_extents"]) {
        out.halfExtents = n["half_extents"].as<Math::Vector3>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Cone, void>
{
  static Node encode(const Math::Cone &v)
  {
    Node n;
    n["position"] = v.position;
    n["rotation"] = v.rotation;
    n["radius"] = v.radius;
    n["height"] = v.height;
    return n;
  }
  static bool decode(const Node &n, Math::Cone &out)
  {
    if (n.is_dict()) {
      if (n["position"]) {
        out.position = n["position"].as<Math::Vector3>();
      }
      if (n["rotation"]) {
        out.rotation = n["rotation"].as<Math::Quaternion>();
      }
      if (n["radius"]) {
        out.radius = n["radius"].as<float>();
      }
      if (n["height"]) {
        out.height = n["height"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Sphere, void>
{
  static Node encode(const Math::Sphere &v)
  {
    Node n;
    n["position"] = v.position;
    n["radius"] = v.radius;
    return n;
  }
  static bool decode(const Node &n, Math::Sphere &out)
  {
    if (n.is_dict()) {
      if (n["position"]) {
        out.position = n["position"].as<Math::Vector3>();
      }
      if (n["radius"]) {
        out.radius = n["radius"].as<float>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Shape, void>
{
  static Node encode(const Math::Shape &v)
  {
    Node n;
    if (v.type == Math::ShapeType::BOX) {
      n["box"] = v.box;
    } else {
      LOW_ASSERT(false, "Unsupported shape type");
    }
    return n;
  }
  static bool decode(const Node &n, Math::Shape &out)
  {
    if (n.is_dict()) {
      if (n["box"]) {
        out.box = n["box"].as<Math::Box>();
      } else {
        LOW_ASSERT(false, "Unsupported shape type");
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::Bounds, void>
{
  static Node encode(const Math::Bounds &v)
  {
    Node n;
    n["min"] = v.min;
    n["max"] = v.max;
    return n;
  }
  static bool decode(const Node &n, Math::Bounds &out)
  {
    if (n.is_dict()) {
      if (n["min"]) {
        out.min = n["min"].as<Math::Vector3>();
      }
      if (n["max"]) {
        out.max = n["max"].as<Math::Vector3>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Math::AABB, void>
{
  static Node encode(const Math::AABB &v)
  {
    Node n;
    n["bounds"] = v.bounds;
    n["center"] = v.center;
    return n;
  }
  static bool decode(const Node &n, Math::AABB &out)
  {
    if (n.is_dict()) {
      if (n["center"]) {
        out.center = n["center"].as<Math::Vector3>();
      }
      if (n["bounds"]) {
        out.bounds = n["bounds"].as<Math::Bounds>();
      }
      return true;
    }
    return false;
  }
};

template <> struct Converter<Name, void>
{
  static Node encode(const Name &v)
  {
    Node n;
    String val = v.c_str();
    n = val;
    return n;
  }
  static bool decode(const Node &n, Name &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      String content = n.as<String>();
      out = Low::Util::Name(content.c_str());
      return true;
    }
    return false;
  }
};

template <> struct Converter<U64Id, void>
{
  static Node encode(const U64Id &v)
  {
    Node n;
    String hashed = hash_to_string(v.val);
    n = hashed;
    return n;
  }
  static bool decode(const Node &n, U64Id &out)
  {
    if (auto sc = std::get_if<Node::Scalar>(&n.data)) {
      if (std::holds_alternative<String>(sc->value)) {
        String hashed = std::get<String>(sc->value);
        out.val = string_to_hash(hashed);
        return true;
      }
      if (std::holds_alternative<u64>(sc->value)) {
        out.val = std::get<u64>(sc->value);
        return true;
      }
      if (std::holds_alternative<i64>(sc->value)) {
        out.val = (u64)std::get<i64>(sc->value);
        return true;
      }
      if (std::holds_alternative<float>(sc->value)) {
        out.val = (u64)std::get<float>(sc->value);
        return true;
      }
    }
    return false;
  }
};
