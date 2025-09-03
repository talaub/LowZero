#include "LowUtilObserverManager.h"

#include "LowUtilContainers.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Util {
    enum class ObserverType
    {
      FUNCTION,
      HANDLE
    };
    struct Observer
    {
      ObserverType type;
      u64 id;
      struct
      {
        Util::Function<void(Util::Handle, Util::Name)> function;
        Util::Handle handle;
      };
    };

    u64 g_GlobalIdCounter = 0;

    std::unordered_map<ObserverKey, List<Observer>, ObserverKeyHash>
        g_Observers;

    u64
    observe(const ObserverKey &key,
            Util::Function<void(Util::Handle, Util::Name)> p_Function)
    {
      Observer l_Observer;
      l_Observer.id = g_GlobalIdCounter++;
      l_Observer.function = p_Function;
      l_Observer.type = ObserverType::FUNCTION;
      g_Observers[key].push_back(l_Observer);

      return l_Observer.id;
    }

    u64 observe(const ObserverKey &key, Handle p_Handle)
    {
      Observer l_Observer;
      l_Observer.id = g_GlobalIdCounter++;
      l_Observer.handle = p_Handle;
      l_Observer.type = ObserverType::HANDLE;
      g_Observers[key].push_back(l_Observer);

      return l_Observer.id;
    }

    void notify(const ObserverKey &key)
    {
      auto it = g_Observers.find(key);
      if (it != g_Observers.end()) {
        for (auto &i_Observer : it->second) {
          if (i_Observer.type == ObserverType::FUNCTION) {
            i_Observer.function(key.handleId, key.observableName);
          } else if (i_Observer.type == ObserverType::HANDLE) {
            RTTI::TypeInfo &i_TypeInfo =
                Handle::get_type_info(i_Observer.handle.get_type());
            // TODO: If not alive remove from list
            if (i_TypeInfo.is_alive(i_Observer.handle)) {
              i_TypeInfo.notify(i_Observer.handle, key.handleId,
                                key.observableName);
            }

          } else {
            LOW_ASSERT(false, "Unknown observer");
          }
        }
      }
    }

    void clear(const ObserverKey &key)
    {
      g_Observers.erase(key);
    }
  } // namespace Util
} // namespace Low
