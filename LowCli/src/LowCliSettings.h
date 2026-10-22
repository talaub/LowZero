#pragma once

#include "LowMath.h"
#include "LowUtilContainers.h"

// Per-machine settings in settings.yaml next to the engine registry
// (engines.yaml). Unlike the registry it is only read by low itself, not
// by the shim, so new keys can be added freely.

namespace Low {
  namespace Cli {
    namespace Settings {
      // Bump on incompatible format changes
      static constexpr u32 FORMAT_VERSION = 1;

      enum class ValueSource
      {
        Environment,
        File,
        Default
      };

      struct Value
      {
        Util::String value;
        ValueSource source = ValueSource::Default;
      };

      Util::String get_settings_path();

      // Fails for unreadable files and for files written in a newer
      // format (see get_file_version). Without it get() only knows
      // environment variables and defaults.
      bool initialize();
      u32 get_file_version();

      // Keys low knows, in display order
      Util::List<Util::String> get_keys();
      bool is_key(const Util::String &p_Key);
      Util::String get_description(const Util::String &p_Key);
      // Empty if the key has no environment variable override
      Util::String get_environment_variable(const Util::String &p_Key);

      // Environment variable > settings.yaml > default. Path values are
      // absolute with forward slashes.
      Value get(const Util::String &p_Key);

      bool set(const Util::String &p_Key, const Util::String &p_Value);
      // Removes the key from settings.yaml, get() falls back to the
      // default again
      bool unset(const Util::String &p_Key);
    } // namespace Settings
  } // namespace Cli
} // namespace Low
