#pragma once

#include "LowUtilApi.h"

#include "LowUtilAssert.h"
#include "LowMath.h"
#include "LowUtilName.h"
#include "LowUtilYaml.h"

#include <any>
#include <variant>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace Low {
  namespace Util {
    namespace Serial {
      struct Node;

      template <class T>
      static constexpr bool IsScalarType =
          std::is_same_v<T, bool> || std::is_same_v<T, float> ||
          std::is_same_v<T, u64> || std::is_same_v<T, i64> ||
          std::is_same_v<T, String>;

      // ---- Converter<T> trait (specialize per type) ----
      template <class T, class = void>
      struct Converter; // you implement encode/decode for custom
                        // types

      template <class T>
      concept HasConverter =
          requires(const T &v, const Node &n, T &out) {
            { Converter<T>::encode(v) } -> std::same_as<Node>;
            { Converter<T>::decode(n, out) } -> std::same_as<bool>;
          };

      struct LOW_EXPORT Node
      {
      public:
        using Seq = List<Node>;
        using Dict = Map<String, Node>;

        struct Scalar
        {
          std::variant<bool, float, u64, i64, String> value;
        };

        using Storage =
            std::variant<std::monostate, Scalar, Seq, Dict>;

        Storage data;

        static Node sequence(Seq p_Sequence)
        {
          Node n;
          n.data = std::move(p_Sequence);
          return n;
        }

        static Node dictionary(Dict p_Dictionary)
        {
          Node n;
          n.data = std::move(p_Dictionary);
          return n;
        }

        Node() = default;

        // Uniform "node = anything" with coercion to scalar types:
        //  - signed integrals -> i64
        //  - unsigned integrals -> u64
        //  - floating point -> float
        //  - string-like -> String
        //  - bool -> bool
        template <class T> Node &operator=(T &&v)
        {
          using U = std::decay_t<T>;

          // 1) Assign from another Node
          if constexpr (std::is_same_v<U, Node>) {
            data = std::move(v.data);
            return *this;
          }

          // 2) Scalar-ish types that we can map into our variant
          else if constexpr (std::is_same_v<U, bool> ||
                             std::is_integral_v<U> ||
                             std::is_floating_point_v<U> ||
                             std::is_same_v<U, String> ||
                             std::is_convertible_v<U, String>) {

            Scalar s;

            if constexpr (std::is_same_v<U, bool>) {
              s.value = v;
            } else if constexpr (std::is_floating_point_v<U>) {
              // any float/double/long double -> float
              s.value = static_cast<float>(v);
            } else if constexpr (std::is_integral_v<U>) {
              if constexpr (std::is_signed_v<U>) {
                // any signed integral -> i64
                s.value = static_cast<i64>(v);
              } else {
                // any unsigned integral -> u64
                s.value = static_cast<u64>(v);
              }
            } else {
              // String or something convertible to String
              s.value = static_cast<String>(v);
            }

            data = std::move(s);
            return *this;
          }

          // 3) Custom / complex types via Converter<T>
          else if constexpr (HasConverter<U>) {
            data = Converter<U>::encode(v).data;
            return *this;
          }

          // 4) Nothing matched: hard error at compile time
          else {
            static_assert(sizeof(U) == 0,
                          "Node::operator= cannot store this type. "
                          "It is neither arithmetic, string-like, "
                          "nor has a Converter<T> specialization.");
          }
        }

        // Decode to T using direct variant access or Converter<T>
        template <class T> T as() const
        {
          // try Converter<T>
          if constexpr (HasConverter<T>) {
            T out{};
            if (Converter<T>::decode(*this, out))
              return out;
          }
          throw std::runtime_error(
              "Node: cannot decode to requested type");
        }

        // --- helpers to materialize containers lazily ---
        Dict &ensure_dict()
        {
          if (std::holds_alternative<std::monostate>(data)) {
            data = Dict{};
          }
          auto mp = std::get_if<Dict>(&data);
          if (!mp)
            throw std::logic_error("Node is not a dict");
          return *mp;
        }
        Seq &ensure_seq()
        {
          if (std::holds_alternative<std::monostate>(data)) {
            data = Seq{};
          }
          auto sp = std::get_if<Seq>(&data);
          if (!sp)
            throw std::logic_error("Node is not a sequence");
          return *sp;
        }

        // helpers
        bool is_null() const
        {
          return std::holds_alternative<std::monostate>(data);
        }
        bool is_scalar() const
        {
          return std::holds_alternative<Scalar>(data);
        }
        bool is_seq() const
        {
          return std::holds_alternative<Seq>(data);
        }
        bool is_dict() const
        {
          return std::holds_alternative<Dict>(data);
        }

        Seq *as_seq()
        {
          return std::get_if<Seq>(&data);
        }
        const Seq *as_seq() const
        {
          return std::get_if<Seq>(&data);
        }
        Dict *as_dict()
        {
          return std::get_if<Dict>(&data);
        }
        const Dict *as_map() const
        {
          return std::get_if<Dict>(&data);
        }

        // --- Map indexing (mutable) ---
        Node &operator[](StringView key)
        {
          auto &mp = ensure_dict();
          auto it = mp.find(String(key.data()));
          if (it == mp.end()) {
            auto [ins, _] = mp.emplace(String(key), Node{});
            return ins->second;
          }
          return it->second;
        }

        // --- Map indexing (const) ---
        const Node &operator[](StringView key) const
        {
          auto mp = std::get_if<Dict>(&data);
          if (!mp)
            throw std::logic_error("Node is not a dict");
          auto it = mp->find(String(key));
          if (it == mp->end())
            throw std::out_of_range("Key not found");
          return it->second;
        }

        // Safer map accessors
        const Node *find(StringView key) const
        {
          auto mp = std::get_if<Dict>(&data);
          if (!mp)
            return nullptr;
          auto it = mp->find(String(key.data()));
          return it == mp->end() ? nullptr : &it->second;
        }
        Node *find(StringView key)
        {
          auto mp = std::get_if<Dict>(&data);
          if (!mp)
            return nullptr;
          auto it = mp->find(String(key.data()));
          return it == mp->end() ? nullptr : &it->second;
        }
        const Node &at(StringView key) const
        {
          return (*this)[key];
        }
        Node &at(StringView key)
        {
          auto mp = std::get_if<Dict>(&data);
          if (!mp)
            throw std::logic_error("Node is not a dict");
          auto it = mp->find(String(key.data()));
          if (it == mp->end()) {
            auto [iter, inserted] = mp->insert({String(key), Node()});
            return iter->second;
          }
          return it->second;
        }

        // --- Sequence indexing (mutable) ---
        Node &operator[](std::size_t idx)
        {
          auto &sp = ensure_seq();
          if (sp.size() <= idx)
            sp.resize(idx + 1);
          return sp[idx];
        }

        // --- Sequence indexing (const) ---
        const Node &operator[](std::size_t idx) const
        {
          auto sp = std::get_if<Seq>(&data);
          if (!sp)
            throw std::logic_error("Node is not a sequence");
          if (idx >= sp->size())
            throw std::out_of_range("Index out of range");
          return (*sp)[idx];
        }

        // Safer sequence accessors
        const Node &at(std::size_t idx) const
        {
          return (*this)[idx];
        }
        Node &at(std::size_t idx)
        {
          auto sp = std::get_if<Seq>(&data);
          if (!sp)
            throw std::logic_error("Node is not a sequence");
          if (idx >= sp->size())
            throw std::out_of_range("Index out of range");
          return (*sp)[idx];
        }

        // --- Sequence helpers (append/construct/insert) ---
        void remove(const String p_Key)
        {
          auto &d = ensure_dict();
          d.erase(p_Key);
        }

        Node &push_back(const Node &n)
        {
          auto &s = ensure_seq();
          s.push_back(n);
          return s.back();
        }
        Node &push_back(Node &&n)
        {
          auto &s = ensure_seq();
          s.push_back(std::move(n));
          return s.back();
        }

        // push_back any T (goes through Converter<T> / direct scalar)
        template <class T> Node &push_back(T &&v)
        {
          auto &s = ensure_seq();
          s.emplace_back();              // append a default Node
          s.back() = std::forward<T>(v); // reuse your operator= path
          return s.back();
        }

        // emplace_back<T>(args...) to append a Node constructed from
        // T(...) using Node::operator= to perform the scalar
        // coercion.
        template <class T, class... Args>
        Node &emplace_back(Args &&...args)
        {
          auto &s = ensure_seq();
          s.emplace_back(); // append a default Node
          s.back() = T(std::forward<Args>(
              args)...); // will go through operator=
          return s.back();
        }

        template <class T> Node &insert(std::size_t index, T &&v)
        {
          auto &s = ensure_seq();
          if (index > s.size()) {
            s.resize(index);
          }
          auto it = s.begin() + static_cast<std::ptrdiff_t>(index);
          it = s.insert(it, Node{});
          *it = std::forward<T>(v);
          return *it;
        }

        void reserve(std::size_t n)
        {
          ensure_seq().reserve(n);
        }
        std::size_t size() const
        {
          if (auto sp = std::get_if<Seq>(&data))
            return sp->size();
          if (auto mp = std::get_if<Dict>(&data))
            return mp->size();
          if (std::holds_alternative<std::monostate>(data))
            return 0;
          throw std::logic_error("Node is not a sequence or dict");
        }
        bool empty() const
        {
          return size() == 0;
        }

        // bool conversions
        explicit operator bool() const
        {
          return !is_null();
        }
        bool operator!() const
        {
          return is_null();
        }

#include "LowUtilSerializationNodeIterator.inl"
      };

      [[nodiscard]] LOW_EXPORT Node
      load_yaml_file(const char *p_Path);
      LOW_EXPORT void write_yaml_file(const char *p_Path,
                                      Node &p_Node);

      LOW_EXPORT void serialize_enum(Node &p_Node, u16 p_EnumId,
                                     u8 p_EnumValue);
      [[nodiscard]] LOW_EXPORT u8 deserialize_enum(Node &p_Node);
#include "LowUtilSerializationConverters.inl"
    } // namespace Serial
  } // namespace Util
} // namespace Low

