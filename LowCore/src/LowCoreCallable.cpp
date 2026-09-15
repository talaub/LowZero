#include "LowCoreCallable.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Core {

    bool NativeCallable::is_bound() const
    {
      return (bool)m_Function;
    }

    void NativeCallable::invoke(
        const Util::List<Util::Variant> &p_Args) const
    {
      _LOW_ASSERT(is_bound());
      m_Function(p_Args);
    }

    ScriptCallable ScriptCallable::bind(Scripting::Function p_Function)
    {
      ScriptCallable l_Callable;
      l_Callable.m_Function = p_Function;
      l_Callable.m_Native = NativeCallable(
          [p_Function](const Util::List<Util::Variant> &p_Args) {
            Scripting::call_function_dynamic(
                p_Function.module, p_Function.declaration, p_Args);
          });
      return l_Callable;
    }

    ScriptCallable
    ScriptCallable::bind(Scripting::Function p_Function,
                         Util::List<Util::Variant> p_BoundArgs)
    {
      ScriptCallable l_Callable;
      l_Callable.m_Function = p_Function;
      l_Callable.m_Native = NativeCallable(
          [p_Function, p_BoundArgs](
              const Util::List<Util::Variant> &p_InvokeArgs) {
            _LOW_ASSERT(p_InvokeArgs.empty());
            Scripting::call_function_dynamic(
                p_Function.module, p_Function.declaration, p_BoundArgs);
          });
      return l_Callable;
    }

  } // namespace Core
} // namespace Low
