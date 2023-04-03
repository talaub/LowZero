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

      static void print_time()
      {
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%F %X", timeinfo);

        g_LogStream << "\x1B[90m" << buffer << "\033[0m\t";
      }

      static void print_thread_id()
      {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        int id = std::stoi(ss.str());
        g_LogStream << "\x1B[35m" << id << "\033[0m\t";
      }

      static void print_log_level(uint8_t p_LogLevel)
      {
        if (p_LogLevel == LogLevel::DEBUG) {
          g_LogStream << "\x1B[96mDEBUG\033[0m\t";
        } else if (p_LogLevel == LogLevel::INFO) {
          g_LogStream << "\x1B[92mINFO \033[0m\t";
        } else if (p_LogLevel == LogLevel::WARN) {
          g_LogStream << "\x1B[33mWARN \033[0m\t";
        } else if (p_LogLevel == LogLevel::ERROR) {
          g_LogStream << "\x1B[31mERROR \033[0m\t";
        } else if (p_LogLevel == LogLevel::PROFILE) {
          g_LogStream << "\x1B[35mPRFLR\033[0m\t";
        }
      }

      static void print_module(const char *p_Module)
      {
        char l_ModBuffer[13];
        for (uint32_t i = 0u; i < 13; ++i) {
          l_ModBuffer[i] = '\0';
          if (i < 12) {
            l_ModBuffer[i] = ' ';
          }
        }

        for (uint32_t i = 0u; i < strlen(p_Module); ++i) {
          if (i == 12) {
            break;
          }
          l_ModBuffer[i] = p_Module[i];
        }

        g_LogStream << "\x1B[90m[" << l_ModBuffer << "]\033[0m ";
      }

      LogStream &begin_log(uint8_t p_LogLevel, const char *p_Module)
      {
        g_LogStream.m_Content = "";

        print_time();
        print_thread_id();
        print_log_level(p_LogLevel);
        print_module(p_Module);
        g_LogStream << " - ";

        return g_LogStream;
      }

      LogStream &LogStream::operator<<(LogLineEnd p_LineEnd)
      {
        printf("%s\n", m_Content.c_str());
        return *this;
      }

      LogStream &LogStream::operator<<(String &p_Message)
      {
        m_Content += p_Message;
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