#define GET_AS_VALUE(ttype)                                          \
  if (std::holds_alternative<ttype>(sc->value)) {                    \
    node = std::get<ttype>(sc->value);                               \
  }

namespace YAML {
  template <> struct convert<Low::Util::Serial::Node>
  {
    static Node encode(const Low::Util::Serial::Node &rhs)
    {
      Node node;
      if (rhs.is_seq()) {
        for (auto [_, value] : rhs) {
          node.push_back(value);
        }
      } else if (rhs.is_dict()) {
        for (auto [key, value] : rhs) {
          node[key->c_str()] = value;
        }

      } else if (rhs.is_scalar()) {
        if (auto sc = std::get_if<Low::Util::Serial::Node::Scalar>(
                &rhs.data)) {
          GET_AS_VALUE(float)
          else GET_AS_VALUE(bool) else GET_AS_VALUE(
              u64) else GET_AS_VALUE(i64) else if (std::
                                                       holds_alternative<
                                                           Low::Util::
                                                               String>(
                                                           sc->value))
          {
            Low::Util::String strValue =
                std::get<Low::Util::String>(sc->value);
            node = strValue.c_str();
          }
        }
      }
      return node;
    }

    static bool decode(const Node &node, Low::Util::Serial::Node &rhs)
    {
      return true;
    }
  };
} // namespace YAML
