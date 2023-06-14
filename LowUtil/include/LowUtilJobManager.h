#pragma once

#include "LowUtilApi.h"

#include <thread>
#include <functional>
#include <mutex>

#include "LowUtilContainers.h"

namespace Low {
  namespace Util {
    namespace JobManager {
      struct LOW_EXPORT ThreadPool
      {
      public:
        ThreadPool(int p_NumThreads);

        template <class F> void enqueue(F &&f)
        {
          {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_JobQueue.emplace(std::forward<F>(f));
          }

          m_Condition.notify_one();
        }

        ~ThreadPool();

      private:
        List<std::thread> m_Workers;
        Queue<std::function<void()>> m_JobQueue;
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop;
      };

      void initialize();
      void cleanup();

      LOW_EXPORT ThreadPool &default_pool();
    } // namespace JobManager
  }   // namespace Util
} // namespace Low
