#include "LowCliEngineRegistry.h"

#include "LowUtilString.h"
#include "LowUtilYaml.h"

#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <stdlib.h>

// The registry format is also read by LowShim/src/main.cpp - keep both
// in sync, the shim has to work across engine versions.

namespace Low {
  namespace Cli {
    namespace EngineRegistry {
      namespace fs = std::filesystem;

      Util::Map<Util::String, Util::String> g_Registry;
      Util::String g_Default;
      u32 g_FileVersion = FORMAT_VERSION;

      bool g_Initialized = false;

      static fs::path to_path(const Util::String &p_Path)
      {
        return fs::path(std::u8string(
            reinterpret_cast<const char8_t *>(p_Path.c_str()),
            p_Path.size()));
      }

      static Util::String from_path(const fs::path &p_Path)
      {
        const std::u8string l_Path = p_Path.generic_u8string();
        return Util::String(
            reinterpret_cast<const char *>(l_Path.data()),
            l_Path.size());
      }

      static Util::String normalize_path(const Util::String &p_Path)
      {
        Util::String l_Path = Util::PathHelper::normalize(p_Path);
        while (l_Path.size() > 1 && l_Path.back() == '/') {
          l_Path.pop_back();
        }
        return l_Path;
      }

      Util::String get_registry_directory()
      {
#ifdef _WIN32
        const wchar_t *l_AppData = _wgetenv(L"LOCALAPPDATA");
        if (!l_AppData || !*l_AppData) {
          return "";
        }
        return from_path(fs::path(l_AppData)) + "/LowEngine";
#else
        const char *l_DataHome = getenv("XDG_DATA_HOME");
        if (l_DataHome && *l_DataHome) {
          return normalize_path(l_DataHome) + "/LowEngine";
        }
        const char *l_Home = getenv("HOME");
        if (!l_Home || !*l_Home) {
          return "";
        }
        return normalize_path(l_Home) + "/.local/share/LowEngine";
#endif
      }

      Util::String get_registry_path()
      {
        const Util::String l_Directory = get_registry_directory();
        if (l_Directory.empty()) {
          return "";
        }
        return l_Directory + "/engines.yaml";
      }

      static bool write_registry()
      {
        const Util::String l_Directory = get_registry_directory();
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
        if (!g_Default.empty()) {
          l_Out << YAML::Key << "default" << YAML::Value
                << g_Default.c_str();
        }
        l_Out << YAML::Key << "engines" << YAML::Value
              << YAML::BeginMap;
        for (auto &i_Entry : g_Registry) {
          l_Out << YAML::Key << i_Entry.first.c_str() << YAML::Value
                << i_Entry.second.c_str();
        }
        l_Out << YAML::EndMap << YAML::EndMap;
        if (!l_Out.good()) {
          return false;
        }

        // Written next to it first and then swapped in, so an
        // interrupted write never leaves a broken registry behind
        const fs::path l_Path = to_path(get_registry_path());
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

        g_Registry.clear();
        g_Default.clear();
        g_FileVersion = FORMAT_VERSION;

        const Util::String l_Path = get_registry_path();
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

          // Files from before the version key are version 1
          g_FileVersion =
              l_Root["version"] ? l_Root["version"].as<u32>() : 1;
          if (g_FileVersion > FORMAT_VERSION) {
            // Never read or rewrite a format we don't know
            return false;
          }

          if (l_Root["default"]) {
            g_Default = l_Root["default"].as<std::string>().c_str();
          }

          const Util::Yaml::Node l_Engines = l_Root["engines"];
          if (l_Engines && l_Engines.IsMap()) {
            for (const auto &i_Entry : l_Engines) {
              g_Registry[i_Entry.first.as<std::string>().c_str()] =
                  normalize_path(
                      i_Entry.second.as<std::string>().c_str());
            }
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

      bool paths_equal(const Util::String &p_A,
                       const Util::String &p_B)
      {
        const Util::String l_A = normalize_path(p_A);
        const Util::String l_B = normalize_path(p_B);
#ifdef _WIN32
        if (l_A.size() != l_B.size()) {
          return false;
        }
        for (size_t i = 0; i < l_A.size(); ++i) {
          if (tolower(static_cast<unsigned char>(l_A[i])) !=
              tolower(static_cast<unsigned char>(l_B[i]))) {
            return false;
          }
        }
        return true;
#else
        return l_A == l_B;
#endif
      }

      bool has_key(const Util::String &p_Key)
      {
        return g_Registry.find(p_Key) != g_Registry.end();
      }

      Util::String get_key(const Util::String &p_Key)
      {
        auto i_It = g_Registry.find(p_Key);
        if (i_It == g_Registry.end()) {
          return "";
        }
        return i_It->second;
      }

      Util::String get_default()
      {
        return g_Default;
      }

      bool force_set(const Util::String &p_Id,
                     const Util::String &p_Path)
      {
        g_Registry[p_Id] = normalize_path(p_Path);
        if (g_Default.empty()) {
          g_Default = p_Id;
        }
        return write_registry();
      }

      bool has_path(const Util::String &p_Path)
      {
        return !get_id_for_path(p_Path).empty();
      }

      Util::String get_id_for_path(const Util::String &p_Path)
      {
        for (auto &i_Entry : g_Registry) {
          if (paths_equal(i_Entry.second, p_Path)) {
            return i_Entry.first;
          }
        }
        return "";
      }
    } // namespace EngineRegistry
  } // namespace Cli
} // namespace Low
