#include "LowCliCommands.h"

#include "LowCliOutput.h"
#include "LowCliSettings.h"

#include <string>

namespace Low {
  namespace Cli {
    namespace Commands {
      // CLI11 writes the arguments here during parse(), so these have
      // to outlive register_config
      static struct ConfigArgs
      {
        std::string key;
        std::string value;
      } g_ConfigArgs;

      static bool initialize_settings()
      {
        if (Settings::initialize()) {
          return true;
        }

        if (Settings::get_file_version() > Settings::FORMAT_VERSION) {
          Output::error(
              "The settings file was written by a newer engine",
              {Settings::get_settings_path(),
               "format version " +
                   Util::String(
                       std::to_string(Settings::get_file_version())
                           .c_str())},
              "change it with the low of a newer engine");
          return false;
        }
        Output::error("Could not read the settings file",
                      {Settings::get_settings_path()},
                      "fix or delete the file and try again");
        return false;
      }

      // Unknown keys are reported with the list of known ones
      static bool require_key(const Util::String &p_Key)
      {
        if (Settings::is_key(p_Key)) {
          return true;
        }

        Util::List<Util::String> l_Keys;
        for (const Util::String &i_Key : Settings::get_keys()) {
          l_Keys.push_back(i_Key + " - " + Settings::get_description(i_Key));
        }
        Output::error("Unknown setting: " + p_Key, l_Keys,
                      "run low config get to see all settings");
        return false;
      }

      static Util::String describe_source(const Util::String &p_Key,
                                          const Settings::Value &p_Value)
      {
        switch (p_Value.source) {
        case Settings::ValueSource::Environment:
          return "from the environment variable " +
                 Settings::get_environment_variable(p_Key);
        case Settings::ValueSource::File:
          return "from " + Settings::get_settings_path();
        case Settings::ValueSource::Default:
          return "default";
        }
        return "";
      }

      // Settings are per machine, the context (project) is not needed
      static int config_get(Context &)
      {
        if (!initialize_settings()) {
          return 1;
        }

        const Util::String l_Key = g_ConfigArgs.key.c_str();

        // Single key: only the value goes to stdout, so scripts can use
        // it directly, e.g. $store = low config get symbol_store
        if (!l_Key.empty()) {
          if (!require_key(l_Key)) {
            return 1;
          }
          const Settings::Value l_Value = Settings::get(l_Key);
          Output::result(l_Value.value);
          Output::detail(describe_source(l_Key, l_Value));
          return 0;
        }

        Output::detail(Settings::get_settings_path());
        for (const Util::String &i_Key : Settings::get_keys()) {
          const Settings::Value l_Value = Settings::get(i_Key);
          Output::result(i_Key + ": " + l_Value.value);
          Output::detail(describe_source(i_Key, l_Value));
        }
        return 0;
      }

      // An environment variable wins over the file, so a set or unset
      // would seem to have no effect
      static void warn_on_environment_override(const Util::String &p_Key)
      {
        const Settings::Value l_Value = Settings::get(p_Key);
        if (l_Value.source != Settings::ValueSource::Environment) {
          return;
        }
        Output::warn("The environment variable " +
                         Settings::get_environment_variable(p_Key) +
                         " overrides this setting",
                     {"currently used: " + l_Value.value});
      }

      static int config_set(Context &)
      {
        if (!initialize_settings()) {
          return 1;
        }

        const Util::String l_Key = g_ConfigArgs.key.c_str();
        if (!require_key(l_Key)) {
          return 1;
        }

        const Util::String l_Value = g_ConfigArgs.value.c_str();
        if (l_Value.empty()) {
          Output::error("The value must not be empty", {},
                        "use low config unset " + l_Key +
                            " to go back to the default");
          return 1;
        }

        if (!Settings::set(l_Key, l_Value)) {
          Output::error("Could not write the settings file",
                        {Settings::get_settings_path()},
                        "check that the folder is writable");
          return 1;
        }

        // Read back, path values are stored absolute
        Output::success("Set " + l_Key);
        Output::detail(Settings::get(l_Key).value);
        warn_on_environment_override(l_Key);
        return 0;
      }

      static int config_unset(Context &)
      {
        if (!initialize_settings()) {
          return 1;
        }

        const Util::String l_Key = g_ConfigArgs.key.c_str();
        if (!require_key(l_Key)) {
          return 1;
        }

        if (!Settings::unset(l_Key)) {
          Output::error("Could not write the settings file",
                        {Settings::get_settings_path()},
                        "check that the folder is writable");
          return 1;
        }

        Output::success("Unset " + l_Key);
        Output::detail("now: " + Settings::get(l_Key).value);
        warn_on_environment_override(l_Key);
        return 0;
      }

      void register_config(CLI::App &p_App)
      {
        CLI::App *l_Config = p_App.add_subcommand(
            "config", "Settings of this machine, e.g. where symbols are "
                      "stored");
        l_Config->require_subcommand(1);

        CLI::App *l_Get = l_Config->add_subcommand(
            "get", "Show one or all settings and where they come from");
        l_Get->add_option("key", g_ConfigArgs.key,
                          "Setting to show, all if omitted");
        add_command(l_Get, config_get);

        CLI::App *l_Set =
            l_Config->add_subcommand("set", "Change a setting");
        l_Set->add_option("key", g_ConfigArgs.key, "Setting to change")
            ->required();
        l_Set->add_option("value", g_ConfigArgs.value, "New value")
            ->required();
        add_command(l_Set, config_set);

        CLI::App *l_Unset = l_Config->add_subcommand(
            "unset", "Reset a setting to its default");
        l_Unset->add_option("key", g_ConfigArgs.key, "Setting to reset")
            ->required();
        add_command(l_Unset, config_unset);
      }
    } // namespace Commands
  } // namespace Cli
} // namespace Low
