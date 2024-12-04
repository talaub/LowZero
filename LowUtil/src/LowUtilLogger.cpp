#include "LowUtilLogger.h"

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <ctime>
#include <string.h>
#include <string>
#include <time.h>
#include <thread>
#include <sstream>

#include "LowUtil.h"
#include "LowUtilHandle.h"
#include "LowUtilJobManager.h"
#include "LowUtilFileIO.h"

#include <microprofile.h>

namespace Low {
  namespace Util {
    namespace Log {
      LogStream g_LogStream;

      List<LogCallback> g_Callbacks;

      JobManager::ThreadPool *g_ThreadPool;

      String g_LogFilePath;

      void initialize()
      {
        g_ThreadPool = new JobManager::ThreadPool(1);

        g_LogFilePath = get_project().rootPath + "/low.log";

        // Clear log file
        FileIO::File l_LogFile = FileIO::open(
            g_LogFilePath.c_str(), FileIO::FileMode::WRITE);

        String l_HeaderString =
            "LowEngine log output\nEngine version: ";
        l_HeaderString += LOW_VERSION_YEAR;
        l_HeaderString += ".";
        l_HeaderString += LOW_VERSION_MAJOR;
        l_HeaderString += ".";
        l_HeaderString += LOW_VERSION_MINOR;
        l_HeaderString += "\n----------------------\n";

        FileIO::write_sync(l_LogFile, l_HeaderString);
        FileIO::close(l_LogFile);
      }

      void cleanup()
      {
        if (g_ThreadPool) {
          delete g_ThreadPool;
        }
      }

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

#ifdef LOW_COLOR_LOG
        printf("\x1B[90m%s\033[0m\t", buffer);
#else
        printf("%s\t", buffer);
#endif
      }

      static void print_thread_id(LogEntry &p_Entry)
      {
#ifdef LOW_COLOR_LOG
        printf("\x1B[35m%i\033[0m\t", p_Entry.threadId);
#else
        printf("%i\t", p_Entry.threadId);
#endif
      }

      static void print_log_level(uint8_t p_LogLevel)
      {
#ifdef LOW_COLOR_LOG
        if (p_LogLevel == LogLevel::DEBUG) {
          printf("\x1B[96mDEBUG\033[0m\t");
        } else if (p_LogLevel == LogLevel::INFO) {
          printf("\x1B[92mINFO \033[0m\t");
        } else if (p_LogLevel == LogLevel::WARN) {
          printf("\x1B[33mWARN \033[0m\t");
        } else if (p_LogLevel == LogLevel::ERROR) {
          printf("\x1B[31mERROR \033[0m\t");
        } else if (p_LogLevel == LogLevel::FATAL) {
          printf("\x1B[31mFATAL \033[0m\t");
        } else if (p_LogLevel == LogLevel::PROFILE) {
          printf("\x1B[35mPRFLR\033[0m\t");
        }
#else
        if (p_LogLevel == LogLevel::DEBUG) {
          printf("DEBUG\t");
        } else if (p_LogLevel == LogLevel::INFO) {
          printf("INFO \t");
        } else if (p_LogLevel == LogLevel::WARN) {
          printf("WARN \t");
        } else if (p_LogLevel == LogLevel::ERROR) {
          printf("ERROR \t");
        } else if (p_LogLevel == LogLevel::FATAL) {
          printf("FATAL \t");
        } else if (p_LogLevel == LogLevel::PROFILE) {
          printf("PRFLR\t");
        }
#endif
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

#ifdef LOW_COLOR_LOG
        printf("\x1B[90m[%s]\033[0m ", l_ModBuffer);
#else
        printf("[%s] ", l_ModBuffer);
#endif
      }

