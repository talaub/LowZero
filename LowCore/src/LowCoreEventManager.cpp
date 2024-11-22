#include "LowCoreEventManager.h"

namespace Low {
  namespace Core {
    namespace EventManager {
      struct EventEntry
      {
        EventCallback callback;
        EventBindingId bindingId;
      };

      Util::UniqueId g_UniqueIdCounter = 1;
      Util::Map<Util::Name, Util::List<EventEntry>> g_Events;

      EventBindingId bind_event(Util::Name p_EventId,
                                EventCallback p_Callback)
      {
        EventEntry l_Entry;
        l_Entry.callback = p_Callback;
        l_Entry.bindingId = g_UniqueIdCounter++;

        g_Events[p_EventId].push_back(l_Entry);

        return l_Entry.bindingId;
      }

      void unbind_event(EventBindingId p_BindingId)
      {
        for (auto it = g_Events.begin(); it != g_Events.end(); ++it) {
          for (auto eit = it->second.begin();
               eit != it->second.end();) {
            if (eit->bindingId == p_BindingId) {
              eit = it->second.erase(eit);
            } else {
              eit++;
            }
          }
        }
      }

      void clear_events()
      {
        g_Events.clear();
      }

      void dispatch_event(Util::Name p_EventId,
                          Util::List<Util::Variant> &p_Parameters)
      {
        auto l_Entry = g_Events.find(p_EventId);

        if (l_Entry != g_Events.end()) {
          for (auto it = l_Entry->second.begin();
               it != l_Entry->second.end(); ++it) {
            it->callback(p_Parameters);
          }
        }
      }

      void dispatch_event(Util::Name p_EventId)
      {
        Util::List<Util::Variant> l_Parameters;

        dispatch_event(p_EventId, l_Parameters);
      }

      void dispatch_event(Util::Name p_EventId,
                          Util::Variant p_Parameter)
      {
        Util::List<Util::Variant> l_Parameters;
        l_Parameters.push_back(p_Parameter);

        dispatch_event(p_EventId, l_Parameters);
      }

      void dispatch_event(Util::Name p_EventId,
                          Util::Variant p_Param1,
                          Util::Variant p_Param2)
      {
        Util::List<Util::Variant> l_Parameters;
        l_Parameters.push_back(p_Param1);
        l_Parameters.push_back(p_Param2);

        dispatch_event(p_EventId, l_Parameters);
      }

      void dispatch_event(Util::Name p_EventId,
                          Util::Variant p_Param1,
                          Util::Variant p_Param2,
                          Util::Variant p_Param3)
      {
        Util::List<Util::Variant> l_Parameters;
        l_Parameters.push_back(p_Param1);
        l_Parameters.push_back(p_Param2);
        l_Parameters.push_back(p_Param3);

        dispatch_event(p_EventId, l_Parameters);
      }

      void dispatch_event(Util::Name p_EventId,
                          Util::Variant p_Param1,
                          Util::Variant p_Param2,
                          Util::Variant p_Param3,
                          Util::Variant p_Param4)
      {
        Util::List<Util::Variant> l_Parameters;
        l_Parameters.push_back(p_Param1);
        l_Parameters.push_back(p_Param2);
        l_Parameters.push_back(p_Param3);
        l_Parameters.push_back(p_Param4);

        dispatch_event(p_EventId, l_Parameters);
      }

      void
      dispatch_event(Util::Name p_EventId, Util::Variant p_Param1,
                     Util::Variant p_Param2, Util::Variant p_Param3,
                     Util::Variant p_Param4, Util::Variant p_Param5)
      {
        Util::List<Util::Variant> l_Parameters;
        l_Parameters.push_back(p_Param1);
        l_Parameters.push_back(p_Param2);
        l_Parameters.push_back(p_Param3);
        l_Parameters.push_back(p_Param4);
        l_Parameters.push_back(p_Param5);

        dispatch_event(p_EventId, l_Parameters);
      }
    } // namespace EventManager
  }   // namespace Core
} // namespace Low
