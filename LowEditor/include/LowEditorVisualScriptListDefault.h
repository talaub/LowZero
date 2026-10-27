#pragma once

#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilSerialization.h"
#include "LowUtilVariant.h"

#include <stdexcept>
#include <utility>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      template <typename T> struct ListDefaultElementSerialization
      {
        static void serialize(Util::Serial::Node &p_Node,
                              const T &p_Value, Util::TypeIdentifier)
        {
          p_Node = p_Value;
        }

        static T deserialize(Util::Serial::Node &p_Node,
                             Util::TypeIdentifier)
        {
          return p_Node.as<T>();
        }
      };

      // Owns a concrete List<T> while preserving independent value
      // copies.
      class ListDefaultValue
      {
      public:
        using ElementVisitor =
            Util::Function<void(const Util::Variant &)>;

        ListDefaultValue() = default;

        ~ListDefaultValue()
        {
          reset();
        }

        ListDefaultValue(const ListDefaultValue &p_Other)
        {
          if (p_Other.m_Value) {
            m_Value = p_Other.m_Clone(p_Other.m_Value);
            m_Destroy = p_Other.m_Destroy;
            m_Clone = p_Other.m_Clone;
            m_Serialize = p_Other.m_Serialize;
            m_Deserialize = p_Other.m_Deserialize;
            m_ReferencedType = p_Other.m_ReferencedType;
            m_ForEach = p_Other.m_ForEach;
          }
        }

        ListDefaultValue &operator=(const ListDefaultValue &p_Other)
        {
          if (this != &p_Other) {
            ListDefaultValue l_Copy(p_Other);
            swap(l_Copy);
          }
          return *this;
        }

        ListDefaultValue(ListDefaultValue &&p_Other) noexcept
        {
          swap(p_Other);
        }

        ListDefaultValue &
        operator=(ListDefaultValue &&p_Other) noexcept
        {
          if (this != &p_Other) {
            reset();
            swap(p_Other);
          }
          return *this;
        }

        template <typename T, typename Serialization =
                                  ListDefaultElementSerialization<T>>
        void create(Util::TypeIdentifier p_ReferencedType = {})
        {
          ListDefaultValue l_New;
          l_New.m_Value = new Util::List<T>();
          l_New.m_Destroy = [](void *p_Value) {
            delete static_cast<Util::List<T> *>(p_Value);
          };
          l_New.m_Clone = [](const void *p_Value) -> void * {
            return new Util::List<T>(
                *static_cast<const Util::List<T> *>(p_Value));
          };
          l_New.m_ReferencedType = p_ReferencedType;
          l_New.m_ForEach = [](const void *p_Value,
                               const ElementVisitor &p_Visitor) {
            for (const T &i_Value :
                 *static_cast<const Util::List<T> *>(p_Value)) {
              p_Visitor(Util::Variant(i_Value));
            }
          };
          l_New.m_Serialize = [](const void *p_Value,
                                 Util::Serial::Node &p_Node,
                                 Util::TypeIdentifier p_Type) {
            const auto &l_List =
                *static_cast<const Util::List<T> *>(p_Value);
            p_Node = Util::Serial::Node::sequence({});
            for (const T &i_Value : l_List) {
              Util::Serial::Node l_Item;
              Serialization::serialize(l_Item, i_Value, p_Type);
              p_Node.push_back(l_Item);
            }
          };
          l_New.m_Deserialize = [](void *p_Value,
                                   Util::Serial::Node &p_Node,
                                   Util::TypeIdentifier p_Type) {
            Util::List<T> l_List;
            if (!p_Node.is_null()) {
              if (!p_Node.is_seq()) {
                throw std::runtime_error(
                    "List default must be a sequence");
              }
              l_List.reserve(p_Node.size());
              for (std::size_t i = 0; i < p_Node.size(); ++i) {
                l_List.push_back(
                    Serialization::deserialize(p_Node[i], p_Type));
              }
            }
            static_cast<Util::List<T> *>(p_Value)->swap(l_List);
          };
          swap(l_New);
        }

        void for_each(const ElementVisitor &p_Visitor) const
        {
          if (m_Value) {
            m_ForEach(m_Value, p_Visitor);
          }
        }

        void serialize(Util::Serial::Node &p_Node) const
        {
          if (m_Value) {
            m_Serialize(m_Value, p_Node, m_ReferencedType);
          } else {
            p_Node = Util::Serial::Node::sequence({});
          }
        }

        void deserialize(Util::Serial::Node &p_Node)
        {
          if (!m_Value) {
            throw std::logic_error("Create a typed list before "
                                   "deserializing its default");
          }
          m_Deserialize(m_Value, p_Node, m_ReferencedType);
        }

        void reset() noexcept
        {
          if (m_Value) {
            m_Destroy(m_Value);
          }
          m_Value = nullptr;
          m_Destroy = nullptr;
          m_Clone = nullptr;
          m_Serialize = nullptr;
          m_Deserialize = nullptr;
          m_ReferencedType = {};
          m_ForEach = nullptr;
        }

        void *get() noexcept
        {
          return m_Value;
        }

        const void *get() const noexcept
        {
          return m_Value;
        }

        explicit operator bool() const noexcept
        {
          return m_Value != nullptr;
        }

        void swap(ListDefaultValue &p_Other) noexcept
        {
          std::swap(m_Value, p_Other.m_Value);
          std::swap(m_Destroy, p_Other.m_Destroy);
          std::swap(m_Clone, p_Other.m_Clone);
          std::swap(m_Serialize, p_Other.m_Serialize);
          std::swap(m_Deserialize, p_Other.m_Deserialize);
          std::swap(m_ReferencedType, p_Other.m_ReferencedType);
          std::swap(m_ForEach, p_Other.m_ForEach);
        }

      private:
        void *m_Value = nullptr;
        void (*m_Destroy)(void *) = nullptr;
        void *(*m_Clone)(const void *) = nullptr;
        void (*m_Serialize)(const void *, Util::Serial::Node &,
                            Util::TypeIdentifier) = nullptr;
        void (*m_Deserialize)(void *, Util::Serial::Node &,
                              Util::TypeIdentifier) = nullptr;
        Util::TypeIdentifier m_ReferencedType;
        void (*m_ForEach)(const void *,
                          const ElementVisitor &) = nullptr;
      };
    } // namespace VisualScript
  }   // namespace Editor
} // namespace Low
