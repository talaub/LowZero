#include "LowUtilLogger.h"

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <ctime>
#include <string.h>
#include <time.h>
#include <thread>
#include <sstream>

#include "LowUtilHandle.h"

#include <microprofile.h>

namespace Low {
  namespace Util {
    namespace Log {
      LogStream g_LogStream;

      List<LogCallback> g_Callbacks;

      void register_log_callback(LogCallback p_Callback)
      {
        g_Callbacks.push_back(p_Callback);
      }

      static void print_time(LogEntry &p_Entry)
      {
        struct tm *timeinfo;
        char buffer[80];
        timeinfo = localtime(&p_Entry.time);

        strftime(buffer, 80, "%F %X", timeinfo);

        printf("\x1B[90m%s\033[0m\t", buffer);
      }

      static void print_thread_id(LogEntry &p_Entry)
      {
        printf("\x1B[35m%i\033[0m\t", p_Entry.threadId);
      }

      static void print_log_level(uint8_t p_LogLevel)
      {
        if (p_LogLevel == LogLevel::DEBUG) {
          printf("\x1B[96mDEBUG\033[0m\t");
        } else if (p_LogLevel == LogLevel::INFO) {
          printf("\x1B[92mINFO \033[0m\t");
        } else if (p_LogLevel == LogLevel::WARN) {
          printf("\x1B[33mWARN \033[0m\t");
        } else if (p_LogLevel == LogLevel::ERROR) {
          printf("\x1B[31mERROR \033[0m\t");
        } else if (p_LogLevel == LogLevel::PROFILE) {
          printf("\x1B[35mPRFLR\033[0m\t");
        }
      }

      static void print_module(LogEntry &p_Entry)
      {
        char l_ModBuffer[13];
        for (uint32_t i = 0u; i < 13; ++i) {
          l_ModBuffer[i] = '\0';
          if (i < 12) {
            l_ModBuffer[i] = ' ';
          }
        }

        for (uint32_t i = 0u; i < p_Entry.module.size(); ++i) {
          if (i == 12) {
            break;
          }
          l_ModBuffer[i] = p_Entry.module[i];
        }

        printf("\x1B[90m[%s]\033[0m ", l_ModBuffer);
      }

      LogStream &begin_log(uint8_t p_LogLevel, const char *p_Module)
      {
        g_LogStream.m_Entry = LogEntry();

        g_LogStream.m_Entry.level = p_LogLevel;
        g_LogStream.m_Entry.module = p_Module;
        time(&g_LogStream.m_Entry.time);

        std::stringstream ss;
        ss << std::this_thread::get_id();
        g_LogStream.m_Entry.threadId = std::stoi(ss.str());

        return g_LogStream;
      }

      LogStream &LogStream::operator<<(LogLineEnd p_LineEnd)
      {
        print_time(m_Entry);
        print_thread_id(m_Entry);
        print_log_level(m_Entry.level);
        print_module(m_Entry);
        printf("- %s\n", m_Entry.message.c_str());

        for (uint32_t i = 0u; i < g_Callbacks.size(); ++i) {
          g_Callbacks[i](m_Entry);
        }

        return *this;
      }

      LogStream &LogStream::operator<<(String &p_Message)
      {
        m_Entry.message += p_Message;
        return g_LogStream;
      }

      LogStream &LogStream::operator<<(const char *p_Message)
      {
        return *this << String(p_Message);
      }

      LogStream &LogStream::operator<<(std::string &p_Message)
      {
        return *this << String(p_Message.c_str());
      }

      LogStream &LogStream::operator<<(Name &p_Name)
      {
        return *this << p_Name.c_str();
      }

      LogStream &LogStream::operator<<(Handle &p_Message)
      {
        RTTI::TypeInfo &l_TypeInfo =
            Handle::get_type_info(p_Message.get_type());
        return *this << l_TypeInfo.name << "(Index: " << p_Message.get_index()
                     << ", Generation: " << p_Message.get_generation()
                     << ", Alive: " << l_TypeInfo.is_alive(p_Message) << ")";
      }

      LogStream &LogStream::operator<<(int p_Message)
      {
        return *this << std::to_string(p_Message);
      }

      LogStream &LogStream::operator<<(uint32_t p_Message)
      {
        return *this << std::to_string(p_Message);
      }

      LogStream &LogStream::operator<<(uint64_t p_Message)
      {
        return *this << std::to_string(p_Message);
      }

      LogStream &LogStream::operator<<(float p_Message)
      {
        return *this << std::to_string(p_Message);
      }

      LogStream &LogStream::operator<<(bool p_Message)
      {
        if (p_Message) {
          return *this << "true";
        } else {
          return *this << "false";
        }
      }

      LogStream &LogStream::operator<<(Math::Vector2 &p_Vec)
      {
        return *this << "Vector2(" << p_Vec.x << ", " << p_Vec.y << ")";
      }

      LogStream &LogStream::operator<<(Math::Vector3 &p_Vec)
      {
        return *this << "Vector3(" << p_Vec.x << ", " << p_Vec.y << ", "
                     << p_Vec.z << ")";
      }

      LogStream &LogStream::operator<<(Math::Vector4 &p_Vec)
      {
        return *this << "Vector4(" << p_Vec.x << ", " << p_Vec.y << ", "
                     << p_Vec.z << ", " << p_Vec.w << ")";
      }

      LogStream &LogStream::operator<<(Math::Quaternion &p_Quat)
      {
        return *this << "Quaternion(" << p_Quat.x << ", " << p_Quat.y << ", "
                     << p_Quat.z << ", " << p_Quat.w << ")";
      }

      LogStream &LogStream::operator<<(Math::UVector2 &p_Vec)
      {
        return *this << "UVector2(" << p_Vec.x << ", " << p_Vec.y << ")";
      }

      LogStream &LogStream::operator<<(Math::UVector3 &p_Vec)
      {
        return *this << "UVector3(" << p_Vec.x << ", " << p_Vec.y << ", "
                     << p_Vec.z << ")";
      }
    } // namespace Log
  }   // namespace Util
} // namespace Low
