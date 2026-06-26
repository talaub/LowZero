#pragma once

#include "LowMath.h"

#include <optional>
#include <utility>
#include <vector>

namespace Low {
  namespace Gfx {
    template <typename Tag> struct Token
    {
      static constexpr u32 INVALID_INDEX = LOW_UINT32_MAX;
      static constexpr u32 INVALID_OWNER = 0;

      u32 index = INVALID_INDEX;
      u32 generation = 0;
      u32 owner_id = INVALID_OWNER;

      explicit operator bool() const
      {
        return index != INVALID_INDEX;
      }

      auto operator<=>(const Token &) const = default;
    };

    template <typename TokenType, typename ObjectType> struct Pool
    {
      void set_owner_id(u32 p_OwnerId)
      {
        m_OwnerId = p_OwnerId;
      }

      u32 get_owner_id() const
      {
        return m_OwnerId;
      }

      template <typename... Args> TokenType create(Args &&...p_Args)
      {
        u32 l_Index = TokenType::INVALID_INDEX;

        if (m_FirstFree != TokenType::INVALID_INDEX) {
          l_Index = m_FirstFree;
          m_FirstFree = m_Slots[l_Index].nextFree;
        } else {
          l_Index = static_cast<u32>(m_Slots.size());
          m_Slots.emplace_back();
        }

        Slot &l_Slot = m_Slots[l_Index];
        l_Slot.object.emplace(std::forward<Args>(p_Args)...);
        l_Slot.occupied = true;
        l_Slot.nextFree = TokenType::INVALID_INDEX;

        return TokenType{l_Index, l_Slot.generation, m_OwnerId};
      }

      void destroy(TokenType p_Token)
      {
        if (!is_valid(p_Token)) {
          return;
        }

        Slot &l_Slot = m_Slots[p_Token.index];
        l_Slot.object.reset();
        l_Slot.occupied = false;
        ++l_Slot.generation;
        if (l_Slot.generation == 0) {
          ++l_Slot.generation;
        }
        l_Slot.nextFree = m_FirstFree;
        m_FirstFree = p_Token.index;
      }

      ObjectType *get(TokenType p_Token)
      {
        if (!is_valid(p_Token)) {
          return nullptr;
        }

        return &*m_Slots[p_Token.index].object;
      }

      const ObjectType *get(TokenType p_Token) const
      {
        if (!is_valid(p_Token)) {
          return nullptr;
        }

        return &*m_Slots[p_Token.index].object;
      }

      bool is_valid(TokenType p_Token) const
      {
        if (!p_Token || p_Token.owner_id != m_OwnerId ||
            p_Token.index >= m_Slots.size()) {
          return false;
        }

        const Slot &l_Slot = m_Slots[p_Token.index];
        return l_Slot.occupied &&
               l_Slot.generation == p_Token.generation &&
               l_Slot.object.has_value();
      }

      template <typename FunctionType>
      void for_each(FunctionType &&p_Function)
      {
        for (Slot &i_Slot : m_Slots) {
          if (i_Slot.occupied && i_Slot.object.has_value()) {
            p_Function(*i_Slot.object);
          }
        }
      }

      void clear()
      {
        m_Slots.clear();
        m_FirstFree = TokenType::INVALID_INDEX;
      }

    private:
      struct Slot
      {
        std::optional<ObjectType> object;
        u32 generation = 1;
        u32 nextFree = TokenType::INVALID_INDEX;
        bool occupied = false;
      };

      std::vector<Slot> m_Slots;
      u32 m_FirstFree = TokenType::INVALID_INDEX;
      u32 m_OwnerId = TokenType::INVALID_OWNER;
    };

  } // namespace Gfx
} // namespace Low
