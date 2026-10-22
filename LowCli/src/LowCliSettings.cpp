#include "LowCliSettings.h"

#include "LowCliCommands.h"
#include "LowCliEngineRegistry.h"

#include "LowUtilYaml.h"

#include <filesystem>
#include <fstream>
#include <stdlib.h>
#include <string.h>

namespace Low {
  namespace Cli {
    namespace Settings {
      namespace fs = std::filesystem;

      struct KeyInfo
      {
        const char *name;
        const char *description;
        // nullptr: no override
        const char *environmentVariable;
        Util::String (*getDefault)();
        bool isPath;
      };

      static Util::String get_default_symbol_store()
      {
        const Util::String l_Directory =
            EngineRegistry::get_registry_directory();
        if (l_Directory.empty()) {
          return "";
        }
        return l_Directory + "/symbols";
      }

      static const KeyInfo g_Keys[] = {
          {"symbol_store",
           "Folder the PDBs of packaged builds are stored in (symbol "
           "server layout)",
           "LOW_SYMBOL_STORE", &get_default_symbol_store, true},
      };

      // Everything from the file, also keys this low does not know
      // (e.g. written by a newer engine), so set() does not drop them
      Util::Map<Util::String, Util::String> g_Values;
      u32 g_FileVersion = FORMAT_VERSION;
      bool g_Initialized = false;

      static const KeyInfo *find_key(const Util::String &p_Key)
      {
        for (const KeyInfo &i_Key : g_Keys) {
          if (p_Key == i_Key.name) {
            return &i_Key;
          }
        }
        return nullptr;
      }

      // Relative paths are relative to the current directory, so
      // `low config set symbol_store .` works as expected
      static Util::String normalize_path_value(const Util::String &p_Value)
      {
        std::error_code l_Error;
        fs::path l_Path = fs::absolute(to_path(p_Value), l_Error);
        if (l_Error) {
          l_Path = to_path(p_Value);
        }

        Util::String l_Result = from_path(l_Path.lexically_normal());
        // Keep the slash of a drive root, "D:" alone means the current
        // directory on D
        while (l_Result.size() > 1 && l_Result.back() == '/' &&
               l_Result[l_Result.size() - 2] != ':') {
          l_Result.pop_back();
        }
        return l_Result;
      }

      static Util::String get_environment(const char *p_Name)
      {
#ifdef _WIN32
        // _wgetenv because getenv returns the ANSI codepage
        const std::wstring l_Name(p_Name, p_Name + strlen(p_Name));
        const wchar_t *l_Value = _wgetenv(l_Name.c_str());
        if (!l_Value || !*l_Value) {
          return "";
        }
        const std::u8string l_Utf8 = fs::path(l_Value).u8string();
        return Util::String(reinterpret_cast<const char *>(l_Utf8.data()),
                            l_Utf8.size());
#else
        const char *l_Value = getenv(p_Name);
        if (!l_Value) {
          return "";
        }
        return l_Value;
#endif
      }

      Util::String get_settings_path()
      {
        const Util::String l_Directory =
            EngineRegistry::get_registry_directory();
        if (l_Directory.empty()) {
          return "";
        }
        return l_Directory + "/settings.yaml";
      }

      static bool write_settings()
      {
        const Util::String l_Directory =
            EngineRegistry::get_registry_directory();
        if (l_Directory.empty()) {
          return false;
        }

        std::error_code l_Error;
        fs::create_directories(to_path(l_Directory), l_Error);
        if (l_Error) {
          return false;
        }

        Util::Yaml::Emitter l_Out;
        l_Out << YAML::BeginMap;
        l_Out << YAML::Key << "version" << YAML::Value << FORMAT_VERSION;
        for (auto &i_Entry : g_Values) {
          l_Out << YAML::Key << i_Entry.first.c_str() << YAML::Value
                << i_Entry.second.c_str();
        }
        l_Out << YAML::EndMap;
        if (!l_Out.good()) {
          return false;
        }

        // Same as the registry: written next to it first and then
        // swapped in, so an interrupted write never leaves a broken file
        const fs::path l_Path = to_path(get_settings_path());
        fs::path l_TempPath = l_Path;
        l_TempPath += ".tmp";
        {
          std::ofstream l_File(l_TempPath,
                               std::ios::binary | std::ios::trunc);
          if (!l_File) {
            return false;
          }
          l_File << l_Out.c_str() << "\n";
          l_File.close();
          if (l_File.fail()) {
            return false;
          }
        }

        fs::rename(l_TempPath, l_Path, l_Error);
        return !l_Error;
      }