      LogStream &begin_log(uint8_t p_LogLevel, const char *p_Module,
                           bool p_Terminate)
      {
        g_LogStream.m_Entry = LogEntry();

        g_LogStream.m_Entry.terminate = p_Terminate;
        if (p_LogLevel == LogLevel::ERR) {
          g_LogStream.m_Entry.level = LogLevel::ERROR;
        } else {
          g_LogStream.m_Entry.level = p_LogLevel;
        }
        g_LogStream.m_Entry.module = p_Module;
        time(&g_LogStream.m_Entry.time);

        std::stringstream ss;
        ss << std::this_thread::get_id();
        g_LogStream.m_Entry.threadId = std::stoi(ss.str());

        return g_LogStream;
      }

      static void log_to_file(LogEntry p_Entry)
      {
        FileIO::File l_LogFile = FileIO::open(
            g_LogFilePath.c_str(), FileIO::FileMode::APPEND);

        struct tm *timeinfo;
        char buffer[80];
        timeinfo = localtime(&p_Entry.time);

        strftime(buffer, 80, "%F %X", timeinfo);

        String l_Content = "";
        l_Content += buffer;
        l_Content += "\t";
        l_Content += std::to_string(p_Entry.threadId).c_str();
        l_Content += "\t";

        if (p_Entry.level == LogLevel::DEBUG) {
          l_Content += "DEBUG\t";
        } else if (p_Entry.level == LogLevel::INFO) {
          l_Content += "INFO \t";
        } else if (p_Entry.level == LogLevel::WARN) {
          l_Content += "WARN \t";
        } else if (p_Entry.level == LogLevel::ERROR) {
          l_Content += "ERROR \t";
        } else if (p_Entry.level == LogLevel::FATAL) {
          l_Content += "FATAL \t";
        } else if (p_Entry.level == LogLevel::PROFILE) {
          l_Content += "PRFLR\t";
        }

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

        l_Content += "[";
        l_Content += l_ModBuffer;
        l_Content += "] - ";
        l_Content += p_Entry.message;

        l_Content += "\n";

        FileIO::write_sync(l_LogFile, l_Content);

        FileIO::close(l_LogFile);
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

        LogEntry l_Entry = m_Entry;
        g_ThreadPool->enqueue([l_Entry] { log_to_file(l_Entry); });

        if (m_Entry.terminate) {
          delete g_ThreadPool;
          g_ThreadPool = nullptr;
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

      LogStream &LogStream::operator<<(Name p_Name)
      {
        return *this << p_Name.c_str();
      }

      LogStream &LogStream::operator<<(Handle &p_Message)
      {
        RTTI::TypeInfo &l_TypeInfo =
            Handle::get_type_info(p_Message.get_type());
        return *this << l_TypeInfo.name
                     << "(Index: " << p_Message.get_index()
                     << ", Generation: " << p_Message.get_generation()
                     << ", Alive: " << l_TypeInfo.is_alive(p_Message)
                     << ")";
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
        return *this << "Vector2(" << p_Vec.x << ", " << p_Vec.y
                     << ")";
      }

      LogStream &LogStream::operator<<(Math::Vector3 &p_Vec)
      {
        return *this << "Vector3(" << p_Vec.x << ", " << p_Vec.y
                     << ", " << p_Vec.z << ")";
      }

      LogStream &LogStream::operator<<(Math::Vector4 &p_Vec)
      {
        return *this << "Vector4(" << p_Vec.x << ", " << p_Vec.y
                     << ", " << p_Vec.z << ", " << p_Vec.w << ")";
      }

      LogStream &LogStream::operator<<(Math::Quaternion &p_Quat)
      {
        return *this << "Quaternion(" << p_Quat.x << ", " << p_Quat.y
                     << ", " << p_Quat.z << ", " << p_Quat.w << ")";
      }

      LogStream &LogStream::operator<<(Math::UVector2 &p_Vec)
      {
        return *this << "UVector2(" << p_Vec.x << ", " << p_Vec.y
                     << ")";
      }

      LogStream &LogStream::operator<<(Math::UVector3 &p_Vec)
      {
        return *this << "UVector3(" << p_Vec.x << ", " << p_Vec.y
                     << ", " << p_Vec.z << ")";
      }
    } // namespace Log
  }   // namespace Util
} // namespace Low
