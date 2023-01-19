#include "LowUtilLogger.h"

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <ctime>
#include <string.h>
#include <time.h>
#include <thread>

namespace Low {
  namespace Util {
    namespace Log {
      static void print_time()
      {
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%F %X", timeinfo);

        printf("\x1B[90m%s\033[0m\t", buffer);
      }

      static void print_thread_id()
      {
        std::thread::id this_id = std::this_thread::get_id();
        printf("\x1B[35m%u\033[0m\t", this_id);
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

        printf("\x1B[90m[%s]\033[0m ", l_ModBuffer);
      }

      void log(uint8_t p_LogLevel, const char *p_Module, const char *p_Message)
      {
        print_time();
        print_thread_id();
        print_log_level(p_LogLevel);
        print_module(p_Module);
        printf(" - %s\n", p_Message);
      }

      void info(const char *p_Module, const char *p_Message)
      {
        log(LogLevel::INFO, p_Module, p_Message);
      }

      void debug(const char *p_Module, const char *p_Message)
      {
        log(LogLevel::DEBUG, p_Module, p_Message);
      }

      void warn(const char *p_Module, const char *p_Message)
      {
        log(LogLevel::WARN, p_Module, p_Message);
      }

      void error(const char *p_Module, const char *p_Message)
      {
        log(LogLevel::ERROR, p_Module, p_Message);
      }
    } // namespace Log
  }   // namespace Util
} // namespace Low
