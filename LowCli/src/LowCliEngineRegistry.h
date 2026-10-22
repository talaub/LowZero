#pragma once

#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Cli {
    namespace EngineRegistry {
      // Bump on incompatible format changes, LowShim checks it too
      static constexpr u32 FORMAT_VERSION = 1;

      Util::String get_registry_directory();
      Util::String get_registry_path();

      // Fails for unreadable files and for files written in a newer
      // format (see get_file_version)
      bool initialize();
      u32 get_file_version();

      bool has_path(const Util::String &p_Path);
      bool has_key(const Util::String &p_Key);

      Util::String get_key(const Util::String &p_Key);
      Util::String get_id_for_path(const Util::String &p_Path);
      Util::String get_default();

      bool force_set(const Util::String &p_Id,
                     const Util::String &p_Path);

      bool paths_equal(const Util::String &p_A,
                       const Util::String &p_B);
    } // namespace EngineRegistry
  } // namespace Cli
} // namespace Low