      bool initialize()
      {
        if (g_Initialized) {
          return true;
        }

        g_Values.clear();
        g_FileVersion = FORMAT_VERSION;

        const Util::String l_Path = get_settings_path();
        if (l_Path.empty()) {
          return false;
        }

        std::error_code l_Error;
        if (!fs::exists(to_path(l_Path), l_Error)) {
          g_Initialized = true;
          return true;
        }

        try {
          std::ifstream l_File(to_path(l_Path), std::ios::binary);
          if (!l_File) {
            return false;
          }

          const Util::Yaml::Node l_Root = YAML::Load(l_File);
          if (l_Root.IsNull()) {
            // Empty file
            g_Initialized = true;
            return true;
          }
          if (!l_Root.IsMap()) {
            return false;
          }

          g_FileVersion =
              l_Root["version"] ? l_Root["version"].as<u32>() : 1;
          if (g_FileVersion > FORMAT_VERSION) {
            // Never read or rewrite a format we don't know
            return false;
          }

          // Scalar() keeps the text as written, as<> would turn
          // values like 2026.6 into numbers
          for (const auto &i_Entry : l_Root) {
            const std::string l_Key = i_Entry.first.Scalar();
            if (l_Key == "version" || !i_Entry.second.IsScalar()) {
              continue;
            }
            g_Values[l_Key.c_str()] = i_Entry.second.Scalar().c_str();
          }
        } catch (const std::exception &) {
          return false;
        }

        g_Initialized = true;
        return true;
      }

      u32 get_file_version()
      {
        return g_FileVersion;
      }

      Util::List<Util::String> get_keys()
      {
        Util::List<Util::String> l_Keys;
        for (const KeyInfo &i_Key : g_Keys) {
          l_Keys.push_back(i_Key.name);
        }
        return l_Keys;
      }

      bool is_key(const Util::String &p_Key)
      {
        return find_key(p_Key) != nullptr;
      }

      Util::String get_description(const Util::String &p_Key)
      {
        const KeyInfo *l_Key = find_key(p_Key);
        return l_Key ? l_Key->description : "";
      }

      Util::String get_environment_variable(const Util::String &p_Key)
      {
        const KeyInfo *l_Key = find_key(p_Key);
        if (!l_Key || !l_Key->environmentVariable) {
          return "";
        }
        return l_Key->environmentVariable;
      }

      Value get(const Util::String &p_Key)
      {
        Value l_Result;
        const KeyInfo *l_Key = find_key(p_Key);
        if (!l_Key) {
          return l_Result;
        }

        if (l_Key->environmentVariable) {
          l_Result.value = get_environment(l_Key->environmentVariable);
          l_Result.source = ValueSource::Environment;
        }

        if (l_Result.value.empty()) {
          auto l_It = g_Values.find(p_Key);
          if (l_It != g_Values.end() && !l_It->second.empty()) {
            l_Result.value = l_It->second;
            l_Result.source = ValueSource::File;
          }
        }

        if (l_Result.value.empty()) {
          l_Result.value = l_Key->getDefault ? l_Key->getDefault() : "";
          l_Result.source = ValueSource::Default;
        }

        if (l_Key->isPath && !l_Result.value.empty()) {
          l_Result.value = normalize_path_value(l_Result.value);
        }
        return l_Result;
      }

      bool set(const Util::String &p_Key, const Util::String &p_Value)
      {
        const KeyInfo *l_Key = find_key(p_Key);
        if (!l_Key || !g_Initialized) {
          return false;
        }

        g_Values[p_Key] =
            l_Key->isPath ? normalize_path_value(p_Value) : p_Value;
        return write_settings();
      }

      bool unset(const Util::String &p_Key)
      {
        if (!find_key(p_Key) || !g_Initialized) {
          return false;
        }

        g_Values.erase(p_Key);
        return write_settings();
      }
    } // namespace Settings
  } // namespace Cli
} // namespace Low
