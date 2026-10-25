#pragma once

#include "LowCoreApi.h"

#include "LowUtilContainers.h"
#include "LowUtilVariant.h"
#include "LowUtilName.h"

namespace Low {
  namespace Core {
    typedef u64 EventBindingId;
    typedef Util::Function<void(Util::List<Util::Variant> &)>
        EventCallback;

    namespace EventManager {
      EventBindingId LOW_CORE_API
      bind_event(Util::Name p_EventId, EventCallback p_Callback);
      void LOW_CORE_API unbind_event(EventBindingId p_BindingId);
      void LOW_CORE_API clear_events();

      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void
      dispatch_event(Util::Name p_EventId,
                     Util::List<Util::Variant> &p_Parameters);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void dispatch_event(Util::Name p_EventId);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void dispatch_event(Util::Name p_EventId,
                                       Util::Variant p_Parameter);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void dispatch_event(Util::Name p_EventId,
                                       Util::Variant p_Param1,
                                       Util::Variant p_Param2);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void dispatch_event(Util::Name p_EventId,
                                       Util::Variant p_Param1,
                                       Util::Variant p_Param2,
                                       Util::Variant p_Param3);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void dispatch_event(Util::Name p_EventId,
                                       Util::Variant p_Param1,
                                       Util::Variant p_Param2,
                                       Util::Variant p_Param3,
                                       Util::Variant p_Param4);
      LOW_FUNCTION(scripting, bind_namespace = "Event",
                   bind_name = "dispatch")
      LOW_CORE_API void
      dispatch_event(Util::Name p_EventId, Util::Variant p_Param1,
                     Util::Variant p_Param2, Util::Variant p_Param3,
                     Util::Variant p_Param4, Util::Variant p_Param5);
    } // namespace EventManager
  } // namespace Core
} // namespace Low
