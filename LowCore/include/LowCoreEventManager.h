#pragma once

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"

namespace Low {
  namespace Core {
    typedef u64 EventBindingId;
    typedef void (*EventCallback)(Util::List<Util::Variant> &);

    namespace EventManager {
      EventBindingId bind_event(Util::UniqueId p_EventId,
                                EventCallback p_Callback);
      void unbind_event(EventBindingId p_BindingId);
      void clear_events();

      void dispatch_event(Util::UniqueId p_EventId,
                          Util::List<Util::Variant> &p_Parameters);
      void dispatch_event(Util::UniqueId p_EventId,
                          Util::Variant &p_Parameter);
      void dispatch_event(Util::UniqueId p_EventId,
                          Util::Variant &p_Param1,
                          Util::Variant &p_Param2);
      void dispatch_event(Util::UniqueId p_EventId,
                          Util::Variant &p_Param1,
                          Util::Variant &p_Param2,
                          Util::Variant &p_Param3);
    } // namespace EventManager
  }   // namespace Core
} // namespace Low
