#include "LowUtilJobManager.h"
#include <string>

#include <windows.h>

namespace Low {
  namespace Util {
    namespace JobManager {
      ThreadPool *g_DefaultThreadPool;

      inline std::wstring to_wstring(const char *str)
      {
        if (!str)
          return {};

        int size_needed = MultiByteToWideChar(
            CP_UTF8, // or CP_ACP if your char* is ANSI, but UTF-8 is
                     // safer
            0, str,
            -1, // -1 means input is null-terminated
            nullptr, 0);
        if (size_needed <= 0)
          return {};

        std::wstring wstr(size_needed - 1,
                          L'\0'); // exclude null terminator
        MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0],
                            size_needed);
        return wstr;
      }

      ThreadPool::ThreadPool(int p_NumThreads) : m_Stop(false)
      {
        for (int i = 0; i < p_NumThreads; ++i) {
          m_Workers.emplace_back([this] {
            while (true) {
              std::function<void()> task;

              {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Condition.wait(lock, [this] {
                  return m_Stop || !m_JobQueue.empty();
                });

                if (m_Stop && m_JobQueue.empty())
                  return;

                task = std::move(m_JobQueue.front());
                m_JobQueue.pop();
              }

              task();
            }
          });

          Util::String i_WorkerName = "Worker ";
          i_WorkerName += std::to_string(i + 1).c_str();
          std::wstring i_WName = to_wstring(i_WorkerName.c_str());

          SetThreadDescription(m_Workers[i].native_handle(),
                               i_WName.c_str());
        }
      }

      ThreadPool::~ThreadPool()
      {
        {
          std::unique_lock<std::mutex> lock(m_QueueMutex);
          m_Stop = true;
        }

        m_Condition.notify_all();

        for (std::thread &worker : m_Workers)
          worker.join();
      }

      void initialize()
      {
        g_DefaultThreadPool = new ThreadPool(4);
      }

      void cleanup()
      {
        delete g_DefaultThreadPool;
      }

      ThreadPool &default_pool()
      {
        return *g_DefaultThreadPool;
      }
    } // namespace JobManager
  } // namespace Util
} // namespace Low
