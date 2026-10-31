#pragma once

#include "LowCoreApi.h"

#include <serializer/serializer.h>

class asIScriptEngine;

namespace Low {
  namespace Core {
    namespace Scripting {
      class LOW_CORE_API ScriptSerializer : public CSerializer
      {
      public:
        void detach_extra_objects();

        bool restore_object(asIScriptObject *p_OldObject,
                            asIScriptObject *p_NewObject);
      };

      void LOW_CORE_API register_generic_user_types(
          CSerializer &p_Serializer, asIScriptEngine *p_Engine);
    } // namespace Scripting
  } // namespace Core
} // namespace Low
