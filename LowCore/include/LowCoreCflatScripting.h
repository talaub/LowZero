#pragma once

#include "LowCoreApi.h"

#include "CflatGlobal.h"
#include "Cflat.h"

#include "LowUtilEnums.h"
#include "LowUtilString.h"
#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      void initialize();
      void tick(float p_Delta, Util::EngineState p_State);
      void cleanup();
      LOW_CORE_API Cflat::Environment *get_environment();

      struct Error
      {
        Util::String scriptName;
        Util::String scriptPath;
        u32 line;
        Util::String message;
      };

      struct Compilation
      {
        Util::String scriptName;
        Util::String scriptPath;
      };

      typedef void (*ErrorCallback)(const Error &);
      typedef void (*CompilationCallback)(const Compilation &);

      void LOW_CORE_API register_error_callback(ErrorCallback);
      void LOW_CORE_API
          register_compilation_callback(CompilationCallback);

    } // namespace Scripting
  }   // namespace Core
} // namespace Low
