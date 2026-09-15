#pragma once

#include "LowCoreApi.h"

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"
#include "LowUtilAssert.h"

#include "LowCoreScripting.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace Low {
  namespace Core {

    struct LOW_CORE_API NativeCallable
    {
      Util::Function<void(const Util::List<Util::Variant> &)>
          m_Function;

      NativeCallable() = default;
      NativeCallable(
          Util::Function<void(const Util::List<Util::Variant> &)>
              p_Function)
          : m_Function(p_Function)
      {
      }

      bool is_bound() const;
      void invoke(const Util::List<Util::Variant> &p_Args) const;

      template <typename... TArgs>
      void invoke(TArgs &&...p_Args) const
      {
        invoke(Util::List<Util::Variant>{
            Util::Variant(std::forward<TArgs>(p_Args))...});
      }

      template <typename T>
      static T unpack_arg(const Util::Variant &p_Variant)
      {
        if constexpr (std::is_same_v<T, Util::String>) {
          return p_Variant.as_string();
        } else if constexpr (std::is_base_of_v<Util::Handle, T>) {
          return T(static_cast<Util::Handle>(p_Variant));
        } else {
          return static_cast<T>(p_Variant);
        }
      }

      template <typename... TArgs, size_t... TIndices>
      static void
      call_unpacked(const Util::Function<void(TArgs...)> &p_Function,
                    const Util::List<Util::Variant> &p_Args,
                    std::index_sequence<TIndices...>)
      {
        p_Function(
            unpack_arg<std::decay_t<TArgs>>(p_Args[TIndices])...);
      }

      // Unbound: args supplied at invoke() time.
      template <typename... TArgs>
      static NativeCallable
      bind(Util::Function<void(TArgs...)> p_Function)
      {
        return NativeCallable(
            [p_Function](const Util::List<Util::Variant> &p_Args) {
              _LOW_ASSERT(p_Args.size() == sizeof...(TArgs));
              call_unpacked(p_Function, p_Args,
                            std::index_sequence_for<TArgs...>{});
            });
      }

      // Curried: args baked in now, invoke() later takes none.
      template <typename... TArgs>
      static NativeCallable
      bind(Util::Function<void(TArgs...)> p_Function, TArgs... p_Args)
      {
        Util::List<Util::Variant> l_BoundArgs{
            Util::Variant(p_Args)...};
        return NativeCallable(
            [p_Function, l_BoundArgs](
                const Util::List<Util::Variant> &p_InvokeArgs) {
              _LOW_ASSERT(p_InvokeArgs.empty());
              call_unpacked(p_Function, l_BoundArgs,
                            std::index_sequence_for<TArgs...>{});
            });
      }

      // Plain function-pointer overloads so callers don't have to
      // spell out Util::Function<...> at the call site.
      template <typename... TArgs>
      static NativeCallable bind(void (*p_Function)(TArgs...))
      {
        return bind(Util::Function<void(TArgs...)>(p_Function));
      }

      template <typename... TArgs>
      static NativeCallable bind(void (*p_Function)(TArgs...),
                                 TArgs... p_Args)
      {
        return bind(Util::Function<void(TArgs...)>(p_Function),
                    p_Args...);
      }
    };

    struct LOW_CORE_API ScriptCallable
    {
      Scripting::Function m_Function;
      NativeCallable m_Native;

      bool is_bound() const
      {
        return m_Native.is_bound();
      }

      void invoke(const Util::List<Util::Variant> &p_Args) const
      {
        m_Native.invoke(p_Args);
      }

      void invoke() const
      {
        m_Native.invoke();
      }

      template <typename... TArgs>
      requires(sizeof...(TArgs) > 0)
      void invoke(TArgs &&...p_Args) const
      {
        Scripting::call_function(m_Function,
                                 std::forward<TArgs>(p_Args)...);
      }

      static ScriptCallable bind(Scripting::Function p_Function);
      static ScriptCallable
      bind(Scripting::Function p_Function,
           Util::List<Util::Variant> p_BoundArgs);

      template <typename... TArgs>
      static ScriptCallable bind(Scripting::Function p_Function,
                                 TArgs... p_Args)
      {
        ScriptCallable l_Callable;
        l_Callable.m_Function = p_Function;
        l_Callable.m_Native = NativeCallable(
            [p_Function, p_Args...](
                const Util::List<Util::Variant> &p_InvokeArgs) {
              _LOW_ASSERT(p_InvokeArgs.empty());
              Scripting::call_function(p_Function, p_Args...);
            });
        return l_Callable;
      }

      static ScriptCallable bind(Scripting::Module p_Module,
                                 const Util::String &p_Declaration)
      {
        return bind(Scripting::Function{p_Module, p_Declaration});
      }

      static ScriptCallable
      bind(Scripting::Module p_Module,
           const Util::String &p_Declaration,
           Util::List<Util::Variant> p_BoundArgs)
      {
        return bind(Scripting::Function{p_Module, p_Declaration},
                    p_BoundArgs);
      }

      template <typename... TArgs>
      static ScriptCallable bind(Scripting::Module p_Module,
                                 const Util::String &p_Declaration,
                                 TArgs... p_Args)
      {
        return bind(Scripting::Function{p_Module, p_Declaration},
                    p_Args...);
      }
    };

    struct LOW_CORE_API Signal
    {
      std::variant<NativeCallable, ScriptCallable> m_Internal;

      Signal() = default;
      Signal(NativeCallable p_Native) : m_Internal(p_Native)
      {
      }
      Signal(ScriptCallable p_Script) : m_Internal(p_Script)
      {
      }

      void invoke(const Util::List<Util::Variant> &p_Args) const
      {
        std::visit(
            [&p_Args](const auto &p_Callable) {
              p_Callable.invoke(p_Args);
            },
            m_Internal);
      }

      template <typename... TArgs>
      void invoke(TArgs &&...p_Args) const
      {
        std::visit(
            [&](const auto &p_Callable) {
              p_Callable.invoke(std::forward<TArgs>(p_Args)...);
            },
            m_Internal);
      }

      template <typename... TArgs>
      void operator()(TArgs &&...p_Args) const
      {
        invoke(std::forward<TArgs>(p_Args)...);
      }
    };

  } // namespace Core
} // namespace Low
