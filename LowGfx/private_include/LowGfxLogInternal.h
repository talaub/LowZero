#pragma once

#include "LowGfxBackend.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <utility>

namespace Low {
  namespace Gfx {
    namespace Detail {
      inline const char *log_level_name(LogLevel p_Level)
      {
        switch (p_Level) {
        case LogLevel::Trace:
          return "trace";
        case LogLevel::Debug:
          return "debug";
        case LogLevel::Info:
          return "info";
        case LogLevel::Warning:
          return "warning";
        case LogLevel::Error:
          return "error";
        case LogLevel::Fatal:
          return "fatal";
        }

        return "unknown";
      }

      inline void log(ContextImpl &p_Context, LogLevel p_Level,
                      const char *p_Message)
      {
        const char *l_Message = p_Message ? p_Message : "";

        if (p_Context.log_callback) {
          p_Context.log_callback(p_Level, l_Message,
                                 p_Context.log_user_data);
          return;
        }

        std::fprintf(stderr, "[LowGfx] %s: %s\n",
                     log_level_name(p_Level), l_Message);
      }

      inline void log(InstanceImpl &p_Instance, LogLevel p_Level,
                      const char *p_Message)
      {
        const char *l_Message = p_Message ? p_Message : "";

        if (p_Instance.log_callback) {
          p_Instance.log_callback(p_Level, l_Message,
                                  p_Instance.log_user_data);
          return;
        }

        std::fprintf(stderr, "[LowGfx] %s: %s\n",
                     log_level_name(p_Level), l_Message);
      }

      inline void append_formatted_value(std::string &p_Output,
                                         const char *p_Value)
      {
        p_Output += p_Value ? p_Value : "(null)";
      }

      inline void append_formatted_value(std::string &p_Output,
                                         char *p_Value)
      {
        append_formatted_value(p_Output,
                               static_cast<const char *>(p_Value));
      }

      inline void append_formatted_value(std::string &p_Output,
                                         bool p_Value)
      {
        p_Output += p_Value ? "true" : "false";
      }

      template <typename T>
      void append_formatted_value(std::string &p_Output,
                                  const T &p_Value)
      {
        std::ostringstream l_Stream;
        l_Stream << p_Value;
        p_Output += l_Stream.str();
      }

      inline void append_format(std::string &p_Output,
                                const char *p_Format)
      {
        p_Output += p_Format ? p_Format : "";
      }

      template <typename Arg, typename... Args>
      void append_format(std::string &p_Output, const char *p_Format,
                         Arg &&p_Arg, Args &&...p_Args)
      {
        if (!p_Format) {
          return;
        }

        const char *l_Read = p_Format;
        while (*l_Read) {
          if (l_Read[0] == '{' && l_Read[1] == '}') {
            p_Output.append(p_Format,
                            static_cast<size_t>(l_Read - p_Format));
            append_formatted_value(p_Output,
                                   std::forward<Arg>(p_Arg));
            append_format(p_Output, l_Read + 2,
                          std::forward<Args>(p_Args)...);
            return;
          }

          ++l_Read;
        }

        p_Output += p_Format;
      }

      template <typename... Args>
      void logf(ContextImpl &p_Context, LogLevel p_Level,
                const char *p_Format, Args &&...p_Args)
      {
        std::string l_Message;
        append_format(l_Message, p_Format,
                      std::forward<Args>(p_Args)...);
        log(p_Context, p_Level, l_Message.c_str());
      }

      template <typename... Args>
      void logf(InstanceImpl &p_Instance, LogLevel p_Level,
                const char *p_Format, Args &&...p_Args)
      {
        std::string l_Message;
        append_format(l_Message, p_Format,
                      std::forward<Args>(p_Args)...);
        log(p_Instance, p_Level, l_Message.c_str());
      }
    } // namespace Detail
  } // namespace Gfx
} // namespace Low
