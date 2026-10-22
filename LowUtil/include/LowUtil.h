#pragma once

#include "LowUtilApi.h"

#include "LowUtilAssert.h"
#include "LowUtilContainers.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"
#include "LowUtilVariant.h"

#include <cstdint>

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace Low {
  namespace Util {
    LOW_EXPORT void initialize(const Util::String &p_ProjectPath);
    LOW_EXPORT void tick(float p_Delta);
    LOW_EXPORT void cleanup();
    LOW_EXPORT void set_main_window_initially_hidden(bool p_Hidden);
    LOW_EXPORT int execute_command(const String &p_Command,
                                   bool p_HideWindow = true,
                                   String *p_Output = nullptr);

    struct ConfigSettings
    {
    private:
      Util::Map<Util::Name, Util::Variant> values;

      void parse_values(const Util::String p_Prefix,
                        const Util::Serial::Node &p_Node)
      {
        LOW_ASSERT(p_Node.is_dict(),
                   "Cannot parse config settings from yaml node that "
                   "is not a dict.");

        for (auto [key, value] : p_Node) {
          Util::String i_FullName =
              p_Prefix + (p_Prefix.empty() ? "" : "/") + *key;

          if (value.is_dict()) {
            parse_values(i_FullName, value);
          } else {
            values[LOW_NAME(i_FullName.c_str())] =
                Serial::node_to_variant(value);
          }
        }
      }

    public:
      void initialize_from(const Util::Serial::Node &p_Node)
      {
        values.clear();
        parse_values("", p_Node);
      }

      ConfigSettings(const Util::Serial::Node &p_Node)
      {
        initialize_from(p_Node);
      }

      ConfigSettings()
      {
      }

      const Util::Variant &get_checked(const Util::Name p_Name) const
      {
        auto l_Pos = values.find(p_Name);

        LOW_ASSERT(l_Pos != values.end(),
                   "Could not find project setting.");

        return l_Pos->second;
      }

      bool has(const Util::Name p_Name) const
      {
        return values.find(p_Name) != values.end();
      }

      void set(const Util::Name p_Name, const Util::Variant &p_Value)
      {
        values[p_Name] = p_Value;
      }

      void set_float(const Util::Name p_Name, const float p_Value)
      {
        values[p_Name] = p_Value;
      }

      void set_u32(const Util::Name p_Name, const u32 p_Value)
      {
        values[p_Name] = p_Value;
      }

      Util::Serial::Node to_serial_node() const
      {
        Util::Serial::Node l_Node;

        for (auto [key, value] : values) {
          LOW_ASSERT(key.is_valid(),
                     "Cannot serialize unnamed config setting.");

          Util::Serial::Node *l_CurrentNode = &l_Node;
          const char *l_Path = key.c_str();
          Util::String l_Part;

          for (uint32_t i = 0u; l_Path[i] != '\0'; ++i) {
            if (l_Path[i] == '/') {
              if (!l_Part.empty()) {
                l_CurrentNode = &(*l_CurrentNode)[l_Part];
                l_Part.clear();
              }
            } else {
              l_Part += l_Path[i];
            }
          }

          if (l_Part.empty()) {
            continue;
          }

          Util::Serial::Node &l_ValueNode = (*l_CurrentNode)[l_Part];
          if (value.m_Type == Util::VariantType::Bool) {
            l_ValueNode = value.m_Bool;
          } else if (value.m_Type == Util::VariantType::Int32) {
            l_ValueNode = value.m_Int32;
          } else if (value.m_Type == Util::VariantType::UInt32) {
            l_ValueNode = value.m_Uint32;
          } else if (value.m_Type == Util::VariantType::UInt64) {
            l_ValueNode = value.m_Uint64;
          } else if (value.m_Type == Util::VariantType::Float) {
            l_ValueNode = value.m_Float;
          } else if (value.m_Type == Util::VariantType::Name) {
            l_ValueNode = value.as_name().c_str();
          } else if (value.m_Type == Util::VariantType::String) {
            l_ValueNode = value.as_string();
          } else if (value.m_Type == Util::VariantType::UVector2) {
            l_ValueNode = value.m_UVector2;
          } else if (value.m_Type == Util::VariantType::Vector2) {
            l_ValueNode = value.m_Vector2;
          } else if (value.m_Type == Util::VariantType::Vector3) {
            l_ValueNode = value.m_Vector3;
          } else if (value.m_Type == Util::VariantType::Vector4) {
            l_ValueNode = value.m_Vector4;
          } else if (value.m_Type == Util::VariantType::Quaternion) {
            l_ValueNode = value.m_Quaternion;
          } else if (value.m_Type == Util::VariantType::Handle) {
            l_ValueNode = value.m_Uint64;
          } else {
            LOW_ASSERT(
                false,
                "Cannot serialize unknown config setting type.");
          }
        }

        return l_Node;
      }

      const float get_float(const Util::Name p_Name,
                            const float p_Default) const
      {
        auto l_Pos = values.find(p_Name);
        if (l_Pos == values.end()) {
          return p_Default;
        }

        const Util::Variant &l_Value = l_Pos->second;
        if (l_Value.m_Type == Util::VariantType::Float) {
          return l_Value.m_Float;
        }
        if (l_Value.m_Type == Util::VariantType::Int32) {
          return static_cast<float>(l_Value.m_Int32);
        }
        if (l_Value.m_Type == Util::VariantType::UInt32) {
          return static_cast<float>(l_Value.m_Uint32);
        }
        if (l_Value.m_Type == Util::VariantType::UInt64) {
          return static_cast<float>(l_Value.m_Uint64);
        }

        LOW_ASSERT(false, "Project setting is not numeric.");
        return p_Default;
      }

      const u32 get_u32(const Util::Name p_Name,
                        const u32 p_Default) const
      {
        auto l_Pos = values.find(p_Name);
        if (l_Pos == values.end()) {
          return p_Default;
        }

        const Util::Variant &l_Value = l_Pos->second;
        if (l_Value.m_Type == Util::VariantType::UInt32) {
          return l_Value.m_Uint32;
        }
        if (l_Value.m_Type == Util::VariantType::UInt64) {
          return static_cast<u32>(l_Value.m_Uint64);
        }
        if (l_Value.m_Type == Util::VariantType::Int32 &&
            l_Value.m_Int32 >= 0) {
          return static_cast<u32>(l_Value.m_Int32);
        }

        LOW_ASSERT(false,
                   "Project setting is not an unsigned integer.");
        return p_Default;
      }

      const Util::String
      get_string_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_string();
      }
      const Util::Name get_name_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_name();
      }
      const u32 get_u32_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_u32();
      }
      const float get_float_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_float();
      }
      const u64 get_u64_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_u64();
      }
      const bool get_bool_checked(const Util::Name p_Name) const
      {
        return get_checked(p_Name).as_bool();
      }
    };

    enum class EngineSource
    {
      Unknown,
      Packaged,
      Source
    };

    struct Project
    {
      // name: and version: from project.yaml, exactly as written
      String name;
      String version;

      String dataPath;
      String rootPath;
      String assetCachePath;
      String editorImagesPath;
      String engineRootPath;
      String engineDataPath;
      EngineSource engineSource = EngineSource::Unknown;
      String managedPath;
      String visualScriptOut;
      String userPath;

      ConfigSettings settings;
    };
    LOW_EXPORT const Project &get_project();
    LOW_EXPORT ConfigSettings &get_project_settings();

    LOW_EXPORT String get_executable_directory();
    // p_OutSource optionally receives which route found the engine
    LOW_EXPORT String
    find_engine_root(EngineSource *p_OutSource = nullptr);

    /** Walks up from p_StartDirectory to the first folder with a
     project.yaml. Empty if there is none.*/
    LOW_EXPORT String find_project_root(StringView p_StartDirectory);

    /** p_Path can be the project folder or its project.yaml. Empty if
     it is not a project.*/
    LOW_EXPORT String resolve_project_path(StringView p_Path);

    LOW_EXPORT bool save_project_settings();

    struct LOW_EXPORT ProjectPathBuilder
    {
      ProjectPathBuilder() = default;
      ProjectPathBuilder(String p_RootPath);

      ProjectPathBuilder &join(const String &p_PathPart);
      ProjectPathBuilder &join(const char *p_PathPart);

      String get() const;
      operator String() const;

    private:
      StringBuilder m_Builder;
      bool m_HasPath = false;
    };

    LOW_EXPORT ProjectPathBuilder project_data_path();
    LOW_EXPORT ProjectPathBuilder project_root_path();
    LOW_EXPORT ProjectPathBuilder project_asset_cache_path();
    LOW_EXPORT ProjectPathBuilder project_visual_script_out_path();
    LOW_EXPORT ProjectPathBuilder project_managed_path();
    LOW_EXPORT ProjectPathBuilder project_editor_images_path();
    LOW_EXPORT ProjectPathBuilder engine_root_path();
    LOW_EXPORT ProjectPathBuilder engine_data_path();

    LOW_EXPORT String project_data_path(const String &p_RelativePath);
    LOW_EXPORT String project_root_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_asset_cache_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_editor_images_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_managed_path(const String &p_RelativePath);
    LOW_EXPORT String
    project_visual_script_out_path(const String &p_RelativePath);
    LOW_EXPORT String engine_root_path(const String &p_RelativePath);
    LOW_EXPORT String engine_data_path(const String &p_RelativePath);

    /** Turns a path into the form stored in project files:
     relative to the project root if it lies inside the project,
     otherwise unchanged
     */
    LOW_EXPORT String to_project_relative_path(StringView p_Path);

    /** Inverse of to_project_relative_path. Absolute paths are
     returned unchanged, relative ones are joined onto the project
     root (a leading "./" is accepted for older files)*/
    LOW_EXPORT String from_project_relative_path(StringView p_Path);

    /** Whether the path (absolute or project relative) lies inside
     the project folder */
    LOW_EXPORT bool is_inside_project(StringView p_Path);

    struct LOW_EXPORT Window
    {
      SDL_Window *sdlwindow;

      bool shouldClose = false;
      bool minimized = false;
      bool customDecorations = false;
      int customTitleBarHeight = 0;
      int customTitleBarInteractiveLeft = 0;
      int customTitleBarControlsLeft = 0;
      int customResizeBorder = 6;
      void *nativeHandle = nullptr;
      void *nativeWndProc = nullptr;
      std::intptr_t nativeStyle = 0;

      typedef bool (*EventCallback)(const SDL_Event *);
      Util::List<EventCallback> eventCallbacks;

      void get_size(int *p_Width, int *p_Height);
      void set_custom_decorations(bool p_Enabled);
      void set_custom_title_bar_hit_zones(int p_Height,
                                          int p_InteractiveLeft,
                                          int p_ControlsLeft);
      void minimize();
      void maximize_or_restore();
      void request_close();
      void show();
      void hide();

      static Window &get_main_window();
    };
  } // namespace Util
} // namespace Low
